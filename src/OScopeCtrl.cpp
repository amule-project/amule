//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <cmath>
#include <algorithm>		// Needed for std::max
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/dcmemory.h>

#include "OScopeCtrl.h"		// Interface declarations.
#include "StatisticsDlg.h"	// Needed for GetHistory()
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "OtherFunctions.h"	// Needed for CastSecondsToHM
#include "StringFunctions.h"
#include "amule.h"		// Needed for theApp
#include "Format.h"

BEGIN_EVENT_TABLE(COScopeCtrl,wxControl)
  EVT_PAINT(COScopeCtrl::OnPaint)
  EVT_SIZE(COScopeCtrl::OnSize)
  EVT_TIMER(TIMER_OSCOPE,COScopeCtrl::OnTimer)
END_EVENT_TABLE()


COScopeCtrl::COScopeCtrl(int cntTrends, int nDecimals, StatsGraphType type, wxWindow*parent)
  : wxControl(parent,-1, wxDefaultPosition, wxDefaultSize), timerRedraw(this, TIMER_OSCOPE)
{
COLORREF crPreset [ 16 ] = {
	RGB( 0xFF, 0x00, 0x00 ),  RGB( 0xFF, 0xC0, 0xC0 ),  
	RGB( 0xFF, 0xFF, 0x00 ),  RGB( 0xFF, 0xA0, 0x00 ),  
	RGB( 0xA0, 0x60, 0x00 ),  RGB( 0x00, 0xFF, 0x00 ),
	RGB( 0x00, 0xA0, 0x00 ),  RGB( 0x00, 0x00, 0xFF ),
	RGB( 0x00, 0xA0, 0xFF ),  RGB( 0x00, 0xFF, 0xFF ),
	RGB( 0x00, 0xA0, 0xA0 ),  RGB( 0xC0, 0xC0, 0xFF ),
	RGB( 0xFF, 0x00, 0xFF ),  RGB( 0xA0, 0x00, 0xA0 ),
	RGB( 0xFF, 0xFF, 0xFF ),  RGB( 0x80, 0x80, 0x80 )
};
	// since plotting is based on a LineTo for each new point
	// we need a starting point (i.e. a "previous" point)
	// use 0.0 as the default first point.
	// these are public member variables, and can be changed outside
	// (after construction).  
	// G.Hayduk: NTrends is the number of trends that will be drawn on
	// the plot. First 15 plots have predefined colors, but others will
	// be drawn with white, unless you call SetPlotColor
	nTrends = cntTrends;
	pdsTrends = new PlotData_t[nTrends];

	dcPlot=NULL;
	dcGrid=NULL;
	bmapPlot=NULL;
	bmapGrid=NULL;

	PlotData_t* ppds = pdsTrends;
	for(unsigned i=0; i<nTrends; ++i, ++ppds){
		ppds->crPlot = (i<15 ? crPreset[i] : RGB(255, 255, 255)); 
		ppds->penPlot=*(wxThePenList->FindOrCreatePen(wxColour(GetRValue(ppds->crPlot),GetGValue(ppds->crPlot),GetBValue(ppds->crPlot)),1,wxSOLID));
		ppds->fPrev = ppds->fLowerLimit = ppds->fUpperLimit = 0.0;
	}

	oldwidth=oldheight=0;
	bRecreateGraph = bRecreateGrid = bStopped = false;
	nDelayedPoints = 0;
	sLastTimestamp = 0.0;
	sLastPeriod = 1.0;
	nShiftPixels = 1;  
	nYDecimals = nDecimals;
	crBackground  = RGB(  0,   0,   0) ;  // see also SetBackgroundColor
	crGrid  = RGB(  0, 255, 255) ;  // see also SetGridColor
	brushBack=*(wxTheBrushList->FindOrCreateBrush(wxColour(GetRValue(crBackground),GetGValue(crBackground),GetBValue(crBackground)),wxSOLID));

	strXUnits = wxT("X");  // can also be set with SetXUnits
	strYUnits = wxT("Y");  // can also be set with SetYUnits

	bmapOldGrid = NULL ;
	bmapOldPlot = NULL ;

	nXGrids = 6;
	nYGrids = 5;
	
	graph_type = type;
	
	timerRedraw.SetOwner(this);
}  // COScopeCtrl



COScopeCtrl::~COScopeCtrl()
{
	// just to be picky restore the bitmaps for the two memory dc's
	// (these dc's are being destroyed so there shouldn't be any leaks)
	if (bmapOldGrid != NULL)
		dcGrid->SelectObject(wxNullBitmap) ;  
	if (bmapOldPlot != NULL)
		dcPlot->SelectObject(wxNullBitmap) ;  
	delete [] pdsTrends;

	delete dcGrid;
	delete dcPlot;
	if(bmapPlot) delete bmapPlot;
	if(bmapGrid) delete bmapGrid;
} // ~COScopeCtrl


void COScopeCtrl::SetRange(float fLower, float fUpper, unsigned iTrend)
{
	PlotData_t* ppds = pdsTrends+iTrend;
	if ((ppds->fLowerLimit == fLower) && (ppds->fUpperLimit == fUpper))
		return;
	ppds->fLowerLimit = fLower;
	ppds->fUpperLimit = fUpper;
	ppds->fVertScale = (float)nPlotHeight / (fUpper-fLower); 
	ppds->yPrev = GetPlotY(ppds->fPrev, ppds);
	if (iTrend==0)
		InvalidateCtrl();
	else
		InvalidateGraph();
}  // SetRange


void COScopeCtrl::SetRanges(float fLower, float fUpper)
{
	for(unsigned iTrend=0; iTrend<nTrends; ++iTrend)
		SetRange(fLower, fUpper, iTrend);
}  // SetRanges


// G.Hayduk: Apart from setting title of axis, now you can optionally set 
// the limits strings (string which will be placed on the left and right of axis)
void COScopeCtrl::SetXUnits(const wxString& strUnits, const wxString& strMin, const wxString& strMax)
{
	strXUnits = strUnits;
	strXMin = strMin;
	strXMax = strMax;
	InvalidateGrid() ;
}  // SetXUnits


void COScopeCtrl::SetYUnits(const wxString& strUnits, const wxString& strMin, const wxString& strMax)
{
	strYUnits = strUnits;
	strYMin = strMin;
	strYMax = strMax;
	InvalidateGrid();
}  // SetYUnits


void COScopeCtrl::SetGridColor(COLORREF cr)
{
	if (cr == crGrid) 
		return;
	crGrid = cr;
	InvalidateGrid() ;
}  // SetGridColor


void COScopeCtrl::SetPlotColor(COLORREF cr, unsigned iTrend)
{
	PlotData_t* ppds = pdsTrends+iTrend;
	if (ppds->crPlot == cr)
		return;
	ppds->crPlot = cr;
	ppds->penPlot=*(wxThePenList->FindOrCreatePen(wxColour(GetRValue(ppds->crPlot),GetGValue(ppds->crPlot),GetBValue(ppds->crPlot)),1,wxSOLID));
	InvalidateGraph();
}  // SetPlotColor


void COScopeCtrl::SetBackgroundColor(COLORREF cr)
{
	if (crBackground == cr)
		return;
	crBackground = cr;
	brushBack=*(wxTheBrushList->FindOrCreateBrush(wxColour(GetRValue(crBackground),GetGValue(crBackground),GetBValue(crBackground)),wxSOLID));
	InvalidateCtrl() ;
}  // SetBackgroundColor


void COScopeCtrl::RecreateGrid()
{
	
	if (!dcGrid) {
		return;
	}
	
	// There is a lot of drawing going on here - particularly in terms of 
	// drawing the grid.  Don't panic, this is all being drawn (only once)
	// to a bitmap.  The result is then BitBlt'd to the control whenever needed.
	bRecreateGrid = false;
	if(nClientWidth==0 || nClientHeight==0)
	  return;
	unsigned j, GridPos;
	int nCharacters ;
	wxPen solidPen=*(wxThePenList->FindOrCreatePen(wxColour(GetRValue(crGrid),GetGValue(crGrid),GetBValue(crGrid)),1,wxSOLID));
	wxString strTemp;

	// fill the grid background
	dcGrid->SetBrush(brushBack);
	dcGrid->SetPen(*wxTRANSPARENT_PEN);
	dcGrid->DrawRectangle(rectClient.left,rectClient.top,rectClient.right-rectClient.left,
				rectClient.bottom-rectClient.top);
	// draw the plot rectangle: determine how wide the y axis scaling values are,
	// add the units digit, decimal point, one decimal place, and an extra space
	nCharacters = std::abs((int)std::log10(std::fabs(pdsTrends[0].fUpperLimit))) ;
	nCharacters = std::max(nCharacters, std::abs((int)std::log10(std::fabs(pdsTrends[0].fLowerLimit)))) + 4;

	// adjust the plot rectangle dimensions
	// assume 6 pixels per character (this may need to be adjusted)
	rectPlot.left = rectClient.left + 6*7+4;
	nPlotWidth    = rectPlot.right-rectPlot.left;
	// draw the plot rectangle
	dcGrid->SetPen(solidPen);
	dcGrid->DrawLine(rectPlot.left-1,rectPlot.top,rectPlot.right+1,rectPlot.top);
	dcGrid->DrawLine(rectPlot.right+1,rectPlot.top,rectPlot.right+1,rectPlot.bottom+1);
	dcGrid->DrawLine(rectPlot.right+1,rectPlot.bottom+1,rectPlot.left-1,rectPlot.bottom+1);
	dcGrid->DrawLine(rectPlot.left-1,rectPlot.bottom+1,rectPlot.left-1,rectPlot.top);
	dcGrid->SetPen(wxNullPen);

	// draw the dotted lines, 
	// G.Hayduk: added configurable number of grids
	wxColour col(GetRValue(crGrid),GetGValue(crGrid),GetBValue(crGrid));
	wxPen grPen(col,1,wxSOLID);
	dcGrid->SetPen(grPen);

	for(j=1; j<(nYGrids+1); ++j){
		GridPos = (rectPlot.bottom-rectPlot.top)*j/( nYGrids + 1 ) + rectPlot.top ;
		for (unsigned int i = rectPlot.left; i < rectPlot.right; i += 4)
		  dcGrid->DrawPoint(i,GridPos);
	}

	// create some fonts (horizontal and vertical)
	wxFont* axisFont = new wxFont(10,wxSWISS,wxNORMAL,wxNORMAL,FALSE);
	dcGrid->SetFont(*axisFont);//,this);

	// y max
	dcGrid->SetTextForeground(wxColour(GetRValue(crGrid),GetGValue(crGrid),GetBValue(crGrid)));
	if( strYMax.IsEmpty() ) {
		strTemp = wxString::Format(wxT("%.*f"), nYDecimals, pdsTrends[ 0 ].fUpperLimit);
	} else {
		strTemp = strYMax;
	}
	wxCoord sizX,sizY;
	dcGrid->GetTextExtent(strTemp,&sizX,&sizY);
	dcGrid->DrawText(strTemp,rectPlot.left-4-sizX,rectPlot.top-7);
	// y min
	if( strYMin.IsEmpty() ) {
		strTemp = wxString::Format(wxT("%.*f"), nYDecimals, pdsTrends[ 0 ].fLowerLimit) ;
	} else {
		strTemp = strYMin;
	}
	dcGrid->GetTextExtent(strTemp,&sizX,&sizY);
	dcGrid->DrawText(strTemp,rectPlot.left-4-sizX, rectPlot.bottom);

	// x units
	strTemp = CastSecondsToHM((nPlotWidth/nShiftPixels) * (int)floor(sLastPeriod+0.5));
		// floor(x + 0.5) is a way of doing round(x) that works with gcc < 3 ...
	if (bStopped) {
		strXUnits = CFormat( _("Disabled [%s]") ) % strTemp;
	} else {
		strXUnits = strTemp;
	}	
	
	dcGrid->GetTextExtent(strXUnits,&sizX,&sizY);
	dcGrid->DrawText(strXUnits,(rectPlot.left+rectPlot.right)/2-sizX/2,rectPlot.bottom+4);

	// y units
	if (!strYUnits.IsEmpty()) {
		dcGrid->GetTextExtent(strYUnits,&sizX,&sizY);
		dcGrid->DrawText(strYUnits, rectPlot.left-4-sizX, (rectPlot.top+rectPlot.bottom)/2-sizY/2);
	}
//	if(!strYUnits.IsEmpty()) 
//	  dcGrid->DrawRotatedText(strYUnits,(rectClient.left+rectPlot.left+4)/2/*+(sizY*2)*/,
//				    ((rectPlot.bottom+rectPlot.top)/2)+sizX/2,90.0);
	// no more drawing to this bitmap is needed until the setting are changed

	delete axisFont;
	
	if (bRecreateGraph)
		RecreateGraph(false);
	// finally, force the plot area to redraw
	wxRect rect;
	rect.x=rectClient.left;
	rect.y=rectClient.top;
	rect.width=rectClient.right-rectClient.left;
	rect.height=rectClient.bottom-rectClient.top;
	Refresh(FALSE,&rect);
} // RecreateGrid


void COScopeCtrl::AppendPoints(double sTimestamp, const float *apf[])
{
	sLastTimestamp = sTimestamp;
	ShiftGraph(1);
	DrawPoints(apf, 1);
	Refresh(FALSE);
} // AppendPoints



void COScopeCtrl::OnPaint(wxPaintEvent& WXUNUSED(evt)) 
{	// no real plotting work is performed here unless we are coming out of a hidden state;
	// normally, just putting the existing bitmaps on the client to avoid flicker, 
	// establish a memory dc and then BitBlt it to the client
	if (bRecreateGrid || bRecreateGraph) {
		timerRedraw.Stop();
		if (bRecreateGrid)
			RecreateGrid();  // this will also recreate the graph if that flag is set
		else if (bRecreateGraph)
			RecreateGraph(true);
	}
	if (nDelayedPoints>0) {				// we've just come out of hiding, so catch up
		int n = std::min(nPlotWidth, nDelayedPoints);		
		nDelayedPoints = 0;				// (this is more efficient than plotting in the 
		PlotHistory(n, true, false);	// background because the bitmap is shifted only 
	}									// once for all delayed points together)

	wxPaintDC dc(this);
	wxMemoryDC memDC;
	wxBitmap* memBitmap;
	memBitmap=new wxBitmap(nClientWidth,nClientHeight);
	memDC.SelectObject(*memBitmap);

	if (memDC.Ok())
	{	// first drop the grid on the memory dc
	  memDC.Blit(0,0,nClientWidth,nClientHeight,dcGrid,0,0);
		// now add the plot on top as a "pattern" via SRCPAINT.
		// works well with dark background and a light plot
	  memDC.Blit(0,0,nClientWidth,nClientHeight,dcPlot,0,0,wxOR);
		// finally send the result to the display
	  dc.Blit(0,0,nClientWidth,nClientHeight,&memDC,0,0);
	}

	memDC.SelectObject(wxNullBitmap);
	delete memBitmap;
} // OnPaint


void COScopeCtrl::OnSize(wxSizeEvent& evt)
{
	// This gets called repeatedly as the user resizes the app;
	// we use the timer mechanism through InvalidateCtrl to avoid unnecessary redrawing
	// NOTE: OnSize automatically gets called during the setup of the control
	if(oldwidth==evt.GetSize().GetWidth() && oldheight==evt.GetSize().GetHeight())
	  return;
	oldwidth=evt.GetSize().GetWidth();
	oldheight=evt.GetSize().GetHeight();
	wxRect myrect=GetClientRect();
	rectClient.left=myrect.x;
	rectClient.top=myrect.y;
	rectClient.right=myrect.x+myrect.width;
	rectClient.bottom=myrect.y+myrect.height;
	if(myrect.width<1 || myrect.height<1)
		return;

	// set some member variables to avoid multiple function calls
	nClientHeight = myrect.height;
	nClientWidth  = myrect.width;
	// the "left" coordinate and "width" will be modified in
	// InvalidateCtrl to be based on the y axis scaling
	rectPlot.left   = 20 ; 
	rectPlot.top    = 10 ;
	rectPlot.right  = rectClient.right-10 ;
	rectPlot.bottom = rectClient.bottom-25 ;
	nPlotHeight = rectPlot.bottom-rectPlot.top;
	nPlotWidth  = rectPlot.right-rectPlot.left;
	PlotData_t* ppds = pdsTrends;
	for(unsigned iTrend=0; iTrend<nTrends; ++iTrend, ++ppds) {
		ppds->fVertScale = (float)nPlotHeight / (ppds->fUpperLimit-ppds->fLowerLimit); 
		ppds->yPrev = GetPlotY(ppds->fPrev, ppds);
	}

	// destroy grid dc and bitmap (InvalidateCtrl recreates them in new size)
	if(dcGrid) {
	  dcGrid->SelectObject(wxNullBitmap);
	  delete dcGrid;
	}
	dcGrid=new wxMemoryDC;
	if(bmapGrid) 
		delete bmapGrid;
	bmapGrid = new wxBitmap(nClientWidth,nClientHeight);
	dcGrid->SelectObject(*bmapGrid);

	if(dcPlot) {
		dcPlot->SelectObject(wxNullBitmap);
		delete dcPlot;
	}
	dcPlot=new wxMemoryDC;
	if(bmapPlot) 
		delete bmapPlot;
	bmapPlot = new wxBitmap(nClientWidth, nClientHeight);
	dcPlot->SelectObject(*bmapPlot);

	InvalidateCtrl();
} // OnSize


void COScopeCtrl::ShiftGraph(unsigned cntPoints)
{
	if (dcPlot == NULL)
		return;
	unsigned cntPixelOffset = cntPoints*nShiftPixels;
	if (cntPixelOffset >= (unsigned)nPlotWidth)
		cntPixelOffset = nPlotWidth;
	else
		dcPlot->Blit(rectPlot.left,rectPlot.top+1, nPlotWidth, nPlotHeight, dcPlot,
	    	rectPlot.left+cntPixelOffset, rectPlot.top+1);	// scroll graph to the left
	// clear a rectangle over the right side of plot prior to adding the new points
	dcPlot->SetPen(*wxTRANSPARENT_PEN);
	dcPlot->SetBrush(brushBack);	// fill with background color
	dcPlot->DrawRectangle(rectPlot.right-cntPixelOffset+1, rectPlot.top, 
		cntPixelOffset, rectPlot.bottom-rectPlot.top+1);
} // ShiftGraph



unsigned COScopeCtrl::GetPlotY(float fPlot, PlotData_t* ppds)
{
	if (fPlot <= ppds->fLowerLimit)
		return rectPlot.bottom;
	else if (fPlot >= ppds->fUpperLimit)
		return rectPlot.top+1;
	else 
  		return rectPlot.bottom - (unsigned)((fPlot - ppds->fLowerLimit) * ppds->fVertScale);
} // GetPlotY



void COScopeCtrl::DrawPoints(const float *apf[], unsigned cntPoints)
{	// this appends a new set of data points to a graph; all of the plotting is 
	// directed to the memory based bitmap associated with dcPlot
	// the will subsequently be BitBlt'd to the client in OnPaint
	if (dcPlot == NULL) {
		printf("COScopeCtrl::DrawPoints - dcPlot==NULL  unexpected\n");
		return;
	}
	// draw the next line segement
	unsigned x, y, yPrev;
	unsigned cntPixelOffset = std::min((unsigned)(nPlotWidth-1), (cntPoints-1)*nShiftPixels);
	PlotData_t* ppds = pdsTrends;

	for (unsigned iTrend=0; iTrend<nTrends; ++iTrend, ++ppds) {
		const float* pf = apf[iTrend] + cntPoints - 1;
		yPrev = ppds->yPrev;
		dcPlot->SetPen(ppds->penPlot);
		for (x=rectPlot.right-cntPixelOffset; x<=rectPlot.right; x+=nShiftPixels) {
			y = GetPlotY(*pf--, ppds);
			dcPlot->DrawLine(x-nShiftPixels, yPrev, x, y);
			yPrev = y;
		}
		ppds->fPrev = *(pf+1);
		ppds->yPrev = yPrev;
	}
} // DrawPoints


void COScopeCtrl::PlotHistory(unsigned cntPoints, bool bShiftGraph, bool bRefresh) 
{
#ifndef CLIENT_GUI
	wxASSERT(graph_type != GRAPH_INVALID);
	
	if (graph_type != GRAPH_INVALID) {
		unsigned i, cntFilled;
		float** apf = new float*[nTrends];
		for (i=0; i<nTrends; ++i)
			apf[i] = new float[cntPoints];
		double sFinal = (bStopped ? sLastTimestamp : -1.0);
		cntFilled = theApp.statistics->GetHistory(cntPoints, sLastPeriod, sFinal, apf, graph_type);
		if (cntFilled >1  ||  (bShiftGraph && cntFilled!=0)) {
			if (bShiftGraph) {  // delayed points - we have an fPrev
				ShiftGraph(cntFilled);
			} else {  // fresh graph, we need to preset fPrev, yPrev
				PlotData_t* ppds = pdsTrends;	
				for(i=0; i<nTrends; ++i, ++ppds)
					ppds->yPrev = GetPlotY(ppds->fPrev = *(apf[i] + cntFilled - 1), ppds);
				cntFilled--;
			}
			DrawPoints((const float**)apf, cntFilled);
			if (bRefresh)
				Refresh(FALSE);
		}
		for (i=0; i<nTrends; ++i)
			delete[] apf[i];
	
		delete[] apf;
	} else {
		// No history (yet) for Kad.
	}
#else
	#warning CORE/GUI -- EC needed
#endif
} // PlotHistory


void COScopeCtrl::RecreateGraph(bool bRefresh)
{
	bRecreateGraph = false;
	nDelayedPoints = 0;
	dcPlot->SetBrush(brushBack);
	dcPlot->SetPen(*wxTRANSPARENT_PEN);
	dcPlot->DrawRectangle(rectClient.left, rectClient.top, rectClient.right-rectClient.left,
								rectClient.bottom-rectClient.top);
	PlotHistory(nPlotWidth, false, bRefresh);
} // RecreateGraph


void COScopeCtrl::Reset(double sNewPeriod)
{ 
	bool bStoppedPrev = bStopped;
	bStopped = false;
	if (sLastPeriod != sNewPeriod  ||  bStoppedPrev) {
		sLastPeriod = sNewPeriod;
		InvalidateCtrl();
	}		
} // Reset


void COScopeCtrl::Stop()
{ 
	bStopped = true;
	bRecreateGraph = false;
	RecreateGrid();
} // Stop


void COScopeCtrl::InvalidateCtrl(bool bInvalidateGraph, bool bInvalidateGrid) 
{
	timerRedraw.Stop();
	timerRedraw.SetOwner(this,TIMER_OSCOPE);
	if (bInvalidateGraph)
		bRecreateGraph = true;
	if (bInvalidateGrid)
		bRecreateGrid = true;
	timerRedraw.Start(100);
} // InvalidateCtrl


void COScopeCtrl::OnTimer(wxTimerEvent& WXUNUSED(evt))
/*	The timer is used to consolidate redrawing of the graphs:  when the user resizes
	the application, we get multiple calls to OnSize.  If he changes several parameters
	in the Preferences, we get several individual SetXYZ calls.  If we were to try to 
	recreate the graphs for each such event, performance would be sluggish, but with 
	the timer, each event (if they come in quick succession) simply restarts the timer 
	until there is a little pause and OnTimer actually gets called and does its work.
*/
{
	if( !theApp.amuledlg || !theApp.amuledlg->SafeState()) {
    		return;
	}
	timerRedraw.Stop();
	if (bRecreateGrid) {
		RecreateGrid();	// this will also recreate the graph if that flag is set
	} else if (bRecreateGraph) {
		RecreateGraph(true);
	}
} // OnTimer

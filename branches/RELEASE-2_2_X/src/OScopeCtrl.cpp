//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <cmath>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>

#include <common/Format.h>

#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "Logger.h"		// Needed for AddLogLineM
#include "OScopeCtrl.h"		// Interface declarations
#include "OtherFunctions.h"	// Needed for CastSecondsToHM


BEGIN_EVENT_TABLE(COScopeCtrl,wxControl)
	EVT_PAINT(COScopeCtrl::OnPaint)
	EVT_SIZE(COScopeCtrl::OnSize)
	EVT_TIMER(TIMER_OSCOPE,COScopeCtrl::OnTimer)
END_EVENT_TABLE()


const COLORREF crPreset [ 16 ] = {
	RGB( 0xFF, 0x00, 0x00 ),  RGB( 0xFF, 0xC0, 0xC0 ),  
	RGB( 0xFF, 0xFF, 0x00 ),  RGB( 0xFF, 0xA0, 0x00 ),  
	RGB( 0xA0, 0x60, 0x00 ),  RGB( 0x00, 0xFF, 0x00 ),
	RGB( 0x00, 0xA0, 0x00 ),  RGB( 0x00, 0x00, 0xFF ),
	RGB( 0x00, 0xA0, 0xFF ),  RGB( 0x00, 0xFF, 0xFF ),
	RGB( 0x00, 0xA0, 0xA0 ),  RGB( 0xC0, 0xC0, 0xFF ),
	RGB( 0xFF, 0x00, 0xFF ),  RGB( 0xA0, 0x00, 0xA0 ),
	RGB( 0xFF, 0xFF, 0xFF ),  RGB( 0x80, 0x80, 0x80 )
};


COScopeCtrl::COScopeCtrl(int cntTrends, int nDecimals, StatsGraphType type, wxWindow* parent)
	: wxControl(parent, -1, wxDefaultPosition, wxDefaultSize)
	, timerRedraw(this, TIMER_OSCOPE)
{
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

	PlotData_t* ppds = pdsTrends;
	for(unsigned i=0; i<nTrends; ++i, ++ppds){
		ppds->crPlot = (i<15 ? crPreset[i] : RGB(255, 255, 255)); 
		ppds->penPlot=*(wxThePenList->FindOrCreatePen(WxColourFromCr(ppds->crPlot), 1, wxSOLID));
		ppds->fPrev = ppds->fLowerLimit = ppds->fUpperLimit = 0.0;
	}

	bRecreateGraph = bRecreateGrid = bStopped = false;
	nDelayedPoints = 0;
	sLastTimestamp = 0.0;
	sLastPeriod = 1.0;
	nShiftPixels = 1;  
	nYDecimals = nDecimals;
	m_bgColour  = wxColour(  0,   0,   0) ;  // see also SetBackgroundColor
	m_gridColour  = wxColour(  0, 255, 255) ;  // see also SetGridColor
	brushBack=*(wxTheBrushList->FindOrCreateBrush(m_bgColour, wxSOLID));

	strXUnits = wxT("X");  // can also be set with SetXUnits
	strYUnits = wxT("Y");  // can also be set with SetYUnits

	nXGrids = 6;
	nYGrids = 5;
	
	graph_type = type;
	
	// Ensure that various size-constraints are calculated (via OnSize).
	SetClientSize(GetClientSize());
}


COScopeCtrl::~COScopeCtrl()
{
	delete [] pdsTrends;
}


void COScopeCtrl::SetRange(float fLower, float fUpper, unsigned iTrend)
{
	PlotData_t* ppds = pdsTrends+iTrend;
	if ((ppds->fLowerLimit == fLower) && (ppds->fUpperLimit == fUpper))
		return;
	ppds->fLowerLimit = fLower;
	ppds->fUpperLimit = fUpper;
	ppds->fVertScale = (float)m_rectPlot.GetHeight() / (fUpper-fLower); 
	ppds->yPrev = GetPlotY(ppds->fPrev, ppds);
	
	if (iTrend == 0) {
		InvalidateCtrl();
	} else {
		InvalidateGraph();
	}
}


void COScopeCtrl::SetRanges(float fLower, float fUpper)
{
	for (unsigned iTrend = 0; iTrend < nTrends; ++iTrend) {
		SetRange(fLower, fUpper, iTrend);
	}
}


void COScopeCtrl::SetYUnits(const wxString& strUnits, const wxString& strMin, const wxString& strMax)
{
	strYUnits = strUnits;
	strYMin = strMin;
	strYMax = strMax;
	InvalidateGrid();
}


void COScopeCtrl::SetGridColor(COLORREF cr)
{
	wxColour newCol = WxColourFromCr(cr);
	if (newCol == m_gridColour) {
		return;
	}
	
	m_gridColour = newCol;
	InvalidateGrid() ;
}


void COScopeCtrl::SetPlotColor(COLORREF cr, unsigned iTrend)
{
	PlotData_t* ppds = pdsTrends+iTrend;
	if (ppds->crPlot == cr)
		return;
	ppds->crPlot = cr;
	ppds->penPlot=*(wxThePenList->FindOrCreatePen(WxColourFromCr(ppds->crPlot), 1, wxSOLID));
	InvalidateGraph();
}


void COScopeCtrl::SetBackgroundColor(COLORREF cr)
{
	wxColour newCol(WxColourFromCr(cr));
	if (m_bgColour == newCol) {
		return;
	}

	m_bgColour = newCol;
	brushBack= *(wxTheBrushList->FindOrCreateBrush(newCol, wxSOLID));
	InvalidateCtrl() ;
}


void COScopeCtrl::RecreateGrid()
{
	// There is a lot of drawing going on here - particularly in terms of 
	// drawing the grid.  Don't panic, this is all being drawn (only once)
	// to a bitmap.  The result is then BitBlt'd to the control whenever needed.
	bRecreateGrid = false;
	if (m_rectClient.GetWidth() == 0 || m_rectClient.GetHeight() == 0) {
		return;
	}

	wxMemoryDC dcGrid(m_bmapGrid);

	int nCharacters ;
	wxPen solidPen=*(wxThePenList->FindOrCreatePen(m_gridColour, 1, wxSOLID));
	wxString strTemp;

	// fill the grid background
	dcGrid.SetBrush(brushBack);
	dcGrid.SetPen(*wxTRANSPARENT_PEN);
	dcGrid.DrawRectangle(m_rectClient);
	// draw the plot rectangle: determine how wide the y axis scaling values are,
	// add the units digit, decimal point, one decimal place, and an extra space
	nCharacters = std::abs((int)std::log10(std::fabs(pdsTrends[0].fUpperLimit))) ;
	nCharacters = std::max(nCharacters, std::abs((int)std::log10(std::fabs(pdsTrends[0].fLowerLimit)))) + 4;

	// adjust the plot rectangle dimensions
	// assume 6 pixels per character (this may need to be adjusted)
	m_rectPlot.x	= m_rectClient.GetLeft() + 6*7+4;
	// draw the plot rectangle
	dcGrid.SetPen(solidPen);
	dcGrid.DrawRectangle(m_rectPlot.x - 1, m_rectPlot.y - 1, m_rectPlot.GetWidth() + 2, m_rectPlot.GetHeight() + 2);
	dcGrid.SetPen(wxNullPen);

	// create some fonts (horizontal and vertical)
	wxFont axisFont(10, wxSWISS, wxNORMAL, wxNORMAL, false);
	dcGrid.SetFont(axisFont);

	// y max
	dcGrid.SetTextForeground(m_gridColour);
	if( strYMax.IsEmpty() ) {
		strTemp = wxString::Format(wxT("%.*f"), nYDecimals, pdsTrends[ 0 ].fUpperLimit);
	} else {
		strTemp = strYMax;
	}
	wxCoord sizX,sizY;
	dcGrid.GetTextExtent(strTemp,&sizX,&sizY);
	dcGrid.DrawText(strTemp,m_rectPlot.GetLeft()-4-sizX,m_rectPlot.GetTop()-7);
	// y min
	if( strYMin.IsEmpty() ) {
		strTemp = wxString::Format(wxT("%.*f"), nYDecimals, pdsTrends[ 0 ].fLowerLimit) ;
	} else {
		strTemp = strYMin;
	}
	dcGrid.GetTextExtent(strTemp,&sizX,&sizY);
	dcGrid.DrawText(strTemp,m_rectPlot.GetLeft()-4-sizX, m_rectPlot.GetBottom());

	// x units
	strTemp = CastSecondsToHM((m_rectPlot.GetWidth()/nShiftPixels) * (int)floor(sLastPeriod+0.5));
		// floor(x + 0.5) is a way of doing round(x) that works with gcc < 3 ...
	if (bStopped) {
		strXUnits = CFormat( _("Disabled [%s]") ) % strTemp;
	} else {
		strXUnits = strTemp;
	}	
	
	dcGrid.GetTextExtent(strXUnits,&sizX,&sizY);
	dcGrid.DrawText(strXUnits,(m_rectPlot.GetLeft() + m_rectPlot.GetRight())/2-sizX/2,m_rectPlot.GetBottom()+4);

	// y units
	if (!strYUnits.IsEmpty()) {
		dcGrid.GetTextExtent(strYUnits,&sizX,&sizY);
		dcGrid.DrawText(strYUnits, m_rectPlot.GetLeft()-4-sizX, (m_rectPlot.GetTop()+m_rectPlot.GetBottom())/2-sizY/2);
	}
	// no more drawing to this bitmap is needed until the setting are changed
	
	if (bRecreateGraph) {
		RecreateGraph(false);
	}

	// finally, force the plot area to redraw
	Refresh(false);
}


void COScopeCtrl::AppendPoints(double sTimestamp, const std::vector<float *> &apf)
{
	sLastTimestamp = sTimestamp;

	if (nDelayedPoints) {
		// Ensures that delayed points get added before the new point.
		// We do this by simply drawing the history up to and including
		// the new point.
		int n = std::min(m_rectPlot.GetWidth(), nDelayedPoints + 1);
		nDelayedPoints = 0; 
		PlotHistory(n, true, false);
	} else {
		ShiftGraph(1);
		DrawPoints(apf, 1);
	}
	
	Refresh(false);
}


void COScopeCtrl::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
	// no real plotting work is performed here unless we are coming out of a hidden state;
	// normally, just putting the existing bitmaps on the client to avoid flicker, 
	// establish a memory dc and then BitBlt it to the client
	if (bRecreateGrid || bRecreateGraph) {
		timerRedraw.Stop();
		
		if (bRecreateGrid) {
			RecreateGrid();  // this will also recreate the graph if that flag is set
		} else if (bRecreateGraph) {
			RecreateGraph(true);
		}
	}
	
	if (nDelayedPoints) {				// we've just come out of hiding, so catch up
		int n = std::min(m_rectPlot.GetWidth(), nDelayedPoints);		
		nDelayedPoints = 0;				// (this is more efficient than plotting in the 
		PlotHistory(n, true, false);	// background because the bitmap is shifted only 
	}									// once for all delayed points together)
	
	wxBufferedPaintDC dc(this);

	// We have assured that we have a valid and resized if needed 
	// wxDc and bitmap. Proceed to blit.
	dc.DrawBitmap(m_bmapGrid, 0, 0, false);
	
	// Overwrites the plot section of the image
	dc.DrawBitmap(m_bmapPlot, m_rectPlot.x, m_rectPlot.y, false);

	// draw the dotted lines.
	// This is done last because wxMAC does't support the wxOR logical
	// operation, preventing us from simply blitting the plot on top of
	// the grid bitmap.
	wxColour col(m_gridColour);
	wxPen grPen(col, 1, wxLONG_DASH);
	dc.SetPen(grPen);
	for (unsigned j = 1; j < (nYGrids + 1); ++j) {
		unsigned GridPos = (m_rectPlot.GetHeight())*j/( nYGrids + 1 ) + m_rectPlot.GetTop();
		
		dc.DrawLine(m_rectPlot.GetLeft(), GridPos, m_rectPlot.GetRight(), GridPos);
	}
}


void COScopeCtrl::OnSize(wxSizeEvent& WXUNUSED(evt))
{
	// This gets called repeatedly as the user resizes the app;
	// we use the timer mechanism through InvalidateCtrl to avoid unnecessary redrawing
	// NOTE: OnSize automatically gets called during the setup of the control
	if(GetClientRect() == m_rectClient) {
		return;
	}

	m_rectClient = GetClientRect();
	if (m_rectClient.GetWidth() < 1 || m_rectClient.GetHeight() < 1) {
		return;
	}

	// the "left" coordinate and "width" will be modified in
	// InvalidateCtrl to be based on the y axis scaling
	m_rectPlot.SetLeft(20); 
	m_rectPlot.SetTop(10);
	m_rectPlot.SetRight(std::max<int>(m_rectPlot.GetLeft() + 1, m_rectClient.GetRight() - 40));
	m_rectPlot.SetBottom(std::max<int>(m_rectPlot.GetTop() + 1, m_rectClient.GetBottom() - 25));
	
	PlotData_t* ppds = pdsTrends;
	for(unsigned iTrend=0; iTrend<nTrends; ++iTrend, ++ppds) {
		ppds->fVertScale = (float)m_rectPlot.GetHeight() / (ppds->fUpperLimit-ppds->fLowerLimit); 
		ppds->yPrev = GetPlotY(ppds->fPrev, ppds);
	}

	if (!m_bmapGrid.IsOk() || (m_rectClient != wxSize(m_bmapGrid.GetWidth(), m_bmapGrid.GetHeight()))) {
		m_bmapGrid.Create(m_rectClient.GetWidth(), m_rectClient.GetHeight());
	}
	if (!m_bmapPlot.IsOk() || (m_rectPlot != wxSize(m_bmapPlot.GetWidth(), m_bmapPlot.GetHeight()))) {
		m_bmapPlot.Create(m_rectPlot.GetWidth(), m_rectPlot.GetHeight());
	}

	InvalidateCtrl();
}


void COScopeCtrl::ShiftGraph(unsigned cntPoints)
{
	wxMemoryDC dcPlot(m_bmapPlot);

	unsigned cntPixelOffset = cntPoints*nShiftPixels;
	if (cntPixelOffset >= (unsigned)m_rectPlot.GetWidth()) {
		cntPixelOffset = m_rectPlot.GetWidth();
	} else {
		dcPlot.Blit(0, 0, m_rectPlot.GetWidth(), m_rectPlot.GetHeight(), &dcPlot,
			cntPixelOffset, 0);	// scroll graph to the left
	}

	// clear a rectangle over the right side of plot prior to adding the new points
	dcPlot.SetPen(*wxTRANSPARENT_PEN);
	dcPlot.SetBrush(brushBack);	// fill with background color
	dcPlot.DrawRectangle(m_rectPlot.GetWidth()-cntPixelOffset, 0, 
		cntPixelOffset, m_rectPlot.GetHeight());
}


unsigned COScopeCtrl::GetPlotY(float fPlot, PlotData_t* ppds)
{
	if (fPlot <= ppds->fLowerLimit) {
		return m_rectPlot.GetBottom();
	} else if (fPlot >= ppds->fUpperLimit) {
		return m_rectPlot.GetTop() + 1;
	} else {
		return m_rectPlot.GetBottom() - (unsigned)((fPlot - ppds->fLowerLimit) * ppds->fVertScale);
	}
}


void COScopeCtrl::DrawPoints(const std::vector<float *> &apf, unsigned cntPoints)
{	
	// this appends a new set of data points to a graph; all of the plotting is 
	// directed to the memory based bitmap associated with dcPlot
	// the will subsequently be BitBlt'd to the client in OnPaint
	// draw the next line segement
	unsigned y, yPrev;
	unsigned cntPixelOffset = std::min((unsigned)(m_rectPlot.GetWidth()-1), (cntPoints-1)*nShiftPixels);
	PlotData_t* ppds = pdsTrends;

	wxMemoryDC dcPlot(m_bmapPlot);

	for (unsigned iTrend=0; iTrend<nTrends; ++iTrend, ++ppds) {
		const float* pf = apf[iTrend] + cntPoints - 1;
		yPrev = ppds->yPrev;
		dcPlot.SetPen(ppds->penPlot);

		for (int x = m_rectPlot.GetRight() - cntPixelOffset; x <= m_rectPlot.GetRight(); x+=nShiftPixels) {
			y = GetPlotY(*pf--, ppds);

			// Map onto the smaller bitmap
			dcPlot.DrawLine(x - nShiftPixels - m_rectPlot.GetX(), 
					yPrev - m_rectPlot.GetY(),
					x - m_rectPlot.GetX(),
					y - m_rectPlot.GetY());

			yPrev = y;
		}
		ppds->fPrev = *(pf+1);
		ppds->yPrev = yPrev;
	}
}


#ifndef CLIENT_GUI
void COScopeCtrl::PlotHistory(unsigned cntPoints, bool bShiftGraph, bool bRefresh) 
{
	wxASSERT(graph_type != GRAPH_INVALID);
	
	if (graph_type != GRAPH_INVALID) {
		unsigned i;
		unsigned cntFilled;
		std::vector<float *> apf(nTrends);
		try {
			for (i = 0; i < nTrends; ++i) {
				apf[i] = new float[cntPoints];
			}
			double sFinal = (bStopped ? sLastTimestamp : -1.0);
			cntFilled = theApp->m_statistics->GetHistory(cntPoints, sLastPeriod, sFinal, apf, graph_type);
			if (cntFilled >1  ||  (bShiftGraph && cntFilled!=0)) {
				if (bShiftGraph) {  // delayed points - we have an fPrev
					ShiftGraph(cntFilled);
				} else {  // fresh graph, we need to preset fPrev, yPrev
					PlotData_t* ppds = pdsTrends;	
					for(i=0; i<nTrends; ++i, ++ppds)
						ppds->yPrev = GetPlotY(ppds->fPrev = *(apf[i] + cntFilled - 1), ppds);
					cntFilled--;
				}
				DrawPoints(apf, cntFilled);
				if (bRefresh)
					Refresh(false);
			}
			for (i = 0; i < nTrends; ++i) {
				delete [] apf[i];
			}
		} catch(std::bad_alloc) {
			// Failed memory allocation
			AddLogLineM(true, wxString(
				wxT("Error: COScopeCtrl::PlotHistory: Insuficient memory, cntPoints == ")) <<
				cntPoints << wxT("."));
			for (i = 0; i < nTrends; ++i) {
				delete [] apf[i];
			}
		}
	} else {
		// No history (yet) for Kad.
	}
}
#else
//#warning CORE/GUI -- EC needed
void COScopeCtrl::PlotHistory(unsigned, bool, bool) 
{
}
#endif


void COScopeCtrl::RecreateGraph(bool bRefresh)
{
	bRecreateGraph = false;
	nDelayedPoints = 0;
	
	wxMemoryDC dcPlot(m_bmapPlot);
	dcPlot.SetBackground(brushBack);
	dcPlot.Clear();
	
	PlotHistory(m_rectPlot.GetWidth(), false, bRefresh);
}


void COScopeCtrl::Reset(double sNewPeriod)
{ 
	bool bStoppedPrev = bStopped;
	bStopped = false;
	if (sLastPeriod != sNewPeriod  ||  bStoppedPrev) {
		sLastPeriod = sNewPeriod;
		InvalidateCtrl();
	}		
}


void COScopeCtrl::Stop()
{ 
	bStopped = true;
	bRecreateGraph = false;
	RecreateGrid();
}


void COScopeCtrl::InvalidateCtrl(bool bInvalidateGraph, bool bInvalidateGrid) 
{
	timerRedraw.Stop();
	timerRedraw.SetOwner(this, TIMER_OSCOPE);
	
	bRecreateGraph |= bInvalidateGraph;
	bRecreateGrid |= bInvalidateGrid;

	timerRedraw.Start(100);
}


void COScopeCtrl::OnTimer(wxTimerEvent& WXUNUSED(evt))
/*	The timer is used to consolidate redrawing of the graphs:  when the user resizes
	the application, we get multiple calls to OnSize.  If he changes several parameters
	in the Preferences, we get several individual SetXYZ calls.  If we were to try to 
	recreate the graphs for each such event, performance would be sluggish, but with 
	the timer, each event (if they come in quick succession) simply restarts the timer 
	until there is a little pause and OnTimer actually gets called and does its work.
*/
{
	if( !theApp->amuledlg || !theApp->amuledlg->SafeState()) {
		return;
	}
	timerRedraw.Stop();
	if (bRecreateGrid) {
		RecreateGrid();	// this will also recreate the graph if that flag is set
	} else if (bRecreateGraph) {
		RecreateGraph(true);
	}
}

// File_checked_for_headers

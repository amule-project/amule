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

#ifndef OSCOPECTRL_H
#define OSCOPECTRL_H

#ifndef NULL
#define NULL 0
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/control.h>		// Needed for wxControl
#include <wx/timer.h>		// Needed for wxTimer
#include <wx/pen.h>
#include <wx/brush.h>

#include "Types.h"		// Needed for RECT
#include "Color.h"		// Needed for COLORREF and RGB
#include "Statistics.h"		// Needed for StatsGraphType

class wxMemoryDC;

/////////////////////////////////////////////////////////////////////////////
// COScopeCtrl window

#define TIMER_OSCOPE 7641

class COScopeCtrl : public wxControl
{
	friend class CStatisticsDlg;
		
public:
	COScopeCtrl(int NTrends, int nDecimals, wxWindow* parent=NULL);
	~COScopeCtrl();

	void SetRange(float dLower, float dUpper, unsigned iTrend = 0);
	void SetRanges(float dLower, float dUpper);
	void SetXUnits(const wxString& string, const wxString& XMin = wxT(""), const wxString& XMax = wxT(""));
	void SetYUnits(const wxString& string, const wxString& YMin = wxT(""), const wxString& YMax = wxT(""));
	void SetBackgroundColor(COLORREF color);
	void SetGridColor(COLORREF color);
	void SetPlotColor(COLORREF color, unsigned iTrend = 0);
	COLORREF GetPlotColor(unsigned iTrend = 0)  {return pdsTrends[iTrend].crPlot;}
	float GetUpperLimit()	{ return pdsTrends[0].fUpperLimit; }
	void Reset(double sNewPeriod);
	void Stop();
	void RecreateGraph(bool bRefresh=true);
	void RecreateGrid();
	void AppendPoints(double sTimestamp, const float *apf[]);
	void DelayPoints()	{ nDelayedPoints++; }
	unsigned GetPlotHeightPixels()		{return nPlotHeight;}
	unsigned GetPlotWidthPoints()		{return nPlotWidth/nShiftPixels;}
	wxBitmap* GetBitmapPlot()			{return bmapPlot;};
	wxBitmap* GetBitmapGrid()			{return bmapGrid;};

	StatsGraphType graph_type;
	
public:
	unsigned nTrends;
	unsigned nXGrids;
	unsigned nYGrids;
	unsigned nShiftPixels;         // amount to shift with each new point 
	unsigned nYDecimals;

	wxString strXUnits, strXMin, strXMax;
	wxString strYUnits, strYMin, strYMax;
	COLORREF crBackground;
	COLORREF crGrid;      

	typedef struct PlotDataStruct {
		COLORREF crPlot;	       // data plot color  
		wxPen  penPlot;
		unsigned yPrev;
		float fPrev;
		float fLowerLimit;         // lower bounds
		float fUpperLimit;         // upper bounds
		float fVertScale;
	} PlotData_t ;


protected:
	DECLARE_EVENT_TABLE()
	PlotData_t *pdsTrends;

	unsigned nClientHeight;
	unsigned nClientWidth;
	int nPlotHeight;
	int nPlotWidth;

	RECT  rectClient;
	RECT  rectPlot;
	wxBrush brushBack;

	wxMemoryDC* dcGrid;
	wxMemoryDC* dcPlot;
	wxBitmap* bmapOldGrid;
	wxBitmap* bmapOldPlot;
	wxBitmap* bmapGrid;
	wxBitmap* bmapPlot;
	void InvalidateGraph()	{ InvalidateCtrl(true, false); }
	void InvalidateGrid()	{ InvalidateCtrl(false, true); }

private:
	int oldwidth, oldheight;
	bool bRecreateGrid, bRecreateGraph, bStopped;
	int nDelayedPoints;
	double sLastTimestamp;
	double sLastPeriod;
	wxTimer timerRedraw;
	void OnTimer(wxTimerEvent& evt);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void ShiftGraph(unsigned cntPoints);
	void PlotHistory(unsigned cntPoints, bool bShiftGraph, bool bRefresh);
	void DrawPoints(const float *apf[], unsigned cntPoints);
	unsigned GetPlotY(float fPlot, PlotData_t* ppds);
	void InvalidateCtrl(bool bInvalidateGraph = true, bool bInvalidateGrid = true);
};

#endif // OSCOPECTRL_H

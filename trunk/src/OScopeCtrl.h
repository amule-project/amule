//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef OSCOPECTRL_H
#define OSCOPECTRL_H

#ifndef NULL
#define NULL 0
#endif

#include <vector>
#include <wx/control.h>		// Needed for wxControl
#include <wx/timer.h>		// Needed for wxTimer
#include <wx/pen.h>
#include <wx/bitmap.h>
#include <wx/colour.h>

#include "Constants.h"		// Needed for StatsGraphType

class wxMemoryDC;

/////////////////////////////////////////////////////////////////////////////
// COScopeCtrl window

class COScopeCtrl : public wxControl
{
	friend class CStatisticsDlg;
		
public:
	COScopeCtrl(int NTrends, int nDecimals, StatsGraphType type, wxWindow* parent = NULL);
	~COScopeCtrl();

	void SetRange(float dLower, float dUpper, unsigned iTrend = 0);
	void SetRanges(float dLower, float dUpper);
	void SetYUnits(const wxString& string,
		const wxString& YMin = wxEmptyString, const wxString& YMax = wxEmptyString);
	void SetBackgroundColor(const wxColour& color);
	void SetGridColor(const wxColour& color);
	void SetPlotColor(const wxColour& color, unsigned iTrend = 0);
	float GetUpperLimit()		{ return pdsTrends[0].fUpperLimit; }
	void Reset(double sNewPeriod);
	void Stop();
	void RecreateGraph(bool bRefresh=true);
	void RecreateGrid();
	void AppendPoints(double sTimestamp, const std::vector<float *> &apf);
	void DelayPoints()		{ nDelayedPoints++; }

	StatsGraphType graph_type;
	
public:
	unsigned nTrends;
	unsigned nXGrids;
	unsigned nYGrids;
	unsigned nShiftPixels;         // amount to shift with each new point 
	unsigned nYDecimals;

	wxString strXUnits;
	wxString strYUnits, strYMin, strYMax;
	wxColour m_bgColour;
	wxColour m_gridColour;

	typedef struct PlotDataStruct {
		wxColour crPlot;	       // data plot color  
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

	wxRect	m_rectClient;
	wxRect	m_rectPlot;
	wxBrush	brushBack;
	wxBitmap m_bmapGrid;
	wxBitmap m_bmapPlot;

	void InvalidateGraph()	{ InvalidateCtrl(true, false); }
	void InvalidateGrid()	{ InvalidateCtrl(false, true); }

private:
	bool bRecreateGrid, bRecreateGraph, bRecreateAll, bStopped;
	int nDelayedPoints;
	double sLastTimestamp;
	double sLastPeriod;
	wxTimer timerRedraw;
	bool m_onPaint;

	void OnTimer(wxTimerEvent& evt);
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void ShiftGraph(unsigned cntPoints);
	void PlotHistory(unsigned cntPoints, bool bShiftGraph, bool bRefresh);
	void DrawPoints(const std::vector<float *> &apf, unsigned cntPoints);
	unsigned GetPlotY(float fPlot, PlotData_t* ppds);
	void InvalidateCtrl(bool bInvalidateGraph = true, bool bInvalidateGrid = true);
};

#endif // OSCOPECTRL_H
// File_checked_for_headers

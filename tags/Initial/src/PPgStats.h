// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef PPGSTATS_H
#define PPGSTATS_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/choice.h>		// Needed for wxChoice

#include "resource.h"		// Needed for IDD_PPG_STATS
#include "Preferences.h"	// Needed for CPreferences
#include "color.h"			// Needed for COLOREF
class CPreferences;

class CPPgStats : public wxPanel //CPropertyPage
{
	//DECLARE_DYNAMIC(CPPgStats)
	DECLARE_DYNAMIC_CLASS(CPPgStats)
	CPPgStats() {};
public:
	CPPgStats(wxWindow* parent);
	virtual ~CPPgStats();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_STATS };
protected:
	CPreferences *app_prefs;
protected:
	DECLARE_EVENT_TABLE();
	wxChoice* m_colors;
	void OnHScroll(wxScrollEvent& evt);
	void OnCbnSelchangeColorselector(wxCommandEvent& evt);
	void ShowInterval();
	void ShowMinsAvgGraph();
	void ShowSecsUpdateGraph();
	void ShowSecsUpdateTree();
	void OnChangeColor(wxEvent& evt);
public:
	//virtual bool OnInitDialog();
private:
	int secsUpdateGraph, secsUpdateTree, minsAvgGraph;
	int secsUpdateGraphPrev, secsUpdateTreePrev, minsAvgGraphPrev;
	COLORREF crNew[cntStatColors];
	COLORREF crPrev[cntStatColors];
public:
	virtual bool OnApply();
	bool OnCancel();
	void Localize(void);
	void LoadSettings();
};

#endif // PPGSTATS_H

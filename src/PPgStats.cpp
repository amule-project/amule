// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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


#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/slider.h>
#include <wx/colordlg.h>

#include "PPgStats.h"		// Interface declarations.
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CamuleAppBase.h"	// Needed for theApp
#include "muuli_wdr.h"		// Needed for PreferencesStatisticsTab

// CPPgStats dialog

IMPLEMENT_DYNAMIC_CLASS(CPPgStats,wxPanel)

CPPgStats::CPPgStats(wxWindow* parent)
: wxPanel(parent,CPPgStats::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;

	PreferencesStatisticsTab(this,TRUE);
	book->AddPage(this,_("Statistics"));
}

CPPgStats::~CPPgStats()
{
}

BEGIN_EVENT_TABLE(CPPgStats,wxPanel)
	EVT_SCROLL(CPPgStats::OnHScroll)
	EVT_BUTTON(IDC_COLOR_BUTTON,CPPgStats::OnChangeColor)
	EVT_CHOICE(IDC_COLORSELECTOR,CPPgStats::OnCbnSelchangeColorselector)
END_EVENT_TABLE()


void CPPgStats::OnChangeColor(wxEvent& evt)
{
	wxColour newcol=wxGetColourFromUser(this,((wxButton*)FindWindowById(IDC_COLOR_BUTTON))->GetBackgroundColour());
	if(newcol.Ok()) {
		((wxButton*)FindWindowById(IDC_COLOR_BUTTON))->SetBackgroundColour(newcol);
		int index = ((wxChoice*)FindWindowById(IDC_COLORSELECTOR))->GetSelection();
		COLORREF cr = RGB(newcol.Red(),newcol.Green(),newcol.Blue());
		crNew[index] = cr;
		CStatisticsDlg::acrStat[index] = cr;  // app_prefs->SetStatsColor(index, cr);
		theApp.amuledlg->statisticswnd->ApplyStatsColor(index);
	}
}


bool CPPgStats::OnApply()
{
/*
	// All Stats preferences now get dynamically applied to give the user visual feedback,
	// like, "what would it mean to change the 'Time for running averages' ?".
	// We've saved the old values in LoadSettings() and restore them if necessary in OnCancel().
	app_prefs->SetTrafficOMeterInterval(secsUpdateGraph);
	app_prefs->SetStatsInterval(secsUpdateTree);
	app_prefs->SetStatsAverageMinutes(minsAvgGraph);
	if (secsUpdateGraph != secsUpdateGraphPrev)
 		theApp.amuledlg->statisticswnd->SetUpdatePeriod((float)secsUpdateGraph);	
	if (minsAvgGraph != minsAvgGraphPrev)
 		theApp.amuledlg->statisticswnd->ResetAveragingTime();	
*/
	return TRUE;
}


bool CPPgStats::OnCancel()
{
	if (secsUpdateGraph != secsUpdateGraphPrev) {
		app_prefs->SetTrafficOMeterInterval(secsUpdateGraphPrev);
 		theApp.amuledlg->statisticswnd->SetUpdatePeriod();	
	}
	if (minsAvgGraph != minsAvgGraphPrev) {
		app_prefs->SetStatsAverageMinutes(minsAvgGraphPrev);
 		theApp.amuledlg->statisticswnd->ResetAveragingTime();	
	}
	if (secsUpdateTree != secsUpdateTreePrev)
		app_prefs->SetStatsInterval(secsUpdateTreePrev);
	for (int i=0; i<cntStatColors; i++) {
		if (crNew[i] != crPrev[i]) {
			CStatisticsDlg::acrStat[i] = crPrev[i];  // app_prefs->SetStatsColor(i, crPrev[i]);
			theApp.amuledlg->statisticswnd->ApplyStatsColor(i);
		}
	}
	return TRUE;
}


void CPPgStats::Localize(void)
{
}

void CPPgStats::OnHScroll(wxScrollEvent& evt)
{
	wxSlider* slider = (wxSlider*)evt.GetEventObject();

	int position = evt.GetPosition();

	if (slider==((wxSlider*)FindWindowById(IDC_SLIDER))) {
		secsUpdateGraph = position;
		ShowSecsUpdateGraph();
		app_prefs->SetTrafficOMeterInterval(secsUpdateGraph);
 		theApp.amuledlg->statisticswnd->SetUpdatePeriod();	
	} else if (slider==((wxSlider*)FindWindowById(IDC_SLIDER2))) {
		secsUpdateTree = position;
		ShowSecsUpdateTree();
		app_prefs->SetStatsInterval(secsUpdateTree);
	} else {
		minsAvgGraph = position;
		ShowMinsAvgGraph();
		app_prefs->SetStatsAverageMinutes(minsAvgGraph);
 		theApp.amuledlg->statisticswnd->ResetAveragingTime();	
	}
}


void CPPgStats::ShowInterval() {
	ShowSecsUpdateGraph();
	ShowSecsUpdateTree();
	ShowMinsAvgGraph();
	((wxStaticText*)FindWindowById(IDC_PREFCOLORS))->SetLabel(_("Select Statistics Colors"));
}


void CPPgStats::ShowSecsUpdateGraph() {
	CString str;
	
	switch (secsUpdateGraph) {
		case 0:
			str.Format(_("Update: Disabled"));
			break;
		case 1:
			str.Format(_("Update period: %i sec"), secsUpdateGraph);
			break;
		default:
			str.Format(_("Update period: %i secs"), secsUpdateGraph);
			break;
	}
	((wxStaticText*)FindWindowById(IDC_SLIDERINFO))->SetLabel(str);
}


void CPPgStats::ShowMinsAvgGraph() {
	CString str;
	
	str.Format(_("Time for running averages: %i mins"), minsAvgGraph);
	((wxStaticText*)FindWindowById(IDC_SLIDERINFO3))->SetLabel(str);
}


void CPPgStats::ShowSecsUpdateTree() {
	CString str;
	
	switch (secsUpdateTree) {
		case 0:
			str.Format(_("Update: Disabled"));
			break;
		case 1:
			str.Format(_("Update period: %i sec"), secsUpdateTree);
			break;
		default:
			str.Format(_("Update period: %i secs"), secsUpdateTree);
			break;
	}
	((wxStaticText*)FindWindowById(IDC_SLIDERINFO2))->SetLabel(str);
}


void CPPgStats::LoadSettings()
{
	secsUpdateGraph = secsUpdateGraphPrev = app_prefs->GetTrafficOMeterInterval();
	secsUpdateTree = secsUpdateTreePrev = app_prefs->GetStatsInterval();
	minsAvgGraph = minsAvgGraphPrev = app_prefs->GetStatsAverageMinutes();

	((wxSlider*)FindWindowById(IDC_SLIDER))->SetValue(secsUpdateGraph);
	((wxSlider*)FindWindowById(IDC_SLIDER2))->SetValue(secsUpdateTree);
	((wxSlider*)FindWindowById(IDC_SLIDER3))->SetValue(minsAvgGraph);

	for (int i=0; i<cntStatColors; i++)
		crNew[i] = crPrev[i] = CStatisticsDlg::acrStat[i];  // = app_prefs->GetStatsColor(i);
	wxCommandEvent nullEvt;
	OnCbnSelchangeColorselector(nullEvt);

	ShowInterval();
}

void CPPgStats::OnCbnSelchangeColorselector(wxCommandEvent& evt)
{
	m_colors=((wxChoice*)FindWindowById(IDC_COLORSELECTOR));
	int sel=m_colors->GetSelection();
	COLORREF selcolor=CStatisticsDlg::acrStat[sel];  // =theApp.glob_prefs->GetStatsColor(sel);
	((wxButton*)FindWindowById(IDC_COLOR_BUTTON))->SetBackgroundColour(wxColour(GetRValue(selcolor),GetGValue(selcolor),GetBValue(selcolor)));
}

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


#include <wx/slider.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>

#include "PPgSourcesDropping.h"	// Interface declarations
#include "Preferences.h"	// Needed for CPreferences
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include "muuli_wdr.h"		// Needed for PreferencesSourcesDroppingTab

IMPLEMENT_DYNAMIC_CLASS(CPPgSourcesDropping,wxPanel)


CPPgSourcesDropping::CPPgSourcesDropping(wxWindow* parent)
: wxPanel(parent,CPPgSourcesDropping::IDD)
{
	// Getting preferences dialog notebook
	wxNotebook* book= (wxNotebook*) parent;

	PreferencesSourcesDroppingTab(this,TRUE);
	book->AddPage(this,_("Sources Dropping"));
}

CPPgSourcesDropping::~CPPgSourcesDropping()
{
}

void CPPgSourcesDropping::Localize(void)
{
}

void CPPgSourcesDropping::LoadSettings(void)
{
	if (app_prefs->prefs->DropNoNeededSources) {
		((wxRadioButton*)FindWindowById(IDC_ENABLE_AUTO_NNS))->SetValue(TRUE);
	} else {
		((wxRadioButton*)FindWindowById(IDC_ENABLE_AUTO_NNS))->SetValue(FALSE);
	}

	if (app_prefs->prefs->SwapNoNeededSources) {
		((wxRadioButton*)FindWindowById(IDC_AUTO_NNS_EXTENDED_RADIO))->SetValue(TRUE);
	} else {
		((wxRadioButton*)FindWindowById(IDC_AUTO_NNS_EXTENDED_RADIO))->SetValue(FALSE);
	}

	if (app_prefs->prefs->DropFullQueueSources) {
		((wxCheckBox*)FindWindowById(IDC_ENABLE_AUTO_FQS))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_ENABLE_AUTO_FQS))->SetValue(FALSE);
	}

	if (app_prefs->prefs->DropHighQueueRankingSources) {
		((wxCheckBox*)FindWindowById(IDC_ENABLE_AUTO_HQRS))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_ENABLE_AUTO_HQRS))->SetValue(FALSE);
	}

	CString strBuffer;
	strBuffer.Format("%d", app_prefs->prefs->HighQueueRanking);
	((wxSpinCtrl*)FindWindowById(IDC_HQR_VALUE))->SetValue(strBuffer);

	strBuffer.Format("%d", app_prefs->prefs->AutoDropTimer);
	((wxSpinCtrl*)FindWindowById(IDC_AUTO_DROP_TIMER))->SetValue(strBuffer);

}

void CPPgSourcesDropping::OnApply()
{
	app_prefs->prefs->DropNoNeededSources = (((wxRadioButton*)FindWindowById(IDC_ENABLE_AUTO_NNS))->GetValue());

	app_prefs->prefs->SwapNoNeededSources = (((wxRadioButton*)FindWindowById(IDC_AUTO_NNS_EXTENDED_RADIO))->GetValue());

	app_prefs->prefs->DropFullQueueSources = ((wxCheckBox*)FindWindowById(IDC_ENABLE_AUTO_FQS))->GetValue();

	app_prefs->prefs->DropHighQueueRankingSources = ((wxCheckBox*)FindWindowById(IDC_ENABLE_AUTO_HQRS))->GetValue();

	/* Get the Queue Ranking from dialog. */
	app_prefs->prefs->HighQueueRanking = ((wxSpinCtrl*)FindWindowById(IDC_HQR_VALUE))->GetValue();

	/* Get the Auto Drop Timer from Dialog */
	app_prefs->prefs->AutoDropTimer = ((wxSpinCtrl*)FindWindowById(IDC_AUTO_DROP_TIMER))->GetValue();
}

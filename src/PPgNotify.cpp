// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>

#include "PPgNotify.h"		// Interface declarations.
#include "Preferences.h"	// Needed for CPreferences
#include "muuli_wdr.h"		// Needed for PreferencesNotifyTab

//IMPLEMENT_DYNAMIC(CPPgNotify, CPropertyPage)
IMPLEMENT_DYNAMIC_CLASS(CPPgNotify,wxPanel)

CPPgNotify::CPPgNotify(wxWindow* parent)
: wxPanel(parent,CPPgNotify::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;

	PreferencesNotifyTab(this,TRUE);
	book->AddPage(this,_("Notify"));
}

CPPgNotify::~CPPgNotify()
{
}

void CPPgNotify::LoadSettings(void)
{
	if (app_prefs->prefs->useSoundInNotifier) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_USESOUND))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_USESOUND))->SetValue(FALSE);
	}

	if (app_prefs->prefs->useLogNotifier) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONLOG))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONLOG))->SetValue(FALSE);
	}

	if (app_prefs->prefs->useChatNotifier) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONCHAT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONCHAT))->SetValue(FALSE);
	}

	if (app_prefs->prefs->notifierPopsEveryChatMsg) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_POP_ALWAYS))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_POP_ALWAYS))->SetValue(FALSE);
	}

	if (app_prefs->prefs->useDownloadNotifier) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONDOWNLOAD))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONDOWNLOAD))->SetValue(FALSE);
	}

	if (app_prefs->prefs->notifierNewVersion) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONNEWVERSION))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONNEWVERSION))->SetValue(FALSE);
	}

	if (app_prefs->prefs->notifierImportantError) {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_IMPORTATNT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CB_TBN_IMPORTATNT))->SetValue(FALSE);
	}

	if (app_prefs->prefs->sendEmailNotifier) {
		((wxCheckBox*)FindWindowById(IDC_SENDMAIL))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SENDMAIL))->SetValue(FALSE);
	}

	((wxTextCtrl*)FindWindowById(IDC_EDIT_TBN_WAVFILE))->SetValue(app_prefs->prefs->notifierSoundFilePath);
}

wxString CPPgNotify::DialogBrowseFile(wxString Filters, wxString DefaultFileName)
{
	return "";
}

void CPPgNotify::Localize(void)
{
}

BEGIN_EVENT_TABLE(CPPgNotify,wxPanel)
	EVT_BUTTON(IDC_BTN_BROWSE_WAV,CPPgNotify::OnBnClickedBtnBrowseWav)
END_EVENT_TABLE()

bool CPPgNotify::OnApply()
{
	wxString buffer;
	app_prefs->prefs->useSoundInNotifier = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_USESOUND))->IsChecked();
	app_prefs->prefs->useLogNotifier = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONLOG))->IsChecked();
	app_prefs->prefs->useChatNotifier = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONCHAT))->IsChecked();
	app_prefs->prefs->notifierPopsEveryChatMsg = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_POP_ALWAYS))->IsChecked();
	app_prefs->prefs->useDownloadNotifier = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONDOWNLOAD))->IsChecked();
	app_prefs->prefs->notifierNewVersion = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_ONNEWVERSION))->IsChecked();
	app_prefs->prefs->notifierImportantError = ((wxCheckBox*)FindWindowById(IDC_CB_TBN_IMPORTATNT))->IsChecked();
	app_prefs->prefs->sendEmailNotifier = ((wxCheckBox*)FindWindowById(IDC_SENDMAIL))->IsChecked();
	buffer=((wxTextCtrl*)FindWindowById(IDC_EDIT_TBN_WAVFILE))->GetValue();
	sprintf(app_prefs->prefs->notifierSoundFilePath,"%s",buffer.GetData());
	return TRUE;
}

void CPPgNotify::OnBnClickedBtnBrowseWav(wxEvent& evt)
{
	wxString buffer;
	buffer = wxFileSelector(_("Browse wav"),"","","*.wav",_("File wav (*.wav)|*.wav||"));
	((wxTextCtrl*)FindWindowById(IDC_EDIT_TBN_WAVFILE))->SetValue((buffer));
}

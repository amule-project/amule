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


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/checkbox.h>	// Needed for wxCheckBox
#include <wx/notebook.h>
#include <wx/slider.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>

#include "PPgDirectories.h"	// Interface declarations
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "CamuleAppBase.h"	// Needed for theApp
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl
#include "Preferences.h"	// Needed for CPreferences
#include "muuli_wdr.h"		// Needed for PreferencesDirectoriesTab

// CPPgDirectories dialog

IMPLEMENT_DYNAMIC_CLASS(CPPgDirectories,wxPanel)

//IMPLEMENT_DYNAMIC(CPPgDirectories, CPropertyPage)
CPPgDirectories::CPPgDirectories(wxWindow* parent)
: wxPanel(parent,CPPgDirectories::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;

	PreferencesDirectoriesTab(this,TRUE);
	book->AddPage(this,_("Directories"));

	m_ShareSelector=((CDirectoryTreeCtrl*)FindWindowById(IDC_SHARESELECTOR));
}

CPPgDirectories::~CPPgDirectories()
{
}

BEGIN_EVENT_TABLE(CPPgDirectories,wxPanel)
	EVT_BUTTON(IDC_SELTEMPDIR,CPPgDirectories::OnBnClickedSeltempdir)
	EVT_BUTTON(IDC_SELINCDIR,CPPgDirectories::OnBnClickedSelincdir)
	EVT_BUTTON(IDC_BROWSEV,CPPgDirectories::BrowseVideoplayer)
END_EVENT_TABLE()

void CPPgDirectories::LoadSettings(void)
{
	((wxTextCtrl*)FindWindowById(IDC_INCFILES))->SetValue(app_prefs->prefs->incomingdir);
	((wxTextCtrl*)FindWindowById(IDC_TEMPFILES))->SetValue(app_prefs->prefs->tempdir);

	m_ShareSelector->SetSharedDirectories(&app_prefs->shareddir_list);

	((wxTextCtrl*)FindWindowById(IDC_VIDEOPLAYER))->SetValue(app_prefs->prefs->VideoPlayer);
	if(app_prefs->prefs->moviePreviewBackup) {
		((wxCheckBox*)FindWindowById(IDC_VIDEOBACKUP))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_VIDEOBACKUP))->SetValue(FALSE);
	}
}


bool CPPgDirectories::SelectDir(char* outdir, char* titletext)
{
	wxString str=wxDirSelector(titletext,"");
	if(str.IsEmpty()) {
		return FALSE;
	}

	strcpy(outdir,str.GetData());
	return TRUE;
}

void CPPgDirectories::OnBnClickedSelincdir(wxEvent& e)
{
	char buffer[MAX_PATH];
	if (SelectDir(buffer,(char*)"Choose a folder for incoming files")) {
		((wxTextCtrl*)FindWindowById(IDC_INCFILES))->SetValue(buffer);
	}
}

void CPPgDirectories::OnBnClickedSeltempdir(wxEvent& e)
{
	char buffer[MAX_PATH];
	if (SelectDir(buffer,(char*)"Choose a folder for temp files")) {
		((wxTextCtrl*)FindWindowById(IDC_TEMPFILES))->SetValue(buffer);
	}
}

void CPPgDirectories::BrowseVideoplayer(wxEvent& e)
{
	wxString str=wxFileSelector(_("Browse for videoplayer"),"","","",_("Executable (*)|*||"));
	if(!str.IsEmpty()) {
		((wxTextCtrl*)FindWindowById(IDC_VIDEOPLAYER))->SetValue(str);
	}
}

bool CPPgDirectories::OnApply()
{
	wxString buffer;
	bool IncomingChanged = false; // Will be set true if Incoming directory changed
	bool TempChanged = false; // Will be set true if Temp directory changed
	
	if (((wxTextCtrl*)FindWindowById(IDC_INCFILES))->GetValue().Length()) {
		buffer=((wxTextCtrl*)FindWindowById(IDC_INCFILES))->GetValue();
		if (strcmp(app_prefs->prefs->incomingdir,buffer.GetData())!=0) {
			IncomingChanged=true;
		}
		strcpy(app_prefs->prefs->incomingdir,buffer.GetData());
	}
	
	if (((wxTextCtrl*)FindWindowById(IDC_TEMPFILES))->GetValue().Length()) {
		buffer=((wxTextCtrl*)FindWindowById(IDC_TEMPFILES))->GetValue();
		if (strcmp(app_prefs->prefs->tempdir,buffer.GetData())!=0) {
			TempChanged=true;
		}
		strcpy(app_prefs->prefs->tempdir,buffer.GetData());
	}

	buffer=((wxTextCtrl*)FindWindowById(IDC_VIDEOPLAYER))->GetValue();
	strcpy(app_prefs->prefs->VideoPlayer ,buffer.GetData());

	app_prefs->prefs->moviePreviewBackup = (((wxCheckBox*)FindWindowById(IDC_VIDEOBACKUP))->IsChecked());
	app_prefs->shareddir_list.Clear();
	m_ShareSelector->GetSharedDirectories(&app_prefs->shareddir_list);

	if (IncomingChanged || TempChanged || m_ShareSelector->HasChanged) {
		theApp.sharedfiles->Reload(true, false);
	}
	
	return TRUE;
}

void CPPgDirectories::Localize(void)
{
}

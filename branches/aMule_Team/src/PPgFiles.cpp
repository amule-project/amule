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
#include <wx/radiobut.h>
#include <wx/checkbox.h>

#include "PPgFiles.h"		// Interface declarations.
#include "Preferences.h"	// Needed for CPreferences
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include "muuli_wdr.h"		// Needed for PreferencesFilesTab

// CPPgFiles dialog

//IMPLEMENT_DYNAMIC(CPPgFiles, CPropertyPage)
IMPLEMENT_DYNAMIC_CLASS(CPPgFiles,wxPanel)
CPPgFiles::CPPgFiles(wxWindow* parent)
: wxPanel(parent,CPPgFiles::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;

	PreferencesFilesTab(this,TRUE);
	book->AddPage(this,_("Files"));
}

CPPgFiles::~CPPgFiles()
{
}

void CPPgFiles::LoadSettings(void)
{	
	switch(app_prefs->prefs->m_iSeeShares) {
		case 0: {
			((wxRadioButton*)FindWindowById(IDC_SEESHARE1))->SetValue(TRUE);
			break;
		}
		case 1: {
			((wxRadioButton*)FindWindowById(IDC_SEESHARE2))->SetValue(TRUE);
			break;
		}
		default: {
			((wxRadioButton*)FindWindowById(IDC_SEESHARE3))->SetValue(TRUE);
			break;
		}
	}

	if(app_prefs->prefs->addnewfilespaused) {
		((wxCheckBox*)FindWindowById(IDC_ADDNEWFILESPAUSED))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_ADDNEWFILESPAUSED))->SetValue(FALSE);
	}
	
	if(app_prefs->prefs->ICH) {
		((wxCheckBox*)FindWindowById(IDC_ICH))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_ICH))->SetValue(FALSE);
	}

	if(app_prefs->prefs->m_bpreviewprio) {
		((wxCheckBox*)FindWindowById(IDC_PREVIEWPRIO))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_PREVIEWPRIO))->SetValue(FALSE);
	}

	if(app_prefs->prefs->m_btransferfullchunks) {
		((wxCheckBox*)FindWindowById(IDC_FULLCHUNKTRANS))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_FULLCHUNKTRANS))->SetValue(FALSE);
	}

	if(app_prefs->prefs->m_bstartnextfile) {
		((wxCheckBox*)FindWindowById(IDC_STARTNEXTFILE))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_STARTNEXTFILE))->SetValue(FALSE);
	}

	if(app_prefs->prefs->m_bUAP) {
		((wxCheckBox*)FindWindowById(IDC_UAP))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_UAP))->SetValue(FALSE);
	}
	
	if(app_prefs->prefs->m_bDAP) {
		((wxCheckBox*)FindWindowById(IDC_DAP))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_DAP))->SetValue(FALSE);
	}
}

void CPPgFiles::OnApply()
{
	if (((wxRadioButton*)FindWindowById(IDC_SEESHARE1))->GetValue()) {
		app_prefs->prefs->m_iSeeShares = 0;
	} else if (((wxRadioButton*)FindWindowById(IDC_SEESHARE2))->GetValue()) {
		app_prefs->prefs->m_iSeeShares = 1;
	} else {
		app_prefs->prefs->m_iSeeShares = 2;
	}
  
	if (((wxCheckBox*)FindWindowById(IDC_PREVIEWPRIO))->IsChecked()) {
		app_prefs->prefs->m_bpreviewprio = true;
	} else {
		app_prefs->prefs->m_bpreviewprio = false;
	}
  
	if (((wxCheckBox*)FindWindowById(IDC_STARTNEXTFILE))->IsChecked()) {
		app_prefs->prefs->m_bstartnextfile = true;
	} else {
		app_prefs->prefs->m_bstartnextfile = false;
	}
  
	if (((wxCheckBox*)FindWindowById(IDC_FULLCHUNKTRANS))->IsChecked()) {
		app_prefs->prefs->m_btransferfullchunks = true;
	} else {
		app_prefs->prefs->m_btransferfullchunks = false;
	}
  
	app_prefs->prefs->addnewfilespaused = (int8)(((wxCheckBox*)FindWindowById(IDC_ADDNEWFILESPAUSED))->IsChecked());
  
	app_prefs->prefs->ICH = (int8)(((wxCheckBox*)FindWindowById(IDC_ICH))->IsChecked());

	app_prefs->prefs->m_bUAP = (((wxCheckBox*)FindWindowById(IDC_UAP))->IsChecked());
	
	app_prefs->prefs->m_bDAP = (((wxCheckBox*)FindWindowById(IDC_DAP))->IsChecked());
  
	LoadSettings();
}


void CPPgFiles::Localize(void)
{
}

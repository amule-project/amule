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


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/checkbox.h>	// Needed for wxCheckBox
#include <wx/notebook.h>
#include <wx/slider.h>
#include <wx/slider.h>

#include "PPgGeneral.h"		// Interface declarations.
#include "Preferences.h"	// Needed for CPreferences
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "muuli_wdr.h"		// Needed for PreferencesGeneralTab
#include "amule.h"		// Needed for Localize_mule

// CPPgGeneral dialog


//IMPLEMENT_DYNAMIC(CPPgGeneral, CPropertyPage)
IMPLEMENT_DYNAMIC_CLASS(CPPgGeneral,wxPanel)

CPPgGeneral::CPPgGeneral(wxWindow* parent)
:wxPanel(parent,CPPgGeneral::IDD) //: CPropertyPage(CPPgGeneral::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;

	PreferencesGeneralTab(this,TRUE);
	book->AddPage(this,_("General"));
}

CPPgGeneral::~CPPgGeneral()
{
}

BEGIN_EVENT_TABLE(CPPgGeneral,wxPanel)
	EVT_SCROLL(CPPgGeneral::OnHScroll)
	EVT_BUTTON(ID_DESKTOPMODE,CPPgGeneral::OnDesktopmode)
END_EVENT_TABLE()

void CPPgGeneral::OnDesktopmode(wxEvent& evt)
{
	theApp.amuledlg->changeDesktopMode();
}

void CPPgGeneral::LoadSettings(void)
{
	// Barry - Controls depth of 3d colour shading
	((wxSlider*)FindWindowById(IDC_3DDEPTH))->SetValue(app_prefs->Get3DDepth());

	((wxSlider*)FindWindowById(IDC_CHECKDAYS))->SetValue(app_prefs->GetUpdateDays());

	((wxTextCtrl*)FindWindowById(IDC_NICK))->SetValue(app_prefs->prefs->nick);

	if(app_prefs->prefs->startMinimized) {
		((wxCheckBox*)FindWindowById(IDC_STARTMIN))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_STARTMIN))->SetValue(FALSE);
	}

	if(app_prefs->prefs->mintotray) {
		((wxCheckBox*)FindWindowById(IDC_MINTRAY))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_MINTRAY))->SetValue(FALSE);
	}
  
	if (app_prefs->prefs->onlineSig) {
		((wxCheckBox*)FindWindowById(IDC_ONLINESIG))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_ONLINESIG))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->beepOnError) {
		((wxCheckBox*)FindWindowById(IDC_BEEPER))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_BEEPER))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->confirmExit) {
		((wxCheckBox*)FindWindowById(IDC_EXIT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_EXIT))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->splashscreen) {
		((wxCheckBox*)FindWindowById(IDC_SPLASHON))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SPLASHON))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->transferDoubleclick) {
		((wxCheckBox*)FindWindowById(IDC_DBLCLICK))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_DBLCLICK))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->bringtoforeground) {
		((wxCheckBox*)FindWindowById(IDC_BRINGTOFOREGROUND))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_BRINGTOFOREGROUND))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->updatenotify) {
		((wxCheckBox*)FindWindowById(IDC_CHECK4UPDATE))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_CHECK4UPDATE))->SetValue(FALSE);
	}
	
	if (app_prefs->prefs->languageID < ((wxChoice*)FindWindowById(IDC_LANGUAGE))->GetCount()) {
		((wxChoice*)FindWindowById(IDC_LANGUAGE))->SetSelection(app_prefs->prefs->languageID);
	} else {
		((wxChoice*)FindWindowById(IDC_LANGUAGE))->SetSelection(1);
	}
	
	// Madcat - Toggle Fast ED2K Links Handler
	((wxCheckBox*)FindWindowById(IDC_FED2KLH))->SetValue(app_prefs->prefs->FastED2KLinksHandler);
  
	CString strBuffer;
	strBuffer.Format("%d", app_prefs->prefs->m_iToolDelayTime);
	((wxSpinCtrl*)FindWindowById(IDC_TOOLTIPDELAY))->SetValue(strBuffer);

	strBuffer.Format("%i Days",app_prefs->prefs->versioncheckdays);
	((wxStaticText*)FindWindowById(IDC_DAYS))->SetLabel(strBuffer);
}

void CPPgGeneral::OnApply()
{
	wxString buffer;
	if (((wxTextCtrl*)FindWindowById(IDC_NICK))->GetValue().Length()) {
		buffer=((wxTextCtrl*)FindWindowById(IDC_NICK))->GetValue();
	} else {
		buffer="Someone using aMule (http://amule.sourceforge.net)";
	}
	strcpy(app_prefs->prefs->nick,buffer);
  
	app_prefs->prefs->startMinimized= (int8)(((wxCheckBox*)FindWindowById(IDC_STARTMIN))->GetValue());
	app_prefs->prefs->mintotray = (int8)(((wxCheckBox*)FindWindowById(IDC_MINTRAY))->GetValue());
	app_prefs->prefs->beepOnError= (int8)(((wxCheckBox*)FindWindowById(IDC_BEEPER))->GetValue());
	app_prefs->prefs->confirmExit= (int8)(((wxCheckBox*)FindWindowById(IDC_EXIT))->GetValue());
	app_prefs->prefs->splashscreen = (int8)(((wxCheckBox*)FindWindowById(IDC_SPLASHON))->GetValue());
	app_prefs->prefs->transferDoubleclick= (int8)(((wxCheckBox*)FindWindowById(IDC_DBLCLICK))->GetValue());
	app_prefs->prefs->bringtoforeground = (int8)(((wxCheckBox*)FindWindowById(IDC_BRINGTOFOREGROUND))->GetValue());
	app_prefs->prefs->updatenotify = (int8)(((wxCheckBox*)FindWindowById(IDC_CHECK4UPDATE))->GetValue());
	app_prefs->prefs->onlineSig= (int8)(((wxCheckBox*)FindWindowById(IDC_ONLINESIG))->GetValue());
	app_prefs->prefs->depth3D = ((wxSlider*)FindWindowById(IDC_3DDEPTH))->GetValue();
  	app_prefs->prefs->versioncheckdays = ((wxSlider*)FindWindowById(IDC_CHECKDAYS))->GetValue();

	// Madcat - Toggle Fast ED2K Links Handler
	app_prefs->prefs->FastED2KLinksHandler = ((wxCheckBox*)FindWindowById(IDC_FED2KLH))->GetValue();
	theApp.amuledlg->ToggleFastED2KLinksHandler();

	WORD newLanguage;	
	newLanguage = ((wxChoice*)FindWindowById(IDC_LANGUAGE))->GetSelection();
	if (newLanguage != app_prefs->prefs->languageID) {
		app_prefs->prefs->languageID = newLanguage;
		//theApp.Localize_mule();
		wxMessageBox(wxString::wxString(_("Language change will not be applied until aMule is restarted.")));
	}
	int bigbobbuffer=(((wxSpinCtrl*)FindWindowById(IDC_TOOLTIPDELAY))->GetValue());
	app_prefs->prefs->m_iToolDelayTime = bigbobbuffer;

	LoadSettings();
}

void CPPgGeneral::Localize(void)
{
}

void CPPgGeneral::OnHScroll(wxScrollEvent& evt)
{
	wxSlider* slider=(wxSlider*)evt.GetEventObject();
	if (slider==((wxSlider*)FindWindowById(IDC_CHECKDAYS))) {
		CString text;
		text.Format("%i Days",evt.GetPosition());
		((wxStaticText*)FindWindowById(IDC_DAYS))->SetLabel(text);
	}
}

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

#include "PPgTweaks.h"		// Interface declarations.
#include "amule.h"		// Needed for theApp.
#include "amuleDlg.h"		// Needed for theApp.
#include "Preferences.h"	// Needed for CPreferences
#include "CString.h"		// Needed for CString
#include "muuli_wdr.h"		// Needed for PreferencesaMuleTweaksTab

// CPPgTweaks dialog

IMPLEMENT_DYNAMIC_CLASS(CPPgTweaks,wxPanel)

BEGIN_EVENT_TABLE(CPPgTweaks,wxPanel)
	EVT_SCROLL(CPPgTweaks::OnHScroll)
	EVT_CHECKBOX(IDC_UPDATEQUEUE,CPPgTweaks::OnUpDownListRefreshChecked)
END_EVENT_TABLE()

CPPgTweaks::CPPgTweaks(wxWindow* parent)
: wxPanel(parent,CPPgTweaks::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;

	PreferencesaMuleTweaksTab(this,TRUE);
	book->AddPage(this,_("Tweaks"));
  
	m_iFileBufferSize = 0;
}

CPPgTweaks::~CPPgTweaks()
{
}

// CPPgTweaks message handlers

bool CPPgTweaks::OnInitDialog()
{
	return TRUE;
}


void CPPgTweaks::LoadSettings(void)
{
	CString strBuffer;

	strBuffer.Format("%d", app_prefs->GetMaxConperFive());
	((wxSpinCtrl*)FindWindowById(IDC_MAXCON5SEC))->SetValue(strBuffer);

	if(app_prefs->prefs->m_bVerbose) {
		((wxCheckBox*)FindWindowById(IDC_VERBOSE))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_VERBOSE))->SetValue(FALSE);
	}
	
	if(app_prefs->prefs->autotakeed2klinks) {
		((wxCheckBox*)FindWindowById(IDC_AUTOTAKEED2KLINKS))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_AUTOTAKEED2KLINKS))->SetValue(FALSE);
	}

	if(app_prefs->prefs->m_bupdatequeuelist) {
		((wxCheckBox*)FindWindowById(IDC_UPDATEQUEUE))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_UPDATEQUEUE))->SetValue(FALSE);
	}

	if(app_prefs->prefs->showRatesInTitle) {
		((wxCheckBox*)FindWindowById(IDC_SHOWRATEONTITLE))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SHOWRATEONTITLE))->SetValue(FALSE);
	}

	((wxSlider*)FindWindowById(IDC_FILEBUFFERSIZE))->SetValue(app_prefs->prefs->m_iFileBufferSize);
	m_iFileBufferSize = app_prefs->prefs->m_iFileBufferSize;

	((wxSlider*)FindWindowById(IDC_QUEUESIZE))->SetValue(app_prefs->prefs->m_iQueueSize);
	m_iQueueSize = app_prefs->prefs->m_iQueueSize;

	((wxSlider*)FindWindowById(IDC_SERVERKEEPALIVE))->SetValue(app_prefs->prefs->m_dwServerKeepAliveTimeoutMins);
	m_uServerKeepAliveTimeout = app_prefs->prefs->m_dwServerKeepAliveTimeoutMins;

	((wxSlider*)FindWindowById(IDC_LISTREFRESH))->SetValue((int) (app_prefs->prefs->m_dwListRefreshSecs));
	
	// must fake 4 scroll events.. setvalue won't cause any events
	wxScrollEvent evt1(0,IDC_QUEUESIZE,app_prefs->prefs->m_iQueueSize);
	evt1.SetEventObject((wxSlider*)FindWindowById(IDC_QUEUESIZE));
	OnHScroll(evt1);

	wxScrollEvent evt2(0,IDC_FILEBUFFERSIZE,app_prefs->prefs->m_iFileBufferSize);
	evt2.SetEventObject((wxSlider*)FindWindowById(IDC_FILEBUFFERSIZE));
	OnHScroll(evt2);

	wxScrollEvent evt3(0,IDC_SERVERKEEPALIVE,app_prefs->prefs->m_dwServerKeepAliveTimeoutMins);
	evt3.SetEventObject((wxSlider*)FindWindowById(IDC_SERVERKEEPALIVE));
	OnHScroll(evt3);

	m_uListRefresh = (uint32) (app_prefs->prefs->m_dwListRefreshSecs);
	wxScrollEvent evt4(0,IDC_LISTREFRESH,(uint32) (app_prefs->prefs->m_dwListRefreshSecs));
	evt4.SetEventObject((wxSlider*)FindWindowById(IDC_LISTREFRESH));
	OnHScroll(evt4);
	
}

bool CPPgTweaks::OnApply()
{
	app_prefs->SetMaxConsPerFive(((wxSpinCtrl*)FindWindowById(IDC_MAXCON5SEC))->GetValue());

	if (((wxCheckBox*)FindWindowById(IDC_VERBOSE))->IsChecked()) {
		app_prefs->prefs->m_bVerbose = true;
	} else {
		app_prefs->prefs->m_bVerbose = false;
	}

	app_prefs->prefs->autotakeed2klinks = (int8)(((wxCheckBox*)FindWindowById(IDC_AUTOTAKEED2KLINKS))->IsChecked());

	if (((wxCheckBox*)FindWindowById(IDC_UPDATEQUEUE))->IsChecked()) {
		app_prefs->prefs->m_bupdatequeuelist = true;
		theApp.amuledlg->transfers_frozen = false;
  		theApp.amuledlg->Thaw_AllTransfering();
	} else {
		app_prefs->prefs->m_bupdatequeuelist = false;
	}

	if (((wxCheckBox*)FindWindowById(IDC_SHOWRATEONTITLE))->IsChecked()) {
		app_prefs->prefs->showRatesInTitle= true;
	} else {
		app_prefs->prefs->showRatesInTitle= false;
	}

	app_prefs->prefs->m_iFileBufferSize = m_iFileBufferSize;
	app_prefs->prefs->m_iQueueSize = m_iQueueSize;
	app_prefs->prefs->m_dwServerKeepAliveTimeoutMins = m_uServerKeepAliveTimeout;
	app_prefs->prefs->m_dwListRefreshSecs = m_uListRefresh;

	if (!theApp.glob_prefs->ShowRatesOnTitle()) {
		// sprintf(buffer,"aMule %s",CURRENT_VERSION_LONG);
		// theApp.amuledlg->SetWindowText(buffer);
	}

	return TRUE;
}

void CPPgTweaks::OnHScroll(wxScrollEvent& evt)
{
	wxSlider* slider=(wxSlider*)evt.GetEventObject();
	CString temp;
	if (slider==((wxSlider*)FindWindowById(IDC_FILEBUFFERSIZE))) {
		m_iFileBufferSize = evt.GetPosition();
		temp.Format(_("File Buffer Size %i bytes"), m_iFileBufferSize*15000 );
		((wxStaticText*)FindWindowById(IDC_FILEBUFFERSIZE_STATIC))->SetLabel(temp);
	} else if (slider==((wxSlider*)FindWindowById(IDC_QUEUESIZE))) {
		m_iQueueSize = evt.GetPosition();
		temp.Format(_("Upload Queue Size %i clients"), m_iQueueSize*100 );
		((wxStaticText*)FindWindowById(IDC_QUEUESIZE_STATIC))->SetLabel(temp);
	} else if (slider==((wxSlider*)FindWindowById(IDC_SERVERKEEPALIVE))) {
		m_uServerKeepAliveTimeout = evt.GetPosition();
		if (m_uServerKeepAliveTimeout == 0) {
			((wxStaticText*)FindWindowById(IDC_SERVERKEEPALIVE_LABEL))->SetLabel(_("Server connection refresh interval: Disabled"));
		} else if (m_uServerKeepAliveTimeout == 1) {
			temp.Format(_("Server connection refresh interval %i min"), m_uServerKeepAliveTimeout);
			((wxStaticText*)FindWindowById(IDC_SERVERKEEPALIVE_LABEL))->SetLabel(temp);
		} else {
			temp.Format(_("Server connection refresh interval %i mins"), m_uServerKeepAliveTimeout);
			((wxStaticText*)FindWindowById(IDC_SERVERKEEPALIVE_LABEL))->SetLabel(temp);
		}
	} else if (slider==((wxSlider*)FindWindowById(IDC_LISTREFRESH))) {
		m_uListRefresh = evt.GetPosition();
		if (((wxCheckBox*)FindWindowById(IDC_UPDATEQUEUE))->IsChecked()) {
			((wxStaticText*)FindWindowById(IDC_LISTREFRESH_LABEL))->SetLabel(_("Upload/Download list refresh time: Disable"));
		} else {
			if (m_uListRefresh == 0) {
				((wxStaticText*)FindWindowById(IDC_LISTREFRESH_LABEL))->SetLabel(_("Upload/Download list refresh time: Realtime"));
			} else if (m_uListRefresh == 1) {
				temp.Format(_("Upload/Download list refresh time: %i sec"), m_uListRefresh);
				((wxStaticText*)FindWindowById(IDC_LISTREFRESH_LABEL))->SetLabel(temp);
			} else {
				temp.Format(_("Upload/Download list refresh time: %i secs"), m_uListRefresh);
				((wxStaticText*)FindWindowById(IDC_LISTREFRESH_LABEL))->SetLabel(temp);
			}
		}
	}	
}

void CPPgTweaks::Localize(void)
{	
}

void CPPgTweaks::OnUpDownListRefreshChecked(wxEvent& evt) 
{
	((wxSlider*)FindWindowById(IDC_LISTREFRESH))->SetValue((uint32) (app_prefs->prefs->m_dwListRefreshSecs));
	wxScrollEvent evt4(0,IDC_LISTREFRESH,(uint32) (app_prefs->prefs->m_dwListRefreshSecs));
	evt4.SetEventObject((wxSlider*)FindWindowById(IDC_LISTREFRESH));
	OnHScroll(evt4);
}

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
#include <wx/msgdlg.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/slider.h>

#include "PPgConnection.h"	// Interface declarations
#include "Wizard.h"		// Needed for Wizard
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CamuleAppBase.h"	// Needed for theApp
#include "opcodes.h"		// Needed for UNLIMITED
#include "Preferences.h"	// Needed for CPreferences
#include "CString.h"	// Needed for CString
#include "muuli_wdr.h"		// Needed for PreferencesConnectionTab

//IMPLEMENT_DYNAMIC(CPPgConnection, CPropertyPage)

IMPLEMENT_DYNAMIC_CLASS(CPPgConnection,wxPanel)

CPPgConnection::CPPgConnection(wxWindow*parent)
:wxPanel(parent,CPPgConnection::IDD) //: CPropertyPage(CPPgConnection::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;
	PreferencesConnectionTab(this,TRUE);
	book->AddPage(this,_("Connection"));
}

CPPgConnection::~CPPgConnection()
{
}

BEGIN_EVENT_TABLE(CPPgConnection,wxPanel)
	//EVT_BUTTON(IDC_WIZARD,CPPgConnection::OnBnClickedWizard)
	EVT_CHECKBOX(IDC_UDPDISABLE,CPPgConnection::OnUDPDISABLEChecked)
END_EVENT_TABLE()


// CPPgConnection message handlers


void CPPgConnection::LoadSettings(void)
{
	CString strBuffer;

	strBuffer.Format("%d", app_prefs->prefs->udpport);
	((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetValue(strBuffer);

	if (app_prefs->prefs->udpport==0) {
		((wxCheckBox*)FindWindowById(IDC_UDPDISABLE))->SetValue(TRUE);
		((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetRange(0,0);
	} else {
		((wxCheckBox*)FindWindowById(IDC_UDPDISABLE))->SetValue(FALSE);
		((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetRange(1025,65535);
	}
		
	strBuffer.Format("%d", app_prefs->prefs->maxGraphDownloadRate);
	((wxSpinCtrl*)FindWindowById(IDC_DOWNLOAD_CAP))->SetValue(strBuffer);
      
	strBuffer.Format("%d", app_prefs->prefs->maxGraphUploadRate);
	((wxSpinCtrl*)FindWindowById(IDC_UPLOAD_CAP))->SetValue(strBuffer);
      
	if (app_prefs->prefs->maxdownload == UNLIMITED) {
		((wxSpinCtrl*)FindWindowById(IDC_MAXDOWN))->SetValue("0");
	} else if (app_prefs->prefs->maxdownload > app_prefs->prefs->maxGraphDownloadRate) {
		((wxSpinCtrl*)FindWindowById(IDC_MAXDOWN))->SetValue(app_prefs->prefs->maxGraphDownloadRate);
	} else {
		((wxSpinCtrl*)FindWindowById(IDC_MAXDOWN))->SetValue(app_prefs->prefs->maxdownload);
	}

	if (app_prefs->prefs->maxupload == UNLIMITED) {
		((wxSpinCtrl*)FindWindowById(IDC_MAXUP))->SetValue("0");
	} else if (app_prefs->prefs->maxupload > app_prefs->prefs->maxGraphUploadRate) {
		((wxSpinCtrl*)FindWindowById(IDC_MAXUP))->SetValue(app_prefs->prefs->maxGraphUploadRate);
	} else {
		((wxSpinCtrl*)FindWindowById(IDC_MAXUP))->SetValue(app_prefs->prefs->maxupload);
	}
      
	if (app_prefs->prefs->maxupload != 0 && app_prefs->prefs->maxupload != UNLIMITED) {
		if(app_prefs->prefs->maxupload < 4 && (app_prefs->prefs->maxupload*3 < app_prefs->prefs->maxdownload)) {
			app_prefs->prefs->maxdownload = app_prefs->prefs->maxupload*3;
		}
		if (app_prefs->prefs->maxupload < 10 && (app_prefs->prefs->maxupload*4 < app_prefs->prefs->maxdownload)) {
			app_prefs->prefs->maxdownload = app_prefs->prefs->maxupload*4;
		}
	}
      
	strBuffer.Format("%d", app_prefs->prefs->slotallocation);
	((wxSpinCtrl*)FindWindowById(IDC_SLOTALLOC))->SetValue(strBuffer);

	strBuffer.Format("%d", app_prefs->prefs->port);
	((wxSpinCtrl*)FindWindowById(IDC_PORT))->SetValue(strBuffer);
      
	strBuffer.Format("%d", app_prefs->prefs->maxconnections);
	((wxSpinCtrl*)FindWindowById(IDC_MAXCON))->SetValue(strBuffer);

	strBuffer.Format("%d", app_prefs->prefs->maxsourceperfile);
	((wxSpinCtrl*)FindWindowById(IDC_MAXSOURCEPERFILE))->SetValue(strBuffer);

	if (app_prefs->prefs->reconnect) {
		((wxCheckBox*)FindWindowById(IDC_RECONN))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_RECONN))->SetValue(FALSE);
	}

	if (app_prefs->prefs->m_bshowoverhead) {
		((wxCheckBox*)FindWindowById(IDC_SHOWOVERHEAD))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SHOWOVERHEAD))->SetValue(FALSE);
	}

	if (app_prefs->prefs->autoconnect) {
		((wxCheckBox*)FindWindowById(IDC_AUTOCONNECT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_AUTOCONNECT))->SetValue(FALSE);
	}
}

void CPPgConnection::OnApply()
{
	wxString buffer;
	int lastmaxgu = app_prefs->prefs->maxGraphUploadRate;
	int lastmaxgd = app_prefs->prefs->maxGraphDownloadRate;
  
	app_prefs->prefs->maxGraphDownloadRate = ((wxSpinCtrl*)FindWindowById(IDC_DOWNLOAD_CAP))->GetValue();

	app_prefs->prefs->maxGraphUploadRate = ((wxSpinCtrl*)FindWindowById(IDC_UPLOAD_CAP))->GetValue();

	if (((wxSpinCtrl*)FindWindowById(IDC_MAXUP))->GetValue() == 0) {
		app_prefs->prefs->maxupload = UNLIMITED;
	} else if (((wxSpinCtrl*)FindWindowById(IDC_MAXUP))->GetValue() > ((wxSpinCtrl*)FindWindowById(IDC_UPLOAD_CAP))->GetValue()) {
		app_prefs->prefs->maxupload = ((wxSpinCtrl*)FindWindowById(IDC_UPLOAD_CAP))->GetValue();
	} else {
		app_prefs->prefs->maxupload = ((wxSpinCtrl*)FindWindowById(IDC_MAXUP))->GetValue();
	}

	if (((wxSpinCtrl*)FindWindowById(IDC_MAXDOWN))->GetValue() == 0) {
		app_prefs->prefs->maxdownload = UNLIMITED;
	} else if (((wxSpinCtrl*)FindWindowById(IDC_MAXDOWN))->GetValue() > ((wxSpinCtrl*)FindWindowById(IDC_DOWNLOAD_CAP))->GetValue()) {
		app_prefs->prefs->maxdownload = ((wxSpinCtrl*)FindWindowById(IDC_DOWNLOAD_CAP))->GetValue();
	} else {
		app_prefs->prefs->maxdownload = ((wxSpinCtrl*)FindWindowById(IDC_MAXDOWN))->GetValue();
	}

	app_prefs->prefs->port = ((wxSpinCtrl*)FindWindowById(IDC_PORT))->GetValue();

	app_prefs->prefs->slotallocation = ((wxSpinCtrl*)FindWindowById(IDC_SLOTALLOC))->GetValue();

	app_prefs->prefs->maxsourceperfile = ((wxSpinCtrl*)FindWindowById(IDC_MAXSOURCEPERFILE))->GetValue();

	app_prefs->prefs->udpport = ((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->GetValue();

	if (((wxCheckBox*)FindWindowById(IDC_SHOWOVERHEAD))->IsChecked()) {
		app_prefs->prefs->m_bshowoverhead = true;
	} else {
		app_prefs->prefs->m_bshowoverhead = false;
	}

	app_prefs->prefs->autoconnect = (int8)(((wxCheckBox*)FindWindowById(IDC_AUTOCONNECT))->IsChecked());

	app_prefs->prefs->reconnect = (int8)(((wxCheckBox*)FindWindowById(IDC_RECONN))->IsChecked());
  
	if (lastmaxgu != app_prefs->prefs->maxGraphUploadRate) {
		theApp.amuledlg->statisticswnd->SetARange(false,app_prefs->prefs->maxGraphUploadRate);
	}

	if (lastmaxgd!=app_prefs->prefs->maxGraphDownloadRate) {
		theApp.amuledlg->statisticswnd->SetARange(true,app_prefs->prefs->maxGraphDownloadRate);
	}

	uint16 tempcon = ((wxSpinCtrl*)FindWindowById(IDC_MAXCON))->GetValue();

	app_prefs->prefs->maxconnections = tempcon;
   
	LoadSettings();
}

void CPPgConnection::Localize(void)
{	
}

void CPPgConnection::OnBnClickedWizard(wxEvent& evt)
{
	Wizard test(this);
	test.SetPrefs(app_prefs);
	test.OnInitDialog();
	test.ShowModal();
}

void CPPgConnection::OnUDPDISABLEChecked(wxEvent& evt) 
{
	if (((wxCheckBox*)FindWindowById(IDC_UDPDISABLE))->IsChecked()) {
		((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetRange(0,0);
		((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetValue("0");
	} else {
		((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetRange(1025,65535);
		((wxSpinCtrl*)FindWindowById(IDC_UDPPORT))->SetValue("4672");
	}
}

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
#include <wx/checkbox.h>

#include "PPgServer.h"		// Interface declarations.
#include "EditServerListDlg.h"	// Needed for EditServerListDlg
#include "CamuleAppBase.h"	// Needed for theApp
#include "Preferences.h"	// Needed for CPreferences
#include "muuli_wdr.h"		// Needed for PreferencesServerTab

// CPPgServer dialog

//IMPLEMENT_DYNAMIC(CPPgServer, CPropertyPage)
IMPLEMENT_DYNAMIC_CLASS(CPPgServer,wxPanel)

CPPgServer::CPPgServer(wxWindow* parent)
:wxPanel(parent,CPPgServer::IDD) //: CPropertyPage(CPPgServer::IDD)
{
	wxNotebook* book=(wxNotebook*)parent;
	PreferencesServerTab(this,TRUE);
	book->AddPage(this,_("Server"));
}

CPPgServer::~CPPgServer()
{
}

BEGIN_EVENT_TABLE(CPPgServer,wxPanel)
	EVT_BUTTON(IDC_EDITADR, CPPgServer::OnBnClickedEditadr)
END_EVENT_TABLE()

// CPPgServer message handlers

void CPPgServer::LoadSettings(void)
{
	if(app_prefs->prefs->deadserver) {
		((wxCheckBox*)FindWindowById(IDC_REMOVEDEAD))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_REMOVEDEAD))->SetValue(FALSE);
	}
  
	CString strBuffer;
	strBuffer.Format("%d", app_prefs->prefs->deadserverretries);
	((wxSpinCtrl*)FindWindowById(IDC_SERVERRETRIES))->SetValue(strBuffer);
  
  
	if(app_prefs->IsSafeServerConnectEnabled()) {
		((wxCheckBox*)FindWindowById(IDC_SAFESERVERCONNECT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SAFESERVERCONNECT))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->m_bmanualhighprio) {
		((wxCheckBox*)FindWindowById(IDC_MANUALSERVERHIGHPRIO))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_MANUALSERVERHIGHPRIO))->SetValue(FALSE);
	}
  
	if(app_prefs->GetSmartIdCheck()) {
		((wxCheckBox*)FindWindowById(IDC_SMARTIDCHECK))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SMARTIDCHECK))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->autoserverlist) {
		((wxCheckBox*)FindWindowById(IDC_AUTOSERVER))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_AUTOSERVER))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->addserversfromserver) {
		((wxCheckBox*)FindWindowById(IDC_UPDATESERVERCONNECT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_UPDATESERVERCONNECT))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->addserversfromclient) {
		((wxCheckBox*)FindWindowById(IDC_UPDATESERVERCLIENT))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_UPDATESERVERCLIENT))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->filterBadIP) {
		((wxCheckBox*)FindWindowById(IDC_FILTER))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_FILTER))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->scorsystem) {
		((wxCheckBox*)FindWindowById(IDC_SCORE))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_SCORE))->SetValue(FALSE);
	}
  
	if(app_prefs->prefs->autoconnectstaticonly) {
		((wxCheckBox*)FindWindowById(IDC_AUTOCONNECTSTATICONLY))->SetValue(TRUE);
	} else {
		((wxCheckBox*)FindWindowById(IDC_AUTOCONNECTSTATICONLY))->SetValue(FALSE);
	}
}

void CPPgServer::OnApply()
{	
	app_prefs->SetSafeServerConnectEnabled((int8)(((wxCheckBox*)FindWindowById(IDC_SAFESERVERCONNECT))->IsChecked()));

	if (((wxCheckBox*)FindWindowById(IDC_SMARTIDCHECK))->IsChecked()) {
		app_prefs->prefs->smartidcheck = true;
	} else {
		app_prefs->prefs->smartidcheck = false;
	}

	if (((wxCheckBox*)FindWindowById(IDC_MANUALSERVERHIGHPRIO))->IsChecked()) {
		app_prefs->prefs->m_bmanualhighprio = true;
	} else {
		app_prefs->prefs->m_bmanualhighprio = false;
	}

	app_prefs->prefs->deadserver = (int8)(((wxCheckBox*)FindWindowById(IDC_REMOVEDEAD))->IsChecked());

	app_prefs->prefs->deadserverretries = ((wxSpinCtrl*)FindWindowById(IDC_SERVERRETRIES))->GetValue();

	app_prefs->prefs->scorsystem = (int8)(((wxCheckBox*)FindWindowById(IDC_SCORE))->IsChecked());

	app_prefs->prefs->autoserverlist = (int8)(((wxCheckBox*)FindWindowById(IDC_AUTOSERVER))->IsChecked());

	app_prefs->prefs->addserversfromserver = (int8)(((wxCheckBox*)FindWindowById(IDC_UPDATESERVERCONNECT))->IsChecked());

	app_prefs->prefs->addserversfromclient = (int8)(((wxCheckBox*)FindWindowById(IDC_UPDATESERVERCLIENT))->IsChecked());

	app_prefs->prefs->filterBadIP = (int8)(((wxCheckBox*)FindWindowById(IDC_FILTER))->IsChecked());

	app_prefs->prefs->autoconnectstaticonly = (int8)(((wxCheckBox*)FindWindowById(IDC_AUTOCONNECTSTATICONLY))->IsChecked());

	LoadSettings();
}


void CPPgServer::Localize(void)
{
}

void CPPgServer::OnBnClickedEditadr(wxEvent& evt)
{
	char* fullpath = new char[strlen(theApp.glob_prefs->GetAppDir())+13];
	sprintf(fullpath,"%saddresses.dat",theApp.glob_prefs->GetAppDir());

	EditServerListDlg* test=new EditServerListDlg(this, _("Edit Serverlist"),
	_("Add here URL's to download server.met files.\nOnly one url on each line."), fullpath);
	test->ShowModal();
  
	delete[] fullpath;
	delete test;
}

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


#include "PreferencesDlg.h"	// Interface declarations
#include "Preferences.h"	// Needed for CPreferences
#include "CamuleAppBase.h"	// Needed for theApp
#include "PPgGuiTweaks.h"	// Needed for CPPgGuiTweaks
#include "PPgSourcesDropping.h"	// Needed for CPPgSourcesDropping
#include "PPgTweaks.h"		// Needed for CPPgTweaks
#include "PPgNotify.h"		// Needed for CPPgNotify
#include "PPgStats.h"		// Needed for CPPgStats
#include "PPgDirectories.h"	// Needed for CPPgDirectories
#include "PPgFiles.h"		// Needed for CPPgFiles
#include "PPgServer.h"		// Needed for CPPgServer
#include "PPgConnection.h"	// Needed for CPPgConnection
#include "PPgGeneral.h"		// Needed for CPPgGeneral
#include "muuli_wdr.h"		// Needed for ID_OK

// CPreferencesDlg

//IMPLEMENT_DYNAMIC(CPreferencesDlg, CPropertySheet)
IMPLEMENT_DYNAMIC_CLASS(CPreferencesDlg,wxDialog)

BEGIN_EVENT_TABLE(CPreferencesDlg,wxDialog)
	EVT_BUTTON(ID_OK,CPreferencesDlg::OnBtnOk)
	EVT_BUTTON(ID_CANCEL,CPreferencesDlg::OnBtnCancel)
END_EVENT_TABLE()

CPreferencesDlg::CPreferencesDlg(wxWindow* parent,CPreferences* prefs) :
    wxDialog(parent,9999,_("OLD Preferences"),wxDefaultPosition,wxDefaultSize,
    wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	wxNotebook* book=new wxNotebook(this,7773,wxPoint(0,0),wxSize(200,200));

	// load pages
	m_wndGeneral=new CPPgGeneral(book);
	m_wndConnection=new CPPgConnection(book);
	m_wndServer=new CPPgServer(book);
	m_wndFiles=new CPPgFiles(book);
	m_wndDirectories=new CPPgDirectories(book);
	m_wndStats=new CPPgStats(book);
	m_wndNotify=new CPPgNotify(book);
	//m_wndIRC=new CPPgIRC();
	//m_wndIRC=NULL;
	m_wndTweaks=new CPPgTweaks(book);
	m_wndSourcesDropping = new CPPgSourcesDropping(book);  // Creating Sources Dropping window
	m_wndGuiTweaks = new CPPgGuiTweaks(book); // Creating GUI Tweaks window

	int bigbob1=m_wndSourcesDropping->GetSize().GetWidth();
	// int bigbob2=(m_wndGeneral->GetSize().GetHeight()+41);
	int bigbob2=(m_wndTweaks->GetSize().GetHeight()+41);
	wxSize pageSize(bigbob1,bigbob2);
	SetSize(pageSize.GetWidth(),pageSize.GetHeight()+40);
	book->SetSize(pageSize);

	// then the ok/cancel buttons please
	new wxButton(this,ID_OK,_("OK"),
	    wxPoint(pageSize.GetWidth()-2*80-16,pageSize.GetHeight()+10),wxSize(80,24));
	new wxButton(this,ID_CANCEL,_("Cancel"),
	    wxPoint(pageSize.GetWidth()-80-8,pageSize.GetHeight()+10),wxSize(80,24));

	SetPrefs(prefs);

	Fit();
	CentreOnParent();
}

CPreferencesDlg::~CPreferencesDlg()
{
}

int CPreferencesDlg::ShowModal()
{
	// setup pages
	m_wndGeneral->LoadSettings();
	m_wndFiles->LoadSettings();
	m_wndServer->LoadSettings();
	m_wndConnection->LoadSettings();
	m_wndDirectories->LoadSettings();
	m_wndStats->LoadSettings();
	m_wndNotify->LoadSettings();
	m_wndTweaks->LoadSettings(); 
	m_wndSourcesDropping->LoadSettings();  // Load settings in Dropping Sources window
	m_wndGuiTweaks->LoadSettings(); // Load Settings in GUI Tweaks
	// and then do the show
	return wxDialog::ShowModal();
}


void CPreferencesDlg::OnBtnOk(wxEvent& evt)
{
	// apply all pages
	m_wndGeneral->OnApply();
	m_wndConnection->OnApply();
	m_wndServer->OnApply();
	m_wndFiles->OnApply();
	m_wndDirectories->OnApply();
	m_wndStats->OnApply();
	m_wndNotify->OnApply();
	m_wndTweaks->OnApply();
	m_wndSourcesDropping->OnApply();  // OnApply in Sources Dropping 
	//m_wndGuiTweaks->OnApply(); // OnApply in GUI Tweaks
	app_prefs->Save();
	EndModal(0);
}

void CPreferencesDlg::OnBtnCancel(wxEvent& evt)
{
	m_wndStats->OnCancel();
	EndModal(0);
}

void CPreferencesDlg::SetPrefs(CPreferences* in_prefs)
{
	app_prefs = in_prefs;
	m_wndGeneral->SetPrefs(in_prefs);
	m_wndConnection->SetPrefs(in_prefs);
	m_wndServer->SetPrefs(in_prefs);
	m_wndDirectories->SetPrefs(in_prefs);
	m_wndFiles->SetPrefs(in_prefs);
	m_wndStats->SetPrefs(in_prefs);
	m_wndNotify->SetPrefs(in_prefs);
	//m_wndIRC->SetPrefs(in_prefs);
	m_wndTweaks->SetPrefs(in_prefs);
	m_wndSourcesDropping->SetPrefs(in_prefs);	// Setting preferences to Sources Dropping
	m_wndGuiTweaks->SetPrefs(in_prefs);		// Setting preferences to GUI Tweaks
}

void CPreferencesDlg::Localize(void)
{
#if 0
	// start changed by InterCeptor (localization) 11.11.02 
	TC_ITEM item; 
	item.mask = TCIF_TEXT; 

	CStringArray buffer; 
	buffer.Add(GetResString(IDS_PW_GENERAL)); 
	buffer.Add(GetResString(IDS_PW_CONNECTION)); 
	buffer.Add(GetResString(IDS_PW_SERVER)); 
	buffer.Add(GetResString(IDS_PW_DIR)); 
	buffer.Add(GetResString(IDS_PW_FILES)); 
	buffer.Add(GetResString(IDS_PW_EKDEV_OPTIONS)); 
	buffer.Add(GetResString(IDS_STATSSETUPINFO)); 
	buffer.Add("IRC");

	for (int i = 0; i <= GetTabControl()->GetItemCount()-1; i++) { 
	item.pszText = buffer[i].GetBuffer(); 
	GetTabControl()->SetItem (i, &item); 
	buffer[i].ReleaseBuffer(); 
	} 
	// end changed by InterCeptor (localization) 11.11.02 
#endif
	//SetWindowText(GetResString(IDS_PREFERENCES_CAPTION));
	m_wndGeneral->Localize();
	m_wndConnection->Localize();
	m_wndServer->Localize();
	m_wndDirectories->Localize();
	m_wndFiles->Localize();
	m_wndStats->Localize();
	m_wndNotify->Localize();
	//m_wndIRC->Localize();
	m_wndTweaks->Localize();
	m_wndSourcesDropping->Localize();  // Localize Source Dropping window
	m_wndGuiTweaks->Localize(); // Localize GUI Tweaks window
}

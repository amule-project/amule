// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


// ServerWnd.cpp : implementation file
//


#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>

#include "muuli_wdr.h"		// Needed for ID_ADDTOLIST
#include "ServerWnd.h"		// Interface declarations.
#include "GetTickCount.h"	// Needed for GetTickCount
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "HTTPDownloadDlg.h"	// Needed for CHTTPDownloadDlg
#include "otherfunctions.h"	// Needed for GetTickCount
#include "Preferences.h"	// Needed for CPreferences
#include "sockets.h"
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for VERSION
#endif

//IMPLEMENT_DYNAMIC(CServerWnd, CDialog)
IMPLEMENT_DYNAMIC_CLASS(CServerWnd,wxPanel)

BEGIN_EVENT_TABLE(CServerWnd,wxPanel)
	EVT_BUTTON(ID_ADDTOLIST,CServerWnd::OnBnClickedAddserver)
	EVT_BUTTON(ID_UPDATELIST,CServerWnd::OnBnClickedUpdateservermetfromurl)
	EVT_TEXT_ENTER(IDC_SERVERLISTURL,CServerWnd::OnBnClickedUpdateservermetfromurl)
	EVT_BUTTON(ID_BTN_RESET, CServerWnd::OnBnClickedResetLog)
	EVT_BUTTON(ID_BTN_RESET_SERVER, CServerWnd::OnBnClickedResetServerLog)
	EVT_SPLITTER_SASH_POS_CHANGED(ID_SRV_SPLITTER,CServerWnd::OnSashPositionChanged)
END_EVENT_TABLE()

CServerWnd::CServerWnd(wxWindow* pParent /*=NULL*/)
: wxPanel(pParent, -1)
{
	wxSizer* content=serverListDlg(this,TRUE);
	content->Show(this,TRUE);

	// init serverlist
	// no use now. too early.
	CServerListCtrl* list=(CServerListCtrl*)FindWindowById(ID_SERVERLIST);
	serverlistctrl=list;

	wxTextCtrl* cv=(wxTextCtrl*)FindWindowById(ID_SERVERINFO);
	cv->AppendText(wxT("This is aMule")+wxString(wxT(" "))+wxString(wxT(VERSION))+wxT(" (based on eMule)\n"));
	cv->AppendText(wxT("Visit http://www.amule.org to check if a new version is available.\n"));

	// Insert two columns, currently without a header
	wxListCtrl* MyInfoList = (wxListCtrl*)FindWindow(ID_MYSERVINFO);
	MyInfoList->InsertColumn(0, wxT(""));	
	MyInfoList->InsertColumn(1, wxT(""));	
}

CServerWnd::~CServerWnd()
{
}

void CServerWnd::UpdateServerMetFromURL(wxString strURL)
{
	if (strURL.Find(wxT("://")) == -1) {
		AddLogLineM(true, _("Invalid URL"));
		return;
	}
	wxString strTempFilename(theApp.ConfigDir + wxString::Format(wxT("temp-%d-server.met"), ::GetTickCount()));
	CHTTPDownloadDlg *dlg=new CHTTPDownloadDlg(this,strURL,strTempFilename);
	int retval=dlg->ShowModal();
	if(retval==0) {
		// curl succeeded. proceed with serverlist processing
		serverlistctrl->AddServermetToList(strTempFilename);
		wxRemoveFile(strTempFilename);
		theApp.serverlist->SaveServermetToFile();
		printf("Saving of server.met file Done !!!\n");
	} else {
		AddLogLineM(true, wxString::Format(_("Failed to download the serverlist from %s"), strURL.c_str()));
	}
	delete dlg;
}

void CServerWnd::Localize()
{
}

// CServerWnd message handlers

void CServerWnd::OnBnClickedAddserver(wxCommandEvent& WXUNUSED(evt))
{
	wxString serveraddr;
	if(((wxTextCtrl*)FindWindowById(IDC_IPADDRESS))->GetLineText(0).IsEmpty()) {
		AddLogLineM(true, _("Please enter a serveraddress"));
		return;
	} else {
		serveraddr=((wxTextCtrl*)FindWindowById(IDC_IPADDRESS))->GetLineText(0);
	}
	if (((wxTextCtrl*)FindWindowById(IDC_SPORT))->GetLineText(0).IsEmpty()) {
		AddLogLineM(true, _("Incomplete serverport: Please enter a serverport"));
		return;
	}
  
	wxString portstr;
	portstr=((wxTextCtrl*)FindWindowById(IDC_SPORT))->GetLineText(0);
	CServer* toadd = new CServer(atoi(unicode2char(portstr)),serveraddr);
	wxString servername;
	servername=((wxTextCtrl*)FindWindowById(IDC_SERVERNAME))->GetLineText(0);
	if (!servername.IsEmpty()) {
		toadd->SetListName(servername);
	} else {
		toadd->SetListName(serveraddr);
	}
	if (!theApp.AddServer(toadd)) {
		CServer* update = theApp.serverlist->GetServerByAddress(toadd->GetAddress(), toadd->GetPort());
		if(update) {
			update->SetListName(toadd->GetListName());
			serverlistctrl->RefreshServer(update);
		}
		delete toadd;
		AddLogLineM(true, _("Server not added!"));
	} else {
		AddLogLineM(true, _("Server added: ") + toadd->GetListName());
	}
	theApp.serverlist->SaveServermetToFile();
	printf("Saving of server.met file Done !!!\n");
	((wxTextCtrl*)FindWindowById(IDC_SERVERNAME))->SetValue(wxEmptyString);
	((wxTextCtrl*)FindWindowById(IDC_IPADDRESS))->SetValue(wxEmptyString);
	((wxTextCtrl*)FindWindowById(IDC_SPORT))->SetValue(wxEmptyString);
}

void CServerWnd::OnBnClickedUpdateservermetfromurl(wxCommandEvent& WXUNUSED(evt))
{
	wxString strURL;
	strURL=((wxTextCtrl*)FindWindowById(IDC_SERVERLISTURL))->GetLineText(0);
	UpdateServerMetFromURL(strURL);
}

void CServerWnd::OnBnClickedResetLog(wxCommandEvent& WXUNUSED(evt))
{
	theApp.amuledlg->ResetLog(1);
}

void CServerWnd::OnBnClickedResetServerLog(wxCommandEvent& WXUNUSED(evt))
{
	theApp.amuledlg->ResetLog(2);
}

void CServerWnd::UpdateMyInfo()
{
	wxListCtrl* MyInfoList = (wxListCtrl*)FindWindow(ID_MYSERVINFO);
	
	wxString buffer;

	MyInfoList->DeleteAllItems();
	MyInfoList->InsertItem(0, wxString(_("Status"))+":");
	if (theApp.serverconnect->IsConnected()) {
		MyInfoList->SetItem(0, 1, _("Connected"));
	}	else {
		MyInfoList->SetItem(0, 1, _("Disconnected"));
	}

	if (theApp.serverconnect->IsConnected()) {
		MyInfoList->InsertItem(1, wxString(_("IP")) +":"+ wxString(_("Port")));
		if (theApp.serverconnect->IsLowID()) {
			buffer=_("Unknown"); 
		} else {
			uint32 myid=theApp.serverconnect->GetClientID();
			uint8 d=myid/(256*256*256);myid-=d*(256*256*256);
			uint8 c=myid/(256*256);myid-=c*256*256;
			uint8 b=myid/(256);myid-=b*256;
			buffer.Printf("%i.%i.%i.%i:%i",myid,b,c,d,theApp.glob_prefs->GetPort());
		}
		MyInfoList->SetItem(1,1,buffer);

		buffer.Printf("%u",theApp.serverconnect->GetClientID());
		MyInfoList->InsertItem(2,_("ID"));
		if (theApp.serverconnect->IsConnected()) {
			MyInfoList->SetItem(2, 1, buffer);
		}

		MyInfoList->InsertItem(3,"");
		if (theApp.serverconnect->IsLowID()) {
			MyInfoList->SetItem(3, 1,_("Low ID"));
		}	else {
			MyInfoList->SetItem(3, 1,_("High ID"));
		}
	}

	// Fit the width of the columns
	MyInfoList->SetColumnWidth(0, -1);
	MyInfoList->SetColumnWidth(1, -1);
}

void CServerWnd::OnSashPositionChanged(wxSplitterEvent& WXUNUSED(evt))
{
	theApp.amuledlg->srv_split_pos = ((wxSplitterWindow*)FindWindow(wxT("SrvSplitterWnd")))->GetSashPosition();
}

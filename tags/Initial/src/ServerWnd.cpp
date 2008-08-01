//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://sourceforge.net/projects/amule )
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

#include "muuli_wdr.h"		// Needed for ID_ADDTOLIST
#include "ServerWnd.h"		// Interface declarations.
#include "GetTickCount.h"	// Needed for GetTickCount
#include "server.h"		// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "HTTPDownloadDlg.h"	// Needed for CHTTPDownloadDlg
#include "otherfunctions.h"	// Needed for GetTickCount
#include "Preferences.h"	// Needed for CPreferences
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CString.h"		// Needed for CString
#include "CamuleAppBase.h"	// Needed for theApp

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
	EVT_SPLITTER_SASH_POS_CHANGED(ID_SRV_SPLITTER,CServerWnd::OnSashPositionChanged)
END_EVENT_TABLE()

CServerWnd::CServerWnd(wxWindow* pParent /*=NULL*/)
: wxPanel(pParent,CServerWnd::IDD)
{
	wxSizer* content=serverListDlg(this,TRUE);
	content->Show(this,TRUE);

	// init serverlist
	// no use now. too early.
	CServerListCtrl* list=(CServerListCtrl*)FindWindowById(ID_SERVERLIST);
	serverlistctrl=list;

	wxTextCtrl* cv=(wxTextCtrl*)FindWindowById(ID_SERVERINFO);
	cv->AppendText(wxString(_("This is "))+wxString("aMule")+wxString(" ")+wxString(VERSION)+wxString(_(" (based on "))+wxString("eMule")+wxString(")\n"));
	cv->AppendText(wxString(_("Visit http://www.amule.org to check if a new version is available.\n")));
}

CServerWnd::~CServerWnd()
{
}

void CServerWnd::UpdateServerMetFromURL(wxString strURL)
{
	if (strURL.Find("://") == -1) {
		theApp.amuledlg->AddLogLine(true, CString(_("Invalid URL")));
		return;
	}
	wxString strTempFilename;
	strTempFilename=wxString::Format("%stemp-%d-server.met", theApp.glob_prefs->GetAppDir(), ::GetTickCount());
	CHTTPDownloadDlg *dlg=new CHTTPDownloadDlg(this,strURL,strTempFilename);
	int retval=dlg->ShowModal();
	if(retval==0) {
		// curl succeeded. proceed with serverlist processing
		serverlistctrl->AddServermetToList(strTempFilename);
		wxRemoveFile(strTempFilename);
		theApp.serverlist->SaveServermetToFile();
		printf("Saving of server.met file Done !!!\n");
	} else {
		theApp.amuledlg->AddLogLine(true, CString(_("Failed to download the serverlist from %s")).GetData(), strURL.GetData());
	}
	delete dlg;
}

void CServerWnd::Localize()
{
}

// CServerWnd message handlers

void CServerWnd::OnBnClickedAddserver(wxEvent& evt)
{
	wxString serveraddr;
	if(((wxTextCtrl*)FindWindowById(IDC_IPADDRESS))->GetLineText(0).IsEmpty()) {
		theApp.amuledlg->AddLogLine(true, CString(_("Please enter a serveraddress")) );
		return;
	} else {
		serveraddr=((wxTextCtrl*)FindWindowById(IDC_IPADDRESS))->GetLineText(0);
	}
	if (((wxTextCtrl*)FindWindowById(IDC_SPORT))->GetLineText(0).IsEmpty()) {
		theApp.amuledlg->AddLogLine(true, CString(_("Incomplete serverport: Please enter a serverport")) );
		return;
	}
  
	wxString portstr;
	portstr=((wxTextCtrl*)FindWindowById(IDC_SPORT))->GetLineText(0);
	CServer* toadd = new CServer(atoi(portstr.GetData()),(char*)serveraddr.GetData());
	wxString servername;
	servername=((wxTextCtrl*)FindWindowById(IDC_SERVERNAME))->GetLineText(0);
	if (!servername.IsEmpty()) {
		toadd->SetListName((char*)servername.GetData());
	} else {
		toadd->SetListName((char*)serveraddr.GetData());
	}
	if (!serverlistctrl->AddServer(toadd,true)) {
		CServer* update = theApp.serverlist->GetServerByAddress(toadd->GetAddress(), toadd->GetPort());
		if(update) {
			update->SetListName(toadd->GetListName());
			serverlistctrl->RefreshServer(update);
		}
		delete toadd;
		theApp.amuledlg->AddLogLine(true, CString(_("Server not added!")));
	} else {
		theApp.amuledlg->AddLogLine(true, CString(_("Server added: ")) + "%s", toadd->GetListName());
	}
	theApp.serverlist->SaveServermetToFile();
	printf("Saving of server.met file Done !!!\n");
	((wxTextCtrl*)FindWindowById(IDC_SERVERNAME))->SetValue("");
	((wxTextCtrl*)FindWindowById(IDC_IPADDRESS))->SetValue("");
	((wxTextCtrl*)FindWindowById(IDC_SPORT))->SetValue("");
}

void CServerWnd::OnBnClickedUpdateservermetfromurl(wxEvent& evt)
{
	wxString strURL;
	strURL=((wxTextCtrl*)FindWindowById(IDC_SERVERLISTURL))->GetLineText(0);
	UpdateServerMetFromURL(strURL);
}

void CServerWnd::OnBnClickedResetLog(wxEvent& evt)
{
	theApp.amuledlg->ResetLog();
}

void CServerWnd::UpdateMyInfo()
{
	#if 0
	CString buffer;

	MyInfoList->DeleteAllItems();
	MyInfoList->InsertItem(0, CString(_("Status"))+":");
	if (theApp.serverconnect->IsConnected()) {
		MyInfoList->SetItemText(0, 1, CString(_("Connected")));
	}	else {
		MyInfoList->SetItemText(0, 1, CString(_("Disconnected")));
	}

	if (theApp.serverconnect->IsConnected()) {
		MyInfoList->InsertItem(1, CString(_("IP")) +":"+ CString(_("Port")));
		if (theApp.serverconnect->IsLowID()) {
			buffer=CString(_("Unknown")); 
		} else {
			uint32 myid=theApp.serverconnect->GetClientID();
			uint8 d=myid/(256*256*256);myid-=d*(256*256*256);
			uint8 c=myid/(256*256);myid-=c*256*256;
			uint8 b=myid/(256);myid-=b*256;
			buffer.Format("%i.%i.%i.%i:%i",myid,b,c,d,theApp.glob_prefs->GetPort());
		}
		MyInfoList->SetItemText(1,1,buffer);

		buffer.Format("%u",theApp.serverconnect->GetClientID());
		MyInfoList->InsertItem(2,CString(_("ID")));
		if (theApp.serverconnect->IsConnected()) {
			MyInfoList->SetItemText(2, 1, buffer);
		}

		MyInfoList->InsertItem(3,"");
		if (theApp.serverconnect->IsLowID()) {
			MyInfoList->SetItemText(3, 1,CString(_("Low ID")));
		}	else {
			MyInfoList->SetItemText(3, 1,CString(_("High ID")));
		}
	}
	#endif
//	printf("TODO: MyInfo @ CServerWnd (GUI missing)\n");
}

void CServerWnd::OnSashPositionChanged()
{
	theApp.amuledlg->srv_split_pos = ((wxSplitterWindow*)FindWindow("SrvSplitterWnd"))->GetSashPosition();
}
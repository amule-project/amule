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


// amuleDlg.cpp : Implementierungsdatei
//

#include <cerrno>
#include <csignal>
#include <cmath>
#include <wx/toolbar.h>
#include <wx/utils.h>
#include <wx/tokenzr.h>
#include <wx/file.h>

#ifndef __SYSTRAY_DISABLED__
#include "pixmaps/mule_TrayIcon.ico.xpm"
#include "pixmaps/mule_Tr_yellow.ico.xpm"
#include "pixmaps/mule_Tr_grey.ico.xpm"
#endif // __SYSTRAY_DISABLED__
#include "UDPSocket.h"		// Needed for CUDPSocket
#include "amuleDlg.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for CastItoIShort
#include "ED2KLink.h"		// Needed for CED2KLink
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "sockets.h"		// Needed for CServerConnect
#include "FriendList.h"		// Needed for CFriendList
#include "ClientList.h"		// Needed for CClientList
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "SearchList.h"		// Needed for CSearchList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "ServerList.h"		// Needed for CServerList
#include "SysTray.h"		// Needed for CSysTray
#include "Preferences.h"	// Needed for CPreferences
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "ChatWnd.h"		// Needed for CChatWnd
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "ServerWnd.h"		// Needed for CServerWnd
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "PartFile.h"		// Needed for CPartFile
#include "KnownFile.h"		// Needed for CKnownFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "opcodes.h"		// Needed for TM_FINISHEDHASHING
#include "muuli_wdr.h"		// Needed for ID_BUTTONSERVERS
#include "ServerSocket.h"	// Needed for ID_SOKETTI

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
#include "aMule.xpm"
#endif

// connection images

#if 0
#define ID_BUTTONCONNECT 42110
#define ID_BUTTONSERVERS 42111
#define ID_BUTTONSEARCH 42112
#define ID_BUTTONTRANSFER 42113
#define ID_BUTTONPREFERENCES 42114
#define ID_BUTTONSHARED 42115
#define ID_BUTTONSTATS 42116
#endif




BEGIN_EVENT_TABLE(CamuleDlg,wxFrame)
	EVT_SOCKET(ID_SOKETTI, CamuleDlg::socketHandler)
	EVT_TOOL(ID_BUTTONCONNECT,CamuleDlg::OnBnConnect)
	EVT_TOOL(ID_BUTTONSERVERS,CamuleDlg::btnServers)
	EVT_TOOL(ID_BUTTONSEARCH,CamuleDlg::btnSearch)
	EVT_TOOL(ID_BUTTONTRANSFER,CamuleDlg::btnTransfer)
	EVT_TOOL(ID_BUTTONSHARED,CamuleDlg::OnBnShared)
	EVT_TOOL(ID_BUTTONMESSAGES,CamuleDlg::OnBnMessages)
	EVT_TOOL(ID_BUTTONSTATISTICS,CamuleDlg::OnBnStats)
	EVT_TOOL(ID_BUTTONNEWPREFERENCES,CamuleDlg::OnBnNewPreferences)
	EVT_TIMER(ID_UQTIMER,CamuleDlg::OnUQTimer)
	EVT_TIMER(TM_UDPSOCKET,CamuleDlg::OnUDPTimer)
	EVT_TIMER(TM_TCPSOCKET,CamuleDlg::OnSocketTimer)
	EVT_MENU(TM_FINISHEDHASHING,CamuleDlg::OnFinishedHashing)
	EVT_MENU(TM_DNSDONE,CamuleDlg::OnDnsDone)
	EVT_MENU(TM_SOURCESDNSDONE,CamuleDlg::OnSourcesDnsDone)
	EVT_CLOSE(CamuleDlg::OnClose)
	EVT_ICONIZE(CamuleDlg::OnMinimize)
	EVT_MENU(TM_HASHTHREADFINISHED,CamuleDlg::OnHashingShutdown)
	EVT_BUTTON(ID_BUTTON_FAST, CamuleDlg::OnBnClickedFast)
	EVT_BUTTON(ID_PREFS_OK_TOP, CamuleDlg::OnBnClickedPrefOk)
	EVT_BUTTON(ID_PREFS_CANCEL_TOP, CamuleDlg::OnBnClickedCancel)
END_EVENT_TABLE()

void CamuleDlg::OnDnsDone(wxCommandEvent& evt) {
	CUDPSocket* socket=(CUDPSocket*)evt.GetClientData();
	struct sockaddr_in *si=(struct sockaddr_in*)evt.GetExtraLong();
	socket->DnsLookupDone(si);
}

void CamuleDlg::OnSourcesDnsDone(wxCommandEvent& evt) {
	struct sockaddr_in *si=(struct sockaddr_in*)evt.GetExtraLong();
	theApp.downloadqueue->OnHostnameResolved(si);
}


void CamuleDlg::OnFinishedHashing(wxCommandEvent& evt) {
	CKnownFile* result = (CKnownFile*)evt.GetClientData();//(CKnownFile*)lParam;
	if (evt.GetExtraLong()) {
		CPartFile* requester = (CPartFile*)evt.GetExtraLong(); //wParam;
		if (theApp.downloadqueue->IsPartFile(requester)) {
			requester->PartFileHashFinished(result);
		}
	} else {
		if(theApp.sharedfiles->filelist->SafeAddKFile(result)) {
			theApp.sharedfiles->SafeAddKFile(result);
		} else {
			delete result;
		}
	}

	return;
}

/* This implementation seem to be smallest than the original ;-) BigBob */

void CamuleDlg::CreateMuleToolBar() {
	wxToolBar *newbar = CreateToolBar(
		wxTB_HORIZONTAL|wxNO_BORDER|wxTB_TEXT|
		wxTB_3DBUTTONS|wxTB_FLAT|wxCLIP_CHILDREN
	);
	newbar->SetToolBitmapSize(wxSize(32, 32));

	muleToolbar( newbar );
	m_wndToolbar=newbar;
}

// CamuleDlg Dialog

#if wxMINOR_VERSION > 4
#define CLOSEBOX wxCLOSE_BOX
#else
#define CLOSEBOX 0
#endif

CamuleDlg::CamuleDlg(wxWindow* pParent, wxString title) : wxFrame(
	pParent, CamuleDlg::IDD, title, wxDefaultPosition, wxSize(800,600),
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxTHICK_FRAME|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|CLOSEBOX
) {
	SetIcon(wxICON(aMule));

	theApp.amuledlg = this;
	
	
	m_app_state=APP_STATE_STARTING;
	srand(time(NULL));

	// get rid of sigpipe
#ifndef __WXMSW__
	signal(SIGPIPE,SIG_IGN);
#endif

	// Create new sizer and stuff a wxPanel in there.
	wxFlexGridSizer *s_main = new wxFlexGridSizer(1);
	s_main->AddGrowableCol(0);
	s_main->AddGrowableRow(0);
	p_cnt = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize);
	s_main->Add(p_cnt, 0, wxGROW|wxEXPAND, 0);
	muleDlg(p_cnt, false, true);

	SetSizer( s_main, true );

	// Create ToolBar from the one designed by wxDesigner (BigBob)
	CreateMuleToolBar();

	serverwnd=new CServerWnd(p_cnt);
	searchwnd=new CSearchDlg(p_cnt);
	transferwnd=new CTransferWnd(p_cnt);
	prefsunifiedwnd=new PrefsUnifiedDlg(p_cnt);
	sharedfileswnd=new CSharedFilesWnd(p_cnt);
	statisticswnd=new CStatisticsDlg(p_cnt);
	chatwnd=new CChatWnd(p_cnt);

	transicons[0]=dlStatusImages(0);
	transicons[1]=dlStatusImages(1);
	transicons[2]=dlStatusImages(2);
	transicons[3]=dlStatusImages(3);

#define ID_BITMAPBUTTON (-1)

	s_main->Show(this,TRUE);

	contentSizer->Add(serverwnd,1,wxALIGN_LEFT|wxEXPAND);
	serverwnd->Show(FALSE);
	searchwnd->Show(FALSE);
	transferwnd->Show(FALSE);
	sharedfileswnd->Show(FALSE);
	statisticswnd->Show(FALSE);
	chatwnd->Show(FALSE);

	if (!LoadRazorPrefs()) {
		// Prefs not loaded for some reason, exit
		printf("ERROR!!! Unable to load Razor Preferences\n");
		return;
	}

	activewnd=serverwnd;
	lastbutton=ID_BUTTONSERVERS;
	m_wndToolbar->ToggleTool(lastbutton,FALSE);

	SetActiveDialog(serverwnd);
	m_nActiveDialog = 1;

	// Set serverlist splitter position
	wxSplitterWindow* srv_split=(wxSplitterWindow*)FindWindow("SrvSplitterWnd");
	srv_split->SetSashPosition(srv_split_pos, true);

	CAddFileThread::Setup();

	ToggleFastED2KLinksHandler();

}

// Madcat - Toggles Fast ED2K Links Handler on/off.
void CamuleDlg::ToggleFastED2KLinksHandler() {
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Hiding FED2KLH aborted."));
		return;
	}
	s_dlgcnt->Show(s_fed2klh, theApp.glob_prefs->GetFED2KLH());
	s_dlgcnt->Layout();
	searchwnd->ToggleLinksHandler();
}

void CamuleDlg::SetActiveDialog(wxWindow* dlg) {
	activewnd->Show(FALSE);
	contentSizer->Remove(activewnd);
	contentSizer->Add(dlg,1,wxALIGN_LEFT|wxEXPAND);
	dlg->Show(TRUE);
	activewnd=dlg;
	s_dlgcnt->Layout();
}

class QueryDlg : public wxDialog {
public:
	QueryDlg(wxWindow* parent) : wxDialog(
		parent, 21373, _("Desktop integration"), wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU
	) {
		wxSizer* content=desktopDlg(this,TRUE);
		content->Show(this,TRUE);
		Centre();
	};
protected:
	void OnOk(wxCommandEvent& evt) { EndModal(0); };
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(QueryDlg,wxDialog)
	EVT_BUTTON(ID_OK,QueryDlg::OnOk)
END_EVENT_TABLE()

void CamuleDlg::changeDesktopMode() {
	static int mID[] = { 0 , ID_GNOME2, ID_KDE3, ID_KDE2, ID_NOSYSTRAY };
	int mode;

	QueryDlg query(this);
	if (query.FindWindowById(mID[theApp.glob_prefs->GetDesktopMode()])) {
		wxStaticCast(query.FindWindowById(mID[theApp.glob_prefs->GetDesktopMode()]),wxRadioButton)->SetValue(true);
	}
	query.ShowModal();
	for (mode=1; mode<=3; mode++) {
		if (query.FindWindowById(mID[mode]) && wxStaticCast(query.FindWindowById(mID[mode]),wxRadioButton)->GetValue() != 0) {
			break;
		}
	}
	theApp.glob_prefs->SetDesktopMode(mode);
}

#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::CreateSystray(const wxString& title) {
	// create the docklet (at this point we already have preferences!)
	if(theApp.glob_prefs->GetDesktopMode()==0) {
		// ok, it's not set yet.
		changeDesktopMode();
	}
	m_wndTaskbarNotifier=new CSysTray(this,theApp.glob_prefs->GetDesktopMode(),title);
}

void CamuleDlg::RemoveSystray() {
	if (m_wndTaskbarNotifier) {
		delete m_wndTaskbarNotifier;
	}
}
#endif // __SYSTRAY_DISABLED__

extern void TimerProc();
void CamuleDlg::OnUQTimer(wxTimerEvent& evt) {
	// call timerproc
	if(!IsRunning()) {
		return;
	}
	TimerProc();
}

void CamuleDlg::InitDialog() {
	theApp.serverlist->Init();
}

bool CamuleDlg::IsRunning() {
	return (m_app_state == APP_STATE_RUNNING);
}

void CamuleDlg::OnBnStats(wxCommandEvent& ev) {
	// Kry - just if the app is ready for it
	if (theApp.IsReady) {
		if(lastbutton == ID_BUTTONSTATISTICS) {
			m_wndToolbar->ToggleTool(lastbutton,TRUE);
		} else {
			m_wndToolbar->ToggleTool(lastbutton,FALSE);
			lastbutton=ID_BUTTONSTATISTICS;
			SetActiveDialog(statisticswnd);
			m_nActiveDialog = 7;
		}
	}
}

void CamuleDlg::OnBnShared(wxCommandEvent& ev) {
	// Kry - just if the app is ready for it
	if (theApp.IsReady) {
		if(lastbutton == ID_BUTTONSHARED) {
			m_wndToolbar->ToggleTool(lastbutton,TRUE);
		} else {
			m_wndToolbar->ToggleTool(lastbutton,FALSE);
			lastbutton=ID_BUTTONSHARED;
			SetActiveDialog(sharedfileswnd);
			m_nActiveDialog = 4;
		}
	}
}

void CamuleDlg::btnServers(wxCommandEvent& ev) {
	// Kry - just if the app is ready for it
	if (theApp.IsReady) {
		if(lastbutton == ID_BUTTONSERVERS) {
			m_wndToolbar->ToggleTool(lastbutton,TRUE);
		} else {
			m_wndToolbar->ToggleTool(lastbutton,FALSE);
			lastbutton=ID_BUTTONSERVERS;
			SetActiveDialog(serverwnd);
			m_nActiveDialog = 1;
		}
		// Set serverlist splitter position
		wxSplitterWindow* srv_split=(wxSplitterWindow*)FindWindow("SrvSplitterWnd");
		srv_split->SetSashPosition(srv_split_pos, true);
	}
}

void CamuleDlg::btnSearch(wxCommandEvent& ev) {
	// Kry - just if the app is ready for it
	if (theApp.IsReady) {
		if(lastbutton == ID_BUTTONSEARCH) {
			m_wndToolbar->ToggleTool(lastbutton,TRUE);
		} else {
			m_wndToolbar->ToggleTool(lastbutton,FALSE);
			lastbutton=ID_BUTTONSEARCH;
			SetActiveDialog(searchwnd);
			m_nActiveDialog = 3;
		}
	}
}

void CamuleDlg::btnTransfer(wxCommandEvent& ev) {
	// Kry - just if the app is ready for it
	if (theApp.IsReady) {
		if(lastbutton == ID_BUTTONTRANSFER) {
			m_wndToolbar->ToggleTool(lastbutton,TRUE);
		} else {
			m_wndToolbar->ToggleTool(lastbutton,FALSE);
			lastbutton=ID_BUTTONTRANSFER;
			SetActiveDialog(transferwnd);
			m_nActiveDialog = 2;
		}
		// Set splitter position
		wxSplitterWindow* split=(wxSplitterWindow*)FindWindow("splitterWnd");
		split->SetSashPosition(split_pos, true);
	}
}

void CamuleDlg::OnBnMessages(wxCommandEvent& ev) {
	// Kry - just if the app is ready for it
	if (theApp.IsReady) {
		if(lastbutton == ID_BUTTONMESSAGES) {
			m_wndToolbar->ToggleTool(lastbutton,TRUE);
		} else {
			m_wndToolbar->ToggleTool(lastbutton,FALSE);
			lastbutton=ID_BUTTONMESSAGES;
			SetActiveDialog(chatwnd);
			m_nActiveDialog = 5;
		}
	}
}

void CamuleDlg::socketHandler(wxSocketEvent& event) {

	wxSocketBase * current_socket = event.GetSocket();
	
	if(!IsRunning() || !current_socket) {
		// we are not mentally ready to receive anything
		// or there is no socket on the event (got deleted?)
		return;
	}


	if(current_socket->IsKindOf(CLASSINFO(CListenSocket))) {
		CListenSocket* soc=(CListenSocket*)current_socket;
		switch(event.GetSocketEvent()) {
			case wxSOCKET_CONNECTION:
				soc->OnAccept(0);
				break;
			default:
				// shouldn't get other than connection events...
				break;
		}
		return;
	}

	if(current_socket->IsKindOf(CLASSINFO(CClientReqSocket))) {
		CClientReqSocket* soc=(CClientReqSocket*)current_socket;
		//printf("request at clientreqsocket\n");
		switch(event.GetSocketEvent()) {
			case wxSOCKET_LOST:
				soc->OnError(errno);
				break;
			case wxSOCKET_INPUT:
				soc->OnReceive(0);
				break;
			case wxSOCKET_OUTPUT:
				soc->OnSend(0);
				break;
			default:
				// connection requests should not arrive here..
				break;
		}
		return;
	}

	if(current_socket->IsKindOf(CLASSINFO(CUDPSocket))) {
		CUDPSocket* soc=(CUDPSocket*)current_socket;
		switch(event.GetSocketEvent()) {
			case wxSOCKET_INPUT:
				soc->OnReceive(0);
				break;
			default:
				break;
		}
		return;
	}

	if(current_socket->IsKindOf(CLASSINFO(CServerSocket))) {
		CServerSocket* soc=(CServerSocket*)current_socket;
		switch(event.GetSocketEvent()) {
			case wxSOCKET_CONNECTION:
				soc->OnConnect(wxSOCKET_NOERROR);
				break;
			case wxSOCKET_LOST:
				soc->OnError(soc->LastError());
				break;
			case wxSOCKET_INPUT:
				soc->OnReceive(wxSOCKET_NOERROR);
				break;
			case wxSOCKET_OUTPUT:
				soc->OnSend(wxSOCKET_NOERROR);
				break;
			default:
				break;
		}
		return;
	}

	if(current_socket->IsKindOf(CLASSINFO(CClientUDPSocket))) {
		CClientUDPSocket* soc=(CClientUDPSocket*)current_socket;
		switch(event.GetSocketEvent()) {
			case wxSOCKET_INPUT:
				soc->OnReceive(0);
				break;
			case wxSOCKET_OUTPUT:
				soc->OnSend(0);
				break;
			default:
				break;
		}
		return;
	}

	printf("*** SHOULD NOT END UP HERE\n");
	printf("** class is %s\n",current_socket->GetClassInfo()->GetClassName());
}

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");
	/* Razor 1a - Modif by MikaelB
	   Save client size and position */

	// Create a config base for saving razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without saving
	if (config == NULL) {
		return;
	}
	// The section where to save in in file
	wxString section = "/Razor_Preferences/";

	// Main window location and size
	int x1,y1,x2,y2;
	GetPosition(&x1, &y1);
	GetSize(&x2,&y2);

	// Saving window size and position
	config->Write(_T(section+"MAIN_X_POS"), (long) x1);
	config->Write(_T(section+"MAIN_Y_POS"), (long) y1);
	config->Write(_T(section+"MAIN_X_SIZE"), (long) x2);
	config->Write(_T(section+"MAIN_Y_SIZE"), (long) y2);
  
	// Saving sash position of splitter in transfer window
	config->Write(_T(section+"SPLITTER_POS"), (long) split_pos);
	printf("split saved to : %u\n", split_pos);
	  
	// Saving sash position of splitter in server window
	config->Write(_T(section+"SRV_SPLITTER_POS"), (long) srv_split_pos);
	printf("srv_split saved to : %u\n", srv_split_pos);
	  
	config->Flush(true);

	/* End modif */


	theApp.OnlineSig(true);

	// TODO: We want to free the memory used by the webserver
	//       but currently it does wait in accept().
	// delete theApp.webserver;
	delete theApp.serverlist; theApp.serverlist = NULL;
	delete theApp.searchlist; theApp.searchlist = NULL;
	delete theApp.clientcredits; theApp.clientcredits = NULL;
	// Destroying CDownloadQueue calls destructor for CPartFile
	// calling CSharedFileList::SafeAddKFile occasally.
	delete theApp.downloadqueue; theApp.downloadqueue = NULL;
	delete theApp.sharedfiles; theApp.sharedfiles = NULL;
	delete theApp.knownfiles; theApp.knownfiles = NULL;
	delete theApp.uploadqueue; theApp.uploadqueue = NULL;
	delete theApp.clientlist; theApp.clientlist = NULL;
	delete theApp.friendlist; theApp.friendlist = NULL;
	delete theApp.glob_prefs; theApp.glob_prefs = NULL;
	delete theApp.serverconnect; theApp.serverconnect = NULL;
	printf("aMule dialog destroyed\n");
}

// CamuleDlg eventhandler

void CamuleDlg::OnBnConnect(wxCommandEvent& evt) {
	if (!theApp.serverconnect->IsConnected()) {
		//connect if not currently connected
		if (!theApp.serverconnect->IsConnecting()) {
			StartConnection();
		} else {
			theApp.serverconnect->StopConnectionTry();
			ShowConnectionState(false);
		}
	} else {     //disconnect if currently connected
		CloseConnection();
	}
	// UpdateBar();
}

void CamuleDlg::ResetLog(uint8 whichone) {
	logtext="";
	wxTextCtrl* ct = NULL;
	//serverwnd.logbox.SetWindowText(logtext);
	switch (whichone){
		case 1:
			ct=(wxTextCtrl*)serverwnd->FindWindowById(ID_LOGVIEW);
			// Delete log file aswell.
			wxRemoveFile(wxString::Format("%s/.aMule/logfile", getenv("HOME")));
			break;
		case 2:
			ct=(wxTextCtrl*)serverwnd->FindWindowById(ID_SERVERINFO);
			break;
		default:
			return;
	}

	if(ct) {
		ct->SetValue("");
	}

}

void CamuleDlg::ResetDebugLog() {
	logtext="";
	serverwnd->logbox.Clear(); //SetWindowText(logtext);
}

void CamuleDlg::AddLogLine(bool addtostatusbar,const wxChar* line,...) {
	char osDate[30],osTime[30]; //<<--9/21/02
	char temp[1060]; //<<--9/21/02
	char bufferline[1000];

	va_list argptr;
	va_start(argptr, line);
	vsnprintf(bufferline, 1000, line, argptr);
	va_end(argptr);
	if (addtostatusbar) {
		// statusbar.SetStatusText(bufferline,0); //,0);
		wxStaticText* text=(wxStaticText*)FindWindow("infoLabel");
		text->SetLabel(bufferline);
		Layout();
	}

	time_t joskus=time(NULL);
	struct tm* nyt=localtime(&joskus);

	strftime(osDate,29,"%F",nyt);
	strftime(osTime,29,"%T",nyt);

	//strtime( osTime ); //<<--9/21/02
	//strdate( osDate ); //<<--9/21/02
	sprintf(temp,"%s %s: %s\n",osDate,osTime,bufferline);//<<--9/21/02

	wxTextCtrl* ct=(wxTextCtrl*)serverwnd->FindWindowById(ID_LOGVIEW);

	if(ct) {
		ct->AppendText(temp);
		//ct->ShowPosition(ct->GetLastPosition());
		ct->ShowPosition(ct->GetValue().Length()-1);
	}

	// Write into log file
	wxString filename;
	filename = filename.Format("%s/.aMule/logfile", getenv("HOME"));
	wxFile file(filename, wxFile::write_append);

	if (file.IsOpened()) {
		file.Write(temp);
		file.Close();
	}
	//printf("***LOG: %s\n",logtext);
}

void CamuleDlg::AddDebugLogLine(bool addtostatusbar,const wxChar* line,...){
	if (theApp.glob_prefs->GetVerbose()) {
		char bufferline[1000];
		va_list argptr;
		va_start(argptr, line);
		vsnprintf(bufferline, 1000, line, argptr);
		va_end(argptr);
		AddLogLine(addtostatusbar, bufferline);
	}
}

void CamuleDlg::AddServerMessageLine(char* line,...) {
	wxString content;
	va_list argptr;
	char bufferline[500];
	va_start(argptr, line);
	vsnprintf(bufferline, 500, line, argptr);
	va_end(argptr);

	wxTextCtrl* cv=(wxTextCtrl*)serverwnd->FindWindowById(ID_SERVERINFO);

	if(cv) {
		cv->AppendText(wxString(bufferline)+wxString("\n"));
		cv->ShowPosition(cv->GetValue().Length()-1);
	}
	//serverwnd.servermsgbox.AppendText(CString(bufferline)+CString("\n"));
}

void CamuleDlg::ShowConnectionState(bool connected) {
	ShowConnectionState(connected,"");
}

#define STAT_LOWID 1
#define STAT_HIGHID 2
#define STAT_CONNECTING 3
#define STAT_NOTCONNECTED 4

void CamuleDlg::ShowConnectionState(bool connected, wxString server,bool iconOnly) {
	static int __oldstatus=(-1);
	bool changed=false;

	serverwnd->UpdateMyInfo();

	if (connected) {
		wxStaticBitmap* bmp=(wxStaticBitmap*)FindWindowByName("connImage");
		if(theApp.serverconnect->IsLowID()) {
			if(__oldstatus!=STAT_LOWID) {
				bmp->SetBitmap(connImages(1));
				__oldstatus=STAT_LOWID;
				changed=true;
			}
		} else {
			if(__oldstatus!=STAT_HIGHID) {
				bmp->SetBitmap(connImages(3));
				__oldstatus=STAT_HIGHID;
				changed=true;
			}
		}

		//butn->SetLabel(CString(_("Disconnect")));
		//butn->SetDefaultBitmap(connButImg(1));
		// we can't modify existing button. so we delete the old and create a new one
		if(changed) {
			m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
			m_wndToolbar->InsertTool(0,ID_BUTTONCONNECT,CString(_("Disconnect")),connButImg(1),CString(_("Disconnect from current server")));
			m_wndToolbar->Realize();
		}

		/*
		this->GetDlgItem(IDC_BUTTON2)->SetWindowText(CString(_("Disconnect")));
		m_btnConnect.SetIcon(IDI_BN_DISCONNECT); m_btnConnect.SetTooltipText(CString(_("Disconnect from current server")));
      		if (theApp.serverconnect->IsLowID()) {
			statusbar.SetIcon(3,connicons[1]);
		} else {
			statusbar.SetIcon(3,connicons[2]);
		}
		*/

		//statusbar.SetText(wxString::Format(_("Connection established on:")) + CString(server),0,0);
		wxStaticText* tx=(wxStaticText*)FindWindow("infoLabel");
		tx->SetLabel(CString(_("Connection established on:")) + wxString(server));
		((wxStaticText*)FindWindow("connLabel"))->SetLabel(server);
		Layout();
		//statusbar.SetTipText(3,server);
		//statusbar.SetText(server,3,0);
	} else {
		
		// Why? We already handle the highlight on connection/disconnection states.		
		//theApp.amuledlg->serverwnd->serverlistctrl->HighlightServer(NULL);

		wxStaticBitmap* bmp=(wxStaticBitmap*)FindWindowByName("connImage");
    
		if (theApp.serverconnect->IsConnecting()) {
			if(__oldstatus!=STAT_CONNECTING) {
				bmp->SetBitmap(connImages(2));
				//butn->SetLabel(CString(_("Cancel")));
				//butn->SetDefaultBitmap(connButImg(2));
				// once again.. no modify
				m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
				m_wndToolbar->InsertTool(0,ID_BUTTONCONNECT,CString(_("Cancel")),connButImg(2),CString(_("Stops the current connection attempts")));
				m_wndToolbar->Realize();
				__oldstatus=STAT_CONNECTING;
			}

			/*
			this->GetDlgItem(IDC_BUTTON2)->SetWindowText(CString(_("Cancel")));
			m_btnConnect.SetIcon(IDI_BN_STOPCONNECTING); m_btnConnect.SetTooltipText(CString(_("Stops the current connection attempts")));
			statusbar.SetIcon(3,connicons[0]);
			*/

			// EI INFOLABEL
			((wxStaticText*)FindWindowByName("connLabel"))->SetLabel(CString(_("Connecting")));
			Layout();
			//statusbar.SetText(CString(_("Connecting")),3,0);
			//statusbar.SetTipText(3,"");
			ShowUserCount(0,0);
		} else {
			if(__oldstatus!=STAT_NOTCONNECTED) {
				bmp->SetBitmap(connImages(0));
				((wxStaticText*)FindWindowByName("connLabel"))->SetLabel(CString(_("Not Connected")));
				Layout();
				//butn->SetLabel(CString(_("Connect")));
				//butn->SetDefaultBitmap(connButImg(0));
				// and again...
				m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
				m_wndToolbar->InsertTool(0,ID_BUTTONCONNECT,CString(_("Connect")),connButImg(0),CString(_("Connect to any server")));
				m_wndToolbar->Realize();
				__oldstatus=STAT_NOTCONNECTED;
			}

			/*
			this->GetDlgItem(IDC_BUTTON2)->SetWindowText(CString(_("Connect")));
			m_btnConnect.SetIcon(IDI_BN_CONNECT);	m_btnConnect.SetTooltipText(CString(_("Connect to any server")));
			statusbar.SetIcon(3,connicons[0]);
			statusbar.SetText(CString(_("Not Connected")),3,0);
			*/

			((wxStaticText*)FindWindow("connLabel"))->SetLabel(CString(_("Not Connected")));
			Layout();
			AddLogLine(true,CString(_("Disconnected")));
			//statusbar.SetTipText(3,"");
			ShowUserCount(0,0);
		}

	}
}

void CamuleDlg::ShowUserCount(uint32 user_toshow,uint32 file_toshow) {
	uint32 totaluser, totalfile;
	totaluser = totalfile = 0;
	if( user_toshow || file_toshow ) {
		theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	}
	char buffer[100];
	sprintf(buffer, "%s: %s(%s) | %s: %s(%s)", CString(_("Users")).GetData(), CastItoIShort(user_toshow).GetData(), CastItoIShort(totaluser).GetData(), CString(_("Files")).GetData(), CastItoIShort(file_toshow).GetData(), CastItoIShort(totalfile).GetData());
	wxStaticCast(FindWindow("userLabel"),wxStaticText)->SetLabel(buffer);
	Layout();
	//statusbar.SetText(buffer,1,0);
}

void CamuleDlg::ShowMessageState(uint8 iconnr) {
	//statusbar.SetIcon(4,imicons[iconnr]);
}

void CamuleDlg::ShowTransferRate(bool foreceAll) {
	char buffer[50];
#ifndef __SYSTRAY_DISABLED__
	char buffer2[100];
#endif

	float	kBpsUp = theApp.uploadqueue->GetKBps();
	float 	kBpsDown = theApp.downloadqueue->GetKBps();
	int lastuprateoverhead = theApp.uploadqueue->GetUpDatarateOverhead();
	int lastdownrateoverhead = theApp.downloadqueue->GetDownDatarateOverhead();

	if( theApp.glob_prefs->ShowOverhead() ) {
		sprintf(buffer,CString(_("Up: %.1f(%.1f) | Down: %.1f(%.1f)")), kBpsUp, (float)lastuprateoverhead/1024, kBpsDown, (float)lastdownrateoverhead/1024);
	} else {
		sprintf(buffer,CString(_("Up: %.1f | Down: %.1f")), kBpsUp, kBpsDown);
	}

	//statusbar.SetStatusText(buffer,2); //,0);
	((wxStaticText*)FindWindowByName("speedLabel"))->SetLabel(buffer);
	Layout();

#ifndef __SYSTRAY_DISABLED__
	// set trayicon-icon
	int percentDown = (int)ceil ((kBpsDown*100) / theApp.glob_prefs->GetMaxGraphDownloadRate());
	UpdateTrayIcon(percentDown>100 ? 100 : percentDown);

	if (theApp.serverconnect->IsConnected()) {
		sprintf(buffer2,wxString("aMule (%s | " + CString(_("Connected")) + ")").GetData(),buffer);
	} else {
		sprintf(buffer2,wxString("aMule (%s | " + CString(_("Disconnected")) + ")").GetData(),buffer);
	}
	m_wndTaskbarNotifier->TraySetToolTip(buffer2);
#endif

	wxStaticBitmap* bmp=(wxStaticBitmap*)FindWindowByName("transferImg");
	bmp->SetBitmap(transicons[(kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)]);
}

extern void URLDecode(wxString& result, const char* buff);

void CamuleDlg::OnHashingShutdown(wxCommandEvent&) {
	if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
		printf("Hashing thread ended, restarting\n");
		CAddFileThread::Setup();
	} else {
		printf("Hashing thread terminated, ready to shutdown\n");
		Destroy();
	}
}

void CamuleDlg::OnClose(wxCloseEvent& evt) {
	if (evt.CanVeto() && theApp.glob_prefs->IsConfirmExitEnabled() ) {
		if (wxNO==wxMessageBox(CString(_("Do you really want to exit aMule?")),CString(_("Exit confirmation")),wxYES_NO)) {
			evt.Veto();
			return;
		}
	}

	// we are going DOWN
	m_app_state=APP_STATE_SHUTINGDOWN;

	// Kry - Save the sources seeds on app exit
	if (theApp.glob_prefs->GetSrcSeedsOn()) {
		theApp.downloadqueue->SaveSourceSeeds();
	}	
	
	theApp.OnlineSig(); // Added By Bouc7

	// Close sockets to avoid new clients coming in
	if (theApp.listensocket) {
		theApp.listensocket->StopListening();
	}
	if (theApp.clientudp) {
		theApp.clientudp->Destroy();
	}
	if (theApp.serverconnect) {
		theApp.serverconnect->Disconnect();
	}

	// Signal the hashing thread to terminate
	CAddFileThread::Shutdown();

	// TODO: Add your message handler code here and/or call default
	//WINDOWPLACEMENT wp;wp.length=sizeof(wp);GetWindowPlacement(&wp);
	//theApp.glob_prefs->SetWindowLayout(wp);

	// saving data & stuff
	if (theApp.knownfiles) {
		theApp.knownfiles->Save();
	}

	if (theApp.glob_prefs) {
		theApp.glob_prefs->Add2TotalDownloaded(theApp.stat_sessionReceivedBytes);
		theApp.glob_prefs->Add2TotalUploaded(theApp.stat_sessionSentBytes);
	}

	// save friends
	if (theApp.friendlist) {
		theApp.friendlist->SaveList();
	}
	if (theApp.glob_prefs) {
		theApp.glob_prefs->Save();
	}

	transferwnd->downloadlistctrl->DeleteAllItems();
	//amuledlg->chatwnd->chatselector->DeleteAllItems();
	if (theApp.clientlist) {
		theApp.clientlist->DeleteAll();
	}

#ifndef __SYSTRAY_DISABLED__
	//We want to delete the systray too!
	RemoveSystray();
#endif

	// if(searchwnd->searchlistctrl) searchwnd->searchlistctrl->DeleteAllItems();

	// Hint: Destroying the window is delayed until the hashing thread
	// does signal its termination.
	//  Destroy();
	//CTrayDialog::OnCancel();
}

void CamuleDlg::StartConnection() {
	AddLogLine(true, CString(_("Connecting")));
	theApp.serverconnect->ConnectToAnyServer();
	ShowConnectionState(false);
}

void CamuleDlg::CloseConnection() {
	theApp.serverconnect->Disconnect();
	theApp.OnlineSig(); // Added By Bouc7
}

void CamuleDlg::RestoreWindow() {
	//if (TrayIsVisible()) TrayHide();
	//ShowWindow(SW_SHOW);
}

void CamuleDlg::ShowStatistics() {
	statisticswnd->ShowStatistics();
}

#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::UpdateTrayIcon(int procent) {
	//HICON mytrayIcon;
	//if (mytrayIcon!=NULL) DestroyIcon(mytrayIcon);

	// set the limits of where the bar color changes (low-high)
	int pLimits16[1] = {100};

	// set the corresponding color for each level
	COLORREF pColors16[1] = {statisticswnd->GetTrayBarColor()};  // ={theApp.glob_prefs->GetStatsColor(11)};

	// load our limit and color info
	m_wndTaskbarNotifier->SetColorLevels(pLimits16,pColors16,1);

	// generate the icon (destroy these icon using DestroyIcon())
	int pVals16[1] = {procent};

	// ei hienostelua. tarvii kuitenki pelleill?gtk:n kanssa
	char** data;
	if(!theApp.serverconnect) {
		data=mule_Tr_grey_ico;
	} else {
		if (theApp.serverconnect->IsConnected()) {
			if(!theApp.serverconnect->IsLowID()) {
				data=mule_TrayIcon_ico;
			} else {
				data=mule_Tr_yellow_ico;
			}
		} else {
			data=mule_Tr_grey_ico;
		}
	}
	m_wndTaskbarNotifier->TraySetIcon(data,true,pVals16);
}
#endif // __SYSTRAY_DISABLED__

//BEGIN - enkeyDEV(kei-kun) -TaskbarNotifier-
void CamuleDlg::ShowNotifier(wxString Text, int MsgType, bool ForceSoundOFF) {
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-

void CamuleDlg::OnUDPTimer(wxTimerEvent& evt) {
	if(!IsRunning()) {
		return;
	}
	theApp.serverlist->SendNextPacket();
}

void CamuleDlg::OnSocketTimer(wxTimerEvent& evt) {
	if(!IsRunning()) {
		return;
	}
	CServerConnect *_this= theApp.serverconnect;
	_this->StopConnectionTry();
	if (_this->IsConnected()) {
		return;
	}
	_this->ConnectToAnyServer();
}

void CamuleDlg::OnQLTimer(wxTimerEvent& evt) {
	printf("QLTimer\n");
	if(!IsRunning()) {
		return;
	}
	CQueueListCtrl::QueueUpdateTimer();
}

void CamuleDlg::OnBnClickedFast(wxCommandEvent& evt) {
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* bigbob=new wxMessageDialog(this,CString(_("You are not connected to a server!")),CString(_("Not Connected")),wxOK|wxCENTRE|wxICON_INFORMATION);
		bigbob->ShowModal();
		StartFast((wxTextCtrl*)FindWindowByName("FastEd2kLinks"));
	} else {
		StartFast((wxTextCtrl*)FindWindowByName("FastEd2kLinks"));
	}
}

// Pass pointer to textctrl which contains the links as argument
void CamuleDlg::StartFast(wxTextCtrl *ctl) {
	#if defined(__WXGTK__)
		wxStringTokenizer tkz(ctl->GetValue(), "\n");
	#elif defined(__WXMAC__)
		wxStringTokenizer tkz(ctl->GetValue(), "\r");
	#elif defined(__WXMSW__)
		wxStringTokenizer tkz(ctl->GetValue(), "\r\n");
	#endif
	while (tkz.HasMoreTokens()) {
		wxString strlink = tkz.GetNextToken();
		if(strlink.Right(1)!="/") { strlink+="/"; }
		ctl->SetValue("");
		try {
			CED2KLink* pLink=CED2KLink::CreateLinkFromUrl(strlink);
			assert(pLink!=0);
			if(pLink->GetKind()==CED2KLink::kFile) {
				theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink());
			} else {
				throw wxString(wxT("Bad link"));
			}
			delete pLink;
		}
		catch(wxString error) {
			char buffer[200];
			sprintf(buffer,CString(_("This ed2k link is invalid (%s)")),error.GetData());
			theApp.amuledlg->AddLogLine(true,CString(_("Invalid link: %s")),buffer);
		}
	}
}

bool CamuleDlg::LoadRazorPrefs() {
	// Create a config base for loading razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without loading
	if (config == NULL) {
		return false;
	}

	// The section where to save in in file
	wxString section = "/Razor_Preferences/";

	// Main window location and size
	int x1,y1,x2,y2;

	// Saving window size and position
	x1 = config->Read(_T(section+"MAIN_X_POS"), 0l);
	y1 = config->Read(_T(section+"MAIN_Y_POS"), 0l);
	x2 = config->Read(_T(section+"MAIN_X_SIZE"), 0l);
	y2 = config->Read(_T(section+"MAIN_Y_SIZE"), 0l);

	printf("Config:\nx1: %d y1: %d x2: %d y2: %d\n", x1, y1, x2, y2);

	split_pos = config->Read(_T(section+"SPLITTER_POS"), 0l);
	printf("split read, found : %u\n",split_pos);

	srv_split_pos = config->Read(_T(section+"SRV_SPLITTER_POS"), 0l);
	printf("srv_split read, found : %u\n",srv_split_pos);
	if (srv_split_pos ==0) {
		// Kry - Random usable pos
		srv_split_pos = 463;
	}


	// If x1 and y1 != 0 Redefine location
	if((x1 != -1) && (y1 != -1)) {	// if((x1 != 0) && (y1 != 0))
		Move(x1,y1);
	}

	if (x2 != 0 && y2 != 0) {
		SetClientSize(x2,y2-58);
	} else {
		SetClientSize(800,542);
	}

	return true;
}

//hides amule
void CamuleDlg::Hide_aMule(bool iconize) {
	if (theApp.amuledlg->IsShown()) {

		theApp.amuledlg->transferwnd->downloadlistctrl->Freeze();
		theApp.amuledlg->transferwnd->uploadlistctrl->Freeze();
		theApp.amuledlg->serverwnd->serverlistctrl->Freeze();
		theApp.amuledlg->sharedfileswnd->sharedfilesctrl->Freeze();
		theApp.amuledlg->transferwnd->downloadlistctrl->Show(FALSE);
		theApp.amuledlg->serverwnd->serverlistctrl->Show(FALSE);
		theApp.amuledlg->transferwnd->uploadlistctrl->Show(FALSE);
		theApp.amuledlg->sharedfileswnd->sharedfilesctrl->Show(FALSE);
		theApp.amuledlg->Freeze();
		if (iconize) {
			theApp.amuledlg->Iconize(TRUE);
		}
		theApp.amuledlg->Show(FALSE);
	} else {
		printf("aMule is already hidden!\n");
	}
}

//shows amule
void CamuleDlg::Show_aMule(bool uniconize) {
	if (!theApp.amuledlg->IsShown()) {

		theApp.amuledlg->transferwnd->downloadlistctrl->Show(TRUE);
		theApp.amuledlg->serverwnd->serverlistctrl->Show(TRUE);
		theApp.amuledlg->transferwnd->uploadlistctrl->Show(TRUE);
		theApp.amuledlg->sharedfileswnd->sharedfilesctrl->Show(TRUE);
		theApp.amuledlg->transferwnd->downloadlistctrl->Thaw();
		theApp.amuledlg->serverwnd->serverlistctrl->Thaw();
		theApp.amuledlg->transferwnd->uploadlistctrl->Thaw();
		theApp.amuledlg->sharedfileswnd->sharedfilesctrl->Thaw();
		theApp.amuledlg->Thaw();
		theApp.amuledlg->Update();
		theApp.amuledlg->Refresh();
		if (uniconize) {
			theApp.amuledlg->Show(TRUE);
		}

	} else {
		printf("aMule is already shown!\n");
	}
}

void CamuleDlg::OnMinimize(wxIconizeEvent& evt) {
#ifndef __SYSTRAY_DISABLED__	
	if (theApp.amuledlg->IsIconized()) {
		if (theApp.glob_prefs->DoMinToTray()) {
			theApp.amuledlg->Hide_aMule(false);
		}
	} else {
		if (theApp.glob_prefs->DoMinToTray()) {
			if (theApp.IsReady) {
				theApp.amuledlg->Show_aMule(false);
			} else {
				theApp.amuledlg->Show_aMule(true);
			}
		}
	}
#endif
}

void CamuleDlg::OnBnNewPreferences(wxCommandEvent& ev) {

	if (theApp.IsReady)
		prefsunifiedwnd->ShowModal();
}

void CamuleDlg::OnBnClickedPrefOk(wxCommandEvent &event) {
	prefsunifiedwnd->EndModal(TRUE);
}

void CamuleDlg::OnBnClickedCancel(wxCommandEvent &event)
{
}

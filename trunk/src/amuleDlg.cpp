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


#include <cerrno>
#include <csignal>
#include <cmath>
#include <wx/toolbar.h>
#include <wx/utils.h>
#include <wx/file.h>
#include <wx/datetime.h>
#include <wx/config.h>

#ifndef __SYSTRAY_DISABLED__
#include "pixmaps/mule_TrayIcon.ico.xpm"
#include "pixmaps/mule_Tr_yellow.ico.xpm"
#include "pixmaps/mule_Tr_grey.ico.xpm"
#endif // __SYSTRAY_DISABLED__
#include "amuleDlg.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for CastItoIShort
#include "ED2KLink.h"		// Needed for CED2KLink
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "sockets.h"		// Needed for CServerConnect
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
#include "amule.h"			// Needed for theApp
#include "opcodes.h"		// Needed for TM_FINISHEDHASHING
#include "muuli_wdr.h"		// Needed for ID_BUTTONSERVERS

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
#include "aMule.xpm"
#endif

BEGIN_EVENT_TABLE(CamuleDlg, wxFrame)

	EVT_TOOL(ID_BUTTONSERVERS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSEARCH, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONTRANSFER, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSHARED, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONMESSAGES, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSTATISTICS, CamuleDlg::OnToolBarButton)

	EVT_TOOL(ID_BUTTONNEWPREFERENCES, CamuleDlg::OnPrefButton)
	
	EVT_TOOL(ID_BUTTONCONNECT, CamuleDlg::OnBnConnect)
	EVT_TIMER(ID_UQTIMER, CamuleDlg::OnUQTimer)

	EVT_MENU(TM_FINISHEDHASHING, CamuleDlg::OnFinishedHashing)
	EVT_MENU(TM_FILECOMPLETIONFINISHED, CamuleDlg::OnFinishedCompletion)
	EVT_CLOSE(CamuleDlg::OnClose)
	EVT_ICONIZE(CamuleDlg::OnMinimize)
	EVT_MENU(TM_HASHTHREADFINISHED, CamuleDlg::OnHashingShutdown)
	EVT_BUTTON(ID_BUTTON_FAST, CamuleDlg::OnBnClickedFast)
	EVT_BUTTON(ID_PREFS_OK_TOP, CamuleDlg::OnBnClickedPrefOk)
END_EVENT_TABLE()

void CamuleDlg::OnFinishedHashing(wxCommandEvent& evt)
{
	static int filecount = 0;
	static int bytecount = 0;

	CKnownFile* result = (CKnownFile*)evt.GetClientData();
	printf("Finished Hashing %s\n",result->GetFileName().c_str());
	if (evt.GetExtraLong()) {
		CPartFile* requester = (CPartFile*)evt.GetExtraLong();
		if (theApp.downloadqueue->IsPartFile(requester)) {
			requester->PartFileHashFinished(result);
		}
	} else {
		if (theApp.knownfiles->SafeAddKFile(result)) {
			theApp.sharedfiles->SafeAddKFile(result);
			
			filecount++;
			bytecount += result->GetFileSize();
			// If we have added 30 files or files with a total size of ~300mb
			if ( ( filecount == 30 ) || ( bytecount >= 314572800 ) ) {
				if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
					theApp.knownfiles->Save();
					filecount = 0;
					bytecount = 0;
				}
			}
		} else {
			delete result;
		}
	}

	return;
}

void CamuleDlg::OnFinishedCompletion(wxCommandEvent& evt)
{
	CPartFile* completed = (CPartFile*)evt.GetClientData();
	
	wxASSERT(completed);
	
	completed->CompleteFileEnded(evt.GetInt(), (wxString*)evt.GetExtraLong());

	return;
}


#ifndef wxCLOSE_BOX
#	define wxCLOSE_BOX 0
#endif

CamuleDlg::CamuleDlg(wxWindow* pParent, wxString title) : wxFrame(
	pParent, CamuleDlg::IDD, title, wxDefaultPosition, wxSize(800, 600),
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxTHICK_FRAME|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX )
{
	
	SetIcon(wxICON(aMule));

	m_app_state = APP_STATE_STARTING;
	srand(time(NULL));

	// get rid of sigpipe
#ifndef __WXMSW__
	signal(SIGPIPE, SIG_IGN);
#endif

	// Create new sizer and stuff a wxPanel in there.
	wxFlexGridSizer *s_main = new wxFlexGridSizer(1);
	s_main->AddGrowableCol(0);
	s_main->AddGrowableRow(0);
	
	wxPanel* p_cnt = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize);
	s_main->Add(p_cnt, 0, wxGROW|wxEXPAND, 0);
	muleDlg(p_cnt, false, true);

	SetSizer( s_main, true );
	
	// Create ToolBar from the one designed by wxDesigner (BigBob)
	m_wndToolbar = CreateToolBar( wxTB_HORIZONTAL|wxNO_BORDER|wxTB_TEXT|
	                                wxTB_3DBUTTONS|wxTB_FLAT|wxCLIP_CHILDREN );
	m_wndToolbar->SetToolBitmapSize(wxSize(32, 32));
	muleToolbar( m_wndToolbar );


	serverwnd = new CServerWnd(p_cnt);
	searchwnd = new CSearchDlg(p_cnt);
	transferwnd = new CTransferWnd(p_cnt);
	prefsunifiedwnd = new PrefsUnifiedDlg(p_cnt);
	sharedfileswnd = new CSharedFilesWnd(p_cnt);
	statisticswnd = new CStatisticsDlg(p_cnt);
	chatwnd = new CChatWnd(p_cnt);

	serverwnd->Show(FALSE);
	searchwnd->Show(FALSE);
	transferwnd->Show(FALSE);
	sharedfileswnd->Show(FALSE);
	statisticswnd->Show(FALSE);
	chatwnd->Show(FALSE);

	
	if (!LoadGUIPrefs()) {
		// Prefs not loaded for some reason, exit
		printf("ERROR!!! Unable to load Preferences\n");
		return;
	}

	
	// Set Serverlist as active window
	activewnd=NULL;
	SetActiveDialog(serverwnd);
	m_wndToolbar->ToggleTool(ID_BUTTONSERVERS, true );	

	CAddFileThread::Setup();

	ToggleFastED2KLinksHandler();
}


// Madcat - Toggles Fast ED2K Links Handler on/off.
void CamuleDlg::ToggleFastED2KLinksHandler()
{
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Hiding FED2KLH aborted."));
		return;
	}
	s_dlgcnt->Show(s_fed2klh, theApp.glob_prefs->GetFED2KLH());
	s_dlgcnt->Layout();
	searchwnd->ToggleLinksHandler();
}


void CamuleDlg::SetActiveDialog(wxWindow* dlg)
{
	switch ( dlg->GetId() ) {
		case IDD_SERVER:
		case IDD_SEARCH:
		case IDD_TRANSFER:
		case IDD_FILES:
		case IDD_CHAT:
		case IDD_STATISTICS:
			m_nActiveDialog = dlg->GetId();
			break;
		default:
			printf("Unknown window passed to SetActiveDialog!\n");
			return;
	}

	if ( activewnd ) {
		activewnd->Show(FALSE);
		contentSizer->Remove(activewnd);
	}
	
	contentSizer->Add(dlg, 1, wxALIGN_LEFT|wxEXPAND);
	dlg->Show(TRUE);
	activewnd=dlg;
	s_dlgcnt->Layout();
}


class QueryDlg : public wxDialog {
public:
	QueryDlg(wxWindow* parent) : wxDialog(
		parent, 21373, _("Desktop integration"), wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU )
	{
		wxSizer* content=desktopDlg(this, TRUE);
		content->Show(this, TRUE);
		Centre();
	};
protected:
	void OnOk(wxCommandEvent& WXUNUSED(evt)) { EndModal(0); };
	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(QueryDlg, wxDialog)
	EVT_BUTTON(ID_OK, QueryDlg::OnOk)
END_EVENT_TABLE()


void CamuleDlg::changeDesktopMode()
{
	QueryDlg query(this);

	wxRadioBox* radiobox = (wxRadioBox*)query.FindWindow(ID_SYSTRAYSELECT);
	
	if ( theApp.glob_prefs->GetDesktopMode() )
		radiobox->SetSelection( theApp.glob_prefs->GetDesktopMode() - 1 );
	else
		radiobox->SetSelection( 0 );
	
	query.ShowModal();
	
	theApp.glob_prefs->SetDesktopMode( radiobox->GetSelection() + 1 );
}


#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::CreateSystray(const wxString& title)
{
	// create the docklet (at this point we already have preferences!)
	if( !theApp.glob_prefs->GetDesktopMode() ) {
		// ok, it's not set yet.
		changeDesktopMode();
	}
	
	m_wndTaskbarNotifier = new CSysTray(this, theApp.glob_prefs->GetDesktopMode(), title);
}


void CamuleDlg::RemoveSystray()
{
	if (m_wndTaskbarNotifier) {
		delete m_wndTaskbarNotifier;
	}
}
#endif // __SYSTRAY_DISABLED__


extern void TimerProc();
void CamuleDlg::OnUQTimer(wxTimerEvent& WXUNUSED(evt))
{
	if( IsRunning() ) {
		TimerProc();
	}
}


void CamuleDlg::OnToolBarButton(wxCommandEvent& ev)
{
	static int lastbutton = ID_BUTTONSERVERS;
	
	// Kry - just if the app is ready for it
	if ( theApp.IsReady ) {

		if ( lastbutton != ev.GetId() ) {
			switch ( ev.GetId() ) {
				case ID_BUTTONSERVERS:
					SetActiveDialog(serverwnd);
					// Set serverlist splitter position
					((wxSplitterWindow*)FindWindow("SrvSplitterWnd"))->SetSashPosition(srv_split_pos, true);
					break;

				case ID_BUTTONSEARCH:
					SetActiveDialog(searchwnd);
					break;

				case ID_BUTTONTRANSFER:
					SetActiveDialog(transferwnd);
					// Set splitter position
					((wxSplitterWindow*)FindWindow("splitterWnd"))->SetSashPosition(split_pos, true);
					break;

				case ID_BUTTONSHARED:
					SetActiveDialog(sharedfileswnd);
					break;

				case ID_BUTTONMESSAGES:
					SetActiveDialog(chatwnd);
					break;

				case ID_BUTTONSTATISTICS:
					SetActiveDialog(statisticswnd);
					break;

				// This shouldn't happen, but just in case
				default:
					printf("Unknown button triggered CamuleApp::OnToolBarButton().\n");
					break;
			}
		}
		
		m_wndToolbar->ToggleTool(lastbutton, lastbutton == ev.GetId() );
		lastbutton = ev.GetId();
	}
}


void CamuleDlg::OnPrefButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( theApp.IsReady ) {
		prefsunifiedwnd->ShowModal();
	}
}

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");

	SaveGUIPrefs();

	theApp.OnlineSig(true);

	printf("aMule dialog destroyed\n");
}


void CamuleDlg::OnBnConnect(wxCommandEvent& WXUNUSED(evt))
{
	if (!theApp.serverconnect->IsConnected()) {
		//connect if not currently connected
		if (!theApp.serverconnect->IsConnecting()) {
			AddLogLine(true, _("Connecting"));
			theApp.serverconnect->ConnectToAnyServer();
			ShowConnectionState(false);
		} else {
			theApp.serverconnect->StopConnectionTry();
			ShowConnectionState(false);
		}
	} else {
		//disconnect if currently connected
		theApp.serverconnect->Disconnect();
		theApp.OnlineSig();
	}
}

void CamuleDlg::ResetLog(uint8 whichone)
{
	wxTextCtrl* ct = NULL;

	switch (whichone){
		case 1:
			ct=(wxTextCtrl*)serverwnd->FindWindow(ID_LOGVIEW);
			// Delete log file aswell.
			wxRemoveFile(wxString::Format("%s/.aMule/logfile", getenv("HOME")));
			break;
		case 2:
			ct=(wxTextCtrl*)serverwnd->FindWindow(ID_SERVERINFO);
			break;
		default:
			return;
	}

	if(ct) {
		ct->SetValue("");
	}
}


void CamuleDlg::ResetDebugLog()
{
	serverwnd->logbox.Clear();
}


void CamuleDlg::AddLogLine(bool addtostatusbar, const wxChar* line, ...)
{

	va_list argptr;
	va_start(argptr, line);
	wxString bufferline = wxString::FormatV( line, argptr );
	bufferline.Truncate(1000); // Max size 1000 chars
	va_end(argptr);

	// Remove newlines at end, they cause problems with the layout...
	while ( bufferline.Last() == '\n' )
		bufferline.RemoveLast();

	if (addtostatusbar) {
		wxStaticText* text=(wxStaticText*)FindWindow("infoLabel");
		text->SetLabel(bufferline);
		Layout();
	}

	bufferline = wxDateTime::Now().FormatDate() + wxT(" ") 
		+ wxDateTime::Now().FormatTime() + wxT(": ") 
		+ bufferline + wxT("\n");

	wxTextCtrl* ct= (wxTextCtrl*)serverwnd->FindWindow(ID_LOGVIEW);
	if(ct) {
		ct->AppendText(bufferline);
		ct->ShowPosition(ct->GetValue().Length()-1);
	}

	// Write into log file
	wxString filename = wxString::Format("%s/.aMule/logfile", getenv("HOME"));
	wxFile file(filename, wxFile::write_append);

	if ( file.IsOpened() ) {
		file.Write(bufferline.c_str());
		file.Close();
	}
}


void CamuleDlg::AddDebugLogLine(bool addtostatusbar, const wxChar* line, ...)
{
	if (theApp.glob_prefs->GetVerbose()) {
		va_list argptr;
		va_start(argptr, line);
		wxString bufferline = wxString::FormatV(line, argptr);
		bufferline.Truncate(1000); // Max size 1000 chars
		va_end(argptr);
		
		AddLogLine(addtostatusbar, "%s", bufferline.c_str());
	}
}


void CamuleDlg::AddServerMessageLine(char* line, ...)
{
	wxString content;
	va_list argptr;
	va_start(argptr, line);
	wxString bufferline = wxString::FormatV( line, argptr );
	bufferline.Truncate(500); // Max size 500 chars
	va_end(argptr);

	wxTextCtrl* cv=(wxTextCtrl*)serverwnd->FindWindow(ID_SERVERINFO);
	if(cv) {
		cv->AppendText(bufferline + "\n");
		cv->ShowPosition(cv->GetValue().Length()-1);
	}
}


void CamuleDlg::ShowConnectionState(bool connected, wxString server, bool iconOnly)
{
	enum state { sUnknown = -1, sDisconnected = 0, sLowID = 1, sConnecting = 2, sHighID = 3 };
	static state LastState = sUnknown;


	serverwnd->UpdateMyInfo();
	
	state NewState = sUnknown;
	if ( connected ) {
		if ( theApp.serverconnect->IsLowID() ) {
			NewState = sLowID;
		} else {
			NewState = sHighID;
		}
	} else if ( theApp.serverconnect->IsConnecting() ) {
		NewState = sConnecting;
	} else {
		NewState = sDisconnected;
	}


	if ( LastState != NewState ) {
		((wxStaticBitmap*)FindWindow("connImage"))->SetBitmap(connImages( NewState ));

		m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
		
		wxStaticText* connLabel = (wxStaticText*)FindWindow("connLabel");
		switch ( NewState ) {
			case sLowID:
			case sHighID: {
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, wxString(_("Disconnect")), connButImg(1), wxString(_("Disconnect from current server")));
				wxStaticText* tx=(wxStaticText*)FindWindow("infoLabel");
				tx->SetLabel(wxString(_("Connection established on:")) + wxString(server));
				connLabel->SetLabel(server);
				break;
			}
			
			case sConnecting:
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, wxString(_("Cancel")), connButImg(2), wxString(_("Stops the current connection attempts")));
				connLabel->SetLabel(wxString(_("Connecting")));
				break;
			
			case sDisconnected:
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, wxString(_("Connect")), connButImg(0), wxString(_("Connect to any server")));
				connLabel->SetLabel(wxString(_("Not Connected")));
				AddLogLine(true, wxString(_("Disconnected")));
				break;

			default:
				break;
		}
		
		m_wndToolbar->Realize();

		ShowUserCount(0, 0);
	}

	LastState = NewState;
}

void CamuleDlg::ShowUserCount(uint32 user_toshow, uint32 file_toshow)
{
	uint32 totaluser = 0, totalfile = 0;
	
	if( user_toshow || file_toshow ) {
		theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	}
	
	wxString buffer = wxString::Format( "%s: %s(%s) | %s: %s(%s)", _("Users"), CastItoIShort(user_toshow).c_str(), CastItoIShort(totaluser).c_str(), _("Files"), CastItoIShort(file_toshow).c_str(), CastItoIShort(totalfile).c_str());
	
	wxStaticCast(FindWindow("userLabel"), wxStaticText)->SetLabel(buffer);

	Layout();
}


void CamuleDlg::ShowTransferRate()
{
	float	kBpsUp = theApp.uploadqueue->GetKBps();
	float 	kBpsDown = theApp.downloadqueue->GetKBps();

	wxString buffer;
	if( theApp.glob_prefs->ShowOverhead() )
	{
		float overhead_up = theApp.uploadqueue->GetUpDatarateOverhead();
		float overhead_down = theApp.downloadqueue->GetDownDatarateOverhead();
		buffer.Printf(_("Up: %.1f(%.1f) | Down: %.1f(%.1f)"), kBpsUp, overhead_up/1024, kBpsDown, overhead_down/1024);
	} else {
		buffer.Printf(_("Up: %.1f | Down: %.1f"), kBpsUp, kBpsDown);
	}
	buffer.Truncate(50); // Max size 50

	((wxStaticText*)FindWindow("speedLabel"))->SetLabel(buffer);
	Layout();


#ifndef __SYSTRAY_DISABLED__
	// set trayicon-icon
	int percentDown = (int)ceil((kBpsDown*100) / theApp.glob_prefs->GetMaxGraphDownloadRate());
	UpdateTrayIcon( ( percentDown > 100 ) ? 100 : percentDown);

	wxString buffer2;
	if ( theApp.serverconnect->IsConnected() ) {
		buffer2.Printf("aMule (%s | %s)", buffer.c_str(), _("Connected") );
	} else {
		buffer2.Printf("aMule (%s | %s)", buffer.c_str(), _("Disconnected") );
	}
	char* buffer3 = nstrdup(buffer2.c_str());
	m_wndTaskbarNotifier->TraySetToolTip(buffer3);
	delete[] buffer3;
#endif

	wxStaticBitmap* bmp=(wxStaticBitmap*)FindWindow("transferImg");
	bmp->SetBitmap(dlStatusImages((kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)));
}


void CamuleDlg::OnHashingShutdown(wxCommandEvent& WXUNUSED(evt))
{
	if ( m_app_state != APP_STATE_SHUTINGDOWN ) {
		printf("Hashing thread ended\n");
		// CAddFileThread::Setup();
		// Save the known.met file
		theApp.knownfiles->Save();
	} else {
		printf("Hashing thread terminated, ready to shutdown\n");
		Destroy();
	}
}


void CamuleDlg::OnClose(wxCloseEvent& evt)
{
	// Are we already shutting down?
	if ( m_app_state == APP_STATE_SHUTINGDOWN )
		return;

	if (evt.CanVeto() && theApp.glob_prefs->IsConfirmExitEnabled() ) {
		if (wxNO == wxMessageBox(wxString(_("Do you really want to exit aMule?")), wxString(_("Exit confirmation")), wxYES_NO)) {
			evt.Veto();
			return;
		}
	}

	// we are going DOWN
	m_app_state = APP_STATE_SHUTINGDOWN;

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

	// saving data & stuff
	if (theApp.knownfiles) {
		theApp.knownfiles->Save();
	}

	if (theApp.glob_prefs) {
		theApp.glob_prefs->Add2TotalDownloaded(theApp.stat_sessionReceivedBytes);
		theApp.glob_prefs->Add2TotalUploaded(theApp.stat_sessionSentBytes);
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

}


#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::UpdateTrayIcon(int procent)
{
	// set the limits of where the bar color changes (low-high)
	int pLimits16[1] = {100};

	// set the corresponding color for each level
	COLORREF pColors16[1] = {statisticswnd->GetTrayBarColor()};  // ={theApp.glob_prefs->GetStatsColor(11)};

	// load our limit and color info
	m_wndTaskbarNotifier->SetColorLevels(pLimits16, pColors16, 1);

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
	m_wndTaskbarNotifier->TraySetIcon(data, true, pVals16);
}
#endif // __SYSTRAY_DISABLED__


//BEGIN - enkeyDEV(kei-kun) -TaskbarNotifier-
void CamuleDlg::ShowNotifier(wxString Text, int MsgType, bool ForceSoundOFF)
{
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-


void CamuleDlg::OnBnClickedFast(wxCommandEvent& WXUNUSED(evt))
{
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* bigbob = new wxMessageDialog(this, wxT(_("The ED2K link has been added but your download won't start until you connect to a server.")), wxT(_("Not Connected")), wxOK|wxICON_INFORMATION);
		bigbob->ShowModal();
		delete bigbob;
	}
	
	StartFast((wxTextCtrl*)FindWindow("FastEd2kLinks"));
}


// Pass pointer to textctrl which contains the links as argument
void CamuleDlg::StartFast(wxTextCtrl *ctl)
{
	for ( int i = 0; i < ctl->GetNumberOfLines(); i++ ) {
		wxString strlink = ctl->GetLineText(i);
		
		if ( strlink.IsEmpty() )
			continue;

		if ( strlink.Last() != '/' )
			strlink += "/";
			
		try {
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(strlink);
			
			if ( pLink ) {
				if( pLink->GetKind() == CED2KLink::kFile ) {
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink());
				} else {
					throw wxString("Bad link");
				}
				
				delete pLink;
			} else {
				printf("Failed to create ED2k link from URL\n");
			}
		}
		catch(wxString error) {
			wxString msg = wxString::Format( _("This ed2k link is invalid (%s)"), error.c_str() );
			theApp.amuledlg->AddLogLine( true, _("Invalid link: %s"), msg.c_str());
		}
	}
ctl->SetValue("");
}


// Formerly known as LoadRazorPrefs()
bool CamuleDlg::LoadGUIPrefs()
{
	// Create a config base for loading razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without loading
	if (config == NULL) {
		return false;
	}

	// The section where to save in in file
	wxString section = "/Razor_Preferences/";

	// Get window size and position
	int x1 = config->Read(_T(section+"MAIN_X_POS"), -1l);
	int y1 = config->Read(_T(section+"MAIN_Y_POS"), -1l);
	int x2 = config->Read(_T(section+"MAIN_X_SIZE"), 0l);
	int y2 = config->Read(_T(section+"MAIN_Y_SIZE"), 0l);

	split_pos = config->Read(_T(section+"SPLITTER_POS"), 463l);
	// Kry - Random usable pos for srv_split_pos
	srv_split_pos = config->Read(_T(section+"SRV_SPLITTER_POS"), 463l);

	// If x1 and y1 != 0 Redefine location
	if((x1 != -1) && (y1 != -1)) {
		Move(x1, y1);
	}

	if (x2 > 0 && y2 > 0) {
		SetClientSize(x2, y2 - 58);
	} else {
		SetClientSize(800, 542);
	}

	return true;
}


bool CamuleDlg::SaveGUIPrefs()
{
	/* Razor 1a - Modif by MikaelB
	   Save client size and position */

	// Create a config base for saving razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without saving
	if (config == NULL) {
		return false;
	}
	// The section where to save in in file
	wxString section = "/Razor_Preferences/";

	// Main window location and size
	int x1, y1, x2, y2;
	GetPosition(&x1, &y1);
	GetSize(&x2, &y2);

	// Saving window size and position
	config->Write(_T(section+"MAIN_X_POS"), (long) x1);
	config->Write(_T(section+"MAIN_Y_POS"), (long) y1);
	config->Write(_T(section+"MAIN_X_SIZE"), (long) x2);
	config->Write(_T(section+"MAIN_Y_SIZE"), (long) y2);

	// Saving sash position of splitter in transfer window
	config->Write(_T(section+"SPLITTER_POS"), (long) split_pos);

	// Saving sash position of splitter in server window
	config->Write(_T(section+"SRV_SPLITTER_POS"), (long) srv_split_pos);

	config->Flush(true);

	/* End modif */

	return true;
}


//hides amule
void CamuleDlg::Hide_aMule(bool iconize)
{
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
void CamuleDlg::Show_aMule(bool uniconize)
{

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


void CamuleDlg::OnMinimize(wxIconizeEvent& evt)
{
#ifndef __SYSTRAY_DISABLED__
	wxPoint unused;
	if (!wxFindWindowAtPointer(unused)) {
		return;
	}
	
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


void CamuleDlg::OnBnClickedPrefOk(wxCommandEvent& WXUNUSED(event))
{
	prefsunifiedwnd->EndModal(TRUE);
}

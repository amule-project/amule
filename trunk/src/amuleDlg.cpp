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

#include <wx/sizer.h> // Must be first or compilation fail on win32 !!!
#include <cerrno>
#include <csignal>
#include <cmath>
#include <curl/curl.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>
#include <wx/utils.h>
#include <wx/file.h>
#include <wx/datetime.h>
#include <wx/config.h>
#include <wx/textfile.h>
#include <wx/radiobox.h>
#include <wx/msgdlg.h>
#include <wx/log.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/mimetype.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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
#include "ChatWnd.h"		// Needed for CChatWnd
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "KadDlg.h"		// Needed for CKadDlg
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
#include "PrefsUnifiedDlg.h"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__) || defined(__WXX11__)
#include "aMule.xpm"
#endif

#ifdef __USE_SPLASH__
#include "splash.xpm"
#endif

BEGIN_EVENT_TABLE(CamuleDlg, wxFrame)

	EVT_TOOL(ID_BUTTONSERVERS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSEARCH, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONTRANSFER, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSHARED, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONMESSAGES, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSTATISTICS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONKAD, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_ABOUT, CamuleDlg::OnAboutButton)
	
	EVT_TOOL(ID_BUTTONNEWPREFERENCES, CamuleDlg::OnPrefButton)

	EVT_TOOL(ID_BUTTONCONNECT, CamuleDlg::OnBnConnect)

	EVT_CLOSE(CamuleDlg::OnClose)
	EVT_ICONIZE(CamuleDlg::OnMinimize)

	EVT_BUTTON(ID_BUTTON_FAST, CamuleDlg::OnBnClickedFast)
	

	EVT_TIMER(ID_GUITIMER, CamuleDlg::OnGUITimer)

END_EVENT_TABLE()

#ifndef wxCLOSE_BOX
#	define wxCLOSE_BOX 0
#endif

CamuleDlg::CamuleDlg(wxWindow* pParent, const wxString &title, wxPoint where, wxSize dlg_size) : wxFrame(
	pParent, -1, title, where, dlg_size,
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxTHICK_FRAME|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX,wxT("aMule") )
{

	wxInitAllImageHandlers();
	curl_global_init(CURL_GLOBAL_ALL);
	imagelist.Create(16,16);
	
	if (theApp.glob_prefs->UseSkin()) {		
		Apply_Clients_Skin(theApp.glob_prefs->GetSkinFile());
	} else {
		for (uint32 i=0; i<22; i++) {
			imagelist.Add(wxBitmap(clientImages(i)));
		}
	}

	bool override_where = (where != wxDefaultPosition);
	bool override_size = ((dlg_size.x != DEFAULT_SIZE_X) || (dlg_size.y != DEFAULT_SIZE_Y));

	if (!LoadGUIPrefs(override_where, override_size)) {
		// Prefs not loaded for some reason, exit
		printf("ERROR!!! Unable to load Preferences\n");
		return;
	}

	is_safe_state = false;
	is_hidden = false;

	SetIcon(wxICON(aMule));

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

	Create_Toolbar(wxT(""));

	serverwnd = new CServerWnd(p_cnt);

	AddLogLine(true, GetMuleVersion());
// GTK2 doesn't play nice with wxWidget versions earlier than 2.5.1, so warn the user
#if !wxCHECK_VERSION(2, 5, 1)
	#ifdef __WXGTK20__
		AddLogLine(false, _("WARNING!"));
		AddLogLine(false, _("\tYou are running aMule linked against GTK2!"));
		AddLogLine(false, _("\tThis is known to cause problems, with the version of wxWidgets that you are using,"));
		AddLogLine(false, _("\tso it is recommended that that you recompile the nescesarry packages."));
		AddLogLine(false, _("\tPlease refer to the following websites for more information:"));
		AddLogLine(false, wxT("\t\thttp://wiki.amule.org"));
		AddLogLine(false, wxT("\t\thttp://www.amule.org"));
		AddLogLine(false, _("WARNING!"));
	#endif
#endif

	searchwnd = new CSearchDlg(p_cnt);
	transferwnd = new CTransferWnd(p_cnt);
	sharedfileswnd = new CSharedFilesWnd(p_cnt);
	statisticswnd = new CStatisticsDlg(p_cnt);
	chatwnd = new CChatWnd(p_cnt);
	kadwnd = new CKadDlg(p_cnt);

	serverwnd->Show(FALSE);
	searchwnd->Show(FALSE);
	transferwnd->Show(FALSE);
	sharedfileswnd->Show(FALSE);
	statisticswnd->Show(FALSE);
	chatwnd->Show(FALSE);
	kadwnd->Show(FALSE);	

	// Create the GUI timer
	gui_timer=new wxTimer(this,ID_GUITIMER);
	if (!gui_timer) {
		AddLogLine(false, _("Fatal Error: Failed to create Timer"));
	}

	// Set Serverlist as active window
	activewnd=NULL;
	SetActiveDialog(ServerWnd, serverwnd);
	m_wndToolbar->ToggleTool(ID_BUTTONSERVERS, true );
	#ifndef __USE_KAD__
	m_wndToolbar->RemoveTool(ID_BUTTONKAD);
	#endif
	ToggleFastED2KLinksHandler();

	is_safe_state = true;

	Show(TRUE);

#ifndef __SYSTRAY_DISABLED__
	CreateSystray(wxString::Format(wxT("%s %s"), wxT(PACKAGE), wxT(VERSION)));
#endif

	// splashscreen
	#ifdef __USE_SPLASH__
	if (theApp.glob_prefs->UseSplashScreen() && !theApp.glob_prefs->GetStartMinimized()) {
		new wxSplashScreen( wxBitmap(splash_xpm),
		                    wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
		                    5000, NULL, -1, wxDefaultPosition, wxDefaultSize,
		                    wxSIMPLE_BORDER|wxSTAY_ON_TOP
		);
	}
	#endif

	// Init statistics stuff, better do it asap
	statisticswnd->Init();
	statisticswnd->SetUpdatePeriod();

	// must do initialisations here..
	serverwnd->serverlistctrl->Init(theApp.serverlist);

	// call the initializers
	transferwnd->OnInitDialog();

	searchwnd->UpdateCatChoice();

	// Must we start minimized?
	if (theApp.glob_prefs->GetStartMinimized() && (theApp.glob_prefs->GetDesktopMode() != 4)) {
		#ifndef __SYSTRAY_DISABLED__
		// Send it to tray?
		if (theApp.glob_prefs->DoMinToTray()) {
			Hide_aMule();
		} else {
			Iconize(TRUE);
		}
		#else
			Iconize(TRUE);
		#endif
	}

}

void CamuleDlg::InitSort()
{
	// Initialize and sort all lists.
	transferwnd->downloadlistctrl->InitSort();
	transferwnd->uploadlistctrl->InitSort();
	transferwnd->queuelistctrl->InitSort();
	serverwnd->serverlistctrl->InitSort();
	sharedfileswnd->sharedfilesctrl->InitSort();
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


void CamuleDlg::SetActiveDialog(DialogType type, wxWindow* dlg)
{
	m_nActiveDialog = type;
	
	if ( type == TransferWnd ) {
		if (theApp.glob_prefs->ShowCatTabInfos()) {
			transferwnd->UpdateCatTabTitles();
		}
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

	m_wndTaskbarNotifier = new CSysTray(this, (DesktopMode)theApp.glob_prefs->GetDesktopMode(), title);
}


void CamuleDlg::RemoveSystray()
{
	if (m_wndTaskbarNotifier) {
		delete m_wndTaskbarNotifier;
	}
}
#endif // __SYSTRAY_DISABLED__


void CamuleDlg::OnToolBarButton(wxCommandEvent& ev)
{
	static int lastbutton = ID_BUTTONSERVERS;

	// Kry - just if the app is ready for it
	if ( theApp.IsReady ) {

		if ( lastbutton != ev.GetId() ) {
			switch ( ev.GetId() ) {
				case ID_BUTTONSERVERS:
					SetActiveDialog(ServerWnd, serverwnd);
					// Set serverlist splitter position
					((wxSplitterWindow*)FindWindow(wxT("SrvSplitterWnd")))->SetSashPosition(srv_split_pos, true);
					break;

				case ID_BUTTONSEARCH:
					SetActiveDialog(SearchWnd, searchwnd);
					break;

				case ID_BUTTONTRANSFER:
					SetActiveDialog(TransferWnd, transferwnd);
					// Set splitter position
					((wxSplitterWindow*)FindWindow(wxT("splitterWnd")))->SetSashPosition(split_pos, true);
					break;

				case ID_BUTTONSHARED:
					SetActiveDialog(SharedWnd, sharedfileswnd);
					break;

				case ID_BUTTONMESSAGES:
					SetActiveDialog(ChatWnd, chatwnd);
					break;

				case ID_BUTTONSTATISTICS:
					SetActiveDialog(StatsWnd, statisticswnd);
					break;

				case ID_BUTTONKAD:
					SetActiveDialog(KadWnd, kadwnd);
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


void CamuleDlg::OnAboutButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( theApp.IsReady )
		wxMessageBox(wxString::wxString( _("aMule - 'all-platform' p2p client based on eMule.\n\n Website: http://www.amule.org\n Forum: http://forum.amule.org \n FAQ: http://wiki.amule.org \n\n Copyright (C) 2003-2004 aMule Project \n")));
}

void CamuleDlg::OnPrefButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( theApp.IsReady ) {
		// Check if there's already a dialog
		if ( PrefsUnifiedDlg::GetPrefsID() ) {
			// Raise the preferences-dialog.
			FindWindow( PrefsUnifiedDlg::GetPrefsID() )->Raise();
		} else {
			// No existing dialog, create a new one	
			PrefsUnifiedDlg* prefswnd = new PrefsUnifiedDlg( this );
		
			prefswnd->TransferToWindow();
		
			prefswnd->Show();
		}
	}
}

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");
	
	curl_global_cleanup();
	
	SaveGUIPrefs();

	theApp.OnlineSig(true);

	theApp.amuledlg = NULL;
	
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
		case 1: {
			ct=(wxTextCtrl*)serverwnd->FindWindow(ID_LOGVIEW);
			// Delete log file aswell.
			wxString logname(theApp.ConfigDir + wxT("logfile"));
			wxRemoveFile(logname);
			wxTextFile file(logname);
			if (!file.Create()) {
				printf("Error creating log file!\n");
				return;
			}
			file.Close();
			break;
		}
		case 2:
			ct=(wxTextCtrl*)serverwnd->FindWindow(ID_SERVERINFO);
			break;
		default:
			return;
	}

	if(ct) {
		ct->SetValue(wxT(""));
	}
}


void CamuleDlg::ResetDebugLog()
{
#warning This isnt the right textctrl!
//	serverwnd->logbox.Clear();
}


void CamuleDlg::AddLogLine(bool addtostatusbar, const wxString& line)
{
	// Max 1000 chars
	wxString bufferline = line.Left(1000);
	// Remove newlines at end, they cause problems with the layout...
	// You should not call Last() in an empty string.
	while ( !bufferline.IsEmpty() && bufferline.Last() == wxT('\n') ) {
		bufferline.RemoveLast();
	}
	// Escape "&"s, which would otherwise not show up
	bufferline.Replace(wxT("&"), wxT("&&"));
	if (addtostatusbar) {
		wxStaticText* text = (wxStaticText *)FindWindow(wxT("infoLabel"));
		text->SetLabel(bufferline);
		Layout();
	}
	bufferline = wxDateTime::Now().FormatDate() + wxT(" ")
		+ wxDateTime::Now().FormatTime() + wxT(": ")
		+ bufferline + wxT("\n");
	wxTextCtrl* ct = (wxTextCtrl*)serverwnd->FindWindow(ID_LOGVIEW);
	if ( ct ) {
		ct->AppendText(bufferline);
		ct->ShowPosition(ct->GetValue().Length()-1);
	}
	// Write into log file
	wxString filename = theApp.ConfigDir + wxT("logfile");
	wxTextFile file(filename);
	if (!file.Open()) {
		printf("Error opening log file!\n");
	}
	file.AddLine(bufferline);
	file.Write();
	file.Close();
}


void CamuleDlg::AddDebugLogLine(bool addtostatusbar, const wxString& line)
{
	if (theApp.glob_prefs->GetVerbose()) {
		AddLogLine(addtostatusbar, line);
	}
}


void CamuleDlg::AddServerMessageLine(wxString& message)
{
	wxTextCtrl* cv=(wxTextCtrl*)serverwnd->FindWindow(ID_SERVERINFO);
	if(cv) {
		if (message.Length() > 500) {
			cv->AppendText(message.Left(500) + wxT("\n"));
		} else {
			cv->AppendText(message + wxT("\n"));
		}
		cv->ShowPosition(cv->GetValue().Length()-1);
	}
}


void CamuleDlg::ShowConnectionState(bool connected, const wxString &server)
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
		((wxStaticBitmap *)FindWindow(wxT("connImage")))->SetBitmap(connImages(NewState));
		m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
		wxStaticText* connLabel = (wxStaticText*)FindWindow(wxT("connLabel"));
		switch ( NewState ) {
			case sLowID:
			case sHighID: {
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Disconnect"),
					connButImg(1), wxNullBitmap, wxITEM_NORMAL,
					_("Disconnect from current server"));
				wxStaticText* tx = (wxStaticText *)FindWindow(wxT("infoLabel"));
				tx->SetLabel(_("Connection established on:") + server);
				connLabel->SetLabel(server);
				break;
			}
			case sConnecting:
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Cancel"),
					connButImg(2), wxNullBitmap, wxITEM_NORMAL,
					_("Stops the current connection attempts"));
				connLabel->SetLabel(_("Connecting"));
				break;

			case sDisconnected:
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Connect"),
					connButImg(0), wxNullBitmap, wxITEM_NORMAL,
					_("Connect to any server"));
				connLabel->SetLabel(_("Not Connected"));
				AddLogLine(true, _("Disconnected"));
				break;

			default:
				break;
		}
		m_wndToolbar->Realize();
		theApp.OnlineSig();
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

	wxString buffer = 	_("Users: ") +
							CastItoIShort(user_toshow) + wxT(" (") + CastItoIShort(totaluser) +
							wxT(") | ") +
							 _("Files: ") +
							CastItoIShort(file_toshow) + wxT(" (") + CastItoIShort(totalfile) +
							wxT(")");

	wxStaticCast(FindWindow(wxT("userLabel")), wxStaticText)->SetLabel(buffer);

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

	((wxStaticText*)FindWindow(wxT("speedLabel")))->SetLabel(buffer);
	Layout();


#ifndef __SYSTRAY_DISABLED__
	// set trayicon-icon
	int percentDown = (int)ceil((kBpsDown*100) / theApp.glob_prefs->GetMaxGraphDownloadRate());
	UpdateTrayIcon( ( percentDown > 100 ) ? 100 : percentDown);

	wxString buffer2;
	if ( theApp.serverconnect->IsConnected() ) {
		buffer2 = wxT("aMule (") +buffer + wxT(" | ") + _("Connected") + wxT(")");
	} else {
		buffer2 = wxT("aMule (") +buffer + wxT(" | ") +  _("Disconnected") + wxT(")");
	}
	m_wndTaskbarNotifier->SetTrayToolTip(buffer2);
#endif

	wxStaticBitmap* bmp=(wxStaticBitmap*)FindWindow(wxT("transferImg"));
	bmp->SetBitmap(dlStatusImages((kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)));
}

void CamuleDlg::OnClose(wxCloseEvent& evt)
{
	// Are we already shutting down or still on init?
	if ( is_safe_state == false ) {
		return;
	}

	if (evt.CanVeto() && theApp.glob_prefs->IsConfirmExitEnabled() ) {
		if (wxNO == wxMessageBox(wxString(_("Do you really want to exit aMule?")), wxString(_("Exit confirmation")), wxYES_NO)) {
			evt.Veto();
			return;
		}
	}

	// we are going DOWN
	is_safe_state = false;

	// Stop the GUI Timer
	delete gui_timer;

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
		PrefsUnifiedDlg::SaveAllItems( wxConfig::Get() );
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

	#warning This will be here till the core close is != app close
	theApp.ShutDown();

}


#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::UpdateTrayIcon(int procent)
{
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
	m_wndTaskbarNotifier->SetTrayIcon(data, pVals16);
}
#endif // __SYSTRAY_DISABLED__


//BEGIN - enkeyDEV(kei-kun) -TaskbarNotifier-
void CamuleDlg::ShowNotifier(wxString WXUNUSED(Text), int WXUNUSED(MsgType), bool WXUNUSED(ForceSoundOFF))
{
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-


void CamuleDlg::OnBnClickedFast(wxCommandEvent& WXUNUSED(evt))
{
	if (!theApp.serverconnect->IsConnected()) {
		wxMessageDialog* bigbob = new wxMessageDialog(this, wxT("The ED2K link has been added but your download won't start until you connect to a server."), wxT("Not Connected"), wxOK|wxICON_INFORMATION);
		bigbob->ShowModal();
		delete bigbob;
	}

	StartFast((wxTextCtrl*)FindWindow(wxT("FastEd2kLinks")));
}


// Pass pointer to textctrl which contains the links as argument
void CamuleDlg::StartFast(wxTextCtrl *ctl)
{
	for ( int i = 0; i < ctl->GetNumberOfLines(); i++ ) {
		wxString strlink = ctl->GetLineText(i);
		strlink.Trim(true);
		strlink.Trim(false);
		if ( strlink.IsEmpty() )
			continue;

		if ( strlink.Last() != '/' )
			strlink += wxT("/");

		try {
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(unicode2char(strlink));

			if ( pLink ) {
				if( pLink->GetKind() == CED2KLink::kFile ) {
					uint8 cat = transferwnd->downloadlistctrl->curTab;
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink(), cat);
				} else {
					throw wxString(wxT("Bad link"));
				}

				delete pLink;
			} else {
				printf("Failed to create ED2k link from URL\n");
			}
		}
		catch(wxString error) {
			wxString msg = _("This ed2k link is invalid: ") + error;
			AddLogLineM( true, _("Invalid link: ") + msg);
		}
	}
ctl->SetValue(wxT(""));
}


// Formerly known as LoadRazorPrefs()
bool CamuleDlg::LoadGUIPrefs(bool override_pos, bool override_size)
{
	// Create a config base for loading razor preferences
	wxConfigBase *config = wxConfigBase::Get();
	// If config haven't been created exit without loading
	if (config == NULL) {
		return false;
	}

	// The section where to save in in file
	wxString section = wxT("/Razor_Preferences/");

	// Get window size and position
	int x1 = config->Read(section+wxT("MAIN_X_POS"), -1l);
	int y1 = config->Read(section+wxT("MAIN_Y_POS"), -1l);
	int x2 = config->Read(section+wxT("MAIN_X_SIZE"), 0l);
	int y2 = config->Read(section+wxT("MAIN_Y_SIZE"), 0l);

	split_pos = config->Read(section+wxT("SPLITTER_POS"), 463l);
	// Kry - Random usable pos for srv_split_pos
	srv_split_pos = config->Read(section+wxT("SRV_SPLITTER_POS"), 463l);

	if (!override_pos) {
		// If x1 and y1 != 0 Redefine location
		if((x1 != -1) && (y1 != -1)) {
			Move(x1, y1);
		}
	}

	if (!override_size) {
		if (x2 > 0 && y2 > 0) {
			SetClientSize(x2, y2);
		}
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
	wxString section = wxT("/Razor_Preferences/");

	// Main window location and size
	int x1, y1, x2, y2;
	GetPosition(&x1, &y1);
	GetSize(&x2, &y2);

	// Saving window size and position
	config->Write(section+wxT("MAIN_X_POS"), (long) x1);
	config->Write(section+wxT("MAIN_Y_POS"), (long) y1);
	config->Write(section+wxT("MAIN_X_SIZE"), (long) x2);
	config->Write(section+wxT("MAIN_Y_SIZE"), (long) y2);

	// Saving sash position of splitter in transfer window
	config->Write(section+wxT("SPLITTER_POS"), (long) split_pos);

	// Saving sash position of splitter in server window
	config->Write(section+wxT("SRV_SPLITTER_POS"), (long) srv_split_pos);

	config->Flush(true);

	/* End modif */

	return true;
}


//hides amule
void CamuleDlg::Hide_aMule(bool iconize)
{

	if (!is_hidden) {
		transferwnd->downloadlistctrl->Freeze();
		transferwnd->uploadlistctrl->Freeze();
		serverwnd->serverlistctrl->Freeze();
		sharedfileswnd->sharedfilesctrl->Freeze();
		transferwnd->downloadlistctrl->Show(FALSE);
		serverwnd->serverlistctrl->Show(FALSE);
		transferwnd->uploadlistctrl->Show(FALSE);
		sharedfileswnd->sharedfilesctrl->Show(FALSE);
		Freeze();
		if (iconize) {
			Iconize(TRUE);
		}
		Show(FALSE);

		is_hidden = true;

	}

}


//shows amule
void CamuleDlg::Show_aMule(bool uniconize)
{

	if (is_hidden) {

		transferwnd->downloadlistctrl->Show(TRUE);
		transferwnd->uploadlistctrl->Show(TRUE);
		serverwnd->serverlistctrl->Show(TRUE);
		sharedfileswnd->sharedfilesctrl->Show(TRUE);
		transferwnd->downloadlistctrl->Thaw();
		serverwnd->serverlistctrl->Thaw();
		transferwnd->uploadlistctrl->Thaw();
		sharedfileswnd->sharedfilesctrl->Thaw();
		Thaw();
		Update();
		Refresh();
		if (uniconize) {
			Show(TRUE);
		}

		is_hidden = false;

	}

}


void CamuleDlg::OnMinimize(wxIconizeEvent& evt)
{
#ifndef __SYSTRAY_DISABLED__
	if (theApp.glob_prefs->DoMinToTray() && (theApp.glob_prefs->GetDesktopMode() != 4)) {
		if (evt.Iconized()) {
			Hide_aMule(false);
		} else {
			if (SafeState()) {
				Show_aMule(true);
			} else {
				Show_aMule(false);
			}
		}
	}
#endif
}


void CamuleDlg::OnGUITimer(wxTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section

	static uint32	msPrev1, msPrev5, msPrevGraph, msPrevStats;
	static uint32	msPrevHist;

	uint32 			msCur = theApp.GetUptimeMsecs();

	// can this actually happen under wxwin ?
	if (!SafeState()) {
		return;
	}

	if (msCur-msPrevHist > 1000) {
		// unlike the other loop counters in this function this one will sometimes
		// produce two calls in quick succession (if there was a gap of more than one
		// second between calls to TimerProc) - this is intentional!  This way the
		// history list keeps an average of one node per second and gets thinned out
		// correctly as time progresses.
		msPrevHist += 1000;
		
		statisticswnd->RecordHistory();
		
	}

	if (msCur-msPrev1 > 950) {  // approximately every second
		msPrev1 = msCur;
		statisticswnd->UpdateConnectionsStatus();
	}

	bool bStatsVisible = (!IsIconized() && StatisticsWindowActive());
	int msGraphUpdate= theApp.glob_prefs->GetTrafficOMeterInterval()*1000;
	if ((msGraphUpdate > 0)  && ((msCur / msGraphUpdate) > (msPrevGraph / msGraphUpdate))) {
		// trying to get the graph shifts evenly spaced after a change in the update period
		msPrevGraph = msCur;
		
		statisticswnd->UpdateStatGraphs(bStatsVisible);
	
	}

	int sStatsUpdate = theApp.glob_prefs->GetStatsInterval();
	if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
		if (bStatsVisible) {
			msPrevStats = msCur;
			statisticswnd->ShowStatistics();
		}
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		ShowTransferRate();
		if (theApp.glob_prefs->ShowCatTabInfos() && theApp.amuledlg->activewnd == theApp.amuledlg->transferwnd) {
			theApp.amuledlg->transferwnd->UpdateCatTabTitles();
		}
	}

}


/*
	Try to launch the specified url:
	 - Windows: The default browser will be used.
	 - Mac: Currently not implemented
	 - Anything else: Try a number of hardcoded browsers. Should be made configuable...
*/
void CamuleDlg::LaunchUrl( const wxString& url )
{
	wxString cmd;

#ifdef __WXMAC__

	#if 0
	// Kry -Uh?
	wxString url1(url);
	if (url1.Left(5) != wxT("file:"))
		url1 = wxNativePathToURL(url1);

	OSStatus err;
	ICInstance inst;
	SInt32 startSel;
	SInt32 endSel;

	err = ICStart(&inst, 'STKA'); // put your app creator code here
	if (err == noErr) {
		#if !TARGET_CARBON
		err = ICFindConfigFile(inst, 0, nil);
		#endif
		if (err == noErr) {
		startSel = 0;
		endSel = wxStrlen(url1);
		err = ICLaunchURL(inst, "\p", url1, endSel, &startSel, &endSel);
		}
		ICStop(inst);
	}
	#endif


#elif defined (__WXMSW__)
wxFileType *ft;                            /* Temporary storage for filetype. */

	ft = wxTheMimeTypesManager->GetFileTypeFromExtension(wxT("html"));
	if (!ft) {
		wxLogError(
			wxT("Impossible to determine the file type for extension html."
			"Please edit your MIME types.")
		);
		return;
	}

	if (!ft->GetOpenCommand(&cmd, wxFileType::MessageParameters(url, _T("")))) {
		// TODO: some kind of configuration dialog here.
		wxMessageBox(
			_("Could not determine the command for running the browser."),
			wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION);
		delete ft;
		return;
	}
	delete ft;

	wxPuts(wxT("Launch Command: ") + cmd);
	if (!wxExecute(cmd, FALSE)) {
		wxLogError(wxT("Error launching browser for FakeCheck."));
	}
#else

	cmd = theApp.glob_prefs->GetBrowser();
	if ( !cmd.IsEmpty() ) {
		wxString tmp = url;
		// Pipes cause problems, so escape them
		tmp.Replace( wxT("|"), wxT("%7C") );


		if ( !cmd.Replace( wxT("%s"), tmp ) ) {
			// No %s found, just append the url
			cmd += tmp;
		}

		if ( wxExecute( cmd, false ) ) {
			printf( "Launch Command: %s\n", unicode2char(cmd));
			return;
		}
	}

	// Unable to execute browser. But this error message doesn't make sense,
	// cosidering that you _can't_ set the browser executable path... =/
	wxLogError( _("Unable to launch browser. Please set correct browser executable path in Preferences.") );

#endif

}



wxString CamuleDlg::GenWebSearchUrl(const wxString &filename, WebSearch provider )
{
	wxString URL;

	switch ( provider )  {
		case wsFileHash:
			URL = wxT("http://www.filehash.com/search.html?pattern=FILENAME&submit=Find");
			break;
		case wsJugle:
			URL = wxT("http://jugle.net/index.php?searchstring=FILENAME");
			break;
		default:
			wxASSERT(0);
	}
		
    URL.Replace(wxT("FILENAME"), filename);
    
	return URL;
}


struct SkinItem {
	bool found;
	wxString filename;
};


enum ClientSkinEnum {
	
	Client_Green_Smiley = 0,
	Client_Red_Smiley,
	Client_Yellow_Smiley,
	Client_Grey_Smiley,
	Client_White_Smiley,
	Client_BadComment_Smiley,
	Client_GoodComment_Smiley,
	Client_ExtendedProtocol_Smiley,
	Client_SecIdent_Smiley,
	Client_BadGuy_Smiley,
	Client_CreditsGrey_Smiley,
	Client_CreditsYellow_Smiley,
	Client_Upload_Smiley,
	Client_Friend_Smiley,
	Client_eMule_Smiley,
	Client_mlDonkey_Smiley,
	Client_eDonkeyHybrid_Smiley,
	Client_aMule_Smiley,
	Client_lphant_Smiley,
	Client_Shareazza_Smiley,
	Client_xMule_Smiley,
	Client_Unknown,
	// Add items here.
	UNUSED
};


void CamuleDlg::Apply_Clients_Skin(wxString file) {
	
	#define ClientItemNumber UNUSED+1
	
	SkinItem bitmaps_found[ClientItemNumber];
	
	for (uint32 i=0; i<ClientItemNumber; i++) {	
		bitmaps_found[i].found = false;
	}
	
	wxTextFile skinfile(file);
	
	try {
		
		if (file.IsEmpty()) {
			throw(_("Skin file name is empty - loading defaults"));
		}
		
		if (!::wxFileExists(file)) {
			throw(_("Skin file ") + file + _(" does not exist - loading defaults"));
		}
			
		if (!skinfile.Open()) {
			throw(_("Unable to open skin file: ") + file);
		}
		
		uint32 client_header_found = 0;
		
		for (uint32 i=0; i < skinfile.GetLineCount(); i++) {
			if (skinfile[i] == wxT("[Client Bitmaps]")) {
				client_header_found = i;	
				break;
			}
		}
		
		
		if (client_header_found) {
			
			wxImage new_image;
			
			for (uint32 i=client_header_found; i < skinfile.GetLineCount(); i++) {
				if (skinfile[i].StartsWith(wxT("["))) {
					break;
				}				
				// Client_Green_Smiley
				if (skinfile[i].StartsWith(wxT("Client_Transfer="))) {
					bitmaps_found[Client_Green_Smiley].found = true;
					bitmaps_found[Client_Green_Smiley].filename = skinfile[i].AfterLast(wxT('='));
				}
				// Client_Red_Smiley
				if (skinfile[i].StartsWith(wxT("Client_Connecting="))) {
					bitmaps_found[Client_Red_Smiley].found = true;
					bitmaps_found[Client_Red_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Yellow_Smiley
				if (skinfile[i].StartsWith(wxT("Client_OnQueue="))) {
					bitmaps_found[Client_Yellow_Smiley].found = true;
				    bitmaps_found[Client_Yellow_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Grey_Smiley
				if (skinfile[i].StartsWith(wxT("Client_A4AFNoNeededPartsQueueFull="))) {
				    bitmaps_found[Client_Grey_Smiley].found = true;
				    bitmaps_found[Client_Grey_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Unknown_Smiley
				if (skinfile[i].StartsWith(wxT("Client_Unknown="))) {
					bitmaps_found[Client_White_Smiley].found = true;
				    bitmaps_found[Client_White_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Bad_Comment_On_File
				if (skinfile[i].StartsWith(wxT("Client_BadCommentOnFile="))) {
					bitmaps_found[Client_BadComment_Smiley].found = true;
				    bitmaps_found[Client_BadComment_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Good_Comment_On_File
				if (skinfile[i].StartsWith(wxT("Client_GoodCommentOnFile="))) {
					bitmaps_found[Client_GoodComment_Smiley].found = true;
				    bitmaps_found[Client_GoodComment_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Extended_Protocol
				if (skinfile[i].StartsWith(wxT("Client_ExtendedProtocol="))) {
					bitmaps_found[Client_ExtendedProtocol_Smiley].found = true;
				    bitmaps_found[Client_ExtendedProtocol_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_SecIdent
				if (skinfile[i].StartsWith(wxT("Client_SecIdent="))) {
					bitmaps_found[Client_SecIdent_Smiley].found = true;
				    bitmaps_found[Client_SecIdent_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_BadGuy
				if (skinfile[i].StartsWith(wxT("Client_BadGuy="))) {
				    bitmaps_found[Client_BadGuy_Smiley].found = true;
				    bitmaps_found[Client_BadGuy_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_CreditsGrey
				if (skinfile[i].StartsWith(wxT("Client_CreditsGrey="))) {
					bitmaps_found[Client_CreditsGrey_Smiley].found = true;
				    bitmaps_found[Client_CreditsGrey_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_CreditsYellow
				if (skinfile[i].StartsWith(wxT("Client_CreditsYellow="))) {
					bitmaps_found[Client_CreditsYellow_Smiley].found = true;
				    bitmaps_found[Client_CreditsYellow_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Upload
				if (skinfile[i].StartsWith(wxT("Client_Upload="))) {
					bitmaps_found[Client_Upload_Smiley].found = true;
				    bitmaps_found[Client_Upload_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_Friend
				if (skinfile[i].StartsWith(wxT("Client_Friend="))) {
					bitmaps_found[Client_Friend_Smiley].found = true;
				    bitmaps_found[Client_Friend_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_eMule
				if (skinfile[i].StartsWith(wxT("Client_eMule="))) {
					bitmaps_found[Client_eMule_Smiley].found = true;
				    bitmaps_found[Client_eMule_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_mlDonkey
				if (skinfile[i].StartsWith(wxT("Client_mlDonkey="))) {
					bitmaps_found[Client_mlDonkey_Smiley].found = true;
				    bitmaps_found[Client_mlDonkey_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_eDonkeyHybrid
				if (skinfile[i].StartsWith(wxT("Client_eDonkeyHybrid="))) {
					bitmaps_found[Client_eDonkeyHybrid_Smiley].found = true;
				    bitmaps_found[Client_eDonkeyHybrid_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_aMule
				if (skinfile[i].StartsWith(wxT("Client_aMule="))) {
					bitmaps_found[Client_aMule_Smiley].found = true;
				    bitmaps_found[Client_aMule_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_lphant
				if (skinfile[i].StartsWith(wxT("Client_lphant="))) {
					bitmaps_found[Client_lphant_Smiley].found = true;
				    bitmaps_found[Client_lphant_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_Shareazza
				if (skinfile[i].StartsWith(wxT("Client_Shareazza="))) {
					bitmaps_found[Client_Shareazza_Smiley].found = true;
				    bitmaps_found[Client_Shareazza_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_xMule
				if (skinfile[i].StartsWith(wxT("Client_xMule="))) {
					bitmaps_found[Client_xMule_Smiley].found = true;
				    bitmaps_found[Client_xMule_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
	            // Client_Unknown
				if (skinfile[i].StartsWith(wxT("Client_Unknown="))) {
					bitmaps_found[Client_Unknown].found = true;
				    bitmaps_found[Client_Unknown].filename=skinfile[i].AfterLast(wxT('='));
				}
				
			}
		}
		
		for (uint32 i=0; i<ClientItemNumber; i++) {
			if (bitmaps_found[i].found) {
				wxImage new_image;
				if (new_image.LoadFile(bitmaps_found[i].filename)) {
					imagelist.Add(wxBitmap(new_image));
				} else {
					printf("Warning: wrong client bitmap file Nº%i: %s",i,unicode2char(bitmaps_found[i].filename));
					imagelist.Add(wxBitmap(clientImages(i)));
				}
			}else {
				imagelist.Add(wxBitmap(clientImages(i)));
			}
		}			
		
		skinfile.Close();
	} catch(wxString error) {
		wxMessageBox(error);
		if (skinfile.IsOpened()) {
			skinfile.Close();
		}
		// Load defaults
		for (uint32 i=0; i<ClientItemNumber; i++) {
			imagelist.Add(wxBitmap(clientImages(i)));
		}	
		return;
	}
	
}

void CamuleDlg::Create_Toolbar(wxString skinfile) {
	// Create ToolBar from the one designed by wxDesigner (BigBob)
	m_wndToolbar = CreateToolBar( wxTB_HORIZONTAL|wxNO_BORDER|wxTB_TEXT|
	                               wxTB_3DBUTTONS|wxTB_FLAT|wxCLIP_CHILDREN );
	m_wndToolbar->SetToolBitmapSize(wxSize(32, 32));
	
	if (skinfile.IsEmpty()) {
		muleToolbar( m_wndToolbar );
	} else {
		muleToolbar( m_wndToolbar );		
	}
}

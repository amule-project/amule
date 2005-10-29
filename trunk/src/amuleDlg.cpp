//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <wx/sizer.h> // Must be first or compilation fail on win32 !!!
#include <cerrno>
#include <cmath>
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
#include <wx/tokenzr.h>
#include <wx/filename.h>

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for CVSDATE, PACKAGE, VERSION
#endif // HAVE_CONFIG_H

#include "amuleDlg.h"		// Interface declarations.

#ifndef __SYSTRAY_DISABLED__
	#ifndef USE_WX_TRAY // WX_TRAY icons are on MuleTrayIcon class
		#include "pixmaps/mule_TrayIcon.ico.xpm"
		#include "pixmaps/mule_Tr_yellow.ico.xpm"
		#include "pixmaps/mule_Tr_grey.ico.xpm"
	#else 
		#include "MuleTrayIcon.h"
	#endif // USE_WX_TRAY
#endif // __SYSTRAY_DISABLED__

#include "OtherFunctions.h"	// Needed for CastItoIShort
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "ClientListCtrl.h"	// Needed for CClientListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "ServerConnect.h"	// Needed for CServerConnect
#include "ClientList.h"		// Needed for CClientList
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "SearchList.h"		// Needed for CSearchList
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
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
#include "ListenSocket.h"	// Needed for CListenSocket
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "OPCodes.h"		// Needed for TM_FINISHEDHASHING
#include "muuli_wdr.h"		// Needed for ID_BUTTON*
#include "PrefsUnifiedDlg.h"
#include "GetTickCount.h"	// Needed for GetTickCount()
#include "StringFunctions.h"	// Needed for unicode2char
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include "Format.h"		// Needed for CFormat
#include "Server.h"		// Needed for CServer
#ifndef CLIENT_GUI
#include "PartFileConvert.h"
#endif

#ifndef __WXMSW__
#include "aMule.xpm"
#endif

#include "kademlia/kademlia/Kademlia.h"


BEGIN_EVENT_TABLE(CamuleDlg, wxFrame)

	EVT_TOOL(ID_BUTTONNETWORKS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSEARCH, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONTRANSFER, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSHARED, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONMESSAGES, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_BUTTONSTATISTICS, CamuleDlg::OnToolBarButton)
	EVT_TOOL(ID_ABOUT, CamuleDlg::OnAboutButton)

	EVT_TOOL(ID_BUTTONNEWPREFERENCES, CamuleDlg::OnPrefButton)
#ifndef CLIENT_GUI
	EVT_TOOL(ID_BUTTONIMPORT, CamuleDlg::OnImportButton)
#endif

	EVT_TOOL(ID_BUTTONCONNECT, CamuleDlg::OnBnConnect)

	EVT_CLOSE(CamuleDlg::OnClose)
	EVT_ICONIZE(CamuleDlg::OnMinimize)

	EVT_BUTTON(ID_BUTTON_FAST, CamuleDlg::OnBnClickedFast)
	EVT_BUTTON(IDC_SHOWSTATUSTEXT, CamuleDlg::OnBnStatusText)

	EVT_TIMER(ID_GUITIMER, CamuleDlg::OnGUITimer)

	EVT_SIZE(CamuleDlg::OnMainGUISizeChange)

END_EVENT_TABLE()

#ifndef wxCLOSE_BOX
	#define wxCLOSE_BOX 0
#endif

CamuleDlg::CamuleDlg(wxWindow* pParent, const wxString &title, wxPoint where, wxSize dlg_size) : wxFrame(
	pParent, -1, title, where, dlg_size,
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxTHICK_FRAME|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX,wxT("aMule") )
{
	is_safe_state = false;
	
	// wxWidgets send idle events to ALL WINDOWS by default... *SIGH*
	#if wxCHECK_VERSION(2,6,0)
		wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);
	#endif
	
	last_iconizing = 0;
	prefs_dialog = NULL;

	#ifndef __SYSTRAY_DISABLED__
		m_wndTaskbarNotifier = NULL;
	#endif

	wxInitAllImageHandlers();
	imagelist.Create(16,16);
	
	static int ClientItemNumber = CLIENT_SKIN_UNUSED;

	if (thePrefs::UseSkin()) {		
		Apply_Clients_Skin(thePrefs::GetSkinFile());
	} else {
		for (int i = 0; i < ClientItemNumber; ++i) {
			imagelist.Add(wxBitmap(clientImages(i)));
		}
	}

	bool override_where = (where != wxDefaultPosition);
	bool override_size = ((dlg_size.x != DEFAULT_SIZE_X) || (dlg_size.y != DEFAULT_SIZE_Y));

	if (!LoadGUIPrefs(override_where, override_size)) {
		// Prefs not loaded for some reason, exit
		AddLogLineM( true, wxT("ERROR! Unable to load Preferences") );
		return;
	}

	SetIcon(wxICON(aMule));

	srand(time(NULL));

	// Create new sizer and stuff a wxPanel in there.
	wxFlexGridSizer *s_main = new wxFlexGridSizer(1);
	s_main->AddGrowableCol(0);
	s_main->AddGrowableRow(0);

	wxPanel* p_cnt = new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize);
	s_main->Add(p_cnt, 0, wxGROW|wxEXPAND, 0);
	muleDlg(p_cnt, false, true);

	SetSizer( s_main, true );

	Create_Toolbar(wxEmptyString, thePrefs::VerticalToolbar());

	serverwnd = new CServerWnd(p_cnt, srv_split_pos);

	AddLogLineM(false, wxEmptyString);
	AddLogLineM(false, wxT(" - ") + CFormat(_("This is aMule %s based on eMule.")) % GetMuleVersion());
	#if wxCHECK_VERSION(2,5,0)
	AddLogLineM(false, wxT("   ") + CFormat(_("Running on %s")) % wxGetOsDescription());
	#endif
	AddLogLineM(false, wxT(" - ") + wxString(_("Visit http://www.amule.org to check if a new version is available.")));
	AddLogLineM(false, wxEmptyString);

	searchwnd = new CSearchDlg(p_cnt);
	transferwnd = new CTransferWnd(p_cnt);
	sharedfileswnd = new CSharedFilesWnd(p_cnt);
	statisticswnd = new CStatisticsDlg(p_cnt, theApp.statistics);
	chatwnd = new CChatWnd(p_cnt);
	//kademliawnd = new CKadDlg(p_cnt);
	serverwnd->Show(FALSE);
	searchwnd->Show(FALSE);
	transferwnd->Show(FALSE);
	sharedfileswnd->Show(FALSE);
	statisticswnd->Show(FALSE);
	chatwnd->Show(FALSE);
	//kademliawnd->Show(FALSE);	

	// Create the GUI timer
	gui_timer=new wxTimer(this,ID_GUITIMER);
	if (!gui_timer) {
		AddLogLine(false, _("Fatal Error: Failed to create Timer"));
		exit(1);
	}

	// Set Serverlist as active window
	activewnd=NULL;
	SetActiveDialog(NetworksWnd, serverwnd);
	m_wndToolbar->ToggleTool(ID_BUTTONNETWORKS, true );
	#ifdef CLIENT_GUI
	m_wndToolbar->DeleteTool(ID_BUTTONIMPORT);
	#endif

	ShowED2KLinksHandler( thePrefs::GetFED2KLH() );

	is_safe_state = true;

	// Init statistics stuff, better do it asap
	statisticswnd->Init();
	
	searchwnd->UpdateCatChoice();

#ifndef  __SYSTRAY_DISABLED__
	if (thePrefs::UseTrayIcon()) {
		CreateSystray();
	}
#endif

	Show(TRUE);

	// Must we start minimized?
	if (thePrefs::GetStartMinimized()) { 
		#ifndef __SYSTRAY_DISABLED__
		 	if (thePrefs::UseTrayIcon() && 
			#ifndef USE_WX_TRAY
				(thePrefs::GetDesktopMode() != 4) &&
			#endif
		 		thePrefs::DoMinToTray()) {
					Hide_aMule();
				} else {
					Iconize(TRUE);
				}
		#else
			Iconize(TRUE);
		#endif
	}
	m_BlinkMessages = false;
	m_CurrentBlinkBitmap = 24;
}

void CamuleDlg::Init() {
	//kademliawnd = new CKadDlg(p_cnt);
	kademliawnd = CastChild( wxT("kadWnd"), CKadDlg );
	kademliawnd->Init();
}

// Madcat - Sets Fast ED2K Links Handler on/off.
void CamuleDlg::ShowED2KLinksHandler( bool show )
{
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Hiding FED2KLH aborted."));
		return;
	}
	
	s_dlgcnt->Show( s_fed2klh, show );
	s_dlgcnt->Layout();
}

// Toogles ed2k link handler.
void CamuleDlg::ToogleED2KLinksHandler()
{
	// Errorchecking in case the pointer becomes invalid ...
	if (s_fed2klh == NULL) {
		wxLogWarning(wxT("Unable to find Fast ED2K Links handler sizer! Toogling FED2KLH aborted."));
		return;
	}
	ShowED2KLinksHandler(!s_dlgcnt->IsShown(s_fed2klh));
}

void CamuleDlg::SetActiveDialog(DialogType type, wxWindow* dlg)
{
	m_nActiveDialog = type;
	
	if ( type == TransferWnd ) {
		if (thePrefs::ShowCatTabInfos()) {
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

	// Since we might be suspending redrawing while hiding the dialog
	// we have to refresh it once it is visible again
	dlg->Refresh( true );
}

#ifndef __SYSTRAY_DISABLED__

	#ifndef USE_WX_TRAY
		class QueryDlg : public wxDialog {
		public:
			QueryDlg(wxWindow* parent) : wxDialog(
				parent, 21373, _("Desktop integration"), wxDefaultPosition, wxDefaultSize,
				wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU )
			{
				wxSizer* content=desktopDlg(this, TRUE);
				content->Show(this, TRUE);
			};
		protected:
			void OnOk(wxCommandEvent& WXUNUSED(evt)) { EndModal(0); };
			DECLARE_EVENT_TABLE()
		};

		BEGIN_EVENT_TABLE(QueryDlg, wxDialog)
			EVT_BUTTON(ID_OK, QueryDlg::OnOk)
		END_EVENT_TABLE()
	
		void CamuleDlg::changeDesktopMode()
		{
			QueryDlg query(this);

			wxRadioBox* radiobox = CastByID( ID_SYSTRAYSELECT, &query, wxRadioBox );

			if ( thePrefs::GetDesktopMode() ) {
				radiobox->SetSelection( thePrefs::GetDesktopMode() - 1 );
			} else {
				radiobox->SetSelection( 0 );
			}

			query.ShowModal();

			thePrefs::SetDesktopMode( radiobox->GetSelection() + 1 );
		}

		void CamuleDlg::UpdateTrayIcon(int percent)
		{
			// ei hienostelua. tarvii kuitenki pelleill?gtk:n kanssa
			// Whatever that means, it's working.
			int pVals16[1] = {percent};
			char** data;
			if(!theApp.serverconnect) {
				data = mule_Tr_grey_ico;
			} else {
				if (theApp.IsConnectedED2K()) {
				if(!theApp.serverconnect->IsLowID()) {
						data = mule_TrayIcon_ico;
					} else {
						data = mule_Tr_yellow_ico;
					}
				} else {
					data = mule_Tr_grey_ico;
				}
			}		
			m_wndTaskbarNotifier->SetTrayIcon(data, pVals16 );
		}

		
		void CamuleDlg::CreateSystray()
		{

			// create the docklet (at this point we already have preferences!)
			if( !thePrefs::GetDesktopMode() ) {
				// ok, it's not set yet.
				changeDesktopMode();
			}

			m_wndTaskbarNotifier = new CSysTray(this, (DesktopMode)thePrefs::GetDesktopMode(), wxString::Format(wxT("%s %s"), wxT(PACKAGE), wxT(VERSION)));
			// This will effectively show the Tray Icon.
			UpdateTrayIcon(0);
		}

		
	#else
		
		void CamuleDlg::UpdateTrayIcon(int percent)
		{
			
			// set trayicon-icon
			if(!theApp.serverconnect) {
				m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_DISCONNECTED, percent);
			} else {
				if (theApp.IsConnectedED2K()) {
					if(!theApp.serverconnect->IsLowID()) {
						m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_HIGHID, percent);
					} else {
					m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_LOWID, percent);
					}
				} else {
					m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_DISCONNECTED, percent);
				}
			}
		}
			
		void CamuleDlg::CreateSystray()
		{
			m_wndTaskbarNotifier = new CMuleTrayIcon();
			wxASSERT(m_wndTaskbarNotifier->IsOk());			
			// This will effectively show the Tray Icon.
			UpdateTrayIcon(0);
		}	
		
	#endif

	// This one is common to both implementations
	void CamuleDlg::RemoveSystray()
	{
		if (m_wndTaskbarNotifier) {
			delete m_wndTaskbarNotifier;
			m_wndTaskbarNotifier = NULL;
		}
	}
	
#endif // __SYSTRAY_DISABLED__

void CamuleDlg::OnToolBarButton(wxCommandEvent& ev)
{
	static int lastbutton = ID_BUTTONNETWORKS;

	// Kry - just if the GUI is ready for it
	if ( is_safe_state ) {

		// Rehide the handler if needed
		if ( lastbutton == ID_BUTTONSEARCH && !thePrefs::GetFED2KLH() ) {
			if (ev.GetId() != ID_BUTTONSEARCH) {
				ShowED2KLinksHandler( false );
			} else {
				// Toogle ED2K handler.
				ToogleED2KLinksHandler();
			}
		}

		if ( lastbutton != ev.GetId() ) {
			switch ( ev.GetId() ) {
				case ID_BUTTONNETWORKS:
					SetActiveDialog(NetworksWnd, serverwnd);
					// Set serverlist splitter position
					CastChild( wxT("SrvSplitterWnd"), wxSplitterWindow )->SetSashPosition(srv_split_pos, true);
					break;

				case ID_BUTTONSEARCH:
					// The search dialog should always display the handler
					if ( !thePrefs::GetFED2KLH() )
						ShowED2KLinksHandler( true );

					SetActiveDialog(SearchWnd, searchwnd);
					break;

				case ID_BUTTONTRANSFER:
					SetActiveDialog(TransferWnd, transferwnd);
					// Prepare the dialog, sets the splitter-position
					transferwnd->Prepare();
					break;

				case ID_BUTTONSHARED:
					SetActiveDialog(SharedWnd, sharedfileswnd);
					break;

				case ID_BUTTONMESSAGES:
					m_BlinkMessages = false;
					SetActiveDialog(ChatWnd, chatwnd);
					break;

				case ID_BUTTONSTATISTICS:
					SetActiveDialog(StatsWnd, statisticswnd);
					break;

				// This shouldn't happen, but just in case
				default:
					AddLogLineM( true, wxT("Unknown button triggered CamuleApp::OnToolBarButton().") );
					break;
			}
		}

		m_wndToolbar->ToggleTool(lastbutton, lastbutton == ev.GetId() );
		lastbutton = ev.GetId();
	}
}


void CamuleDlg::OnAboutButton(wxCommandEvent& WXUNUSED(ev))
{
	wxString msg = wxT(" ");
#ifdef CLIENT_GUI
	msg << _("aMule remote control ") << wxT(VERSION);
#else
	msg << wxT("aMule ") << wxT(VERSION);
#endif
	msg << wxT(" ");
#ifdef CVSDATE
	msg << _("Snapshot:") << wxT("\n ") << wxT(CVSDATE);
#endif
	msg << wxT("\n\n") << _(
		" 'All-Platform' p2p client based on eMule \n\n"
		" Website: http://www.amule.org \n"
		" Forum: http://forum.amule.org \n"
		" FAQ: http://wiki.amule.org \n\n"
		" Contact: admin@amule.org (administrative issues) \n"
		" Copyright (C) 2003-2005 aMule Team \n\n"
		" Part of aMule is based on \n"
		" Kademlia: Peer-to-peer routing based on the XOR metric.\n"
		" Copyright (C) 2002 Petar Maymounkov\n"
		" http://kademlia.scs.cs.nyu.edu\n");
	
	if (is_safe_state) {
		wxMessageBox(msg);
	}
}


void CamuleDlg::OnPrefButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( is_safe_state ) {
		// Try to create a new dialog-window
		if (!prefs_dialog) {
			prefs_dialog = PrefsUnifiedDlg::NewPrefsDialog( this );
	
			// Check if a dialog was created and show it
			if ( prefs_dialog ) {
				prefs_dialog->TransferToWindow();
		
				prefs_dialog->ShowModal();
			}
		}
	}
}

#ifndef CLIENT_GUI
void CamuleDlg::OnImportButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( is_safe_state ) {
		CPartFileConvert::ShowGUI(NULL);
	}
}
#endif

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");
	
	SaveGUIPrefs();

	theApp.amuledlg = NULL;
	
	printf("aMule dialog destroyed\n");
}


void CamuleDlg::OnBnConnect(wxCommandEvent& WXUNUSED(evt))
{

	bool connect = (!theApp.IsConnectedED2K() && !theApp.serverconnect->IsConnecting()) 
						#ifndef CLIENT_GUI
						|| (!Kademlia::CKademlia::isRunning())
						#endif
						;	

	if (connect && thePrefs::GetNetworkED2K()) {
		//connect if not currently connected
		AddLogLine(true, _("Connecting"));
		theApp.serverconnect->ConnectToAnyServer();
	} else {
		//disconnect if currently connected
		if (theApp.serverconnect->IsConnecting()) {
			theApp.serverconnect->StopConnectionTry();
		} else {
			theApp.serverconnect->Disconnect();
		}
	}

	// Connect Kad also
	if( connect && thePrefs::GetNetworkKademlia()) {
		theApp.StartKad();
	} else {
		theApp.StopKad();
	}

}

void CamuleDlg::OnBnStatusText(wxCommandEvent& WXUNUSED(evt))
{
	wxMessageBox(CastChild( wxT("infoLabel"), wxStaticText )->GetLabel(), wxString(_("Status text")), wxOK|wxICON_INFORMATION);
}

void CamuleDlg::ResetLog(uint32 whichone)
{
	wxTextCtrl* ct = CastByID( whichone, serverwnd, wxTextCtrl );

	if(ct) {
		ct->Clear();
	}
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

	// Create the timestamp
	wxString stamp = wxDateTime::Now().FormatISODate() + wxT(" ") + wxDateTime::Now().FormatISOTime() + wxT(": ");

	// Add the message to the log-view
	wxTextCtrl* ct = CastByID( ID_LOGVIEW, serverwnd, wxTextCtrl );
	if ( ct ) {
		if ( bufferline.IsEmpty() ) {
			// If it's empty we just write a blank line with no timestamp.
			ct->AppendText( wxT("\n") );
		} else {
			// Split multi-line messages into individual lines
			wxStringTokenizer tokens( bufferline, wxT("\n") );		
		
			while ( tokens.HasMoreTokens() ) {
				ct->AppendText( stamp + tokens.GetNextToken() + wxT("\n") );	
			} 
		}
			
		ct->ShowPosition( ct->GetLastPosition() - 1 );
	}
	

	// Set the status-bar if the event warrents it
	if ( addtostatusbar ) {
		// Escape "&"s, which would otherwise not show up
		bufferline.Replace( wxT("&"), wxT("&&") );
		wxStaticText* text = CastChild( wxT("infoLabel"), wxStaticText );
		// Only show the first line if multiple lines
		text->SetLabel( bufferline.BeforeFirst( wxT('\n') ) );
		text->GetParent()->Layout();
	}
	
}


void CamuleDlg::AddServerMessageLine(wxString& message)
{
	wxTextCtrl* cv= CastByID( ID_SERVERINFO, serverwnd, wxTextCtrl );
	if(cv) {
		if (message.Length() > 500) {
			cv->AppendText(message.Left(500) + wxT("\n"));
		} else {
			cv->AppendText(message + wxT("\n"));
		}
		cv->ShowPosition(cv->GetLastPosition()-1);
	}
}

void CamuleDlg::ShowConnectionState(uint32 connection_state)
{
	enum ed2k_state { sUnknown = -1, sDisconnected = 0, sLowID = 1, sConnecting = 2, sHighID = 3 };
	static ed2k_state LastState = sUnknown;
	serverwnd->UpdateED2KInfo();
	serverwnd->UpdateKadInfo();
	ed2k_state NewState = sUnknown;
	
	wxStaticText* connLabel = CastChild( wxT("connLabel"), wxStaticText );
	
	wxString connected_server;
	CServer* ed2k_server = theApp.serverconnect->GetCurrentServer();
	if (ed2k_server) {
		connected_server = ed2k_server->GetListName();
	}	
	
	if ( connection_state & CONNECTED_ED2K ) {
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
		CastChild( wxT("connImage"), wxStaticBitmap )->SetBitmap(connImages(NewState));
		m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
		switch ( NewState ) {
			case sLowID:
				// Display a warning about LowID connections
				AddLogLine(true,  _("WARNING: You have recieved Low-ID!"));
				AddLogLine(false, _("\tMost likely this is because you're behind a firewall or router."));
				AddLogLine(false, _("\tFor more information, please refer to http://wiki.amule.org"));
			
			case sHighID: {
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Disconnect"),
					connButImg(1), wxNullBitmap, wxITEM_NORMAL,
					_("Disconnect from current server"));
				wxStaticText* tx = CastChild( wxT("infoLabel"), wxStaticText );
				tx->SetLabel(CFormat(_("Connection established on: %s")) % connected_server);
				connLabel->SetLabel(connected_server);
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
				break;
	
			default:
				break;
		}
		m_wndToolbar->Realize();
	} else if (connection_state & CONNECTED_ED2K) {
		connLabel->SetLabel(connected_server);
	}

	int index = connLabel->GetLabel().Find(wxT(" (Kad:"));
	if (index == -1) {
		index = connLabel->GetLabel().Length();
	}
	if (connection_state & CONNECTED_KAD_OK) {
		connLabel->SetLabel(connLabel->GetLabel().Left(index) + wxT(" (Kad: ok)"));
	} else if (connection_state & CONNECTED_KAD_FIREWALLED) {
		connLabel->SetLabel(connLabel->GetLabel().Left(index) + wxT(" (Kad: firewalled)"));
	} else {
		connLabel->SetLabel(connLabel->GetLabel().Left(index) + wxT(" (Kad: off)"));
	}
	connLabel->GetParent()->Layout();
	LastState = NewState;
}

void CamuleDlg::ShowUserCount(const wxString& info)
{
	wxStaticText* label = CastChild( wxT("userLabel"), wxStaticText );
	
	// Update Kad tab
	serverwnd->UpdateKadInfo();
	
	label->SetLabel(info);
	label->GetParent()->Layout();
}

void CamuleDlg::ShowTransferRate()
{
	float kBpsUp = theStats::GetUploadRate() / 1024.0;
	float kBpsDown = theStats::GetDownloadRate() / 1024.0;
	wxString buffer;
	if( thePrefs::ShowOverhead() )
	{
		buffer = wxString::Format(_("Up: %.1f(%.1f) | Down: %.1f(%.1f)"), kBpsUp, theStats::GetUpOverheadRate() / 1024.0, kBpsDown, theStats::GetDownOverheadRate() / 1024.0);
	} else {
		buffer = wxString::Format(_("Up: %.1f | Down: %.1f"), kBpsUp, kBpsDown);
	}
	buffer.Truncate(50); // Max size 50

	wxStaticText* label = CastChild( wxT("speedLabel"), wxStaticText );
	label->SetLabel(buffer);
	label->GetParent()->Layout();

	// Show upload/download speed in title
	if (thePrefs::GetShowRatesOnTitle()) {
		wxString UpDownSpeed = wxString::Format(wxT(" -- Up: %.1f | Down: %.1f"), kBpsUp, kBpsDown);
		SetTitle(theApp.m_FrameTitle + UpDownSpeed);
	}

#ifndef __SYSTRAY_DISABLED__
	if (m_wndTaskbarNotifier) {
		// set trayicon-icon
		int percentDown = (int)ceil((kBpsDown*100) / thePrefs::GetMaxGraphDownloadRate());
		UpdateTrayIcon( ( percentDown > 100 ) ? 100 : percentDown);
	
		wxString buffer2;
		if ( theApp.IsConnectedED2K() ) {
			buffer2 = CFormat(_("aMule (%s | Connected)")) % buffer;
		} else {
			buffer2 = CFormat(_("aMule (%s | Disconnected)")) % buffer;
		}
		m_wndTaskbarNotifier->SetTrayToolTip(buffer2);
	}
#endif

	wxStaticBitmap* bmp = CastChild( wxT("transferImg"), wxStaticBitmap );
	bmp->SetBitmap(dlStatusImages((kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)));
}

void CamuleDlg::DlgShutDown()
{
	// Are we already shutting down or still on init?
	if ( is_safe_state == false ) {
		return;
	}

	// we are going DOWN
	is_safe_state = false;

	// Stop the GUI Timer
	delete gui_timer;
	transferwnd->downloadlistctrl->DeleteAllItems();

#ifndef __SYSTRAY_DISABLED__
	//We want to delete the systray too!
	RemoveSystray();
#endif
}

void CamuleDlg::OnClose(wxCloseEvent& evt)
{
	// This will be here till the core close is != app close
	if (evt.CanVeto() && thePrefs::IsConfirmExitEnabled() ) {
		if (wxNO == wxMessageBox(wxString(_("Do you really want to exit aMule?")),
				wxString(_("Exit confirmation")), wxYES_NO)) {
			evt.Veto();
			return;
		}
	}
	
	theApp.ShutDown(evt);
}

//BEGIN - enkeyDEV(kei-kun) -TaskbarNotifier-
void CamuleDlg::ShowNotifier(wxString WXUNUSED(Text), int WXUNUSED(MsgType), bool WXUNUSED(ForceSoundOFF))
{
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-


void CamuleDlg::OnBnClickedFast(wxCommandEvent& WXUNUSED(evt))
{
	wxTextCtrl* ctl = CastChild( wxT("FastEd2kLinks"), wxTextCtrl );

	for ( int i = 0; i < ctl->GetNumberOfLines(); i++ ) {
		wxString strlink = ctl->GetLineText(i);
		strlink.Trim(true);
		strlink.Trim(false);
		if ( !strlink.IsEmpty() ) {
			theApp.downloadqueue->AddED2KLink( strlink, transferwnd->downloadlistctrl->GetCategory() );
		}
	}
	
	ctl->SetValue(wxEmptyString);
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
			SetSize(x2, y2);
		} else {
#ifndef __WXGTK__
			// Probably first run. Only works for gtk2
			Maximize();
#endif
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

	// Saving sash position of splitter in server window
	config->Write(section+wxT("SRV_SPLITTER_POS"), (long) srv_split_pos);

	config->Flush(true);

	/* End modif */

	return true;
}


//hides amule
void CamuleDlg::Hide_aMule(bool iconize)
{
	if (IsShown() && ((last_iconizing + 2000) < GetTickCount())) { // 1 secs for sanity
	//	is_hidden = true;
		last_iconizing = GetTickCount();

		if (prefs_dialog) {
			prefs_dialog->Iconize(true);;
			prefs_dialog->Show(false);
		}
		
		if (iconize) {
			Iconize(TRUE);
		}
		
		Show(FALSE);
	}

}


//shows amule
void CamuleDlg::Show_aMule(bool uniconize)
{
	if (!IsShown() && ((last_iconizing + 1000) < GetTickCount())) { // 1 secs for sanity
	//	is_hidden = false;
		last_iconizing = GetTickCount();
		
		if (uniconize) {
			Show(TRUE);
			Raise();
		}
		
		if (prefs_dialog) {
			prefs_dialog->Show(true);
			prefs_dialog->Raise();
		}
	
	}

}


#ifndef __SYSTRAY_DISABLED__
void CamuleDlg::OnMinimize(wxIconizeEvent& evt)
{
	if (m_wndTaskbarNotifier && thePrefs::DoMinToTray() 
		#ifndef USE_WX_TRAY
			&& (thePrefs::GetDesktopMode() != 4) && thePrefs::UseTrayIcon()
		#endif
		) {
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
}
#else
void CamuleDlg::OnMinimize(wxIconizeEvent& WXUNUSED(evt))
{
}
#endif


void CamuleDlg::OnGUITimer(wxTimerEvent& WXUNUSED(evt))
{
	// Former TimerProc section

	static uint32	msPrev1, msPrev5, msPrevStats;

	uint32 			msCur = theStats::GetUptimeMillis();

	// can this actually happen under wxwin ?
	if (!SafeState()) {
		return;
	}

	bool bStatsVisible = (!IsIconized() && StatisticsWindowActive());
	
#ifndef CLIENT_GUI
	static uint32 msPrevGraph;
	int msGraphUpdate = thePrefs::GetTrafficOMeterInterval() * 1000;
	if ((msGraphUpdate > 0)  && ((msCur / msGraphUpdate) > (msPrevGraph / msGraphUpdate))) {
		// trying to get the graph shifts evenly spaced after a change in the update period
		msPrevGraph = msCur;
		
		GraphUpdateInfo update = theApp.statistics->GetPointsForUpdate();
		
		statisticswnd->UpdateStatGraphs(bStatsVisible, theStats::GetPeakConnections(), update);
	}
#else
	#warning TODO: CORE/GUI -- EC needed
#endif
	
	int sStatsUpdate = thePrefs::GetStatsInterval();
	if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
		if (bStatsVisible) {
			msPrevStats = msCur;
			statisticswnd->ShowStatistics();
		}
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		ShowTransferRate();
		if (thePrefs::ShowCatTabInfos() && theApp.amuledlg->activewnd == theApp.amuledlg->transferwnd) {
			transferwnd->UpdateCatTabTitles();
		}
		if (thePrefs::AutoSortDownload()) {
			transferwnd->downloadlistctrl->SortList();
		}
	}
	
	if (msCur-msPrev1 > 1000) {  // every second
		msPrev1 = msCur;
		if (m_CurrentBlinkBitmap == 33) {
			m_CurrentBlinkBitmap = 24;
			SetMessagesTool();		
		} else {
			if (m_BlinkMessages) {
				m_CurrentBlinkBitmap = 33;
				SetMessagesTool();
			}
		}
		
	}
}


void CamuleDlg::SetMessagesTool()
{
#if wxCHECK_VERSION(2, 5, 0)
	int pos = m_wndToolbar->GetToolPos(ID_BUTTONMESSAGES);
	wxASSERT(pos == 6); // so we don't miss a change on wx2.4
#else
	int pos = 6;
#endif	
	m_wndToolbar->DeleteTool(ID_BUTTONMESSAGES);
	m_wndToolbar->InsertTool(pos,ID_BUTTONMESSAGES, _("Messages"), 
		amuleDlgImages( m_CurrentBlinkBitmap ), 
		wxNullBitmap, 
		wxITEM_CHECK, 
		_("Messages Window") );
	m_wndToolbar->Realize();

}


/*
	Try to launch the specified url:
	 - Windows: Default or custom browser will be used.
	 - Mac: Currently not implemented
	 - Anything else: Try a number of hardcoded browsers. Should be made configurable...
*/
void CamuleDlg::LaunchUrl( const wxString& url )
{
	wxString cmd;

	cmd = thePrefs::GetBrowser();
	if ( !cmd.IsEmpty() ) {
		wxString tmp = url;
		// Pipes cause problems, so escape them
		tmp.Replace( wxT("|"), wxT("%7C") );


		if ( !cmd.Replace( wxT("%s"), tmp ) ) {
			// No %s found, just append the url
			cmd += wxT(" ") + tmp;
		}

		if ( wxExecute( cmd, false ) ) {
			printf( "Launch Command: %s\n", (const char *)unicode2char(cmd));
			return;
		}
#ifdef __WXMSW__
	} else {
		wxFileType* ft = wxTheMimeTypesManager->GetFileTypeFromExtension(wxT("html"));
		if (!ft) {
			wxLogError(
				wxT("Impossible to determine the file type for extension html."
				"Please edit your MIME types.")
			);
			return;
		}

		bool ok = ft->GetOpenCommand(&cmd, wxFileType::MessageParameters(url, wxT("")));
		delete ft;

		if (!ok) {
			wxMessageBox(
				_("Could not determine the command for running the browser."),
				wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION);
			return;
		}

		wxPuts(wxT("Launch Command: ") + cmd);
		if (wxExecute(cmd, false)) {
			return;
		}
#endif
	}
	// Unable to execute browser. But this error message doesn't make sense,
	// cosidering that you _can't_ set the browser executable path... =/
	wxLogError( wxT("Unable to launch browser. Please set correct browser executable path in Preferences.") );
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




void CamuleDlg::Apply_Clients_Skin(wxString file)
{	
	#define ClientItemNumber CLIENT_SKIN_UNUSED

	SkinItem bitmaps_found[ClientItemNumber];

	for (uint32 i = 0; i < ClientItemNumber; i++) {	
		bitmaps_found[i].found = false;
	}

	wxTextFile skinfile;
	
	printf("Testing skins\n");
	
	try {
		if (file.IsEmpty()) {			
			throw wxString(_("Skin file name is empty - loading defaults"));
		}

		
		if (!::wxFileExists(file)) {
			throw (CFormat(_("Skin file %s does not exist - loading defaults")) % file ).GetString();
		}
	
		
		if (!skinfile.Open(file)) {
			throw (CFormat(_("Unable to open skin file: %s")) % file).GetString();
		}
		
		int client_header_found = -1;
		
		for (uint32 i=0; i < skinfile.GetLineCount(); i++) {
			if (skinfile[i] == wxT("[Client Bitmaps]")) {
				client_header_found = i;	
				break;
			}
		}
		
		
		if (client_header_found != -1) {
			
			wxImage new_image;
			
			for (uint32 i=client_header_found+1; i < skinfile.GetLineCount(); i++) {
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
	            // Client_Shareaza
				if (skinfile[i].StartsWith(wxT("Client_Shareaza="))) {
					bitmaps_found[Client_Shareaza_Smiley].found = true;
				    bitmaps_found[Client_Shareaza_Smiley].filename=skinfile[i].AfterLast(wxT('='));
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
				// Client_Invalid_Rating_On_File
				if (skinfile[i].StartsWith(wxT("Client_InvalidRatingOnFile="))) {
					bitmaps_found[Client_InvalidRating_Smiley].found = true;
					bitmaps_found[Client_InvalidRating_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Poor_Rating_On_File
				if (skinfile[i].StartsWith(wxT("Client_PoorRatingOnFile="))) {
					bitmaps_found[Client_PoorRating_Smiley].found = true;
					bitmaps_found[Client_PoorRating_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}				
				// Client_Good_Rating_On_File
				if (skinfile[i].StartsWith(wxT("Client_GoodRatingOnFile="))) {
					bitmaps_found[Client_GoodRating_Smiley].found = true;
					bitmaps_found[Client_GoodRating_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Fair_Rating_On_File
				if (skinfile[i].StartsWith(wxT("Client_FairRatingOnFile="))) {
					bitmaps_found[Client_FairRating_Smiley].found = true;
					bitmaps_found[Client_FairRating_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
				// Client_Excellent_Rating_On_File
				if (skinfile[i].StartsWith(wxT("Client_ExcellentRatingOnFile="))) {
					bitmaps_found[Client_ExcellentRating_Smiley].found = true;
					bitmaps_found[Client_ExcellentRating_Smiley].filename=skinfile[i].AfterLast(wxT('='));
				}
			}
		}
		
		for (uint32 i=0; i<ClientItemNumber; i++) {
			if (bitmaps_found[i].found) {
				wxImage new_image;
				wxFileName file_name(bitmaps_found[i].filename);
				file_name.Normalize();
				if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
					imagelist.Add(wxBitmap(new_image));
				} else {
					printf(	"Warning: wrong client bitmap file No.%i: %s",
						i, (const char *)unicode2char(bitmaps_found[i].filename));
					imagelist.Add(wxBitmap(clientImages(i)));
				}
			} else {
				imagelist.Add(wxBitmap(clientImages(i)));
			}
		}			
		
		skinfile.Close();
	} catch (const wxString& error) {
		wxMessageBox(error);
		
		// Load defaults
		for (uint32 i = 0; i < ClientItemNumber; i++) {
			imagelist.Add(wxBitmap(clientImages(i)));
		}
	}
}

void CamuleDlg::Create_Toolbar(wxString skinfile, bool orientation) {
	// Create ToolBar from the one designed by wxDesigner (BigBob)
	wxToolBar* current = GetToolBar();
	if (current) {
		current->Destroy();
		SetToolBar(NULL); // Remove old one if present
	}
	m_wndToolbar = CreateToolBar( (orientation ? wxTB_VERTICAL : wxTB_HORIZONTAL)
									| wxNO_BORDER | wxTB_TEXT | wxTB_3DBUTTONS
									| wxTB_FLAT | wxCLIP_CHILDREN );
	m_wndToolbar->SetToolBitmapSize(wxSize(32, 32));
	
	if (skinfile.IsEmpty()) {
		muleToolbar( m_wndToolbar );
	} else {
		muleToolbar( m_wndToolbar );		
	}
}

void CamuleDlg::OnMainGUISizeChange(wxSizeEvent& evt) {
	
	wxFrame::OnSize(evt);	
	
	#if !defined(__WXMAC__) && !defined(__WXCOCOA__)
	// Crashing on mac, why?
	
	if (transferwnd && transferwnd->clientlistctrl) {
	
		// Transfer window's splitter set again if it's hidden.
		if ( transferwnd->clientlistctrl->GetListView() == vtNone ) {
			int height  = transferwnd->clientlistctrl->GetSize().GetHeight();
		
			wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
			height += splitter->GetWindow1()->GetSize().GetHeight();
		
			splitter->SetSashPosition( height );
		}
	}
	#endif
	
}

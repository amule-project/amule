//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/config.h> // Do_not_auto_remove (MacOS 10.3, wx 2.7)
#include <wx/confbase.h> // Do_not_auto_remove (MacOS 10.3, wx 2.7)
#include <wx/stattext.h>
#include <wx/tokenzr.h>
#include <wx/html/htmlwin.h>
#include <wx/mimetype.h>	// Do_not_auto_remove (win32)
#include <wx/textfile.h>	// Do_not_auto_remove (win32)

#include <include/common/EventIDs.h>

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for CVSDATE, PACKAGE, VERSION
#endif // HAVE_CONFIG_H

#include "amuleDlg.h"		// Interface declarations.

#include <common/Format.h>	// Needed for CFormat
#include "amule.h"		// Needed for theApp
#include "ChatWnd.h"		// Needed for CChatWnd
#include "ClientListCtrl.h"	// Needed for CClientListCtrl
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "KadDlg.h"		// Needed for CKadDlg
#include "Logger.h"
#include "MuleTrayIcon.h"
#include "muuli_wdr.h"		// Needed for ID_BUTTON*
#include "Preferences.h"	// Needed for CPreferences
#include "PrefsUnifiedDlg.h"
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "Server.h"		// Needed for CServer
#include "ServerConnect.h"	// Needed for CServerConnect
#include "ServerWnd.h"		// Needed for CServerWnd
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "Statistics.h"		// Needed for theStats
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "TerminationProcess.h"	// Needed for CTerminationProcess
#include "TransferWnd.h"	// Needed for CTransferWnd
#ifndef CLIENT_GUI
#include "PartFileConvert.h"
#endif

#ifndef __WXMSW__
#include "aMule.xpm"
#endif

#include "kademlia/kademlia/Kademlia.h"

#ifdef ENABLE_IP2COUNTRY
	#include "IP2Country.h"		// Needed for IP2Country
#endif

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

	EVT_TIMER(ID_GUI_TIMER_EVENT, CamuleDlg::OnGUITimer)

	EVT_SIZE(CamuleDlg::OnMainGUISizeChange)

	EVT_KEY_UP(CamuleDlg::OnKeyPressed)

	EVT_MENU(wxID_EXIT, CamuleDlg::OnExit)
	
END_EVENT_TABLE()

#ifndef wxCLOSE_BOX
	#define wxCLOSE_BOX 0
#endif

CamuleDlg::CamuleDlg(
	wxWindow* pParent,
	const wxString &title,
	wxPoint where,
	wxSize dlg_size)
:
wxFrame(
	pParent, -1, title, where, dlg_size,
	wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxDIALOG_NO_PARENT|
	wxRESIZE_BORDER|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX,
	wxT("aMule")),
m_transferwnd(NULL)
{
	m_is_safe_state = false;
	
	// wxWidgets send idle events to ALL WINDOWS by default... *SIGH*
	wxIdleEvent::SetMode(wxIDLE_PROCESS_SPECIFIED);
	wxUpdateUIEvent::SetMode(wxUPDATE_UI_PROCESS_SPECIFIED);
	
	m_last_iconizing = 0;
	m_prefsDialog = NULL;
	m_prefsVisible = false;
	
	m_wndTaskbarNotifier = NULL;

	wxInitAllImageHandlers();
	m_imagelist.Create(16,16);
	m_tblist.Create(32,32);
	
	static int ClientItemNumber = CLIENT_SKIN_UNUSED;

	if (thePrefs::UseSkin()) {		
		Apply_Clients_Skin(thePrefs::GetSkinFile());
	} else {
		for (int i = 0; i < ClientItemNumber; ++i) {
			m_imagelist.Add(wxBitmap(clientImages(i)));
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

	Create_Toolbar(thePrefs::GetSkinFile(), thePrefs::VerticalToolbar());

	m_serverwnd = new CServerWnd(p_cnt, m_srv_split_pos);

	AddLogLineM(false, wxEmptyString);
	AddLogLineM(false, wxT(" - ") + CFormat(_("This is aMule %s based on eMule.")) % GetMuleVersion());
	AddLogLineM(false, wxT("   ") + CFormat(_("Running on %s")) % wxGetOsDescription());
	AddLogLineM(false, wxT(" - ") + wxString(_("Visit http://www.amule.org to check if a new version is available.")));
	AddLogLineM(false, wxEmptyString);

#ifdef ENABLE_IP2COUNTRY
	m_IP2Country = new CIP2Country();
#endif
	m_searchwnd = new CSearchDlg(p_cnt);
	m_transferwnd = new CTransferWnd(p_cnt);
	m_sharedfileswnd = new CSharedFilesWnd(p_cnt);
	m_statisticswnd = new CStatisticsDlg(p_cnt, theApp->m_statistics);
	m_chatwnd = new CChatWnd(p_cnt);
	m_kademliawnd = CastChild(wxT("kadWnd"), CKadDlg);
	m_serverwnd->Show(FALSE);
	m_searchwnd->Show(FALSE);
	m_transferwnd->Show(FALSE);
	m_sharedfileswnd->Show(FALSE);
	m_statisticswnd->Show(FALSE);
	m_chatwnd->Show(FALSE);

	// Create the GUI timer
	gui_timer=new wxTimer(this,ID_GUI_TIMER_EVENT);
	if (!gui_timer) {
		AddLogLine(false, _("Fatal Error: Failed to create Timer"));
		exit(1);
	}

	// Set Serverlist as active window
	m_activewnd=NULL;
	SetActiveDialog(DT_NETWORKS_WND, m_serverwnd);
	m_wndToolbar->ToggleTool(ID_BUTTONNETWORKS, true );

	m_is_safe_state = true;

	// Init statistics stuff, better do it asap
	m_statisticswnd->Init();
	m_kademliawnd->Init();
	
	m_searchwnd->UpdateCatChoice();

	if (thePrefs::UseTrayIcon()) {
		CreateSystray();
	}

	Show(TRUE);

	// Must we start minimized?
	if (thePrefs::GetStartMinimized()) { 
		if (thePrefs::UseTrayIcon() && thePrefs::DoMinToTray()) {
			Hide_aMule();
		} else {
			Iconize(TRUE);
		}
	}
	m_BlinkMessages = false;
	m_CurrentBlinkBitmap = 24;


	// Set shortcut keys
	wxAcceleratorEntry entries[] = { 
		wxAcceleratorEntry(wxACCEL_CTRL, wxT('Q'), wxID_EXIT)
	};
	
	SetAcceleratorTable(wxAcceleratorTable(itemsof(entries), entries));	
	
	ShowED2KLinksHandler( thePrefs::GetFED2KLH() );
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
	
	if ( type == DT_TRANSFER_WND ) {
		if (thePrefs::ShowCatTabInfos()) {
			m_transferwnd->UpdateCatTabTitles();
		}
	}
	
	if ( m_activewnd ) {
		m_activewnd->Show(FALSE);
		contentSizer->Detach(m_activewnd);
	}

	contentSizer->Add(dlg, 1, wxALIGN_LEFT|wxEXPAND);
	dlg->Show(TRUE);
	m_activewnd=dlg;
	s_dlgcnt->Layout();

	// Since we might be suspending redrawing while hiding the dialog
	// we have to refresh it once it is visible again
	dlg->Refresh( true );
	dlg->SetFocus();
}


void CamuleDlg::UpdateTrayIcon(int percent)
{
	// set trayicon-icon
	if(!theApp->IsConnected()) {
		m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_DISCONNECTED, percent);
	} else {
		if(theApp->IsConnectedED2K() && theApp->serverconnect->IsLowID()) {
			m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_LOWID, percent);
		} else {
			m_wndTaskbarNotifier->SetTrayIcon(TRAY_ICON_HIGHID, percent);					
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
	

void CamuleDlg::RemoveSystray()
{
	delete m_wndTaskbarNotifier;
	m_wndTaskbarNotifier = NULL;
}


void CamuleDlg::OnToolBarButton(wxCommandEvent& ev)
{
	static int lastbutton = ID_BUTTONNETWORKS;

	// Kry - just if the GUI is ready for it
	if ( m_is_safe_state ) {

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
					SetActiveDialog(DT_NETWORKS_WND, m_serverwnd);
					// Set serverlist splitter position
					CastChild( wxT("SrvSplitterWnd"), wxSplitterWindow )->SetSashPosition(m_srv_split_pos, true);
					break;

				case ID_BUTTONSEARCH:
					// The search dialog should always display the handler
					if ( !thePrefs::GetFED2KLH() )
						ShowED2KLinksHandler( true );

					SetActiveDialog(DT_SEARCH_WND, m_searchwnd);
					break;

				case ID_BUTTONTRANSFER:
					SetActiveDialog(DT_TRANSFER_WND, m_transferwnd);
					// Prepare the dialog, sets the splitter-position
					m_transferwnd->Prepare();
					break;

				case ID_BUTTONSHARED:
					SetActiveDialog(DT_SHARED_WND, m_sharedfileswnd);
					break;

				case ID_BUTTONMESSAGES:
					m_BlinkMessages = false;
					SetActiveDialog(DT_CHAT_WND, m_chatwnd);
					break;

				case ID_BUTTONSTATISTICS:
					SetActiveDialog(DT_STATS_WND, m_statisticswnd);
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
		" Copyright (C) 2003-2006 aMule Team \n\n"
		" Part of aMule is based on \n"
		" Kademlia: Peer-to-peer routing based on the XOR metric.\n"
		" Copyright (C) 2002 Petar Maymounkov\n"
		" http://kademlia.scs.cs.nyu.edu\n");
	
	if (m_is_safe_state) {
		wxMessageBox(msg, _("Message"), wxOK | wxICON_INFORMATION, this);
	}
}


void CamuleDlg::OnPrefButton(wxCommandEvent& WXUNUSED(ev))
{
	if (m_is_safe_state) {
		if (m_prefsDialog == NULL) {
			m_prefsDialog = new PrefsUnifiedDlg(this);
			m_prefsDialog->TransferToWindow();
		}
		
		m_prefsDialog->Show(true);
		m_prefsDialog->Raise();
	}
}


#ifndef CLIENT_GUI
void CamuleDlg::OnImportButton(wxCommandEvent& WXUNUSED(ev))
{
	if ( m_is_safe_state ) {
		CPartFileConvert::ShowGUI(NULL);
	}
}
#endif

CamuleDlg::~CamuleDlg()
{
	printf("Shutting down aMule...\n");
	
	SaveGUIPrefs();

	theApp->amuledlg = NULL;
	
	printf("aMule dialog destroyed\n");
}


void CamuleDlg::OnBnConnect(wxCommandEvent& WXUNUSED(evt))
{

	bool disconnect = (theApp->IsConnectedED2K() || theApp->serverconnect->IsConnecting()) 
						#ifndef CLIENT_GUI
						|| (Kademlia::CKademlia::IsRunning())
						#endif
						;	
	if (thePrefs::GetNetworkED2K()) {
		if (disconnect) {
			//disconnect if currently connected
			if (theApp->serverconnect->IsConnecting()) {
				theApp->serverconnect->StopConnectionTry();
			} else {
				theApp->serverconnect->Disconnect();
			}
		} else {		
			//connect if not currently connected
			AddLogLine(true, _("Connecting"));
			theApp->serverconnect->ConnectToAnyServer();
		}
	} else {
		wxASSERT(!theApp->IsConnectedED2K());
	}

	// Connect Kad also
	if (thePrefs::GetNetworkKademlia()) {
		if( disconnect ) {
			theApp->StopKad();
		} else {
			theApp->StartKad();
		}
	} else {
		#ifndef CLIENT_GUI
			wxASSERT(!Kademlia::CKademlia::IsRunning());
		#endif
	}

}


void CamuleDlg::OnBnStatusText(wxCommandEvent& WXUNUSED(evt))
{
	wxString line = CastChild(wxT("infoLabel"), wxStaticText)->GetLabel();

	if (!line.IsEmpty()) {
		wxMessageBox(line, wxString(_("Status text")), wxOK|wxICON_INFORMATION, this);
	}
}


void CamuleDlg::ResetLog(int id)
{
	wxTextCtrl* ct = CastByID(id, m_serverwnd, wxTextCtrl);
	wxCHECK_RET(ct, wxT("Resetting unknown log"));

	ct->Clear();
	
	if (id == ID_LOGVIEW) {
		// Also clear the log line
		wxStaticText* text = CastChild(wxT("infoLabel"), wxStaticText);
		text->SetLabel(wxEmptyString);
		text->GetParent()->Layout();
	}
}


void CamuleDlg::AddLogLine(bool addtostatusbar, const wxString& line)
{
	// Remove newspace at end, it causes problems with the layout...
	wxString bufferline = line.Strip(wxString::trailing);

	// Create the timestamp
	wxString stamp = wxDateTime::Now().FormatISODate() + wxT(" ") + wxDateTime::Now().FormatISOTime() + wxT(": ");

	// Add the message to the log-view
	wxTextCtrl* ct = CastByID( ID_LOGVIEW, m_serverwnd, wxTextCtrl );
	if ( ct ) {
		if ( bufferline.IsEmpty() ) {
			// If it's empty we just write a blank line with no timestamp.
			ct->AppendText( wxT("\n") );
		} else {
			// Bold critical log-lines
			wxTextAttr style = ct->GetDefaultStyle();
			wxFont font = style.GetFont();
			font.SetWeight(addtostatusbar ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
			style.SetFont(font);
			ct->SetDefaultStyle(style);
			
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
	wxTextCtrl* cv= CastByID( ID_SERVERINFO, m_serverwnd, wxTextCtrl );
	if(cv) {
		if (message.Length() > 500) {
			cv->AppendText(message.Left(500) + wxT("\n"));
		} else {
			cv->AppendText(message + wxT("\n"));
		}
		cv->ShowPosition(cv->GetLastPosition()-1);
	}
}

void CamuleDlg::ShowConnectionState()
{
	enum ed2k_state { sED2KUnknown = -1, sDisconnected = 0, sLowID = 1, sConnecting = 2, sHighID = 3 };
	enum kad_state { sKadUnknown = -1, sOff = 4, sFirewalled = 5, sOK = 6 };
	static ed2k_state LastED2KState = sED2KUnknown;
	static kad_state LastKadState = sKadUnknown;
	static wxImageList status_arrows(16,16,true,0);
	static wxMemoryDC bitmap_dc;
	
	theApp->downloadqueue->OnConnectionState(theApp->IsConnected());
	
	if (!status_arrows.GetImageCount()) {
		// Generate the image list (This is only done once)
		for (int t = 0; t < 7; ++t) {
			status_arrows.Add(connImages(t));
		}
	}
	
	m_serverwnd->UpdateED2KInfo();
	m_serverwnd->UpdateKadInfo();
	
	ed2k_state NewED2KState;
	kad_state NewKadState;
	
	wxStaticText* connLabel = CastChild( wxT("connLabel"), wxStaticText );
	
	wxString connected_server;
	CServer* ed2k_server = theApp->serverconnect->GetCurrentServer();
	if (ed2k_server) {
		connected_server = ed2k_server->GetListName();
	}	
	
	if ( theApp->IsConnectedED2K() ) {
		if ( theApp->serverconnect->IsLowID() ) {
			NewED2KState = sLowID;
		} else {
			NewED2KState = sHighID;
		}
	} else if ( theApp->serverconnect->IsConnecting() ) {
		NewED2KState = sConnecting;
	} else {
		NewED2KState = sDisconnected;
	}

	if (theApp->IsConnectedKad()) {
		if (!theApp->IsFirewalledKad()) {
			NewKadState = sOK;
		} else {
			NewKadState = sFirewalled;
		}
	} else {
		NewKadState = sOff;
	}
	
	if ( (LastED2KState != NewED2KState) || (LastKadState != NewKadState)) {
		
		wxStaticBitmap* conn_bitmap = CastChild( wxT("connImage"), wxStaticBitmap );
		wxASSERT(conn_bitmap);
		
		wxBitmap conn_image = conn_bitmap->GetBitmap();
		
		bitmap_dc.SelectObject(conn_image);	
		
		m_wndToolbar->DeleteTool(ID_BUTTONCONNECT);
		
		if ((NewED2KState != sDisconnected) || (NewKadState != sOff)) {
			if (NewED2KState == sConnecting) {
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Cancel"),
					thePrefs::UseSkin() ? m_tblist.GetBitmap(2) : connButImg(2), wxNullBitmap, wxITEM_NORMAL,
					_("Stops the current connection attempts"));
			} else {
				/* ED2K connected or Kad connected */
				wxString popup = _("Disconnect from ");
				
				if (NewED2KState != sDisconnected) {
					popup += _("current server");
					if (NewKadState != sOff) {
						popup += _(" and ");
					}
				}
				
				if (NewKadState != sOff) {
					popup += wxT("Kad");
				}					
				
				m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Disconnect"),
					thePrefs::UseSkin() ? m_tblist.GetBitmap(1) : connButImg(1), wxNullBitmap, wxITEM_NORMAL,
					popup);
				
			}
		} else {
			m_wndToolbar->InsertTool(0, ID_BUTTONCONNECT, _("Connect"),
				thePrefs::UseSkin() ? m_tblist.GetBitmap(0) : connButImg(0), wxNullBitmap, wxITEM_NORMAL,
				_("Connect to any server and/or Kad"));
		}
		
		m_wndToolbar->Realize();
		
		if ( LastED2KState != NewED2KState ) {
			
			switch ( NewED2KState ) {
				case sLowID:
					// Display a warning about LowID connections
					AddLogLine(true,  _("WARNING: You have recieved Low-ID!"));
					AddLogLine(false, _("\tMost likely this is because you're behind a firewall or router."));
					AddLogLine(false, _("\tFor more information, please refer to http://wiki.amule.org"));
			
				case sHighID: {
					wxStaticText* tx = CastChild( wxT("infoLabel"), wxStaticText );
					tx->SetLabel(CFormat(_("Connection established on: %s")) % connected_server);
					connLabel->SetLabel(connected_server);
					
					break;
				}
				case sConnecting:
					connLabel->SetLabel(_("Connecting"));
					break;
		
				case sDisconnected:
					connLabel->SetLabel(_("Not Connected"));
					break;
		
				default:
					break;
			}
			/* Draw ED2K arrow */
			status_arrows.Draw(NewED2KState, bitmap_dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);
			
			LastED2KState = NewED2KState;
			
		}
		
		if (NewKadState != LastKadState) {
			int index = connLabel->GetLabel().Find(wxT(" (Kad:"));
			
			if (index == -1) {
				index = connLabel->GetLabel().Length();
			}
			
			if (NewKadState == sOK) {
				connLabel->SetLabel(connLabel->GetLabel().Left(index) + wxT(" (Kad: ok)"));
			} else if (NewKadState == sFirewalled) {
				connLabel->SetLabel(connLabel->GetLabel().Left(index) + wxT(" (Kad: firewalled)"));
			} else {
				connLabel->SetLabel(connLabel->GetLabel().Left(index) + wxT(" (Kad: off)"));
			}
	
			/* Kad Connecting arrow */
			status_arrows.Draw(NewKadState, bitmap_dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);
			
			LastKadState = NewKadState;
		}
		
		connLabel->GetParent()->Layout();
		
		bitmap_dc.SelectObject(wxNullBitmap);
		
		conn_bitmap->SetBitmap(conn_image);
		
	} else {
		if (theApp->IsConnectedED2K()) {
			connLabel->SetLabel(connected_server);
			connLabel->GetParent()->Layout();
		}
	}
}

void CamuleDlg::ShowUserCount(const wxString& info)
{
	wxStaticText* label = CastChild( wxT("userLabel"), wxStaticText );
	
	// Update Kad tab
	m_serverwnd->UpdateKadInfo();
	
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
		SetTitle(theApp->m_FrameTitle + UpDownSpeed);
	}

	if (m_wndTaskbarNotifier) {
		// set trayicon-icon
		int percentDown = (int)ceil((kBpsDown*100) / thePrefs::GetMaxGraphDownloadRate());
		UpdateTrayIcon( ( percentDown > 100 ) ? 100 : percentDown);
	
		wxString buffer2;
		if ( theApp->IsConnected() ) {
			buffer2 = CFormat(_("aMule (%s | Connected)")) % buffer;
		} else {
			buffer2 = CFormat(_("aMule (%s | Disconnected)")) % buffer;
		}
		m_wndTaskbarNotifier->SetTrayToolTip(buffer2);
	}

	wxStaticBitmap* bmp = CastChild( wxT("transferImg"), wxStaticBitmap );
	bmp->SetBitmap(dlStatusImages((kBpsUp>0.01 ? 2 : 0) + (kBpsDown>0.01 ? 1 : 0)));
}

void CamuleDlg::DlgShutDown()
{
	// Are we already shutting down or still on init?
	if ( m_is_safe_state == false ) {
		return;
	}

	// we are going DOWN
	m_is_safe_state = false;

	// Stop the GUI Timer
	delete gui_timer;
	m_transferwnd->downloadlistctrl->DeleteAllItems();

	// We want to delete the systray too!
	RemoveSystray();
}

void CamuleDlg::OnClose(wxCloseEvent& evt)
{
	// This will be here till the core close is != app close
	if (evt.CanVeto() && thePrefs::IsConfirmExitEnabled() ) {
		if (wxNO == wxMessageBox(wxString(_("Do you really want to exit aMule?")),
				wxString(_("Exit confirmation")), wxYES_NO, this)) {
			evt.Veto();
			return;
		}
	}
	
	theApp->ShutDown(evt);
}


void CamuleDlg::OnBnClickedFast(wxCommandEvent& WXUNUSED(evt))
{
	wxTextCtrl* ctl = CastChild( wxT("FastEd2kLinks"), wxTextCtrl );

	for ( int i = 0; i < ctl->GetNumberOfLines(); i++ ) {
		wxString strlink = ctl->GetLineText(i);
		strlink.Trim(true);
		strlink.Trim(false);
		if ( !strlink.IsEmpty() ) {
			theApp->downloadqueue->AddLink( strlink, m_transferwnd->downloadlistctrl->GetCategory() );
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

	// Kry - Random usable pos for m_srv_split_pos
	m_srv_split_pos = config->Read(section+wxT("SRV_SPLITTER_POS"), 463l);

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

	if (!override_pos) {
		// If x1 and y1 != 0 Redefine location
		if((x1 != -1) && (y1 != -1)) {
			Move(x1, y1);
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
	config->Write(section+wxT("SRV_SPLITTER_POS"), (long) m_srv_split_pos);

	config->Flush(true);

	/* End modif */

	return true;
}


void CamuleDlg::Hide_aMule(bool iconize)
{
	if (IsShown() && ((m_last_iconizing + 2000) < GetTickCount())) { // 1 secs for sanity
		m_last_iconizing = GetTickCount();

		if (m_prefsDialog and m_prefsDialog->IsShown()) {
			m_prefsVisible = true;
			m_prefsDialog->Iconize(true);;
			m_prefsDialog->Show(false);
		} else {
			m_prefsVisible = false;
		}
		
		if (iconize) {
			Iconize(TRUE);
		}
		
		Show(FALSE);
	}

}


void CamuleDlg::Show_aMule(bool uniconize)
{
	if (!IsShown() && ((m_last_iconizing + 1000) < GetTickCount())) { // 1 secs for sanity
		m_last_iconizing = GetTickCount();
	
		if (m_prefsDialog && m_prefsVisible) {
			m_prefsDialog->Show(true);
			m_prefsDialog->Raise();
		}
		
		if (uniconize) {
			Show(TRUE);
			Raise();
		}
	}
}


void CamuleDlg::OnMinimize(wxIconizeEvent& evt)
{
	if (m_wndTaskbarNotifier && thePrefs::DoMinToTray()) {
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
		
		GraphUpdateInfo update = theApp->m_statistics->GetPointsForUpdate();
		
		m_statisticswnd->UpdateStatGraphs(bStatsVisible, theStats::GetPeakConnections(), update);
		m_kademliawnd->UpdateGraph(!IsIconized() && (m_activewnd == m_serverwnd), update);
	}
#else
	#warning TODO: CORE/GUI -- EC needed
#endif
	
	int sStatsUpdate = thePrefs::GetStatsInterval();
	if ((sStatsUpdate > 0) && ((int)(msCur - msPrevStats) > sStatsUpdate*1000)) {
		if (bStatsVisible) {
			msPrevStats = msCur;
			m_statisticswnd->ShowStatistics();
		}
	}

	if (msCur-msPrev5 > 5000) {  // every 5 seconds
		msPrev5 = msCur;
		ShowTransferRate();
		if (thePrefs::ShowCatTabInfos() && theApp->amuledlg->m_activewnd == theApp->amuledlg->m_transferwnd) {
			m_transferwnd->UpdateCatTabTitles();
		}
		if (thePrefs::AutoSortDownload()) {
			m_transferwnd->downloadlistctrl->SortList();
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
	int pos = m_wndToolbar->GetToolPos(ID_BUTTONMESSAGES);
	wxASSERT(pos == 6); // so we don't miss a change on wx2.4
	
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
	CTerminationProcess *p = new CTerminationProcess(cmd);
	if ( !cmd.IsEmpty() ) {
		wxString tmp = url;
		// Pipes cause problems, so escape them
		tmp.Replace( wxT("|"), wxT("%7C") );

		if (!cmd.Replace(wxT("%s"), tmp)) {
			// No %s found, just append the url
			cmd += wxT(" ") + tmp;
		}

		if (wxExecute(cmd, wxEXEC_ASYNC, p)) {
			printf( "Launch Command: %s\n", (const char *)unicode2char(cmd));
			return;
		}
#ifdef __WXMSW__
	} else {
		wxFileType* ft =
			wxTheMimeTypesManager->GetFileTypeFromExtension(wxT("html"));
		if (!ft) {
			wxLogError(
				wxT("Impossible to determine the file type for extension html."
				"Please edit your MIME types."));
			return;
		}

		bool ok = ft->GetOpenCommand(
			&cmd, wxFileType::MessageParameters(url, wxT("")));
		delete ft;

		if (!ok) {
			wxMessageBox(
				_("Could not determine the command for running the browser."),
				wxT("Browsing problem"), wxOK|wxICON_EXCLAMATION, this);
			return;
		}

		wxPuts(wxT("Launch Command: ") + cmd);
		if (wxExecute(cmd, wxEXEC_ASYNC, p)) {
			return;
		}
#endif
	}
	// Unable to execute browser. But this error message doesn't make sense,
	// cosidering that you _can't_ set the browser executable path... =/
	wxLogError(wxT(
		"Unable to launch browser. "
		"Please set correct browser executable path in Preferences."));
}



wxString CamuleDlg::GenWebSearchUrl(const wxString &filename, WebSearch wsProvider )
{
	wxString URL;
	switch (wsProvider)  {
		case WS_FILEHASH:
			URL = wxT("http://www.filehash.com/search.html?pattern=FILENAME&submit=Find");
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


void CamuleDlg::Apply_Clients_Skin(wxString skinFileName)
{	
	static int ClientItemNumber = CLIENT_SKIN_UNUSED;

	SkinItem bitmaps_found[ClientItemNumber];
	for (int i = 0; i < ClientItemNumber; ++i) {	
		bitmaps_found[i].found = false;
	}

	wxTextFile skinfile;
	try {
		if (skinFileName.IsEmpty()) {			
			throw wxString(_("Skin file name is empty"));
		}
		if (!::wxFileExists(skinFileName)) {
			throw (CFormat(_("Skin file %s does not exist")) % skinFileName ).GetString();
		}
		if (!skinfile.Open(skinFileName)) {
			throw (CFormat(_("Unable to open skin file: %s")) % skinFileName).GetString();
		}
		
		int client_header_found = -1;
		for (uint32 i=0; i < skinfile.GetLineCount(); ++i) {
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
		
		for (int i = 0; i < ClientItemNumber; ++i) {
			if (bitmaps_found[i].found) {
				wxImage new_image;
				wxFileName file_name(bitmaps_found[i].filename);
				file_name.Normalize();
				if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
					m_imagelist.Add(wxBitmap(new_image));
				} else {
					printf(	"Warning: wrong client bitmap file No.%i: %s",
						i, (const char *)unicode2char(bitmaps_found[i].filename));
					m_imagelist.Add(wxBitmap(clientImages(i)));
				}
			} else {
				m_imagelist.Add(wxBitmap(clientImages(i)));
			}
		}		

		skinfile.Close();
	} catch (const wxString &error) {
		AddLogLineM( true, error + _(" - loading defaults"));
		
		// Load defaults
		for (int i = 0; i < ClientItemNumber; ++i) {
			m_imagelist.Add(wxBitmap(clientImages(i)));
		}
	}
}


void CamuleDlg::Apply_Toolbar_Skin(wxString skinFileName, wxToolBar* wndToolbar)
{
	try {
		wxTextFile skinFile;
		if (skinFileName.IsEmpty()) {			
			throw wxString(_("Skin file name is empty"));
		}
		if (!::wxFileExists(skinFileName)) {
			throw (CFormat(_("Skin file %s does not exist")) % skinFileName ).GetString();
		}
		if (!skinFile.Open(skinFileName)) {
			throw (CFormat(_("Unable to open skin file: %s")) % skinFileName).GetString();
		}

		int client_header_found = -1;    
		for (uint32 i=0; i < skinFile.GetLineCount(); ++i) {
			if (skinFile[i] == wxT("[Toolbar Bitmaps]")) {
				client_header_found = i;	
				break;
			}
		}
			
		if (client_header_found != -1) {
			for (uint32 i=client_header_found+1; i < skinFile.GetLineCount(); i++) {
				if (skinFile[i].StartsWith(wxT("["))) {
					break;
				}				
				// Connect Icon
				if (skinFile[i].StartsWith(wxT("Connect_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(connButImg(0)));
					}
				}
				// Disconnect Icon
				if (skinFile[i].StartsWith(wxT("Disconnect_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(connButImg(1)));
					}
				}
				// Connecting Icon
				if (skinFile[i].StartsWith(wxT("Connecting_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						connButImg(2) = wxBitmap(new_image);
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(connButImg(2)));
					}
				}
				// Network Icon
				if (skinFile[i].StartsWith(wxT("Network_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(20)));
					}
				}
				// Transfer Icon
				if (skinFile[i].StartsWith(wxT("Transfer_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(21)));
					}
				}
				// Search Icon
				if (skinFile[i].StartsWith(wxT("Search_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(22)));
					}
				}
				// Share Icon
				if (skinFile[i].StartsWith(wxT("Share_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(23)));
					}
				}
				// Messages Icon
				if (skinFile[i].StartsWith(wxT("Messages_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(24)));
					}
				}
				// Stat Icon
				if (skinFile[i].StartsWith(wxT("Stat_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(25)));
					}
				}
				// Pref Icon
				if (skinFile[i].StartsWith(wxT("Pref_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(26)));
					}
				}
				// Import Icon
				if (skinFile[i].StartsWith(wxT("Import_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(29)));
					}
				}
				// Help Icon
				if (skinFile[i].StartsWith(wxT("Help_icon="))) {
					wxImage new_image;
					wxFileName file_name(skinFile[i].AfterLast(wxT('=')));
					file_name.Normalize();
					if (file_name.FileExists() && new_image.LoadFile(file_name.GetFullPath())) {
						m_tblist.Add(wxBitmap(new_image));
					} else {
						m_tblist.Add(wxBitmap(amuleDlgImages(32)));
					}
				}
			}

			wndToolbar->SetMargins(0, 0);
			wndToolbar->AddTool( ID_BUTTONCONNECT, _("Connect"), m_tblist.GetBitmap(0),
				wxNullBitmap, wxITEM_NORMAL, _("Connect to any server and/or Kad") );
			wndToolbar->AddSeparator();
			wndToolbar->AddTool( ID_BUTTONNETWORKS, _("Networks"), m_tblist.GetBitmap(3),
				wxNullBitmap, wxITEM_CHECK, _("Networks Window") );
			wndToolbar->ToggleTool( ID_BUTTONNETWORKS, TRUE );
			wndToolbar->AddTool( ID_BUTTONSEARCH, _("Searches"), m_tblist.GetBitmap(5),
				wxNullBitmap, wxITEM_CHECK, _("Searches Window") );
			wndToolbar->AddTool( ID_BUTTONTRANSFER, _("Transfers"), m_tblist.GetBitmap(4),
				wxNullBitmap, wxITEM_CHECK, _("Files Transfers Window") );
			wndToolbar->AddTool( ID_BUTTONSHARED, _("Shared Files"), m_tblist.GetBitmap(6),
				wxNullBitmap, wxITEM_CHECK, _("Shared Files Window") );
			wndToolbar->AddTool( ID_BUTTONMESSAGES, _("Messages"), m_tblist.GetBitmap(7),
				wxNullBitmap, wxITEM_CHECK, _("Messages Window") );
			wndToolbar->AddTool( ID_BUTTONSTATISTICS, _("Statistics"), m_tblist.GetBitmap(8),
				wxNullBitmap, wxITEM_CHECK, _("Statistics Graph Window") );
			wndToolbar->AddSeparator();
			wndToolbar->AddTool( ID_BUTTONNEWPREFERENCES, _("Preferences"), m_tblist.GetBitmap(9),
				wxNullBitmap, wxITEM_NORMAL, _("Preferences Settings Window") );
			wndToolbar->AddTool( ID_BUTTONIMPORT, _("Import"), m_tblist.GetBitmap(10),
				wxNullBitmap, wxITEM_NORMAL, _("The partfile importer tool") );
			wndToolbar->AddTool( ID_ABOUT, _("About"), m_tblist.GetBitmap(11),
				wxNullBitmap, wxITEM_NORMAL, _("About/Help") );
			wndToolbar->Realize();
		} else {
			muleToolbar(m_wndToolbar);
		}
	} catch (const wxString &error) {
		AddLogLineM( true, error + _(" - loading toolbar defaults"));

		// Load defaults
		muleToolbar(m_wndToolbar);
	}
}


void CamuleDlg::Create_Toolbar(wxString skinFileName, bool orientation)
{
	Freeze();
	// Create ToolBar from the one designed by wxDesigner (BigBob)
	wxToolBar* current = GetToolBar();
	if (current) {
		current->Destroy();
		SetToolBar(NULL); // Remove old one if present
	}
	m_wndToolbar = CreateToolBar(
		(orientation ? wxTB_VERTICAL : wxTB_HORIZONTAL) |
		wxNO_BORDER | wxTB_TEXT | wxTB_3DBUTTONS |
		wxTB_FLAT | wxCLIP_CHILDREN | wxTB_NODIVIDER);
	m_wndToolbar->SetToolBitmapSize(wxSize(32, 32));
	
	if (!thePrefs::UseSkin()) {
		muleToolbar( m_wndToolbar );
	} else {
		Apply_Toolbar_Skin( skinFileName, m_wndToolbar );		
	}

	#ifdef CLIENT_GUI
	m_wndToolbar->DeleteTool(ID_BUTTONIMPORT);
	#endif

	Thaw();
}


void CamuleDlg::OnMainGUISizeChange(wxSizeEvent& evt)
{
	wxFrame::OnSize(evt);	
	if (m_transferwnd && m_transferwnd->clientlistctrl) {
		// Transfer window's splitter set again if it's hidden.
		if ( m_transferwnd->clientlistctrl->GetListView() == vtNone ) {
			int height  = m_transferwnd->clientlistctrl->GetSize().GetHeight();
		
			wxSplitterWindow* splitter = CastChild( wxT("splitterWnd"), wxSplitterWindow );
			height += splitter->GetWindow1()->GetSize().GetHeight();
		
			splitter->SetSashPosition( height );
		}
	}
	
}


void CamuleDlg::OnKeyPressed(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_F1) {
		LaunchUrl(wxT("http://wiki.amule.org"));
	} else {
		event.Skip();
	}
}


void CamuleDlg::OnExit(wxCommandEvent& WXUNUSED(evt))
{
	Close();
}
// File_checked_for_headers

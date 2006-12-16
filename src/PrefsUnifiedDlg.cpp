//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Original author: Emilio Sandoz
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


#include "PrefsUnifiedDlg.h"

#include <include/common/Constants.h>

#include <wx/colordlg.h>
#include <wx/tooltip.h>

#include "amule.h"				// Needed for theApp
#include "amuleDlg.h"
#include "Color.h"
#include "EditServerListDlg.h"
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "StatisticsDlg.h"		// Needed for graph parameters, colors
#include "IPFilter.h"			// Needed for CIPFilter
#include "SearchList.h"
#include "ClientList.h"
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl
#include "Preferences.h"
#include "muuli_wdr.h"
#include "Logger.h"
#include <common/Format.h>				// Needed for CFormat
#include "TransferWnd.h"		// Needed for CTransferWnd::UpdateCatTabTitles()
#include "KadDlg.h"				// Needed for CKadDlg
#include "OScopeCtrl.h"			// Needed for OScopeCtrl
#include "ServerList.h"
#include "UserEvents.h"

BEGIN_EVENT_TABLE(PrefsUnifiedDlg,wxDialog)
	// Events
#define USEREVENTS_EVENT(ID, NAME, VARS) \
	EVT_CHECKBOX(USEREVENTS_FIRST_ID + CUserEvents::ID * USEREVENTS_IDS_PER_EVENT + 1,	PrefsUnifiedDlg::OnCheckBoxChange) \
	EVT_CHECKBOX(USEREVENTS_FIRST_ID + CUserEvents::ID * USEREVENTS_IDS_PER_EVENT + 3,	PrefsUnifiedDlg::OnCheckBoxChange)
	USEREVENTS_EVENTLIST()
#undef USEREVENTS_EVENT

	// Proxy
	EVT_CHECKBOX(ID_PROXY_ENABLE_PROXY,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(ID_PROXY_ENABLE_PASSWORD,	PrefsUnifiedDlg::OnCheckBoxChange)
//	EVT_CHECKBOX(ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY,	PrefsUnifiedDlg::OnCheckBoxChange)

	// Connection
	EVT_SPINCTRL(IDC_PORT,			PrefsUnifiedDlg::OnTCPClientPortChange)

	// The rest. Organize it!
	EVT_CHECKBOX(IDC_UDPDISABLE,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_CHECKDISKSPACE,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_USESKIN,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ONLINESIG,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_REMOVEDEAD,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLE_AUTO_HQRS,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_AUTOSERVER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_AUTOIPFILTER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER_ALL,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER_WORD,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_STARTNEXTFILE,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLETRAYICON,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_VERTTOOLBAR,	PrefsUnifiedDlg::OnCheckBoxChange)

	EVT_BUTTON(ID_PREFS_OK_TOP,		PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_OK,			PrefsUnifiedDlg::OnOk)

	EVT_BUTTON(ID_PREFS_CANCEL_TOP,		PrefsUnifiedDlg::OnCancel)

	// Browse buttons
	EVT_BUTTON(IDC_SELSKINFILE,		PrefsUnifiedDlg::OnButtonBrowseSkin)
	EVT_BUTTON(IDC_BTN_BROWSE_WAV,		PrefsUnifiedDlg::OnButtonBrowseWav)
	EVT_BUTTON(IDC_BROWSEV,			PrefsUnifiedDlg::OnButtonBrowseApplication)
	EVT_BUTTON(IDC_SELTEMPDIR,		PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELINCDIR,		PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELOSDIR,		PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELBROWSER,		PrefsUnifiedDlg::OnButtonBrowseApplication)

	EVT_SPINCTRL(IDC_TOOLTIPDELAY,		PrefsUnifiedDlg::OnToolTipDelayChange)

	EVT_BUTTON(IDC_EDITADR,			PrefsUnifiedDlg::OnButtonEditAddr)
	EVT_BUTTON(IDC_IPFRELOAD,		PrefsUnifiedDlg::OnButtonIPFilterReload)
	EVT_BUTTON(IDC_COLOR_BUTTON,		PrefsUnifiedDlg::OnButtonColorChange)
	EVT_BUTTON(IDC_IPFILTERUPDATE,		PrefsUnifiedDlg::OnButtonIPFilterUpdate)
	EVT_CHOICE(IDC_COLORSELECTOR,		PrefsUnifiedDlg::OnColorCategorySelected)
	EVT_CHOICE(IDC_BROWSER,			PrefsUnifiedDlg::OnBrowserChange)
	EVT_LIST_ITEM_SELECTED(ID_PREFSLISTCTRL,PrefsUnifiedDlg::OnPrefsPageChange)

	EVT_INIT_DIALOG(PrefsUnifiedDlg::OnInitDialog)

	EVT_COMMAND_SCROLL(IDC_SLIDER,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER3,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER4,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SLIDER2,		PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_FILEBUFFERSIZE,	PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_QUEUESIZE,	PrefsUnifiedDlg::OnScrollBarChange)
	EVT_COMMAND_SCROLL(IDC_SERVERKEEPALIVE,	PrefsUnifiedDlg::OnScrollBarChange)

	EVT_SPINCTRL(IDC_MAXUP,			PrefsUnifiedDlg::OnRateLimitChanged)

	EVT_LIST_ITEM_SELECTED(IDC_EVENTLIST,	PrefsUnifiedDlg::OnUserEventSelected)

	EVT_CLOSE(PrefsUnifiedDlg::OnClose)

END_EVENT_TABLE()


/**
 * This struct provides a general way to represent config-tabs.
 */
struct PrefsPage
{
	//! The title of the page, used on the listctrl.
	wxString	m_title;
	//! Function pointer to the wxDesigner function creating the dialog.
	wxSizer*	(*m_function)(wxWindow*, bool, bool );
	//! The index of the image used on the list.
	int 		m_imageidx;
	//! The actual widget. To be set later.
	wxPanel*	m_widget;
};


PrefsPage pages[] =
{
	{ wxTRANSLATE("General"),		PreferencesGeneralTab,		13, NULL },
	{ wxTRANSLATE("Connection"),		PreferencesConnectionTab,	14, NULL },
	{ wxTRANSLATE("Proxy"),			PreferencesProxyTab,		24, NULL },
	{ wxTRANSLATE("Message Filter"),	PreferencesMessagesTab,		23, NULL },
	{ wxTRANSLATE("Remote Controls"),	PreferencesRemoteControlsTab,	11, NULL },
	{ wxTRANSLATE("Online Signature"),	PreferencesOnlineSigTab,	21, NULL },
	{ wxTRANSLATE("Server"),		PreferencesServerTab,		15, NULL },
	{ wxTRANSLATE("Files"),			PreferencesFilesTab,		16, NULL },
	{ wxTRANSLATE("Sources Dropping"),	PreferencesSourcesDroppingTab,	20, NULL },
	{ wxTRANSLATE("Directories"),		PreferencesDirectoriesTab,	17, NULL },
	{ wxTRANSLATE("Statistics"),		PreferencesStatisticsTab,	10, NULL },
	{ wxTRANSLATE("Security"),		PreferencesSecurityTab,		22, NULL },
	//Notications are disabled since they havent been implemented
	//{ wxTRANSLATE("Notifications"),	PreferencesNotifyTab,		18, NULL },
	{ wxTRANSLATE("Gui Tweaks"),		PreferencesGuiTweaksTab,	19, NULL },
	{ wxTRANSLATE("Core Tweaks"),		PreferencesaMuleTweaksTab,	12, NULL },
	{ wxTRANSLATE("Events"),			PreferencesEventsTab,		5,	NULL }
#ifdef __DEBUG__
	,{ wxTRANSLATE("Debugging"),		PreferencesDebug,			25, NULL }
#endif
};


PrefsUnifiedDlg::PrefsUnifiedDlg(wxWindow *parent)
:
wxDialog(parent, -1, _("Preferences"), wxDefaultPosition, wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	preferencesDlgTop( this, FALSE );
	
	wxListCtrl* PrefsIcons = CastChild( ID_PREFSLISTCTRL, wxListCtrl );

	wxImageList* icon_list = new wxImageList(16, 16);
	PrefsIcons->AssignImageList( icon_list, wxIMAGE_LIST_SMALL);

	// Add the single column used
	PrefsIcons->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT, PrefsIcons->GetSize().GetWidth()-5);

	// Temp variables for finding the smallest height and width needed
	int width = 0;
	int height = 0;

	// Add each page to the page-list
	for ( unsigned int i = 0; i < itemsof(pages); i++ ) {
		// Add the icon and label assosiated with the page
		icon_list->Add( amuleSpecial(pages[i].m_imageidx) );
		PrefsIcons->InsertItem(i, wxGetTranslation(pages[i].m_title), i);
	}
	
	// Set list-width so that there arn't any scrollers
	PrefsIcons->SetColumnWidth( 0, wxLIST_AUTOSIZE );
	PrefsIcons->SetMinSize(wxSize(PrefsIcons->GetColumnWidth( 0 ) + 10, -1));
	PrefsIcons->SetMaxSize(wxSize(PrefsIcons->GetColumnWidth( 0 ) + 10, -1));

	// Now add the pages and calculate the minimum size	
	for ( unsigned int i = 0; i < itemsof(pages); i++ ) {
		// Create a container widget and the contents of the page
		pages[i].m_widget = new wxPanel( this, -1 );
		pages[i].m_function( pages[i].m_widget, true, true );

		// Add it to the sizer
		prefs_sizer->Add( pages[i].m_widget, 0, wxGROW|wxEXPAND );

		if (pages[i].m_function == PreferencesGeneralTab) {
			// This must be done now or pages won't Fit();
			#ifdef __WXMSW__ 
				CastChild(IDC_BROWSERTABS, wxCheckBox)->Enable(false);
				wxChoice *browserCheck = CastChild(IDC_BROWSER, wxChoice);
				browserCheck->Clear();
				browserCheck->Append(_("System default"));
				browserCheck->Append(_("User Defined"));
			#endif /* __WXMSW__ */
		} else if (pages[i].m_function == PreferencesEventsTab) {

#define USEREVENTS_REPLACE_VAR(VAR, DESC, CODE)	+ wxString(wxT("\n  %") VAR wxT(" - ")) + wxGetTranslation(DESC)
#define USEREVENTS_EVENT(ID, NAME, VARS)	case CUserEvents::ID: CreateEventPanels(idx, wxEmptyString VARS, pages[i].m_widget); break;

			wxListCtrl *list = CastChild(IDC_EVENTLIST, wxListCtrl);
			list->InsertColumn(0, wxEmptyString);
			for (unsigned int idx = 0; idx < CUserEvents::GetCount(); ++idx) {
				long lidx = list->InsertItem(idx,
					wxGetTranslation(CUserEvents::GetDisplayName(
						static_cast<enum CUserEvents::EventType>(idx))));
				if (lidx != -1) {
					list->SetItemData(lidx,
						USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT);
					switch (idx) {
						USEREVENTS_EVENTLIST()
					}
				}
			}
			list->SetColumnWidth(0, wxLIST_AUTOSIZE);
		}

		// Align and resize the page
		Fit();
		Layout();

		// Find the greatest sizes
		wxSize size = prefs_sizer->GetSize();
		if ( size.GetWidth() > width ) {
			width = size.GetWidth();
		}

		if ( size.GetHeight() > height ) {
			height = size.GetHeight();
		}

		// Hide it for now
		prefs_sizer->Remove( pages[i].m_widget );
		pages[i].m_widget->Show( false );
	}
	
	// Default to the General tab
	m_CurrentPanel = pages[0].m_widget;
	prefs_sizer->Add( pages[0].m_widget, 0, wxGROW|wxEXPAND );
	m_CurrentPanel->Show( true );

	// Select the first item
	PrefsIcons->SetItemState( 0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );

	// We now have the needed minimum height and width
	prefs_sizer->SetMinSize( width, height );

	// Store some often used pointers
	m_ShareSelector = CastChild( IDC_SHARESELECTOR, CDirectoryTreeCtrl );
	m_buttonColor   = CastChild( IDC_COLOR_BUTTON, wxButton );
	m_choiceColor   = CastChild( IDC_COLORSELECTOR, wxChoice );

	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->ConnectToWidget( it->first, this ) ) {
			printf("Failed to connect Cfg to widget with the ID %d and key %s\n",
				it->first, (const char *)unicode2char(it->second->GetKey()));
		}
	}
	Fit();

	// It must not be resized to something smaller than what it currently is
	wxSize size = GetClientSize();
	SetSizeHints( size.GetWidth(), size.GetHeight() );
	
	#ifdef __WXMSW__
		FindWindow(IDC_VERTTOOLBAR)->Enable(false);
	#endif
}


Cfg_Base* PrefsUnifiedDlg::GetCfg(int id)
{
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.find( id );

	if ( it != thePrefs::s_CfgList.end() ) {
		return it->second;
	}

	return NULL;
}


bool PrefsUnifiedDlg::TransferToWindow()
{
	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferToWindow() ) {
			printf("Failed to transfer data from Cfg to Widget with the ID %d and key %s\n",
				it->first, (const char *)unicode2char(it->second->GetKey()));
		}
	}

	m_ShareSelector->SetSharedDirectories(&theApp.glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		thePrefs::s_colors[i] = CStatisticsDlg::acrStat[i];
		thePrefs::s_colors_ref[i] = CStatisticsDlg::acrStat[i];
	}
	
	// Connection tab
	wxSpinEvent e;
	OnTCPClientPortChange(e);
	
	// Proxy tab initialization
	if (!CastChild(ID_PROXY_ENABLE_PROXY, wxCheckBox)->IsChecked()) {
		FindWindow(ID_PROXY_TYPE)->Enable(false);
		FindWindow(ID_PROXY_NAME)->Enable(false);
		FindWindow(ID_PROXY_PORT)->Enable(false);
	}
	if (!CastChild(ID_PROXY_ENABLE_PASSWORD, wxCheckBox)->IsChecked()) {
		FindWindow(ID_PROXY_USER)->Enable(false);
		FindWindow(ID_PROXY_PASSWORD)->Enable(false);
	}
	// This option from the proxy tab is currently unused
	FindWindow(ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY)->Enable(false);
	
	// Enable/Disable some controls
	bool customBrowser = CastChild( IDC_BROWSER, wxChoice )->GetSelection() == CastChild( IDC_BROWSER, wxChoice )->GetCount() - 1;
	FindWindow( IDC_BROWSERSELF )->Enable( customBrowser );
	FindWindow( IDC_SELBROWSER )->Enable( customBrowser );
	#ifndef __WXMSW__
		FindWindow( IDC_BROWSERTABS )->Enable( !customBrowser );
	#endif
	FindWindow( IDC_MINDISKSPACE )->Enable( thePrefs::IsCheckDiskspaceEnabled() );
	FindWindow( IDC_SKINFILE )->Enable( thePrefs::UseSkin() );
	FindWindow( IDC_OSDIR )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_OSUPDATE )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_UDPPORT )->Enable( !thePrefs::s_UDPDisable );
	FindWindow( IDC_SERVERRETRIES )->Enable( thePrefs::DeadServer() );
	FindWindow( IDC_HQR_VALUE )->Enable( thePrefs::DropHighQueueRankingSources() );
	FindWindow( IDC_STARTNEXTFILE_SAME )->Enable(thePrefs::StartNextFile());

#ifdef __WXMAC__
	FindWindow(IDC_ENABLETRAYICON)->Enable(false);
	FindWindow(IDC_MINTRAY)->Enable(false);
#else
	FindWindow(IDC_MINTRAY)->Enable(thePrefs::UseTrayIcon());
#endif

	if (!CastChild(IDC_MSGFILTER, wxCheckBox)->IsChecked()) {
		FindWindow(IDC_MSGFILTER_ALL)->Enable(false);
		FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(false);
		FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(false);
		FindWindow(IDC_MSGFILTER_WORD)->Enable(false);		
		FindWindow(IDC_MSGWORD)->Enable(false);
	} else if (CastChild(IDC_MSGFILTER_ALL, wxCheckBox)->IsChecked()) {
		FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(false);
		FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(false);
		FindWindow(IDC_MSGFILTER_WORD)->Enable(false);		
		FindWindow(IDC_MSGWORD)->Enable(false);	
	}

	FindWindow(IDC_MSGWORD)->Enable(CastChild(IDC_MSGFILTER_WORD, wxCheckBox)->IsChecked());	
	
	// Set debugging toggles
#ifdef __DEBUG__
	int count = CLogger::GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		const CDebugCategory& cat = CLogger::GetDebugCategory( i );
		
		list->Append( cat.GetName() );
		list->Check( i, cat.IsEnabled() );
	}
#endif
	
	return true;
}


bool PrefsUnifiedDlg::TransferFromWindow()
{
	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferFromWindow() ) {
			printf("Failed to transfer data from Widget to Cfg with the ID %d and key %s\n",
				it->first, (const char *)unicode2char(it->second->GetKey()));
		}
	}

	theApp.glob_prefs->shareddir_list.Clear();
	m_ShareSelector->GetSharedDirectories(&theApp.glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		if ( thePrefs::s_colors[i] != thePrefs::s_colors_ref[i] ) {
			CStatisticsDlg::acrStat[i] = thePrefs::s_colors[i];
			theApp.amuledlg->m_statisticswnd->ApplyStatsColor(i);
		}

		theApp.amuledlg->m_kademliawnd->SetGraphColors();
	}

	// Get debugging toggles
#ifdef __DEBUG__
	int count = CLogger::GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		const CDebugCategory& cat = CLogger::GetDebugCategory( i );
		
		CLogger::SetEnabled( cat.GetType(), list->IsChecked( i ) );
	}
#endif

	#ifdef CLIENT_GUI
	// Send preferences to core.
	theApp.glob_prefs->SendToRemote();
	#endif
	
	return true;
}


bool PrefsUnifiedDlg::CfgChanged(int ID)
{
	Cfg_Base* cfg = GetCfg(ID);

	if ( cfg ) {
		return cfg->HasChanged();
	}

	return false;
}


void PrefsUnifiedDlg::OnOk(wxCommandEvent& WXUNUSED(event))
{
	TransferFromWindow();

	bool restart_needed = false;
	wxString restart_needed_msg = _("aMule must be restarted to enable these changes:\n\n");
	
	// do sanity checking, special processing, and user notifications here
	thePrefs::CheckUlDlRatio();

	if (CfgChanged(IDC_PORT)) {
		restart_needed = true;
		restart_needed_msg += _("- TCP port changed.\n");
	}

	if (CfgChanged(IDC_UDPPORT)) {
		restart_needed = true;
		restart_needed_msg += _("- UDP port changed.\n");
	}

	// Force port checking
	thePrefs::SetPort(thePrefs::GetPort());
	
	if (	IsEmptyFile(theApp.ConfigDir + wxT("addresses.dat")) && 
		CastChild(IDC_AUTOSERVER, wxCheckBox)->IsChecked() ) {
		thePrefs::UnsetAutoServerStart();
		wxMessageBox(wxString::wxString( _(
			"Your Auto-update servers list is in blank.\n"
			"'Auto-update serverlist at startup' will be disabled.")),
			_("Message"), wxOK | wxICON_INFORMATION, this);
	}

	if (thePrefs::AcceptExternalConnections() && thePrefs::ECPassword().IsEmpty()) {
		thePrefs::EnableExternalConnections( false );

		wxMessageBox( _(
			"You have enabled external connections but have not specified a password.\n"
			"External connections cannot be enabled unless a valid password is specified."));
	}
	
	// save the preferences on ok
	theApp.glob_prefs->Save();

	if (CfgChanged(IDC_FED2KLH) && theApp.amuledlg->GetActiveDialog() != CamuleDlg::DT_SEARCH_WND) {
		theApp.amuledlg->ShowED2KLinksHandler( thePrefs::GetFED2KLH() );
	}

	if (CfgChanged(IDC_LANGUAGE)) {
		restart_needed = true;
		restart_needed_msg += _("- Language changed.\n");
	}

	if (CfgChanged(IDC_TEMPFILES)) {
		restart_needed = true;
		restart_needed_msg += _("- Temp folder changed.\n");
	}

	if (CfgChanged(IDC_INCFILES) || CfgChanged(IDC_TEMPFILES) || m_ShareSelector->HasChanged ) {
		theApp.sharedfiles->Reload();
	}

	if (CfgChanged(IDC_OSDIR) || CfgChanged(IDC_ONLINESIG)) {
		wxTextCtrl* widget = CastChild( IDC_OSDIR, wxTextCtrl );

		// Build the filenames for the two OS files
		theApp.SetOSFiles( widget->GetValue() );
	}

	if (CfgChanged(IDC_IPFCLIENTS) or CfgChanged(IDC_IPFSERVERS) or CfgChanged(ID_IPFILTERLEVEL)) {
		if (thePrefs::IsFilteringClients()) {
			theApp.clientlist->FilterQueues();
		}
		if (thePrefs::IsFilteringServers()) {
			theApp.serverlist->FilterServers();
		}
	}

	if (thePrefs::GetShowRatesOnTitle()) {
		// This avoids a 5 seconds delay to show the title
		theApp.amuledlg->SetTitle(theApp.m_FrameTitle + wxT(" -- ") + _("Up: 0.0 | Down: 0.0"));
	} else {
		// This resets the title
		theApp.amuledlg->SetTitle(theApp.m_FrameTitle);
	}

	if (CfgChanged(IDC_EXTCATINFO)) {
		theApp.amuledlg->m_transferwnd->UpdateCatTabTitles();
	}

	// Changes related to the statistics-dlg
	if (CfgChanged(IDC_SLIDER)) {
		theApp.amuledlg->m_statisticswnd->SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
		theApp.amuledlg->m_kademliawnd->SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
	}

	if ( CfgChanged(IDC_SLIDER3) ) {
		theApp.amuledlg->m_statisticswnd->ResetAveragingTime();
	}

	if (CfgChanged(IDC_DOWNLOAD_CAP)) {
		theApp.amuledlg->m_statisticswnd->SetARange( true, thePrefs::GetMaxGraphDownloadRate() );
	}

	if (CfgChanged(IDC_UPLOAD_CAP)) {
		theApp.amuledlg->m_statisticswnd->SetARange( false, thePrefs::GetMaxGraphUploadRate() );
	}

	if (CfgChanged(IDC_SKINFILE) || CfgChanged(IDC_USESKIN)) {
		theApp.amuledlg->Create_Toolbar(thePrefs::GetSkinFile(), thePrefs::VerticalToolbar());
	}

	if (!thePrefs::GetNetworkED2K() && theApp.IsConnectedED2K()) {
		theApp.DisconnectED2K();
	}
	
	if (!thePrefs::GetNetworkKademlia() && theApp.IsConnectedKad()) {
		theApp.StopKad();
	}	

	if (!thePrefs::GetNetworkED2K() && !thePrefs::GetNetworkKademlia()) {
		wxMessageBox(wxString::wxString(
			_("Both ED2K and Kad network are disabled.\nYou won't be able to connect until you enable at least one of them.")));
	}	
	
	if (thePrefs::GetNetworkKademlia() && thePrefs::IsUDPDisabled()) {
		wxMessageBox(_("Kad will not start if your UDP port is disabled.\nEnable UDP port or disable Kad."),
			 _("Message"), wxOK | wxICON_INFORMATION, this);
	}
	
	if (restart_needed) {
		wxMessageBox(restart_needed_msg + _("\nYou MUST restart aMule now.\nIf you do not restart now, don't complain if anything bad happens.\n"), _("WARNING"),wxICON_EXCLAMATION,this);
	}
	
	Show(false);
}


void PrefsUnifiedDlg::OnClose(wxCloseEvent& event)
{
	Show(false);
	
	// Try to keep the window alive when possible
	if (event.CanVeto()) {
		event.Veto();
	} else {
		if (theApp.amuledlg) {
			theApp.amuledlg->m_prefsDialog = NULL;
		}
	
		// Un-Connect the Cfgs
		thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
		for (; it != thePrefs::s_CfgList.end(); ++it) {
			// Checking for failures
			it->second->ConnectToWidget( 0 );
		}
		
		Destroy();
	}
}


void PrefsUnifiedDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	Show(false);
}


void PrefsUnifiedDlg::OnCheckBoxChange(wxCommandEvent& event)
{
	bool	value = event.IsChecked();
	int	id = event.GetId();	

	// Check if this checkbox is one of the User Events checkboxes
	if (id >= USEREVENTS_FIRST_ID &&
	    id < USEREVENTS_FIRST_ID +
	    	(int)CUserEvents::GetCount() * USEREVENTS_IDS_PER_EVENT) {
		// The corresponding text control always has
		// an ID one greater than the checkbox
		FindWindow(id + 1)->Enable(value);
		return;
	}

	switch ( id ) {
		case IDC_UDPDISABLE:
			// UDP is disable rather than enable, so we flip the value
			FindWindow( IDC_UDPPORT )->Enable(!value);
			break;
			
		case IDC_CHECKDISKSPACE:
			FindWindow( IDC_MINDISKSPACE )->Enable(value);
			break;	
		
		case IDC_USESKIN:
			FindWindow( IDC_SKINFILE )->Enable(value);;
			break;

		case IDC_ONLINESIG:
			FindWindow( IDC_OSDIR )->Enable(value);;
			FindWindow(IDC_OSUPDATE)->Enable(value);
			break;

		case IDC_REMOVEDEAD:
			FindWindow( IDC_SERVERRETRIES )->Enable(value);;
			break;

		case IDC_ENABLE_AUTO_HQRS:
			FindWindow( IDC_HQR_VALUE )->Enable(value);;
			break;

		case IDC_AUTOSERVER:
			if (	IsEmptyFile(theApp.ConfigDir + wxT("addresses.dat")) && 
				CastChild(event.GetId(), wxCheckBox)->IsChecked() ) {
				wxMessageBox(wxString::wxString( _(
					"Your Auto-update servers list is in blank.\n"
					"Please fill in at least one URL to point to a valid server.met file.\n"
					"Click on the button \"List\" by this checkbox to enter an URL.")),
					_("Message"), wxOK | wxICON_INFORMATION);
				CastChild(event.GetId(), wxCheckBox)->SetValue(false);
			}
			break;

		case IDC_MSGFILTER:
			// Toogle All filter options
			FindWindow(IDC_MSGFILTER_ALL)->Enable(value);
			FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(value);
			FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(value);
			FindWindow(IDC_MSGFILTER_WORD)->Enable(value);		
			if (value) {
				FindWindow(IDC_MSGWORD)->Enable(
					CastChild(IDC_MSGFILTER_WORD, wxCheckBox)->IsChecked());
			} else {
				FindWindow(IDC_MSGWORD)->Enable(false);
			}
			break;
		
		case IDC_MSGFILTER_ALL:
			// Toogle filtering by data.
			FindWindow(IDC_MSGFILTER_NONSECURE)->Enable(!value);
			FindWindow(IDC_MSGFILTER_NONFRIENDS)->Enable(!value);
			FindWindow(IDC_MSGFILTER_WORD)->Enable(!value);		
			if (!value) {
				FindWindow(IDC_MSGWORD)->Enable(
					CastChild(IDC_MSGFILTER_WORD, wxCheckBox)->IsChecked());
			} else {
				FindWindow(IDC_MSGWORD)->Enable(false);
			}
			break;
		
		case IDC_MSGFILTER_WORD:
			// Toogle filter word list.
			FindWindow(IDC_MSGWORD)->Enable(value);
			break;

		case ID_PROXY_ENABLE_PROXY:
			FindWindow(ID_PROXY_TYPE)->Enable(value);
			FindWindow(ID_PROXY_NAME)->Enable(value);
			FindWindow(ID_PROXY_PORT)->Enable(value);
			break;
			
		case ID_PROXY_ENABLE_PASSWORD:
			FindWindow(ID_PROXY_USER)->Enable(value);
			FindWindow(ID_PROXY_PASSWORD)->Enable(value);
			break;
			
		case IDC_STARTNEXTFILE:
			FindWindow(IDC_STARTNEXTFILE_SAME)->Enable(value);
			break;
		
		case IDC_ENABLETRAYICON:
			FindWindow(IDC_MINTRAY)->Enable(value);
			if (value) {
				theApp.amuledlg->CreateSystray();
			} else {
				theApp.amuledlg->RemoveSystray();
			}
			break;
		
		case ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY:
			break;
		case IDC_VERTTOOLBAR:
			theApp.amuledlg->Create_Toolbar(thePrefs::GetSkinFile(), value);
			// Update the first tool (conn button)
			theApp.amuledlg->ShowConnectionState();
			break;
	}
}


void PrefsUnifiedDlg::OnButtonColorChange(wxCommandEvent& WXUNUSED(event))
{
	int index = m_choiceColor->GetSelection();
	wxColour col = WxColourFromCr( thePrefs::s_colors[index] );
	col = wxGetColourFromUser( this, col );
	if ( col.Ok() ) {
		m_buttonColor->SetBackgroundColour( col );
		thePrefs::s_colors[index] = CrFromWxColour(col);
	}
}


void PrefsUnifiedDlg::OnColorCategorySelected(wxCommandEvent& WXUNUSED(evt))
{
	m_buttonColor->SetBackgroundColour(
		WxColourFromCr( thePrefs::s_colors[ m_choiceColor->GetSelection() ] ) );
}


void PrefsUnifiedDlg::OnBrowserChange( wxCommandEvent& evt )
{
	wxTextCtrl* textctrl = CastChild( IDC_BROWSERSELF, wxTextCtrl );
	wxButton* btn = CastChild( IDC_SELBROWSER, wxButton );
	bool enable = evt.GetSelection() == CastChild( IDC_BROWSER, wxChoice )->GetCount() - 1;

	if (textctrl) {
		textctrl->Enable( enable );
	}
	if (btn) {
		btn->Enable( enable );
	}
#ifndef __WXMSW__
	FindWindow( IDC_BROWSERTABS )->Enable( !enable );
#endif
}


void PrefsUnifiedDlg::OnButtonDir(wxCommandEvent& event)
{
	wxString type;

	int id = 0;
	switch ( event.GetId() ) {
	case IDC_SELTEMPDIR:
		id = IDC_TEMPFILES;
		type = _("Temporary files");
		break;

	case IDC_SELINCDIR:
		id = IDC_INCFILES;
		type = _("Incoming files");
		break;

	case IDC_SELOSDIR:
		id = IDC_OSDIR;
		type = _("Online Signatures");
		break;

	default:
		wxASSERT( false );
		return;
	}

	type = CFormat(_("Choose a folder for %s")) % type;

	wxTextCtrl* widget = CastChild( id, wxTextCtrl );
	wxString dir = widget->GetValue();

	wxString str = wxDirSelector( type, dir, 0, wxDefaultPosition, this );

	if ( !str.IsEmpty() ) {
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseWav(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector( 
		_("Browse wav"), wxEmptyString, wxEmptyString,
		wxT("*.wav"), _("File wav (*.wav)|*.wav||"), 0, this );
	
	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( IDC_EDIT_TBN_WAVFILE, wxTextCtrl );

		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseSkin(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector(
		_("Browse skin file"), wxEmptyString, wxEmptyString, wxT("*"),  wxT("*.*"), 0, this );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( IDC_SKINFILE, wxTextCtrl );

		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseApplication(wxCommandEvent& event)
{
	wxString title;
	int id = 0;
	switch ( event.GetId() ) {
		case IDC_BROWSEV:
			id = IDC_VIDEOPLAYER;
			title = _("Browse for videoplayer");
			break;
		case IDC_SELBROWSER:
			id = IDC_BROWSERSELF;
			title = _("Select browser");
			break;
		default:
			wxASSERT( false );
			return;
	}
	wxString wildcard = CFormat(_("Executable%s"))
#ifdef __WINDOWS__
		% wxT(" (*.exe)|*.exe");
#else
		% wxT("|*");
#endif
	
	wxString str = wxFileSelector( title, wxEmptyString, wxEmptyString,
		wxEmptyString, wildcard, 0, this );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( id, wxTextCtrl );
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonEditAddr(wxCommandEvent& WXUNUSED(evt))
{
	wxString fullpath( theApp.ConfigDir + wxT("addresses.dat") );

	EditServerListDlg* test = new EditServerListDlg(this, _("Edit Serverlist"),
		_("Add here URL's to download server.met files.\nOnly one url on each line."),
		fullpath );

	test->ShowModal();
	delete test;
}


void PrefsUnifiedDlg::OnButtonIPFilterReload(wxCommandEvent& WXUNUSED(event))
{
	theApp.ipfilter->Reload();
}


void PrefsUnifiedDlg::OnButtonIPFilterUpdate(wxCommandEvent& WXUNUSED(event))
{
	theApp.ipfilter->Update( CastChild( IDC_IPFILTERURL, wxTextCtrl )->GetValue() );
}


void PrefsUnifiedDlg::OnPrefsPageChange(wxListEvent& event)
{
	prefs_sizer->Remove( m_CurrentPanel );
	m_CurrentPanel->Show( false );

	m_CurrentPanel = pages[ event.GetIndex() ].m_widget;

	prefs_sizer->Add( m_CurrentPanel, 0, wxGROW|wxEXPAND );
	m_CurrentPanel->Show( true );

	Layout();

	event.Skip();
}


void PrefsUnifiedDlg::OnToolTipDelayChange(wxSpinEvent& event)
{
	wxToolTip::SetDelay( event.GetPosition() * 1000 );
}


void PrefsUnifiedDlg::OnInitDialog( wxInitDialogEvent& WXUNUSED(evt) )
{
// This function exists solely to avoid automatic transfer-to-widget calls
}


void PrefsUnifiedDlg::OnScrollBarChange( wxScrollEvent& event )
{
	int id = 0;
	wxString label;

	switch ( event.GetId() ) {
	case IDC_SLIDER:
		id = IDC_SLIDERINFO;
		label = wxString::Format( _("Update delay: %d secs"), event.GetPosition() );
		theApp.amuledlg->m_statisticswnd->SetUpdatePeriod(event.GetPosition());
		theApp.amuledlg->m_kademliawnd->SetUpdatePeriod(event.GetPosition());
		break;

	case IDC_SLIDER3:
		id = IDC_SLIDERINFO3;
		label = wxString::Format( _("Time for average graph: %d mins"), event.GetPosition() );
		theApp.m_statistics->SetAverageMinutes(event.GetPosition());
		break;

	case IDC_SLIDER4:
		id = IDC_SLIDERINFO4;
		label = wxString::Format( _("Connections Graph Scale: %d"), event.GetPosition() );
		theApp.amuledlg->m_statisticswnd->GetConnScope()->SetRanges(0,event.GetPosition());
		break;

	case IDC_SLIDER2:
		id = IDC_SLIDERINFO2;
		label = wxString::Format( _("Update delay : %d secs"), event.GetPosition() );
		break;

	case IDC_FILEBUFFERSIZE:
		id = IDC_FILEBUFFERSIZE_STATIC;
		label = wxString::Format( _("File Buffer Size: %d bytes"), event.GetPosition() * 15000 );
		break;

	case IDC_QUEUESIZE:
		id = IDC_QUEUESIZE_STATIC;
		label = wxString::Format( _("Upload Queue Size: %d clients"), event.GetPosition() * 100 );
		break;

	case IDC_SERVERKEEPALIVE:
		id = IDC_SERVERKEEPALIVE_LABEL;

		if ( event.GetPosition() ) {
			label = wxString::Format( _("Server connection refresh interval: %d minutes"),
				event.GetPosition() );
		} else {
			label = wxString::Format( _("Server connection refresh interval: Disabled") );
		}
		break;

	default:
		return;
	}

	wxStaticText* widget = CastChild( id, wxStaticText );

	if (widget) {
		widget->SetLabel( label );
		widget->GetParent()->Layout();
	}
}


void PrefsUnifiedDlg::OnRateLimitChanged( wxSpinEvent& event )
{
	// Here we do immediate sainity checking of the up/down ratio,
	// so that the user can see if his choice is illegal

	// We only do checks if the rate is limited
	if ( event.GetPosition() != (int)UNLIMITED ) {
		wxSpinCtrl* dlrate = CastChild( IDC_MAXDOWN, wxSpinCtrl );
	
		if ( event.GetPosition() < 4 ) {
			if (	( event.GetPosition() * 3 < dlrate->GetValue() ) ||
				( dlrate->GetValue() == (int)UNLIMITED ) ) {
				dlrate->SetValue( event.GetPosition() * 3 );
			}
		} else if ( event.GetPosition() < 10  ) {
			if (	( event.GetPosition() * 4 < dlrate->GetValue() ) ||
				( dlrate->GetValue() == (int)UNLIMITED ) ) {
				dlrate->SetValue( event.GetPosition() * 4 );
			}
		}
	}
}


void PrefsUnifiedDlg::OnTCPClientPortChange(wxSpinEvent& WXUNUSED(event))
{
	int port = CastChild(IDC_PORT, wxSpinCtrl)->GetValue();
	wxString txt;
	txt << wxT("UDP port for extended server requests (TCP+3):") << port + 3;
	CastChild(ID_TEXT_CLIENT_UDP_PORT, wxStaticText)->SetLabel(txt);
}

void PrefsUnifiedDlg::OnUserEventSelected(wxListEvent& event)
{
	for (unsigned int i = 0; i < CUserEvents::GetCount(); ++i) {
		CastChild(USEREVENTS_FIRST_ID + i * USEREVENTS_IDS_PER_EVENT, wxPanel)->Hide();
	}

	CastChild(event.GetData(), wxPanel)->Show();

	Layout();

	event.Skip();
}

void PrefsUnifiedDlg::CreateEventPanels(const int idx, const wxString& vars, wxWindow* parent)
{
	wxPanel *item0 = new wxPanel(parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT);

	wxBoxSizer *item1 = new wxBoxSizer(wxVERTICAL);
	wxStaticBox *item8 = new wxStaticBox( item0, -1, CFormat(_("Execute command on `%s' event")) % wxGetTranslation(CUserEvents::GetDisplayName(static_cast<enum CUserEvents::EventType>(idx))) );
	wxStaticBoxSizer *item7 = new wxStaticBoxSizer( item8, wxVERTICAL );

	wxCheckBox *item9 = new wxCheckBox( item0, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 1, _("Enable command execution on core"), wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer *item10 = new wxFlexGridSizer( 3, 0, 0 );
	item10->AddGrowableCol( 2 );

	item10->Add( 20, 20, 0, wxALIGN_CENTER|wxALL, 0 );

	wxStaticText *item11 = new wxStaticText( item0, -1, _("Core command:"), wxDefaultPosition, wxDefaultSize, 0 );
	item10->Add( item11, 0, wxALIGN_CENTER|wxALL, 5 );

	wxTextCtrl *item12 = new wxTextCtrl( item0, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 2, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	item12->Enable(CUserEvents::IsCoreCommandEnabled(static_cast<enum CUserEvents::EventType>(idx)));
	item10->Add( item12, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	item7->Add( item10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

	wxCheckBox *item14 = new wxCheckBox( item0, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 3, _("Enable command execution on GUI"), wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item14, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer *item15 = new wxFlexGridSizer( 3, 0, 0 );
	item15->AddGrowableCol( 2 );

	item15->Add( 20, 20, 0, wxALIGN_CENTER|wxALL, 0 );

	wxStaticText *item16 = new wxStaticText( item0, -1, _("GUI command:"), wxDefaultPosition, wxDefaultSize, 0 );
	item15->Add( item16, 0, wxALIGN_CENTER|wxALL, 5 );

	wxTextCtrl *item17 = new wxTextCtrl( item0, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 4, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	item17->Enable(CUserEvents::IsGUICommandEnabled(static_cast<enum CUserEvents::EventType>(idx)));
	item15->Add( item17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	item7->Add( item15, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

	wxStaticText *item13 = new wxStaticText( item0, -1, _("The following variables will be replaced:") + vars, wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item13, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	item1->Add( item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	item0->SetSizer(item1);
	item1->SetSizeHints(item0);

	IDC_PREFS_EVENTS_PAGE->Add(item0, 0, wxGROW | wxEXPAND);

	item0->Hide();
}
// File_checked_for_headers

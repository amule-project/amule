//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <common/Constants.h>

#include <wx/colordlg.h>
#include <wx/tooltip.h>

#include "amule.h"				// Needed for theApp
#include "amuleDlg.h"
#include "MuleColour.h"
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
#include "Statistics.h"
#include "UserEvents.h"
#include "PlatformSpecific.h"

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
	EVT_CHECKBOX(IDC_UDPENABLE,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_CHECKDISKSPACE,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ONLINESIG,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_REMOVEDEAD,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_AUTOSERVER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_AUTOIPFILTER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER_ALL,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_MSGFILTER_WORD,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_FILTERCOMMENTS,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_STARTNEXTFILE,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLETRAYICON,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_VERTTOOLBAR,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_SUPPORT_PO,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLE_PO_OUTGOING,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENFORCE_PO_INCOMING,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_SHOWRATEONTITLE,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_NETWORKED2K,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_NETWORKKAD,		PrefsUnifiedDlg::OnCheckBoxChange)

	EVT_BUTTON(ID_PREFS_OK_TOP,		PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_PREFS_CANCEL_TOP,		PrefsUnifiedDlg::OnCancel)

	// Browse buttons
//	EVT_BUTTON(IDC_SELSKIN,		PrefsUnifiedDlg::OnButtonDir)
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

	EVT_CHOICE(IDC_LANGUAGE,		PrefsUnifiedDlg::OnLanguageChoice)

	EVT_CLOSE(PrefsUnifiedDlg::OnClose)

END_EVENT_TABLE()


/**
 * Creates an command-event for the given checkbox.
 *
 * This can be used enforce logical constraints by passing by
 * sending a check-box event for each checkbox, when transfering
 * to the UI. However, it should also be used for checkboxes that 
 * have no side-effects other than enabling/disabling other
 * widgets in the preferences dialogs.
 */
void SendCheckBoxEvent(wxWindow* parent, int id)
{
	wxCheckBox* widget = CastByID(id, parent, wxCheckBox);
	wxCHECK_RET(widget, wxT("Invalid widget in CreateEvent"));

	wxCommandEvent evt(wxEVT_COMMAND_CHECKBOX_CLICKED, id);
	evt.SetInt(widget->IsChecked() ? 1 : 0);

	parent->GetEventHandler()->ProcessEvent(evt);
}



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
};


PrefsPage pages[] =
{
	{ wxTRANSLATE("General"),			PreferencesGeneralTab,		13 },
	{ wxTRANSLATE("Connection"),		PreferencesConnectionTab,	14 },
	{ wxTRANSLATE("Directories"),		PreferencesDirectoriesTab,	17 },
	{ wxTRANSLATE("Servers"),			PreferencesServerTab,		15 },
	{ wxTRANSLATE("Files"),				PreferencesFilesTab,		16 },
	{ wxTRANSLATE("Security"),			PreferencesSecurityTab,		22 },
	{ wxTRANSLATE("Interface"),			PreferencesGuiTweaksTab,	19 },
	{ wxTRANSLATE("Statistics"),		PreferencesStatisticsTab,	10 },
	{ wxTRANSLATE("Proxy"),				PreferencesProxyTab,		24 },
	{ wxTRANSLATE("Filters"),			PreferencesFilteringTab,	23 },
	{ wxTRANSLATE("Remote Controls"),	PreferencesRemoteControlsTab,	11 },
	{ wxTRANSLATE("Online Signature"),	PreferencesOnlineSigTab,	21 },
	{ wxTRANSLATE("Advanced"),			PreferencesaMuleTweaksTab,	12 },
	{ wxTRANSLATE("Events"),			PreferencesEventsTab,		5 }
#ifdef __DEBUG__
	,{ wxTRANSLATE("Debugging"),		PreferencesDebug,			25 }
#endif
};


PrefsUnifiedDlg::PrefsUnifiedDlg(wxWindow *parent)
:
wxDialog(parent, -1, _("Preferences"),
	wxDefaultPosition, wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
{
	preferencesDlgTop(this, false);
	
	m_PrefsIcons = CastChild(ID_PREFSLISTCTRL, wxListCtrl);
	wxImageList *icon_list = new wxImageList(16, 16);
	m_PrefsIcons->AssignImageList(icon_list, wxIMAGE_LIST_SMALL);

	// Add the single column used
	m_PrefsIcons->InsertColumn(
		0, wxEmptyString, wxLIST_FORMAT_LEFT,
		m_PrefsIcons->GetSize().GetWidth()-5);

	// Temp variables for finding the smallest height and width needed
	int width = 0;
	int height = 0;

	// Add each page to the page-list
	for (unsigned int i = 0; i < itemsof(pages); ++i) {
		// Add the icon and label associated with the page
		icon_list->Add(amuleSpecial(pages[i].m_imageidx));
		m_PrefsIcons->InsertItem(i, wxGetTranslation(pages[i].m_title), i);
	}
	
	// Set list-width so that there aren't any scrollers
	m_PrefsIcons->SetColumnWidth(0, wxLIST_AUTOSIZE);
	m_PrefsIcons->SetMinSize(wxSize(m_PrefsIcons->GetColumnWidth(0) + 10, -1));
	m_PrefsIcons->SetMaxSize(wxSize(m_PrefsIcons->GetColumnWidth(0) + 10, -1));

	// Now add the pages and calculate the minimum size
	wxPanel * DefaultWidget = NULL;
	for (unsigned int i = 0; i < itemsof(pages); ++i) {
		// Create a container widget and the contents of the page
		wxPanel * Widget = new wxPanel(this, -1);
		// Widget is stored as user data in the list control
		m_PrefsIcons->SetItemPtrData(i, (wxUIntPtr) Widget);
		pages[i].m_function(Widget, true, true);
		if (i == 0) {
			DefaultWidget = Widget;
		}

		// Add it to the sizer
		prefs_sizer->Add(Widget, 0, wxGROW|wxEXPAND);

		if (pages[i].m_function == PreferencesGeneralTab) {
			// This must be done now or pages won't Fit();
			#ifdef __WXMSW__ 
				CastChild(IDC_BROWSERTABS, wxCheckBox)->Enable(false);
			#endif /* __WXMSW__ */
			CastChild(IDC_PREVIEW_NOTE, wxStaticText)->SetLabel(_("The following variables will be substituted:\n    %PARTFILE - full path to the file\n    %PARTNAME - file name only"));
		} else if (pages[i].m_function == PreferencesGuiTweaksTab) {
			#ifndef ENABLE_IP2COUNTRY
				CastChild(IDC_SHOW_COUNTRY_FLAGS, wxCheckBox)->Enable(false);
				thePrefs::SetGeoIPEnabled(false);
			#endif
		} else if (pages[i].m_function == PreferencesEventsTab) {

#define USEREVENTS_REPLACE_VAR(VAR, DESC, CODE)	+ wxString(wxT("\n  %") VAR wxT(" - ")) + wxGetTranslation(DESC)
#define USEREVENTS_EVENT(ID, NAME, VARS) case CUserEvents::ID: CreateEventPanels(idx, wxEmptyString VARS, Widget); break;

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
						/* This macro expands to handle all user event types. Here is an example:
						   case CUserEvents::NewChatSession: {
						       CreateEventPanels(idx, wxString(wxT("\n %SENDER - ")) + wxTRANSLATE("Message sender."), Widget);
						       break;
						   } */							
					}
				}
			}
			list->SetColumnWidth(0, wxLIST_AUTOSIZE);
		}
		else if (pages[i].m_function == PreferencesServerTab) {
			m_IndexServerTab = i;
			m_ServerWidget = Widget;
		}
		else if (pages[i].m_function == PreferencesaMuleTweaksTab) {
			wxStaticText *txt = CastChild(IDC_AMULE_TWEAKS_WARNING, wxStaticText);
			// Do not wrap this line, Windows _() can't handle wrapped strings
			txt->SetLabel(_("Do not change these setting unless you know\nwhat you are doing, otherwise you can easily\nmake things worse for yourself.\n\naMule will run fine without adjusting any of\nthese settings."));
			#if defined CLIENT_GUI || !PLATFORMSPECIFIC_CAN_PREVENT_SLEEP_MODE
				CastChild(IDC_PREVENT_SLEEP, wxCheckBox)->Enable(false);
				thePrefs::SetPreventSleepWhileDownloading(false);
			#endif
		}
#ifdef __DEBUG__
		else if (pages[i].m_function == PreferencesDebug) {
			int count = theLogger.GetDebugCategoryCount();
			wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

			for ( int j = 0; j < count; j++ ) {
				list->Append( theLogger.GetDebugCategory( j ).GetName() );
			}
		}
#endif

		// Align and resize the page
		Fit();
		Layout();

		// Find the greatest sizes
		wxSize size = prefs_sizer->GetSize();
		if (size.GetWidth() > width) {
			width = size.GetWidth();
		}

		if (size.GetHeight() > height) {
			height = size.GetHeight();
		}

		// Hide it for now
		prefs_sizer->Detach(Widget);
		Widget->Show(false);
	}
	
	// Default to the General tab
	m_CurrentPanel = DefaultWidget;
	prefs_sizer->Add(DefaultWidget, 0, wxGROW|wxEXPAND);
	m_CurrentPanel->Show( true );

	// Select the first item
	m_PrefsIcons->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);

	// We now have the needed minimum height and width
	prefs_sizer->SetMinSize(width, height);

	// Don't show server prefs if ED2K is disabled
	m_ServerTabVisible = true;
	EnableServerTab(thePrefs::GetNetworkED2K());

	// Store some often used pointers
	m_ShareSelector = CastChild(IDC_SHARESELECTOR, CDirectoryTreeCtrl);
	m_buttonColor   = CastChild(IDC_COLOR_BUTTON, wxButton);
	m_choiceColor   = CastChild(IDC_COLORSELECTOR, wxChoice);

	// Connect the Cfgs with their widgets
	thePrefs::CFGMap::iterator it = thePrefs::s_CfgList.begin();
	for ( ; it != thePrefs::s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->ConnectToWidget(it->first, this) ) {
			AddLogLineNS(CFormat(_("Failed to connect Cfg to widget with the ID %d and key %s"))
				% it->first % it->second->GetKey());
		}
	}
	Fit();

	// It must not be resized to something smaller than what it currently is
	wxSize size = GetClientSize();
	SetSizeHints(size.GetWidth(), size.GetHeight());

	// Position the dialog.
	Center();
}


void PrefsUnifiedDlg::EnableServerTab(bool enable)
{
	if (enable && !m_ServerTabVisible) {
	// turn server widget on
		m_PrefsIcons->InsertItem(m_IndexServerTab, wxGetTranslation(pages[m_IndexServerTab].m_title), m_IndexServerTab);
		m_PrefsIcons->SetItemPtrData(m_IndexServerTab, (wxUIntPtr) m_ServerWidget);
		m_ServerTabVisible = true;
	} else if (!enable && m_ServerTabVisible) {
	// turn server widget off
		m_PrefsIcons->DeleteItem(m_IndexServerTab);
		m_ServerTabVisible = false;
	}
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
			AddLogLineNS(CFormat(_("Failed to transfer data from Cfg to Widget with the ID %d and key %s"))
				% it->first % it->second->GetKey());
		}
	}

	m_ShareSelector->SetSharedDirectories(&theApp->glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		thePrefs::s_colors[i] = CMuleColour(CStatisticsDlg::acrStat[i]).GetULong();
		thePrefs::s_colors_ref[i] = CMuleColour(CStatisticsDlg::acrStat[i]).GetULong();
	}
	
	// Connection tab
	wxSpinEvent e;
	OnTCPClientPortChange(e);
	
	// Proxy tab initialization
	FindWindow(ID_PROXY_TYPE)->SetToolTip(_("The type of proxy you are connecting to"));
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
	FindWindow( IDC_MINDISKSPACE )->Enable( thePrefs::IsCheckDiskspaceEnabled() );
	FindWindow( IDC_OSDIR )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_OSUPDATE )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_UDPENABLE )->Enable( !thePrefs::GetNetworkKademlia());
	FindWindow( IDC_UDPPORT )->Enable( thePrefs::s_UDPEnable );
	FindWindow( IDC_SERVERRETRIES )->Enable( thePrefs::DeadServer() );
	FindWindow( IDC_STARTNEXTFILE_SAME )->Enable(thePrefs::StartNextFile());
	FindWindow( IDC_STARTNEXTFILE_ALPHA )->Enable(thePrefs::StartNextFile());

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
	FindWindow(IDC_COMMENTWORD)->Enable(CastChild(IDC_FILTERCOMMENTS, wxCheckBox)->IsChecked());

#ifdef CLIENT_GUI
	// Disable dirpickers unless it's a localhost connection
	if (!theApp->m_connect->IsConnectedToLocalHost()) {
		FindWindow(IDC_SELINCDIR)->Enable(false);	
		FindWindow(IDC_SELTEMPDIR)->Enable(false);	
	}
#endif

	// Protocol obfuscation
	::SendCheckBoxEvent(this, IDC_SUPPORT_PO);
	::SendCheckBoxEvent(this, IDC_ENABLE_PO_OUTGOING);
	::SendCheckBoxEvent(this, IDC_ENFORCE_PO_INCOMING);

	// Show rates on title
	FindWindow(IDC_RATESBEFORETITLE)->Enable(thePrefs::GetShowRatesOnTitle() != 0);
	FindWindow(IDC_RATESAFTERTITLE)->Enable(thePrefs::GetShowRatesOnTitle() != 0);
	CastChild(IDC_SHOWRATEONTITLE, wxCheckBox)->SetValue(thePrefs::GetShowRatesOnTitle() != 0);
	CastChild(IDC_RATESBEFORETITLE, wxRadioButton)->SetValue(thePrefs::GetShowRatesOnTitle() == 2);
	CastChild(IDC_RATESAFTERTITLE, wxRadioButton)->SetValue(thePrefs::GetShowRatesOnTitle() != 2);

	// UPNP
#ifndef ENABLE_UPNP
	FindWindow(IDC_UPNP_ENABLED)->Enable(false);	
	FindWindow(IDC_UPNPTCPPORT)->Enable(false);
	FindWindow(IDC_UPNPTCPPORTTEXT)->Enable(false);
	thePrefs::SetUPnPEnabled(false);
	FindWindow(IDC_UPNP_WEBSERVER_ENABLED)->Enable(false);	
	FindWindow(IDC_WEBUPNPTCPPORT)->Enable(false);
	FindWindow(IDC_WEBUPNPTCPPORTTEXT)->Enable(false);
	thePrefs::SetUPnPWebServerEnabled(false);
	FindWindow(IDC_UPNP_EC_ENABLED)->Enable(false);
	thePrefs::SetUPnPECEnabled(false);
#endif

#ifdef __DEBUG__
	// Set debugging toggles
	int count = theLogger.GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		list->Check( i, theLogger.GetDebugCategory( i ).IsEnabled() );
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
			AddLogLineNS(CFormat(_("Failed to transfer data from Widget to Cfg with the ID %d and key %s"))
				% it->first % it->second->GetKey());
		}
	}

	theApp->glob_prefs->shareddir_list.clear();
	m_ShareSelector->GetSharedDirectories(&theApp->glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		if ( thePrefs::s_colors[i] != thePrefs::s_colors_ref[i] ) {
			CStatisticsDlg::acrStat[i] = thePrefs::s_colors[i];
			theApp->amuledlg->m_statisticswnd->ApplyStatsColor(i);
		}

		theApp->amuledlg->m_kademliawnd->SetGraphColors();
	}

#ifdef __DEBUG__
	// Get debugging toggles
	int count = theLogger.GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		theLogger.SetEnabled( theLogger.GetDebugCategory( i ).GetType(), list->IsChecked( i ) );
	}
#endif

	thePrefs::SetShowRatesOnTitle(CastChild(IDC_SHOWRATEONTITLE, wxCheckBox)->GetValue() ? (CastChild(IDC_RATESBEFORETITLE, wxRadioButton)->GetValue() ? 2 : 1) : 0);

	#ifdef CLIENT_GUI
	// Send preferences to core.
	theApp->glob_prefs->SendToRemote();
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

	if (CfgChanged(IDC_EXT_CONN_TCP_PORT)) {
		restart_needed = true;
		restart_needed_msg += _("- External connect port changed.\n");
	}
	if (CfgChanged(IDC_EXT_CONN_ACCEPT)) {
		restart_needed = true;
		restart_needed_msg += _("- External connect acceptance changed.\n");
	}
	if (CfgChanged(IDC_EXT_CONN_IP)) {
		restart_needed = true;
		restart_needed_msg += _("- External connect interface changed.\n");
	}

	// Force port checking
	thePrefs::SetPort(thePrefs::GetPort());
	
	if ((CPath::GetFileSize(theApp->ConfigDir + wxT("addresses.dat")) == 0) && 
		CastChild(IDC_AUTOSERVER, wxCheckBox)->IsChecked() ) {
		thePrefs::UnsetAutoServerStart();
		wxMessageBox(_("Your Auto-update server list is empty.\n'Auto-update server list at startup' will be disabled."),
			_("Message"), wxOK | wxICON_INFORMATION, this);
	}

	if (thePrefs::AcceptExternalConnections() && thePrefs::ECPassword().IsEmpty()) {
		thePrefs::EnableExternalConnections( false );

		wxMessageBox( _("You have enabled external connections but have not specified a password.\nExternal connections cannot be enabled unless a valid password is specified."));
	}
	
	// save the preferences on ok
	theApp->glob_prefs->Save();

	if (CfgChanged(IDC_FED2KLH) && theApp->amuledlg->GetActiveDialog() != CamuleDlg::DT_SEARCH_WND) {
		theApp->amuledlg->ShowED2KLinksHandler( thePrefs::GetFED2KLH() );
	}

	if (CfgChanged(IDC_LANGUAGE)) {
		restart_needed = true;
		restart_needed_msg += _("- Language changed.\n");
	}

	if (CfgChanged(IDC_TEMPFILES)) {
		restart_needed = true;
		restart_needed_msg += _("- Temp folder changed.\n");
	}

	if (CfgChanged(IDC_NETWORKED2K) && thePrefs::GetNetworkED2K()) {
		restart_needed = true;
		restart_needed_msg += _("- ED2K network enabled.\n");
	}

	if (CfgChanged(IDC_INCFILES) || CfgChanged(IDC_TEMPFILES) || m_ShareSelector->HasChanged ) {
		theApp->sharedfiles->Reload();
	}

	if (CfgChanged(IDC_OSDIR) || CfgChanged(IDC_ONLINESIG)) {
		wxTextCtrl* widget = CastChild( IDC_OSDIR, wxTextCtrl );

		// Build the filenames for the two OS files
		theApp->SetOSFiles( widget->GetValue() );
	}

	if (CfgChanged(IDC_IPFCLIENTS) && thePrefs::IsFilteringClients()) {
		theApp->clientlist->FilterQueues();
	}

	if (CfgChanged(IDC_IPFSERVERS) && thePrefs::IsFilteringServers()) {
		theApp->serverlist->FilterServers();
	}

	if (CfgChanged(ID_IPFILTERLEVEL)) {
		theApp->ipfilter->Reload();
	}
	
	if (thePrefs::GetShowRatesOnTitle()) {
		// This avoids a 5 seconds delay to show the title
		theApp->amuledlg->ShowTransferRate();
	} else {
		// This resets the title
		theApp->amuledlg->SetTitle(theApp->m_FrameTitle);
	}

	if (CfgChanged(IDC_EXTCATINFO)) {
		theApp->amuledlg->m_transferwnd->UpdateCatTabTitles();
	}

	// Changes related to the statistics-dlg
	if (CfgChanged(IDC_SLIDER)) {
		theApp->amuledlg->m_statisticswnd->SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
		theApp->amuledlg->m_kademliawnd->SetUpdatePeriod(thePrefs::GetTrafficOMeterInterval());
	}

	if ( CfgChanged(IDC_SLIDER3) ) {
		theApp->amuledlg->m_statisticswnd->ResetAveragingTime();
	}

	if (CfgChanged(IDC_DOWNLOAD_CAP)) {
		theApp->amuledlg->m_statisticswnd->SetARange( true, thePrefs::GetMaxGraphDownloadRate() );
	}

	if (CfgChanged(IDC_UPLOAD_CAP)) {
		theApp->amuledlg->m_statisticswnd->SetARange( false, thePrefs::GetMaxGraphUploadRate() );
	}

	if (CfgChanged(IDC_SKIN)) {
		theApp->amuledlg->Create_Toolbar(thePrefs::VerticalToolbar());
	}

	if (!thePrefs::GetNetworkED2K() && theApp->IsConnectedED2K()) {
		theApp->DisconnectED2K();
	}
	
	if (!thePrefs::GetNetworkKademlia() && theApp->IsConnectedKad()) {
		theApp->StopKad();
	}	

	if (!thePrefs::GetNetworkED2K() && !thePrefs::GetNetworkKademlia()) {
		wxMessageBox(
			_("Both eD2k and Kad network are disabled.\nYou won't be able to connect until you enable at least one of them."));
	}	
	
	if (thePrefs::GetNetworkKademlia() && thePrefs::IsUDPDisabled()) {
		wxMessageBox(_("Kad will not start if your UDP port is disabled.\nEnable UDP port or disable Kad."),
			 _("Message"), wxOK | wxICON_INFORMATION, this);
	}
	
	if (CfgChanged(IDC_NETWORKKAD) || CfgChanged(IDC_NETWORKED2K)) {
		theApp->amuledlg->DoNetworkRearrange();
	}
	
	if (CfgChanged(IDC_SHOW_COUNTRY_FLAGS)) {
		theApp->amuledlg->EnableIP2Country();
	}
	
	if (restart_needed) {
		wxMessageBox(restart_needed_msg + _("\nYou MUST restart aMule now.\nIf you do not restart now, don't complain if anything bad happens.\n"), 
			_("WARNING"), wxOK | wxICON_EXCLAMATION, this);
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
		if (theApp->amuledlg) {
			theApp->amuledlg->m_prefsDialog = NULL;
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
	// restore state of server tab if necessary
	EnableServerTab(thePrefs::GetNetworkED2K());
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
		case IDC_UDPENABLE:
			FindWindow( IDC_UDPPORT )->Enable(value);
			break;

		case IDC_NETWORKKAD: {
			wxCheckBox * udpPort = (wxCheckBox *) FindWindow(IDC_UDPENABLE);
			if (value) {
				// Kad enabled: disable check box, turn UDP on, enable port spin control
				udpPort->Enable(false);
				udpPort->SetValue(true);
				FindWindow(IDC_UDPPORT)->Enable(true);
			} else {
				// Kad disabled: enable check box
				udpPort->Enable(true);
			}
			break;
		}

		case IDC_CHECKDISKSPACE:
			FindWindow( IDC_MINDISKSPACE )->Enable(value);
			break;	

		case IDC_ONLINESIG:
			FindWindow( IDC_OSDIR )->Enable(value);;
			FindWindow(IDC_OSUPDATE)->Enable(value);
			break;

		case IDC_REMOVEDEAD:
			FindWindow( IDC_SERVERRETRIES )->Enable(value);;
			break;

		case IDC_AUTOSERVER:
			if ((CPath::GetFileSize(theApp->ConfigDir + wxT("addresses.dat")) == 0) && 
				CastChild(event.GetId(), wxCheckBox)->IsChecked() ) {
				wxMessageBox(_("Your Auto-update servers list is in blank.\nPlease fill in at least one URL to point to a valid server.met file.\nClick on the button \"List\" by this checkbox to enter an URL."),
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

		case IDC_FILTERCOMMENTS:
			FindWindow(IDC_COMMENTWORD)->Enable(value);
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
			FindWindow(IDC_STARTNEXTFILE_ALPHA)->Enable(value);
			break;

		case IDC_ENABLETRAYICON:
			FindWindow(IDC_MINTRAY)->Enable(value);
			if (value) {
				theApp->amuledlg->CreateSystray();
			} else {
				theApp->amuledlg->RemoveSystray();
			}
			thePrefs::SetUseTrayIcon(value);
			break;

		case ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY:
			break;

		case IDC_VERTTOOLBAR:
			theApp->amuledlg->Create_Toolbar(value);
			// Update the first tool (conn button)
			theApp->amuledlg->ShowConnectionState();
			theApp->amuledlg->Layout();
			break;

		case IDC_ENFORCE_PO_INCOMING:
			FindWindow(IDC_ENABLE_PO_OUTGOING)->Enable(!value);
			break;

		case IDC_ENABLE_PO_OUTGOING:
			FindWindow(IDC_SUPPORT_PO)->Enable(!value);
			FindWindow(IDC_ENFORCE_PO_INCOMING)->Enable(value);
			break;

		case IDC_SUPPORT_PO:
			FindWindow(IDC_ENABLE_PO_OUTGOING)->Enable(value);
			break;

		case IDC_SHOWRATEONTITLE:
			FindWindow(IDC_RATESBEFORETITLE)->Enable(value);
			FindWindow(IDC_RATESAFTERTITLE)->Enable(value);
			break;

		case IDC_NETWORKED2K: {
			EnableServerTab(value);
			wxSpinEvent e;
			OnTCPClientPortChange(e);
			break;
		}

		default:
			break;
	}
}


void PrefsUnifiedDlg::OnButtonColorChange(wxCommandEvent& WXUNUSED(event))
{
	int index = m_choiceColor->GetSelection();
	wxColour col = wxGetColourFromUser( this, CMuleColour(thePrefs::s_colors[index]) );
	if ( col.Ok() ) {
		m_buttonColor->SetBackgroundColour( col );
		thePrefs::s_colors[index] = CMuleColour(col).GetULong();
	}
}


void PrefsUnifiedDlg::OnColorCategorySelected(wxCommandEvent& WXUNUSED(evt))
{
	m_buttonColor->SetBackgroundColour(CMuleColour(thePrefs::s_colors[ m_choiceColor->GetSelection() ] ) );
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

//	case IDC_SELSKIN:
//		id = IDC_SKIN;
//		type = _("Skins directory");
//		break;

	default:
		wxFAIL;
		return;
	}

	type = CFormat(_("Choose a folder for %s")) % type;
	wxTextCtrl* widget = CastChild( id, wxTextCtrl );
	wxString dir = widget->GetValue();
	wxString str = wxDirSelector(
		type, dir,
		wxDD_DEFAULT_STYLE,
		wxDefaultPosition, this);
	if (!str.IsEmpty()) {
		widget->SetValue(str);
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
			wxFAIL;
			return;
	}
	wxString wildcard = CFormat(_("Executable%s"))
#ifdef __WXMSW__
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
	wxString fullpath( theApp->ConfigDir + wxT("addresses.dat") );

	EditServerListDlg* test = new EditServerListDlg(this, _("Edit server list"),
		_("Add here URL's to download server.met files.\nOnly one url on each line."),
		fullpath );

	test->ShowModal();
	delete test;
}


void PrefsUnifiedDlg::OnButtonIPFilterReload(wxCommandEvent& WXUNUSED(event))
{
	theApp->ipfilter->Reload();
}


void PrefsUnifiedDlg::OnButtonIPFilterUpdate(wxCommandEvent& WXUNUSED(event))
{
	theApp->ipfilter->Update( CastChild( IDC_IPFILTERURL, wxTextCtrl )->GetValue() );
}


void PrefsUnifiedDlg::OnPrefsPageChange(wxListEvent& event)
{
	prefs_sizer->Detach( m_CurrentPanel );
	m_CurrentPanel->Show( false );

	m_CurrentPanel = (wxPanel *) m_PrefsIcons->GetItemData(event.GetIndex());
	if (pages[event.GetIndex()].m_function == PreferencesDirectoriesTab) {
		CastChild(IDC_SHARESELECTOR, CDirectoryTreeCtrl)->Init();
	}

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
		label = CFormat(wxPLURAL("Update delay: %d second", "Update delay: %d seconds", event.GetPosition())) % event.GetPosition();
		theApp->amuledlg->m_statisticswnd->SetUpdatePeriod(event.GetPosition());
		theApp->amuledlg->m_kademliawnd->SetUpdatePeriod(event.GetPosition());
		break;

	case IDC_SLIDER3:
		id = IDC_SLIDERINFO3;
		label = CFormat(wxPLURAL("Time for average graph: %d minute", "Time for average graph: %d minutes", event.GetPosition())) % event.GetPosition();
		theApp->m_statistics->SetAverageMinutes(event.GetPosition());
		break;

	case IDC_SLIDER4:
		id = IDC_SLIDERINFO4;
		label = CFormat(_("Connections Graph Scale: %d")) % event.GetPosition();
		theApp->amuledlg->m_statisticswnd->GetConnScope()->SetRanges(0, event.GetPosition());
		break;

	case IDC_SLIDER2:
		id = IDC_SLIDERINFO2;
		label = CFormat(wxPLURAL("Update delay: %d second", "Update delay: %d seconds", event.GetPosition())) % event.GetPosition();
		break;

	case IDC_FILEBUFFERSIZE:
		id = IDC_FILEBUFFERSIZE_STATIC;
		// Yes, it seems odd to add the singular form here, but other languages might need to know the number to select the appropriate translation
		label = CFormat(wxPLURAL("File Buffer Size: %d byte", "File Buffer Size: %d bytes", event.GetPosition() * 15000)) % (event.GetPosition() * 15000);
		break;

	case IDC_QUEUESIZE:
		id = IDC_QUEUESIZE_STATIC;
		// Yes, it seems odd to add the singular form here, but other languages might need to know the number to select the appropriate translation
		label = CFormat(wxPLURAL("Upload Queue Size: %d client", "Upload Queue Size: %d clients", event.GetPosition() * 100)) % (event.GetPosition() * 100);
		break;

	case IDC_SERVERKEEPALIVE:
		id = IDC_SERVERKEEPALIVE_LABEL;

		if ( event.GetPosition() ) {
			label = CFormat(wxPLURAL("Server connection refresh interval: %d minute", "Server connection refresh interval: %d minutes", event.GetPosition())) % event.GetPosition();
		} else {
			label = _("Server connection refresh interval: Disabled");
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
	// Here we do immediate sanity checking of the up/down ratio,
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
	CastChild(ID_TEXT_CLIENT_UDP_PORT, wxStaticText)->SetLabel( 
		m_ServerTabVisible ? (wxString() << (CastChild(IDC_PORT, wxSpinCtrl)->GetValue() + 3)) 
							: wxString(_("disabled")));
}

void PrefsUnifiedDlg::OnUserEventSelected(wxListEvent& event)
{
	for (unsigned int i = 0; i < CUserEvents::GetCount(); ++i) {
		IDC_PREFS_EVENTS_PAGE->Hide(i+1);
	}

	IDC_PREFS_EVENTS_PAGE->Show((event.GetData() - USEREVENTS_FIRST_ID) / USEREVENTS_IDS_PER_EVENT + 1, true);

	IDC_PREFS_EVENTS_PAGE->Layout();

	event.Skip();
}

void PrefsUnifiedDlg::OnLanguageChoice(wxCommandEvent &evt)
{
	thePrefs::GetCfgLang()->UpdateChoice(evt.GetSelection());
}

void PrefsUnifiedDlg::CreateEventPanels(const int idx, const wxString& vars, wxWindow* parent)
{
	wxStaticBox *item8 = new wxStaticBox( parent, -1, CFormat(_("Execute command on '%s' event")) % wxGetTranslation(CUserEvents::GetDisplayName(static_cast<enum CUserEvents::EventType>(idx))) );
	wxStaticBoxSizer *item7 = new wxStaticBoxSizer( item8, wxVERTICAL );

	wxCheckBox *item9 = new wxCheckBox( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 1, _("Enable command execution on core"), wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item9, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer *item10 = new wxFlexGridSizer( 3, 0, 0 );
	item10->AddGrowableCol( 2 );

	item10->Add( 20, 20, 0, wxALIGN_CENTER|wxALL, 0 );

	wxStaticText *item11 = new wxStaticText( parent, -1, _("Core command:"), wxDefaultPosition, wxDefaultSize, 0 );
	item10->Add( item11, 0, wxALIGN_CENTER|wxALL, 5 );

	wxTextCtrl *item12 = new wxTextCtrl( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 2, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	item12->Enable(CUserEvents::IsCoreCommandEnabled(static_cast<enum CUserEvents::EventType>(idx)));
	item10->Add( item12, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	item7->Add( item10, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

	wxCheckBox *item14 = new wxCheckBox( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 3, _("Enable command execution on GUI"), wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item14, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxFlexGridSizer *item15 = new wxFlexGridSizer( 3, 0, 0 );
	item15->AddGrowableCol( 2 );

	item15->Add( 20, 20, 0, wxALIGN_CENTER|wxALL, 0 );

	wxStaticText *item16 = new wxStaticText( parent, -1, _("GUI command:"), wxDefaultPosition, wxDefaultSize, 0 );
	item15->Add( item16, 0, wxALIGN_CENTER|wxALL, 5 );

	wxTextCtrl *item17 = new wxTextCtrl( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 4, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
	item17->Enable(CUserEvents::IsGUICommandEnabled(static_cast<enum CUserEvents::EventType>(idx)));
	item15->Add( item17, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	item7->Add( item15, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );

	wxStaticText *item13 = new wxStaticText( parent, -1, _("The following variables will be replaced:") + vars, wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item13, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	IDC_PREFS_EVENTS_PAGE->Add(item7, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5);

	IDC_PREFS_EVENTS_PAGE->Layout();
	IDC_PREFS_EVENTS_PAGE->Hide(idx + 1);
}
// File_checked_for_headers

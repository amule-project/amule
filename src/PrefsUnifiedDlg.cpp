//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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
#include <common/Macros.h>		// Needed for itemsof()

#include <wx/colordlg.h>
#include <wx/progdlg.h>
#include <wx/stdpaths.h>
#include <wx/tooltip.h>
#include <wx/utils.h>		// wxGetUserHome

#include "amule.h"				// Needed for theApp
#include "amuleDlg.h"
#include "AutostartManager.h"			// Autostart-on-login toggle backend
#ifdef ENABLE_IP2COUNTRY
#include "IP2Country.h"				// CIP2Country::Update / GetDatabasePath
#include <wx/artprov.h>				// wxArtProvider::GetBitmap for the IP2Country tab icon
#include <wx/filename.h>			// wxFileName for status-line size lookup
#endif
#include "MuleColour.h"
#include "EditServerListDlg.h"
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "StatisticsDlg.h"		// Needed for graph parameters, colors
#include "IPFilter.h"			// Needed for CIPFilter
#include "ClientList.h"
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl
#include "Preferences.h"
#include "SharedDirsApplyTask.h"		// Recursive-share expansion worker
#include "muuli_wdr.h"
#include "Logger.h"
#include <common/Format.h>				// Needed for CFormat
#include "TransferWnd.h"		// Needed for CTransferWnd::UpdateCatTabTitles()
#include "KadDlg.h"				// Needed for CKadDlg
#include "OScopeCtrl.h"			// Needed for OScopeCtrl
#include "ServerList.h"
#include "Statistics.h"
#include "UserEvents.h"
#include "PlatformSpecific.h"		// Needed for PLATFORMSPECIFIC_CAN_PREVENT_SLEEP_MODE

wxBEGIN_EVENT_TABLE(PrefsUnifiedDlg,wxDialog)
	// Events
#define USEREVENTS_EVENT(ID, NAME, VARS) \
	EVT_CHECKBOX(USEREVENTS_FIRST_ID + CUserEvents::ID * USEREVENTS_IDS_PER_EVENT + 1,	PrefsUnifiedDlg::OnCheckBoxChange) \
	EVT_CHECKBOX(USEREVENTS_FIRST_ID + CUserEvents::ID * USEREVENTS_IDS_PER_EVENT + 3,	PrefsUnifiedDlg::OnCheckBoxChange)
	USEREVENTS_EVENTLIST()
#undef USEREVENTS_EVENT

	// Proxy
	EVT_CHECKBOX(ID_PROXY_ENABLE_PROXY,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(ID_PROXY_ENABLE_PASSWORD,	PrefsUnifiedDlg::OnCheckBoxChange)

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
	EVT_CHECKBOX(IDC_MACHIDEONCLOSE,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_VERTTOOLBAR,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_SUPPORT_PO,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENABLE_PO_OUTGOING,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_ENFORCE_PO_INCOMING,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_SHOWRATEONTITLE,	PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_NETWORKED2K,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_NETWORKKAD,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_UPNP_ENABLED,		PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_UPNP_WEBSERVER_ENABLED,PrefsUnifiedDlg::OnCheckBoxChange)

	// Autostart-on-login: state lives in the OS (registry / plist /
	// .desktop), not aMule.conf, so it gets its own handler that
	// writes immediately on toggle rather than waiting for OnOk.
	EVT_CHECKBOX(IDC_AUTOSTART_LOGIN,	PrefsUnifiedDlg::OnAutostartToggle)


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
#ifdef ENABLE_IP2COUNTRY
	EVT_CHOICE(IDC_GEOIP_SOURCE,		PrefsUnifiedDlg::OnGeoIPSourceChange)
	EVT_BUTTON(IDC_GEOIP_UPDATE_NOW,	PrefsUnifiedDlg::OnGeoIPUpdateNow)
	EVT_CHECKBOX(IDC_SHOW_COUNTRY_FLAGS,	PrefsUnifiedDlg::OnGeoIPMasterToggle)
#endif
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

wxEND_EVENT_TABLE()


/**
 * Creates an command-event for the given checkbox.
 *
 * This can be used enforce logical constraints by passing by
 * sending a check-box event for each checkbox, when transferring
 * to the UI. However, it should also be used for checkboxes that
 * have no side-effects other than enabling/disabling other
 * widgets in the preferences dialogs.
 */
static void SendCheckBoxEvent(wxWindow* parent, int id)
{
	wxCheckBox* widget = CastByID(id, parent, wxCheckBox);
	wxCHECK_RET(widget, "Invalid widget in CreateEvent");

	wxCommandEvent evt(wxEVT_CHECKBOX, id);
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
	int		m_imageidx;
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
#ifdef ENABLE_IP2COUNTRY
	// Inserted between Interface and Statistics so the GeoIP / country-flag
	// settings sit next to the related display option (the master
	// IDC_SHOW_COUNTRY_FLAGS checkbox lives in this new tab too). Hidden
	// from the page list when ENABLE_IP2COUNTRY is off so users who built
	// without libmaxminddb don't see a panel that can't function.
	{ wxTRANSLATE("IP2Country"),		PreferencesIP2CountryTab,	13 },
#endif
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
#ifdef ENABLE_IP2COUNTRY
	s_activeInstance = this;
#endif
	preferencesDlgTop(this, false);

	m_PrefsIcons = CastChild(ID_PREFSLISTCTRL, wxListCtrl);
	wxImageList *icon_list = new wxImageList(16, 16);
	m_PrefsIcons->AssignImageList(icon_list, wxIMAGE_LIST_SMALL);

	// Add the single column used
	m_PrefsIcons->InsertColumn(
		0, "", wxLIST_FORMAT_LEFT,
		m_PrefsIcons->GetSize().GetWidth()-5);

	// Temp variables for finding the smallest height and width needed
	int width = 0;
	int height = 0;

	// Add each page to the page-list
	for (unsigned int i = 0; i < itemsof(pages); ++i) {
		// The IP2Country tab uses an embedded-PNG icon shipped via
		// CamuleArtProvider (registered in CamuleGuiApp::OnInit) rather
		// than the hardcoded amuleSpecial XPM data the other tabs use.
		// Existing tabs are kept on amuleSpecial to avoid a wholesale
		// migration; new tabs should prefer the PNG path.
#ifdef ENABLE_IP2COUNTRY
		if (pages[i].m_function == PreferencesIP2CountryTab) {
			icon_list->Add(wxArtProvider::GetBitmap(
				"amule:prefs_ip2country", wxART_OTHER, wxSize(16, 16)));
		} else
#endif
		{
			icon_list->Add(amuleSpecial(pages[i].m_imageidx));
		}
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
		prefs_sizer->Add(Widget, wxSizerFlags().Expand().Expand());

		if (pages[i].m_function == PreferencesGeneralTab) {
			// This must be done now or pages won't Fit();
			#ifdef __WINDOWS__
				CastChild(IDC_BROWSERTABS, wxCheckBox)->Enable(false);
			#endif /* __WINDOWS__ */
			CastChild(IDC_PREVIEW_NOTE, wxStaticText)->SetLabel(_("The following variables will be substituted:\n    %PARTFILE - full path to the file\n    %PARTNAME - file name only"));
			// Tray-icon checkboxes (IDC_ENABLETRAYICON,
			// IDC_MINTRAY) are visible on every platform now,
			// including macOS. wxTaskBarIcon → NSStatusItem on
			// Mac, NOTIFYICONDATA on Windows, GtkStatusIcon /
			// libayatana SNI on Linux. macOS users who prefer
			// the menu-bar status-item pattern (Spotify / Slack
			// / Discord style) can opt in.
#if defined(__WXGTK__) && !defined(WITH_LIBAYATANA_APPINDICATOR)
			// On Linux without libayatana-appindicator3 the only
			// backend wxTaskBarIcon can fall back to is the legacy
			// GtkStatusIcon API, which GNOME Shell dropped in 3.26
			// and wlroots-based compositors never implemented — the
			// tray icon is silently invisible. Disable the option
			// so users don't enable a feature that does nothing.
			// (CamuleApp::OnInit force-clears UseTrayIcon at startup
			// for the same reason, so dependent options cascade off
			// even before the user opens this panel.)
			FindWindow(IDC_ENABLETRAYICON)->Enable(false);
			FindWindow(IDC_ENABLETRAYICON)->SetToolTip(
				_("Tray icon support requires libayatana-appindicator3 at compile time."));
#endif

#ifdef __WXGTK__
			// xdg-shell intentionally doesn't deliver iconified-state
			// notifications to clients, so the system minimize button
			// on Wayland cannot trigger our hide-to-tray path. Same
			// gap is documented across qBittorrent / Telegram /
			// KeePassXC / Slack — none of them have a fix either.
			// Grey out the option with a tooltip so the user
			// understands why; the runtime sanity check in
			// CamuleApp::OnInit keeps DoMinToTray() returning false
			// regardless of the saved value.
			if (CamuleAppCommon::IsWaylandSession()) {
				FindWindow(IDC_MINTRAY)->SetToolTip(
					_("Not available on Wayland: the protocol does not "
					  "report when a window is minimized, so this option "
					  "cannot intercept the system minimize button. "
					  "Workaround: launch aMule with `GDK_BACKEND=x11` "
					  "to use XWayland instead."));
			}
#endif
		} else if (pages[i].m_function == PreferencesEventsTab) {

#define USEREVENTS_REPLACE_VAR(VAR, DESC, CODE)	+ wxString("\n  %" VAR " - ") + wxGetTranslation(DESC)
#define USEREVENTS_EVENT(ID, NAME, VARS) case CUserEvents::ID: CreateEventPanels(idx, "" VARS, Widget); break;

			wxListCtrl *list = CastChild(IDC_EVENTLIST, wxListCtrl);
			list->InsertColumn(0, "");
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
						       CreateEventPanels(idx, wxString("\n %SENDER - ") + wxTRANSLATE("Message sender."), Widget);
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
	prefs_sizer->Add(DefaultWidget, wxSizerFlags().Expand().Expand());
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

	// Load the user's intent (explicit non-recursive vs marked-recursive
	// roots) into the tree control's two maps. shareddir_list itself
	// is the runtime expansion -- not useful as UI state since it
	// includes auto-discovered subdirs that shouldn't render as
	// user-selected.
	m_ShareSelector->SetSharedDirectories(&theApp->glob_prefs->shareddir_explicit_list);
	m_ShareSelector->SetRecursiveSharedDirectories(&theApp->glob_prefs->shareddir_recursive_list);

	// Autostart checkbox: state lives in the OS, never aMule.conf, so
	// read live each time the dialog opens. The thePrefs Cfg machinery
	// above doesn't know about it.
	wxCheckBox *autostartCb = static_cast<wxCheckBox *>(FindWindow(IDC_AUTOSTART_LOGIN));
	if (autostartCb) {
		autostartCb->SetValue(AutostartManager::IsEnabled());
	}

#ifdef ENABLE_IP2COUNTRY
	// Sync the GeoIP source dropdown to the persisted source; the
	// Cfg_ system above handles the credential / URL / auto-update
	// fields, but the dropdown is driven through a custom handler so
	// hide/show of the source sub-panels stays consistent. Also
	// refresh the status block from the live CIP2Country state.
	wxChoice *geoipSource = CastChild(IDC_GEOIP_SOURCE, wxChoice);
	if (geoipSource) {
		geoipSource->SetSelection(static_cast<int>(thePrefs::GetGeoIPSource()));
		UpdateGeoIPSourcePanel();
		UpdateGeoIPStatus();
		UpdateGeoIPControlsEnabled();
	}
	// Snapshot the source + credential values that aren't tracked
	// by the Cfg system, so OnOk can tell whether anything
	// download-affecting changed during the dialog session.
	m_GeoIPSourceAtOpen = static_cast<int>(thePrefs::GetGeoIPSource());
	m_GeoIPMaxMindLicenseAtOpen = thePrefs::GetGeoIPMaxMindLicense();
	m_GeoIPCustomUrlAtOpen = thePrefs::GetGeoIPCustomUrl();
#endif

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

	// Enable/Disable some controls
	FindWindow( IDC_MINDISKSPACE )->Enable( thePrefs::IsCheckDiskspaceEnabled() );
	FindWindow( IDC_OSDIR )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_OSUPDATE )->Enable( thePrefs::IsOnlineSignatureEnabled() );
	FindWindow( IDC_UDPENABLE )->Enable( !thePrefs::GetNetworkKademlia());
	FindWindow( IDC_UDPPORT )->Enable( thePrefs::s_UDPEnable );
	FindWindow( IDC_SERVERRETRIES )->Enable( thePrefs::DeadServer() );
	FindWindow( IDC_STARTNEXTFILE_SAME )->Enable(thePrefs::StartNextFile());
	FindWindow( IDC_STARTNEXTFILE_ALPHA )->Enable(thePrefs::StartNextFile());

	// The tray icon is the only recovery surface for a window hidden
	// via the close button: on Linux/Windows the option needs the tray
	// to bring the window back, and on macOS the matching code path
	// (NSApplicationActivationPolicyAccessory) drops the Dock icon
	// while hidden, so the tray is also the only way back there.
	FindWindow(IDC_MACHIDEONCLOSE)->Enable(thePrefs::UseTrayIcon());

#ifdef __WXGTK__
	const bool minTrayUsable = thePrefs::UseTrayIcon()
		&& !CamuleAppCommon::IsWaylandSession();
#else
	const bool minTrayUsable = thePrefs::UseTrayIcon();
#endif
	FindWindow(IDC_MINTRAY)->Enable(minTrayUsable);

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

	// Hide preferences that are persisted only to amulegui's local
	// remote.conf but never sent to amuled via EC_OP_SET_PREFERENCES
	// (i.e. not packed by CEC_Prefs_Packet at all). The widget would
	// otherwise show amulegui's stale local default -- not amuled's
	// real value -- and editing it would silently affect nothing on
	// the daemon side. Same gap holds whether amulegui is on a
	// loopback or remote connection: amuled never reads remote.conf,
	// so the control is dead in both cases. Hide unconditionally for
	// CLIENT_GUI. Proxy settings (ID_PROXY_*) are deliberately *not*
	// in this list -- they're consumed by amulegui's own HTTP client
	// in ApplyProxyToDefaultSession() for the GeoIP database fetch,
	// so they remain meaningful in CLIENT_GUI builds.
	const int amuledOnlyPrefs[] = {
		IDC_ADDRESS,
		IDC_UPNP_ENABLED,
		IDC_UPNPTCPPORT,
		IDC_UPNPTCPPORTTEXT,
		IDC_UPNP_WEBSERVER_ENABLED,
		IDC_WEBUPNPTCPPORT,
		IDC_WEBUPNPTCPPORTTEXT,
		IDC_UPNP_EC_ENABLED,
		IDC_OSDIR,
		IDC_OSUPDATE,
		IDC_EXT_CONN_ACCEPT,
		IDC_EXT_CONN_IP,
		IDC_EXT_CONN_TCP_PORT,
		IDC_EXT_CONN_PASSWD,
		IDC_PARANOID,
		IDC_IPFILTERSYS,
		IDC_STARTNEXTFILE_ALPHA,
	};
	for (int id : amuledOnlyPrefs) {
		if (wxWindow* w = FindWindow(id)) {
			w->Hide();
		}
	}
#endif

	// Protocol obfuscation
	::SendCheckBoxEvent(this, IDC_SUPPORT_PO);
	::SendCheckBoxEvent(this, IDC_ENABLE_PO_OUTGOING);
	::SendCheckBoxEvent(this, IDC_ENFORCE_PO_INCOMING);

#ifndef ENABLE_IP2COUNTRY
	// The country-flags checkbox + the rest of the IP2Country controls
	// only live in the dedicated PreferencesIP2CountryTab, which is
	// `#ifdef ENABLE_IP2COUNTRY`-gated in the pages[] table. With
	// libmaxminddb missing, neither the tab nor any of its widgets
	// exists, so there's nothing to disable here -- the *.NewCfgItem
	// bindings below are gated the same way, and SetGeoIPEnabled stays
	// at its default false.
#endif

#ifdef __GIT__
	// Version is always shown on the title in development versions
	CastChild(IDC_SHOWVERSIONONTITLE, wxCheckBox)->SetValue(true);
	CastChild(IDC_SHOWVERSIONONTITLE, wxCheckBox)->Enable(false);
#endif

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
#else
	FindWindow(IDC_UPNPTCPPORT)->Enable(thePrefs::GetUPnPEnabled());
	FindWindow(IDC_UPNPTCPPORTTEXT)->Enable(thePrefs::GetUPnPEnabled());
	FindWindow(IDC_WEBUPNPTCPPORT)->Enable(thePrefs::GetUPnPWebServerEnabled());
	FindWindow(IDC_WEBUPNPTCPPORTTEXT)->Enable(thePrefs::GetUPnPWebServerEnabled());
#endif

#ifdef __DEBUG__
	// Set debugging toggles
	int count = theLogger.GetDebugCategoryCount();
	wxCheckListBox* list = CastChild( ID_DEBUGCATS, wxCheckListBox );

	for ( int i = 0; i < count; i++ ) {
		list->Check( i, theLogger.GetDebugCategory( i ).IsEnabled() );
	}
#endif

	m_verticalToolbar = thePrefs::VerticalToolbar();
	m_toolbarOrientationChanged = false;

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

	// shareddir_list is committed separately from OnOk (see
	// CommitSharedDirsWithProgress) so that recursive-share expansion
	// can run on a worker thread with a progress dialog and a cancel
	// button. Doing it eagerly here would re-introduce the multi-
	// minute UI freeze that issue #592 hit on /home-sized roots.

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

	// Commit the share list with the recursive-expand-on-worker-
	// thread path. Done after TransferFromWindow (so other prefs
	// are already populated in glob_prefs) but before Save() so a
	// successful commit ends up in shareddir.dat alongside the rest.
	// If the user cancels at the confirm or the progress dialog,
	// bail out of OnOk *before* anything is persisted — so the prefs
	// dialog stays open and the user can adjust their selection
	// without losing the rest of their pending pref changes.
	const SharedDirsCommitResult shareResult = CommitSharedDirsWithProgress();
	if (shareResult == SharedDirsCommitResult::CancelledByUser) {
		return;
	}
	const bool sharedDirsCommitted =
		(shareResult == SharedDirsCommitResult::Committed);

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
	if (CfgChanged(IDC_SUPPORT_PO)) {
		restart_needed = true;
		restart_needed_msg += _("- Protocol obfuscation support changed.\n");
	}

	// Force port checking
	thePrefs::SetPort(thePrefs::GetPort());

	if ((CPath::GetFileSize(thePrefs::GetConfigDir() + "addresses.dat") == 0) &&
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

	// CommitSharedDirsWithProgress already ran Reload (with progress
	// + cancel) when shareddir_list itself changed. We only need to
	// trigger a fresh Reload here for the other paths IDC_INCFILES /
	// IDC_TEMPFILES affect.
	if (!sharedDirsCommitted
		&& (CfgChanged(IDC_INCFILES) || CfgChanged(IDC_TEMPFILES)))
	{
		theApp->sharedfiles->Reload();
	}

	if (CfgChanged(IDC_AUTO_RESCAN_SHARED)) {
		// Start or stop the fs-watcher immediately so the user sees the
		// effect of toggling without needing to restart amuled.
		theApp->sharedfiles->EnableDirectoryWatcher(thePrefs::AutoRescanSharedDirs());
	}

	if (CfgChanged(IDC_FOLLOW_SYMLINKS_SHARED) && !sharedDirsCommitted) {
		// Re-scan so the new symlink policy takes effect on the existing
		// shared tree: turning the toggle off should drop symlinked
		// entries already in the shareset, turning it on should pick
		// them up.
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

	theApp->ResetTitle();

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

#ifdef ENABLE_IP2COUNTRY
	// Auto-download on OK when the user changed anything that affects
	// *which* file should be on disk. Without this, switching source
	// DB-IP→MaxMind (or pasting a new license) leaves the old file
	// loaded until the user remembers to click Update now. We skip
	// the download if:
	//   * IP2Country is disabled (the file is irrelevant), or
	//   * the user just toggled the master enable on — EnableIP2Country
	//     above already handles missing-file → Update() in that case.
	// Triggered as a manual update so the user sees a popup if their
	// new credentials are bad, rather than a silent log line.
	if (thePrefs::IsGeoIPEnabled()
		&& !CfgChanged(IDC_SHOW_COUNTRY_FLAGS)
		&& theApp->amuledlg && theApp->amuledlg->m_IP2Country) {
		const bool sourceChanged =
			static_cast<int>(thePrefs::GetGeoIPSource()) != m_GeoIPSourceAtOpen;
		const bool licenseChanged =
			thePrefs::GetGeoIPMaxMindLicense() != m_GeoIPMaxMindLicenseAtOpen;
		const bool urlChanged =
			thePrefs::GetGeoIPCustomUrl() != m_GeoIPCustomUrlAtOpen;
		// Only re-download if the change matters for the *currently*
		// selected source. Editing the MaxMind license while DB-IP
		// is selected shouldn't trigger an unrelated DB-IP fetch.
		bool credentialChangedForActive = false;
		switch (thePrefs::GetGeoIPSource()) {
		case CPreferences::GeoIPSourceMaxMind:
			credentialChangedForActive = licenseChanged;
			break;
		case CPreferences::GeoIPSourceCustom:
			credentialChangedForActive = urlChanged;
			break;
		default:
			break;
		}
		if (sourceChanged || credentialChangedForActive) {
			theApp->amuledlg->m_IP2Country->Update(true);
		}
	}
#endif

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

	if (m_toolbarOrientationChanged) {
			theApp->amuledlg->Create_Toolbar(m_verticalToolbar);
			// Update the first tool (conn button)
			theApp->amuledlg->ShowConnectionState();
			theApp->amuledlg->Layout();
	}
}


void PrefsUnifiedDlg::OnAutostartToggle(wxCommandEvent& event)
{
	// Apply immediately. The OS is the source of truth — we don't
	// persist intent in aMule.conf, so there's no Apply-on-OK step
	// for this widget. If the write fails (e.g. read-only LaunchAgent
	// dir on a sandboxed macOS install), roll the checkbox back so
	// the UI reflects reality.
	bool wanted = event.IsChecked();
	bool ok = wanted ? AutostartManager::Enable() : AutostartManager::Disable();
	if (!ok) {
		wxCheckBox *cb = static_cast<wxCheckBox *>(FindWindow(IDC_AUTOSTART_LOGIN));
		if (cb) {
			cb->SetValue(!wanted);
		}
		wxMessageBox(
			wanted
				? _("Could not register aMule for autostart at login. The autostart store may be read-only.")
				: _("Could not remove the autostart-at-login entry."),
			_("Autostart"), wxOK | wxICON_WARNING, this);
	}
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

		case IDC_UPNP_ENABLED:
			FindWindow(IDC_UPNPTCPPORT)->Enable(value);
			FindWindow(IDC_UPNPTCPPORTTEXT)->Enable(value);
			break;

		case IDC_UPNP_WEBSERVER_ENABLED:
			FindWindow(IDC_WEBUPNPTCPPORT)->Enable(value);
			FindWindow(IDC_WEBUPNPTCPPORTTEXT)->Enable(value);
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
			if ((CPath::GetFileSize(thePrefs::GetConfigDir() + "addresses.dat") == 0) &&
				CastChild(event.GetId(), wxCheckBox)->IsChecked() ) {
				wxMessageBox(_("Your Auto-update servers list is in blank.\nPlease fill in at least one URL to point to a valid server.met file.\nClick on the button \"List\" by this checkbox to enter an URL."),
					_("Message"), wxOK | wxICON_INFORMATION);
				CastChild(event.GetId(), wxCheckBox)->SetValue(false);
			}
			break;

		case IDC_MSGFILTER:
			// Toggle All filter options
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
			// Toggle filtering by data.
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
			// Toggle filter word list.
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
			// HideOnClose's recovery surface is the tray icon, so its
			// checkbox follows tray-icon state too. Live-update both
			// here so the user doesn't have to close + reopen prefs to
			// see dependent options gate correctly.
			FindWindow(IDC_MACHIDEONCLOSE)->Enable(value);
			if (value) {
				theApp->amuledlg->CreateSystray();
			} else {
				theApp->amuledlg->RemoveSystray();
			}
			thePrefs::SetUseTrayIcon(value);
			break;

		case IDC_NOTIF:
			FindWindow(IDC_NOTIF)->Enable(value);
			break;

		case IDC_VERTTOOLBAR:
			m_toolbarOrientationChanged = (m_verticalToolbar != value);
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
#ifdef __WINDOWS__
		% " (*.exe)|*.exe";
#else
		% "|*";
#endif

	wxString str = wxFileSelector( title, "", "",
		"", wildcard, 0, this );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = CastChild( id, wxTextCtrl );
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonEditAddr(wxCommandEvent& WXUNUSED(evt))
{
	wxString fullpath( thePrefs::GetConfigDir() + "addresses.dat" );

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


#ifdef ENABLE_IP2COUNTRY
PrefsUnifiedDlg *PrefsUnifiedDlg::s_activeInstance = NULL;

PrefsUnifiedDlg::~PrefsUnifiedDlg()
{
	// Clear the active-instance pointer so the IP2Country download
	// callback can't poke a freed dialog if a download completes after
	// the user has closed Preferences.
	if (s_activeInstance == this) {
		s_activeInstance = NULL;
	}
}

void PrefsUnifiedDlg::RefreshIP2CountryStatusIfOpen()
{
	// IP2Country download-completion hook. CamuleDlg::IP2CountryDownloadFinished
	// calls this after the new MMDB has been opened, so an open prefs
	// dialog can refresh its status line without the user having to
	// flip the source dropdown to trigger a redraw.
	if (s_activeInstance) {
		s_activeInstance->UpdateGeoIPStatus();
	}
}


void PrefsUnifiedDlg::NotifyIP2CountryUpdateFailedIfOpen(const wxString& msg)
{
	// Manual "Update now" failure popup. Skipped if the prefs dialog
	// has been closed in the meantime: the user has already moved on,
	// and an unparented popup with no obvious trigger would be more
	// confusing than the log line they can find under Network → Log.
	if (s_activeInstance) {
		wxMessageBox(msg, _("IP2Country update failed"),
			wxICON_WARNING | wxOK, s_activeInstance);
	}
}

void PrefsUnifiedDlg::OnGeoIPSourceChange(wxCommandEvent& WXUNUSED(event))
{
	// Translate the dropdown index back into the canonical persisted
	// source string. The dropdown order is fixed: 0 = DB-IP, 1 = MaxMind,
	// 2 = Custom — matching CPreferences::GeoIPSource numeric values.
	const int sel = CastChild(IDC_GEOIP_SOURCE, wxChoice)->GetSelection();
	thePrefs::SetGeoIPSource(static_cast<thePrefs::GeoIPSource>(sel));
	UpdateGeoIPSourcePanel();
	UpdateGeoIPStatus();
}


void PrefsUnifiedDlg::OnGeoIPUpdateNow(wxCommandEvent& WXUNUSED(event))
{
	// Persist the credential / URL fields the user just edited *before*
	// kicking off the download — the URL helper reads them from the
	// static backing store, not the live widget values. The full prefs
	// commit happens on OK, but for "Update now" we need a partial save.
	thePrefs::SetGeoIPMaxMindLicense(CastChild(IDC_GEOIP_MAXMIND_LIC, wxTextCtrl)->GetValue());
	thePrefs::SetGeoIPCustomUrl(    CastChild(IDC_GEOIP_CUSTOM_URL,  wxTextCtrl)->GetValue());

	// Kick off the download. CIP2Country::DownloadFinished will swap
	// the new file in and re-open the database asynchronously; the
	// status line refreshes next time the panel is shown (the running
	// download isn't blocking, so polling would just show "...").
	if (theApp->amuledlg && theApp->amuledlg->m_IP2Country) {
		theApp->amuledlg->m_IP2Country->Update(true);
	}
}


void PrefsUnifiedDlg::UpdateGeoIPSourcePanel()
{
	// Show exactly one of the three sub-panels based on the selected
	// source. Each is a discrete wxPanel hosting its own labels and
	// fields, so toggling the panel collapses the slot cleanly without
	// leaving orphaned ID-less labels visible.
	//
	// Critical: use the *sizer's* Show(window, bool) — wxWindow::Show()
	// only hides the window, leaving the sizer item still occupying its
	// slot. Going through wxSizer::Show() releases the slot AND hides
	// the window, which is what actually re-flows the layout.
	const thePrefs::GeoIPSource src = thePrefs::GetGeoIPSource();
	wxWindow *dbip    = FindWindow(IDC_GEOIP_INFO_DBIP);
	wxWindow *maxmind = FindWindow(IDC_GEOIP_INFO_MAXMIND);
	wxWindow *custom  = FindWindow(IDC_GEOIP_INFO_CUSTOM);
	if (!dbip || !maxmind || !custom) {
		return;
	}

	wxSizer *containerSizer = dbip->GetContainingSizer();
	if (!containerSizer) {
		return;
	}
	containerSizer->Show(dbip,    src == thePrefs::GeoIPSourceDBIP);
	containerSizer->Show(maxmind, src == thePrefs::GeoIPSourceMaxMind);
	containerSizer->Show(custom,  src == thePrefs::GeoIPSourceCustom);

	// Re-layout the prefs page so the height delta from the now-hidden
	// panel propagates upward through the wxStaticBoxSizer chain. Each
	// sub-panel is a real wxPanel (leaf from the layout engine's view),
	// so the cascade-loop risk that motivated dropping Layout() earlier
	// does not apply here.
	if (m_CurrentPanel) {
		m_CurrentPanel->Layout();
	}
}


void PrefsUnifiedDlg::OnGeoIPMasterToggle(wxCommandEvent& event)
{
	// Forward to the generic CfgChanged tracker so the OK button noticed
	// the dirty state, then grey out everything below the master switch.
	OnCheckBoxChange(event);
	UpdateGeoIPControlsEnabled();
}


void PrefsUnifiedDlg::UpdateGeoIPControlsEnabled()
{
	// Master "Show country flags for clients" gates every downstream
	// control: source selector, all three sub-panel fields, the Update
	// Now button, auto-update checkbox, and the status line. Each
	// control is looked up by ID so missing widgets (e.g. earlier
	// init failure) don't crash.
	wxCheckBox *master = CastChild(IDC_SHOW_COUNTRY_FLAGS, wxCheckBox);
	if (!master) {
		return;
	}
	const bool on = master->IsChecked();

	const int ids[] = {
		IDC_GEOIP_SOURCE,
		IDC_GEOIP_INFO_DBIP,
		IDC_GEOIP_INFO_MAXMIND,
		IDC_GEOIP_INFO_CUSTOM,
		IDC_GEOIP_MAXMIND_LIC,
		IDC_GEOIP_CUSTOM_URL,
		IDC_GEOIP_UPDATE_NOW,
		IDC_GEOIP_AUTOUPDATE,
		IDC_GEOIP_STATUS,
	};
	for (size_t i = 0; i < WXSIZEOF(ids); ++i) {
		wxWindow *w = FindWindow(ids[i]);
		if (w) {
			w->Enable(on);
		}
	}
}


void PrefsUnifiedDlg::UpdateGeoIPStatus()
{
	if (!theApp->amuledlg || !theApp->amuledlg->m_IP2Country) {
		return;
	}
	CIP2Country *ip2c = theApp->amuledlg->m_IP2Country;
	wxStaticText *st = CastChild(IDC_GEOIP_STATUS, wxStaticText);
	if (!st) {
		return;
	}

	// Attribution for the *loaded* file (the source that actually wrote
	// it) — not the currently-selected dropdown source. If the file was
	// hand-installed (LoadedSource is empty), no attribution is shown:
	// we don't know who to credit and the per-source sub-panel below
	// already covers the legal-display obligation.
	wxString attribution;
	const wxString& loaded = thePrefs::GetGeoIPLoadedSource();
	if (loaded == "maxmind") {
		attribution = _("Data by MaxMind GeoLite2");
	} else if (loaded == "custom") {
		attribution = _("Custom source");
	} else if (loaded == "dbip") {
		attribution = _("Data by DB-IP.com");
	}

	if (ip2c->IsEnabled()) {
		// Loaded — single-line summary keeps the dialog height bounded
		// across sources. We deliberately omit the on-disk path: it's
		// already in the log line at load time, and wxStaticText
		// tooltips on wxOSX are unreliable enough that promising it in
		// the UI would mislead Mac users.
		const wxFileName fn(ip2c->GetDatabasePath());
		wxString sizeLabel;
		if (fn.FileExists()) {
			const wxULongLong bytes = fn.GetSize();
			if (bytes != wxInvalidSize) {
				sizeLabel = wxString::Format(" (%.1f MB)", bytes.ToDouble() / (1024.0 * 1024.0));
			}
		}
		if (attribution.IsEmpty()) {
			// Hand-installed / migrated file — no attribution to show.
			st->SetLabel(wxString::Format(
				_("Status: Loaded%s"), sizeLabel));
		} else {
			st->SetLabel(wxString::Format(
				_("Status: Loaded%s - %s"),
				sizeLabel, attribution));
		}
	} else if (wxFileName::FileExists(ip2c->GetDatabasePath())) {
		// File exists but database failed to open — corrupt / wrong format.
		st->SetLabel(_("Status: Failed to load - click 'Update now' to refresh."));
	} else {
		st->SetLabel(_("Status: Not found - click 'Update now' to download."));
	}
}
#endif // ENABLE_IP2COUNTRY


void PrefsUnifiedDlg::OnPrefsPageChange(wxListEvent& event)
{
	prefs_sizer->Detach( m_CurrentPanel );
	m_CurrentPanel->Show( false );

	m_CurrentPanel = reinterpret_cast<wxPanel*>(m_PrefsIcons->GetItemData(event.GetIndex()));
	if (pages[event.GetIndex()].m_function == PreferencesDirectoriesTab) {
		CastChild(IDC_SHARESELECTOR, CDirectoryTreeCtrl)->Init();
	}

	prefs_sizer->Add( m_CurrentPanel, wxSizerFlags().Expand().Expand());
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
	item7->Add( item9, wxSizerFlags().CenterVertical().Border(wxLEFT|wxRIGHT, 5) );

	wxFlexGridSizer *item10 = new wxFlexGridSizer( 3, 0, 0 );
	item10->AddGrowableCol( 2 );

	item10->Add( 20, 20, wxSizerFlags().Center() );

	wxStaticText *item11 = new wxStaticText( parent, -1, _("Core command:"), wxDefaultPosition, wxDefaultSize, 0 );
	item10->Add( item11, wxSizerFlags().Center().Border(wxALL, 5) );

	wxTextCtrl *item12 = new wxTextCtrl( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 2, "", wxDefaultPosition, wxDefaultSize, 0 );
	item12->Enable(CUserEvents::IsCoreCommandEnabled(static_cast<enum CUserEvents::EventType>(idx)));
	item10->Add( item12, wxSizerFlags().Expand().CenterVertical().Border(wxALL, 5) );

	item7->Add( item10, wxSizerFlags().Expand().CenterVertical().Border(wxALL, 0) );

	wxCheckBox *item14 = new wxCheckBox( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 3, _("Enable command execution on GUI"), wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item14, wxSizerFlags().CenterVertical().Border(wxLEFT|wxRIGHT, 5) );

	wxFlexGridSizer *item15 = new wxFlexGridSizer( 3, 0, 0 );
	item15->AddGrowableCol( 2 );

	item15->Add( 20, 20, wxSizerFlags().Center() );

	wxStaticText *item16 = new wxStaticText( parent, -1, _("GUI command:"), wxDefaultPosition, wxDefaultSize, 0 );
	item15->Add( item16, wxSizerFlags().Center().Border(wxALL, 5) );

	wxTextCtrl *item17 = new wxTextCtrl( parent, USEREVENTS_FIRST_ID + idx * USEREVENTS_IDS_PER_EVENT + 4, "", wxDefaultPosition, wxDefaultSize, 0 );
	item17->Enable(CUserEvents::IsGUICommandEnabled(static_cast<enum CUserEvents::EventType>(idx)));
	item15->Add( item17, wxSizerFlags().Expand().CenterVertical().Border(wxALL, 5) );

	item7->Add( item15, wxSizerFlags().Expand().CenterVertical().Border(wxALL, 0) );

	wxStaticText *item13 = new wxStaticText( parent, -1, _("The following variables will be replaced:") + vars, wxDefaultPosition, wxDefaultSize, 0 );
	item7->Add( item13, wxSizerFlags().Expand().CenterVertical().Border(wxALL, 5) );

	IDC_PREFS_EVENTS_PAGE->Add(item7, wxSizerFlags().Expand().CenterVertical().Border(wxALL, 5));

	IDC_PREFS_EVENTS_PAGE->Layout();
	IDC_PREFS_EVENTS_PAGE->Hide(idx + 1);
}


namespace {

// Hard-coded list of paths that look like obvious "did you really mean
// to share this?" candidates. Matched by IsSensitiveSharePath as either
// the exact path or a strict descendant (separator-boundary aware),
// so e.g. ~/Documents/Tax2024 is flagged because ~/Documents is on
// the list. Empty list entries are skipped.
//
// This is not meant to be exhaustive — its job is to catch the most
// common "accidental right-click" cases (issue #592) by raising a
// confirmation dialog, not to be a privacy boundary. Users can always
// say "Yes I really do want this" and proceed.
wxArrayString BuildSensitivePathList()
{
	wxArrayString out;

	auto pushIfNotEmpty = [&out](const wxString & s) {
		if (!s.IsEmpty()) {
			out.Add(s);
		}
	};

	const wxString home = wxGetUserHome();
	pushIfNotEmpty(home);
	if (!home.IsEmpty()) {
		const wxChar sep = wxFileName::GetPathSeparator();
		pushIfNotEmpty(home + sep + "Documents");
		pushIfNotEmpty(home + sep + "Desktop");
		pushIfNotEmpty(home + sep + "Pictures");
		pushIfNotEmpty(home + sep + "Library");        // macOS
		pushIfNotEmpty(home + sep + "AppData");        // Windows
		pushIfNotEmpty(home + sep + ".aMule");
		pushIfNotEmpty(home + sep + ".config");
		pushIfNotEmpty(home + sep + ".ssh");
		pushIfNotEmpty(home + sep + ".gnupg");
	}

#ifdef __WINDOWS__
	pushIfNotEmpty("C:\\");
	pushIfNotEmpty("C:\\Windows");
	pushIfNotEmpty("C:\\Program Files");
	pushIfNotEmpty("C:\\Program Files (x86)");
	pushIfNotEmpty("C:\\ProgramData");
	pushIfNotEmpty("C:\\Users");          // parent of every user's home
#else
	pushIfNotEmpty("/");
	pushIfNotEmpty("/etc");
	pushIfNotEmpty("/var");
	pushIfNotEmpty("/tmp");
	pushIfNotEmpty("/boot");
	pushIfNotEmpty("/usr");
	pushIfNotEmpty("/home");              // parent of every user's home on Linux
	pushIfNotEmpty("/root");              // root user's home on Linux
	pushIfNotEmpty("/Applications");      // macOS
	pushIfNotEmpty("/System");            // macOS
	pushIfNotEmpty("/Users");             // parent of every user's home on macOS
#endif

	return out;
}

bool IsSensitiveSharePath(const CPath & path)
{
	static const wxArrayString sensitive = BuildSensitivePathList();
	const wxString raw = path.GetRaw();
	const wxChar sep = wxFileName::GetPathSeparator();
	for (size_t i = 0; i < sensitive.GetCount(); ++i) {
		const wxString & root = sensitive[i];
		if (raw == root) {
			return true;
		}
		// Prefix match with a separator boundary so /home doesn't
		// also flag /home2 or /homework. The length floor at 4 keeps
		// "filesystem-root" entries — `/` (1 char) and `C:\` (3 chars)
		// — exact-match-only: otherwise every path on the platform
		// would be a descendant of the root and every share would
		// trip the confirm dialog. Real subtrees like `/etc`, `/var`,
		// `C:\Windows` keep their prefix-match behaviour.
		if (root.length() >= 4
			&& raw.length() > root.length()
			&& raw.StartsWith(root)
			&& (root.Last() == sep || raw[root.length()] == sep))
		{
			return true;
		}
	}
	return false;
}

} // namespace


PrefsUnifiedDlg::SharedDirsCommitResult
PrefsUnifiedDlg::CommitSharedDirsWithProgress()
{
	if (!m_ShareSelector || !m_ShareSelector->HasChanged) {
		return SharedDirsCommitResult::NothingToCommit;
	}

	CDirectoryTreeCtrl::PathList explicitShares;
	CDirectoryTreeCtrl::PathList recursiveIntents;
	m_ShareSelector->GetSharedDirectories(&explicitShares);
	m_ShareSelector->GetRecursiveSharedDirectories(&recursiveIntents);

	// Strip entries that are descendants of a recursive root: the
	// UI's right-click handler populates m_lstShared with the already-
	// rendered subtree as a side-effect of MarkChildren (so the
	// in-tree visual stays consistent), but those subdirs aren't
	// "explicit" intent -- they're the recursive expansion. Without
	// this filter they'd land in shareddir-explicit.dat and stick
	// around as orphan pinned paths if the user later removed the
	// recursive marker externally (no DelSharesUnder cleanup runs
	// outside the UI). Filter at the commit boundary keeps the
	// canonical files semantically clean.
	if (!recursiveIntents.empty()) {
		const wxChar sep = wxFileName::GetPathSeparator();
		auto isInsideRecursive = [&recursiveIntents, sep](const CPath & p) {
			const wxString target = p.GetRaw();
			for (const CPath & root : recursiveIntents) {
				const wxString r = root.GetRaw();
				if (r.IsEmpty()) {
					continue;
				}
				if (target == r) {
					return true;
				}
				if (target.length() > r.length() &&
					target.StartsWith(r) &&
					(r.Last() == sep || target[r.length()] == sep)) {
					return true;
				}
			}
			return false;
		};
		CDirectoryTreeCtrl::PathList filtered;
		filtered.reserve(explicitShares.size());
		for (const CPath & p : explicitShares) {
			if (!isInsideRecursive(p)) {
				filtered.push_back(p);
			}
		}
		explicitShares.swap(filtered);
	}

	// Confirm before expanding sensitive recursive roots.
	std::vector<CPath> sensitive;
	for (const CPath & p : recursiveIntents) {
		if (IsSensitiveSharePath(p)) {
			sensitive.push_back(p);
		}
	}
	if (!sensitive.empty()) {
		wxString msg = _("The following folders look like system or sensitive locations:\n\n");
		for (const CPath & p : sensitive) {
			msg += "  " + p.GetPrintable() + "\n";
		}
		msg += "\n";
		msg += _("Sharing them recursively will publish every file underneath on the eD2k/Kad networks. Do you really want to do this?");
		const int rv = wxMessageBox(msg,
			_("Confirm shared folders"),
			wxICON_QUESTION | wxYES_NO | wxNO_DEFAULT, this);
		if (rv != wxYES) {
			return SharedDirsCommitResult::CancelledByUser;
		}
	}

	// Snapshot the current shared-dirs state so we can restore it
	// atomically if the user cancels at any phase (expansion or
	// reload). Cancel means "leave my saved state alone" — we do not
	// persist a half-committed list to disk. All three lists are
	// captured: the explicit/recursive intent + the runtime union,
	// since SaveSharedFolders persists all three together.
	const CDirectoryTreeCtrl::PathList originalShares =
		theApp->glob_prefs->shareddir_list;
	const CDirectoryTreeCtrl::PathList originalExplicit =
		theApp->glob_prefs->shareddir_explicit_list;
	const CDirectoryTreeCtrl::PathList originalRecursive =
		theApp->glob_prefs->shareddir_recursive_list;

	// One progress dialog covers both phases: the optional recursive
	// expansion, plus the always-present Reload phase. Even a single
	// double-click add of one folder still triggers a full Reload of
	// CSharedFileList, which on a large library (200k+ files) freezes
	// the UI for a couple of minutes — so the progress UI applies
	// regardless of whether expansion preceded it. The dialog
	// auto-hides on Update(100), so on small libraries where Reload
	// finishes in milliseconds it just flashes briefly.
	//
	// The initial body text reflects which phase will run first: if
	// there is a recursive intent we start in the expansion walk, if
	// not we go straight into the file-list Reload.
	const wxString initialBody = recursiveIntents.empty()
		? _("Reloading shared files…")
		: _("Scanning for subdirectories…");
	wxProgressDialog progress(_("Updating shared folders"),
		initialBody,
		/*maximum=*/100,
		this,
		wxPD_CAN_ABORT | wxPD_APP_MODAL | wxPD_AUTO_HIDE);

	CDirectoryTreeCtrl::PathList finalShares = explicitShares;

	// ----- Phase 1: optional recursive expansion ---------------------
	//
	// Pass `this` as the event owner so progress and done events flow
	// back into our event handlers below — keeping the GTK main loop
	// alive during the walk, which in turn keeps the Cancel button on
	// the progress dialog responsive. A pure polling loop with
	// wxMilliSleep+Yield works on Cocoa but starves GTK's event queue
	// and makes the whole UI feel frozen.
	if (!recursiveIntents.empty()) {
		CSharedDirsApplyTask task(explicitShares, recursiveIntents, this);
		if (task.Create() != wxTHREAD_NO_ERROR
			|| task.Run() != wxTHREAD_NO_ERROR)
		{
			// Worker couldn't start. Fall back to the explicit list
			// (no recursion) so the user at least gets the non-
			// recursive part of their selection saved.
			finalShares = explicitShares;
		} else {
			bool done = false;
			bool userCancelled = false;
			auto onProgress = [&](wxThreadEvent & ev) {
				const wxString status = CFormat(_("Scanned %u directories…"))
					% static_cast<unsigned>(ev.GetInt());
				if (!progress.Pulse(status)) {
					task.Cancel();   // wxThread::Delete joins the worker
					userCancelled = true;
					done = true;
				}
			};
			auto onDone = [&](wxThreadEvent & ev) {
				userCancelled = userCancelled || (ev.GetInt() != 0);
				done = true;
			};
			Bind(wxEVT_SHARED_DIRS_APPLY_PROGRESS, onProgress);
			Bind(wxEVT_SHARED_DIRS_APPLY_DONE,     onDone);

			while (!done) {
				wxTheApp->Yield(/*onlyIfNeeded=*/false);
			}

			Unbind(wxEVT_SHARED_DIRS_APPLY_PROGRESS, onProgress);
			Unbind(wxEVT_SHARED_DIRS_APPLY_DONE,     onDone);
			task.Wait();

			if (userCancelled || task.WasCancelled()) {
				// shareddir_list was never touched yet — nothing to
				// roll back.
				progress.Update(100);
				return SharedDirsCommitResult::CancelledByUser;
			}
			finalShares = task.GetExpandedShares();
		}
	}

	// ----- Phase 2: persist + reload --------------------------------
	//
	// Update all three canonical/derived lists on the Preferences
	// instance and persist them to disk *before* invoking Reload():
	// FindSharedFiles starts by calling ReloadSharedFolders() which
	// re-reads from disk, so the in-memory state alone isn't enough.
	// shareddir-explicit.dat and shareddir-recursive.dat record the
	// user's intent (non-recursive vs recursive roots); shareddir.dat
	// is regenerated as the runtime union (= finalShares from the
	// apply walk) so older binaries still see the right paths.
	theApp->glob_prefs->shareddir_explicit_list  = explicitShares;
	theApp->glob_prefs->shareddir_recursive_list = recursiveIntents;
	theApp->glob_prefs->shareddir_list           = finalShares;
	theApp->glob_prefs->SaveSharedFolders();

	bool reloadAborted = false;
	auto reloadYield = [&progress, &reloadAborted](size_t filesScanned) -> bool {
		const wxString msg = CFormat(_("Reloading shared files: %u files scanned"))
			% static_cast<unsigned>(filesScanned);
		if (!progress.Pulse(msg)) {
			reloadAborted = true;
			return false;
		}
		return true;
	};

	const bool reloadOk = theApp->sharedfiles->Reload(reloadYield);
	progress.Update(100);

	if (!reloadOk || reloadAborted) {
		// Roll back: both the in-memory state and the on-disk
		// files (shareddir.dat + shareddir-explicit.dat +
		// shareddir-recursive.dat) have to be reverted. The
		// in-memory shared-file map is partially populated against
		// the new list, so rebuild it from the restored list
		// (without a yield callback — the restored list is by
		// construction the one the user was already running with
		// before this OK click).
		theApp->glob_prefs->shareddir_list           = originalShares;
		theApp->glob_prefs->shareddir_explicit_list  = originalExplicit;
		theApp->glob_prefs->shareddir_recursive_list = originalRecursive;
		theApp->glob_prefs->SaveSharedFolders();
		theApp->sharedfiles->Reload(nullptr);
		return SharedDirsCommitResult::CancelledByUser;
	}

	return SharedDirsCommitResult::Committed;
}
// File_checked_for_headers

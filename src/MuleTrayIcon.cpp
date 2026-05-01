//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 Patrizio Bassi ( hetfield@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/app.h>

#include "MuleTrayIcon.h"

#include <common/ClientVersion.h>
#include <common/Constants.h>

#include "amule.h"			// Needed for theApp
#include "amuleDlg.h"		// Needed for IsShown
#include "Preferences.h"	// Needed for thePrefs
#include "ServerConnect.h"	// Needed for CServerConnect
#include "Server.h"			// Needed for CServer
#include "Statistics.h"		// Needed for theStats
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP
#include "OtherFunctions.h"	// Needed for CastItoXBytes / CastSecondsToHM
#include <common/Format.h>	// Needed for CFormat
#include <common/MenuIDs.h>	// Needed to access menu item constants


// =====================================================================
// Common action handlers — invoked from either backend.
// =====================================================================

void CMuleTrayIcon::DoConnectDisconnect()
{
	wxCommandEvent evt;
	theApp->amuledlg->OnBnConnect(evt);
}

void CMuleTrayIcon::DoShowHide()
{
	// Toggle main-window visibility. The pre-existing implementation
	// of this toggle in CMuleTrayIcon::ShowHide / SwitchShow called
	// Iconize() + Show() in sequence and re-read IsShown() between
	// them — on macOS that left the window half-minimized (a weird
	// mini-Dock tile) AND half-hidden, with the next Dock click only
	// un-hiding the frame and producing the "only the toolbar shows"
	// state. Iconize is for the green-button minimize-to-Dock UX, a
	// separate gesture from this hide-to-tray flow. Drop it: Show
	// (true/false) toggles visibility cleanly on every platform, and
	// hidden windows aren't in the OS taskbar regardless of Iconize
	// state, so non-Mac platforms see no behavioural change.
	if (theApp->amuledlg->IsShown()) {
		theApp->amuledlg->Show(false);
	} else {
		theApp->amuledlg->Show(true);
		theApp->amuledlg->Raise();
	}
#ifdef WITH_LIBAYATANA_APPINDICATOR
	// Refresh so the menu label flips to "Hide aMule" / "Show aMule"
	// to match the new window state. SNI menu is static between
	// state changes — without this poke the label would lag a click.
	RebuildMenu();
#endif
}

void CMuleTrayIcon::DoExit()
{
	if (theApp->amuledlg->IsEnabled()) {
		theApp->amuledlg->Close();
	}
}

void CMuleTrayIcon::DoSetUploadLimit(long kBytesPerSec)
{
	thePrefs::SetMaxUpload(kBytesPerSec < 0 ? UNLIMITED : (uint16)kBytesPerSec);
#ifdef CLIENT_GUI
	theApp->glob_prefs->SendToRemote();
#endif
}

void CMuleTrayIcon::DoSetDownloadLimit(long kBytesPerSec)
{
	thePrefs::SetMaxDownload(kBytesPerSec < 0 ? UNLIMITED : (uint16)kBytesPerSec);
#ifdef CLIENT_GUI
	theApp->glob_prefs->SendToRemote();
#endif
}


// =====================================================================
// Backend selection — see MuleTrayIcon.h for rationale.
// =====================================================================

#ifdef WITH_LIBAYATANA_APPINDICATOR

// ---------------------------------------------------------------------
//  StatusNotifierItem (SNI) backend via libayatana-appindicator3.
//
//  This is what GNOME Shell with the AppIndicators extension (Ubuntu's
//  default), KDE Plasma, Sway/Hyprland with waybar, and most other
//  modern Linux desktops actually render. The legacy GtkStatusIcon API
//  that wxTaskBarIcon talks was dropped in GNOME 3.26 (2017) and never
//  implemented in wlroots-based compositors, so without this backend
//  the tray icon is silently invisible on most current distros.
// ---------------------------------------------------------------------

#include <libayatana-appindicator/app-indicator.h>
#include <gtk/gtk.h>

namespace {

// All menu items reach the C++ side through this single callback. The
// item carries two int "action" + "arg" fields via g_object_set_data,
// so we don't need a separate static function per menu entry.
enum TrayAction {
	TRAY_ACTION_CONNECT_DISCONNECT = 1,
	TRAY_ACTION_SHOW_HIDE,
	TRAY_ACTION_EXIT,
	TRAY_ACTION_SET_UPLOAD_LIMIT,
	TRAY_ACTION_SET_DOWNLOAD_LIMIT,
};

void on_menu_item_activated(GtkMenuItem* item, gpointer user_data)
{
	CMuleTrayIcon* tray = static_cast<CMuleTrayIcon*>(user_data);
	intptr_t action = reinterpret_cast<intptr_t>(
		g_object_get_data(G_OBJECT(item), "action"));
	intptr_t arg = reinterpret_cast<intptr_t>(
		g_object_get_data(G_OBJECT(item), "arg"));

	switch (action) {
		case TRAY_ACTION_CONNECT_DISCONNECT: tray->DoConnectDisconnect(); break;
		case TRAY_ACTION_SHOW_HIDE:          tray->DoShowHide(); break;
		case TRAY_ACTION_EXIT:               tray->DoExit(); break;
		case TRAY_ACTION_SET_UPLOAD_LIMIT:   tray->DoSetUploadLimit((long)arg); break;
		case TRAY_ACTION_SET_DOWNLOAD_LIMIT: tray->DoSetDownloadLimit((long)arg); break;
	}
}

GtkWidget* make_action_item(const char* label, TrayAction action,
                            long arg, gpointer user_data)
{
	GtkWidget* item = gtk_menu_item_new_with_label(label);
	g_object_set_data(G_OBJECT(item), "action",
		reinterpret_cast<gpointer>(static_cast<intptr_t>(action)));
	g_object_set_data(G_OBJECT(item), "arg",
		reinterpret_cast<gpointer>(static_cast<intptr_t>(arg)));
	g_signal_connect(item, "activate",
		G_CALLBACK(on_menu_item_activated), user_data);
	return item;
}

GtkWidget* make_speed_submenu(uint32 max_speed, TrayAction action,
                              gpointer user_data)
{
	GtkWidget* submenu = gtk_menu_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu),
		make_action_item("Unlimited", action, -1, user_data));
	for (int i = 0; i < 5; i++) {
		unsigned int spd = (unsigned int)((double)max_speed / 5) * (5 - i);
		char label[64];
		g_snprintf(label, sizeof(label), "%u kB/s", spd);
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu),
			make_action_item(label, action, (long)spd, user_data));
	}
	gtk_widget_show_all(submenu);
	return submenu;
}

// Append a non-clickable "info" label to the menu. SNI menus support
// disabled items, but rendering varies between desktops — KDE shows
// them grey, GNOME-with-AppIndicators shows them in the menu's normal
// style. Either way they're not interactive.
void append_info(GtkWidget* menu, const wxString& text)
{
	GtkWidget* item = gtk_menu_item_new_with_label(
		(const char*)text.utf8_str());
	gtk_widget_set_sensitive(item, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

} // anonymous namespace


CMuleTrayIcon::CMuleTrayIcon()
	: m_indicator(nullptr)
	, m_menu(nullptr)
	, m_lastIconState(-1)
{
	// `org.amule.aMule` is both the AppStream/.desktop id and the icon
	// name installed under share/icons/hicolor/*/apps/. AppIndicator3
	// looks the icon up via the standard XDG icon-theme path.
	m_indicator = app_indicator_new(
		"org.amule.aMule",
		"org.amule.aMule",
		APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

	// ACTIVE = visible. The user already opted in by enabling the tray
	// icon in Preferences, so showing it immediately is the expected
	// behaviour; SetTrayIcon below only updates the menu's
	// Connect/Disconnect label and never re-hides the indicator.
	// (We deliberately don't use APP_INDICATOR_STATUS_ATTENTION for
	// the disconnected state — that requires a separately-set
	// attention icon via app_indicator_set_attention_icon_full(), and
	// without it some SNI hosts render the indicator as invisible.)
	app_indicator_set_status(m_indicator, APP_INDICATOR_STATUS_ACTIVE);
	app_indicator_set_title(m_indicator, "aMule");

	RebuildMenu();
}

CMuleTrayIcon::~CMuleTrayIcon()
{
	if (m_indicator) {
		g_object_unref(m_indicator);
		m_indicator = nullptr;
	}
	// m_menu is owned by the indicator (set_menu took the floating ref)
	// so we don't unref it explicitly.
}

void CMuleTrayIcon::SetTrayIcon(int Icon, uint32 /*percent*/)
{
	// SNI doesn't support per-frame icon overlays — the percent bar
	// from the legacy backend is dropped on this path. We reflect the
	// connection state purely through the menu (the Connect /
	// Disconnect item label flips), not via the indicator's status,
	// because flipping ACTIVE↔ATTENTION can hide the indicator on
	// some hosts when no attention icon is configured.
	if (Icon != m_lastIconState) {
		m_lastIconState = Icon;
		// Rebuild so Connect/Disconnect label reflects current state.
		RebuildMenu();
	}
}

void CMuleTrayIcon::SetTrayToolTip(const wxString& Tip)
{
	// SNI doesn't surface tooltips on hover (compositors disagree on
	// whether to render them). Use it as the accessible title — screen
	// readers and KDE's hover popup pick it up.
	app_indicator_set_title(m_indicator, (const char*)Tip.utf8_str());
}

void CMuleTrayIcon::RebuildMenu()
{
	// Static layout, rebuilt only on connection-state changes
	// (driven by SetTrayIcon below). app_indicator_set_menu posts a
	// dbusmenu LayoutUpdated D-Bus signal which some SNI hosts react
	// to with a brief icon redraw — so refreshing on a 2 s timer
	// would visibly flicker. Keeping the menu lean (action items
	// only, no live stats) means we rebuild only when state actually
	// changes, eliminating the flicker. Live values like download /
	// upload speed are visible in the main aMule window.
	GtkWidget* menu = gtk_menu_new();

	// ---- Version banner ------------------------------------------
	append_info(menu, MOD_VERSION_LONG);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_separator_menu_item_new());

	// ---- Client information submenu ------------------------------
	// Snapshot at the moment of the last connection-state change.
	// Skips truly-live fields (uptime, totals, queued clients) so the
	// menu can stay static between state changes — putting those in
	// would force a periodic rebuild and bring back the flicker.
	{
		GtkWidget* sub = gtk_menu_new();

		// ED2k status
		{
			wxString s = _("eD2k: ");
			if (theApp->IsConnectedED2K()) {
				s += theApp->IsFirewalled()
					? wxString(_("Connected (LowID)"))
					: wxString(_("Connected (HighID)"));
			} else {
				s += _("Disconnected");
			}
			append_info(sub, s);
		}

		// Kad status
		{
			wxString s = _("Kad: ");
			if (theApp->IsConnectedKad()) {
				s += theApp->IsFirewalledKad()
					? wxString(_("Connected (firewalled)"))
					: wxString(_("Connected"));
			} else {
				s += _("Disconnected");
			}
			append_info(sub, s);
		}

		// Server identity (only meaningful while connected)
		{
			wxString name = _("Server: ");
			wxString ip   = _("Server IP: ");
			if (theApp->serverconnect->GetCurrentServer()) {
				name += theApp->serverconnect->GetCurrentServer()->GetListName();
				ip   += theApp->serverconnect->GetCurrentServer()->GetFullIP();
			} else {
				name += _("Not connected");
				ip   += _("Not connected");
			}
			append_info(sub, name);
			append_info(sub, ip);
		}

		// Public IP — populated post-connect
		append_info(sub, CFormat(_("IP: %s"))
			% (theApp->GetPublicIP()
				? Uint32toStringIP(theApp->GetPublicIP())
				: wxString(_("Unknown"))));

		// Listen ports — change only on prefs save
		append_info(sub, thePrefs::GetPort()
			? wxString(CFormat(_("TCP port: %d")) % thePrefs::GetPort())
			: wxString(_("TCP port: Not ready")));

		append_info(sub, thePrefs::GetEffectiveUDPPort()
			? wxString(CFormat(_("UDP port: %d")) % thePrefs::GetEffectiveUDPPort())
			: wxString(_("UDP port: Not ready")));

		gtk_widget_show_all(sub);
		GtkWidget* item = gtk_menu_item_new_with_label(
			(const char*)wxString(_("Client Information")).utf8_str());
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_separator_menu_item_new());

	// ---- Action items --------------------------------------------

	// Upload limit submenu
	{
		uint32 max_ul = thePrefs::GetMaxGraphUploadRate();
		if (max_ul == UNLIMITED) max_ul = 100;
		else if (max_ul < 10)    max_ul = 10;

		GtkWidget* sub = make_speed_submenu(max_ul,
			TRAY_ACTION_SET_UPLOAD_LIMIT, this);
		GtkWidget* item = gtk_menu_item_new_with_label(_("Upload limit").utf8_str());
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	// Download limit submenu
	{
		uint32 max_dl = thePrefs::GetMaxGraphDownloadRate();
		if (max_dl == UNLIMITED) max_dl = 100;
		else if (max_dl < 10)    max_dl = 10;

		GtkWidget* sub = make_speed_submenu(max_dl,
			TRAY_ACTION_SET_DOWNLOAD_LIMIT, this);
		GtkWidget* item = gtk_menu_item_new_with_label(_("Download limit").utf8_str());
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_separator_menu_item_new());

	// Connect / Disconnect — label depends on current connection state.
	{
		const wxString label = theApp->IsConnected()
			? wxString(_("Disconnect")) : wxString(_("Connect"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			make_action_item((const char*)label.utf8_str(),
				TRAY_ACTION_CONNECT_DISCONNECT, 0, this));
	}

	// Show / Hide — label depends on whether the main window is visible.
	{
		const bool shown = theApp->amuledlg && theApp->amuledlg->IsShown();
		const wxString label = shown ? wxString(_("Hide aMule"))
		                             : wxString(_("Show aMule"));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
			make_action_item((const char*)label.utf8_str(),
				TRAY_ACTION_SHOW_HIDE, 0, this));
	}

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		gtk_separator_menu_item_new());

	gtk_menu_shell_append(GTK_MENU_SHELL(menu),
		make_action_item((const char*)wxString(_("Exit")).utf8_str(),
			TRAY_ACTION_EXIT, 0, this));

	gtk_widget_show_all(menu);

	// app_indicator_set_menu sinks the floating ref and unrefs any
	// previously-set menu, so we can replace it on every rebuild
	// without leaking.
	app_indicator_set_menu(m_indicator, GTK_MENU(menu));
	m_menu = menu;
}


#else  // !WITH_LIBAYATANA_APPINDICATOR

// ---------------------------------------------------------------------
//  Legacy wxTaskBarIcon backend. Works correctly on Windows
//  (NOTIFYICONDATA), macOS (NSStatusItem), and on X11 desktops that
//  still consume GtkStatusIcon. On modern Wayland desktops the icon
//  goes nowhere — build with libayatana-appindicator3 to fix that.
// ---------------------------------------------------------------------

#include "pixmaps/mule_TrayIcon_big.ico.xpm"
#include "pixmaps/mule_Tr_yellow_big.ico.xpm"
#include "pixmaps/mule_Tr_grey_big.ico.xpm"

#include <wx/menu.h>

#include "StatisticsDlg.h"	// Needed for CStatisticsDlg::getColors()


/****************************************************/
/******************* Event Table ********************/
/****************************************************/

wxBEGIN_EVENT_TABLE(CMuleTrayIcon, wxTaskBarIcon)
	EVT_TASKBAR_LEFT_DCLICK(CMuleTrayIcon::SwitchShow)
	EVT_MENU( TRAY_MENU_EXIT, CMuleTrayIcon::Close)
	EVT_MENU( TRAY_MENU_CONNECT, CMuleTrayIcon::ServerConnection)
	EVT_MENU( TRAY_MENU_DISCONNECT, CMuleTrayIcon::ServerConnection)
	EVT_MENU( TRAY_MENU_HIDE, CMuleTrayIcon::ShowHide)
	EVT_MENU( TRAY_MENU_SHOW, CMuleTrayIcon::ShowHide)
	EVT_MENU( UPLOAD_ITEM1, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM2, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM3, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM4, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM5, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM6, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( DOWNLOAD_ITEM1, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM2, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM3, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM4, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM5, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM6, CMuleTrayIcon::SetDownloadSpeed)
wxEND_EVENT_TABLE()


/****************************************************/
/************ Constructor / Destructor **************/
/****************************************************/

static long GetSpeedFromString(wxString label){
	long temp;
	label.Replace(_("kB/s"),"",TRUE);
	label.Trim(FALSE);
	label.Trim(TRUE);
	label.ToLong(&temp);
	return temp;
}

void CMuleTrayIcon::SetUploadSpeed(wxCommandEvent& event){

	wxObject* obj=event.GetEventObject();
	if (obj!=NULL) {
		wxMenu *menu = dynamic_cast<wxMenu *>(obj);
		if (menu) {
			wxMenuItem* item=menu->FindItem(event.GetId());
			if (item!=NULL) {
				if (item->GetItemLabelText()==(_("Unlimited"))) {
					DoSetUploadLimit(-1);
				}
				else {
					DoSetUploadLimit(GetSpeedFromString(item->GetItemLabelText()));
				}
			}
		}
	}
}

void CMuleTrayIcon::SetDownloadSpeed(wxCommandEvent& event){

	wxObject* obj=event.GetEventObject();
	if (obj!=NULL) {
		wxMenu *menu = dynamic_cast<wxMenu *>(obj);
		if (menu) {
			wxMenuItem* item=menu->FindItem(event.GetId());
			if (item!=NULL) {
				if (item->GetItemLabelText()==(_("Unlimited"))) {
					DoSetDownloadLimit(-1);
				}
				else {
					DoSetDownloadLimit(GetSpeedFromString(item->GetItemLabelText()));
				}
			}
		}
	}
}


void CMuleTrayIcon::ServerConnection(wxCommandEvent& WXUNUSED(event))
{
	DoConnectDisconnect();
}


void CMuleTrayIcon::ShowHide(wxCommandEvent& WXUNUSED(event))
{
	DoShowHide();
}


void CMuleTrayIcon::Close(wxCommandEvent& WXUNUSED(event))
{
	DoExit();
}


CMuleTrayIcon::CMuleTrayIcon()
{
	Old_Icon = -1;
	Old_SpeedSize = 0xFFFF; // must be > any possible one.

	// Create the background icons (speed improvement)
	HighId_Icon_size = wxIcon(mule_TrayIcon_big_ico_xpm).GetHeight();
	LowId_Icon_size = wxIcon(mule_Tr_yellow_big_ico_xpm).GetHeight();
	Disconnected_Icon_size = wxIcon(mule_Tr_grey_big_ico_xpm).GetHeight();
}

CMuleTrayIcon::~CMuleTrayIcon()
{
}


/****************************************************/
/***************** Public Functions *****************/
/****************************************************/

void CMuleTrayIcon::SetTrayIcon(int Icon, uint32 percent)
{
	int Bar_ySize = 0;

	switch (Icon) {
		case TRAY_ICON_HIGHID:
			// Most likely case, test first
			Bar_ySize = HighId_Icon_size;
			break;
		case TRAY_ICON_LOWID:
			Bar_ySize = LowId_Icon_size;
			break;
		case TRAY_ICON_DISCONNECTED:
			Bar_ySize = Disconnected_Icon_size;
			break;
		default:
			wxFAIL;
	}

	// Lookup this values for speed improvement: don't draw if not needed
	int NewSize = (Bar_ySize * percent) / 100;

	if ((Old_Icon != Icon) || (Old_SpeedSize != NewSize)) {

		if ((Old_SpeedSize > NewSize) || (Old_Icon != Icon)) {
			// We have to rebuild the icon, because bar is lower now.
			switch (Icon) {
				case TRAY_ICON_HIGHID:
					// Most likely case, test first
					CurrentIcon = wxIcon(mule_TrayIcon_big_ico_xpm);
					break;
				case TRAY_ICON_LOWID:
					CurrentIcon = wxIcon(mule_Tr_yellow_big_ico_xpm);
					break;
				case TRAY_ICON_DISCONNECTED:
					CurrentIcon = wxIcon(mule_Tr_grey_big_ico_xpm);
					break;
				default:
					wxFAIL;
			}
		}

		Old_Icon = Icon;
		Old_SpeedSize = NewSize;

		// Do whatever to the icon before drawing it (percent)

		wxBitmap TempBMP;
		TempBMP.CopyFromIcon(CurrentIcon);

		TempBMP.SetMask(NULL);

		IconWithSpeed.SelectObject(TempBMP);


		// Speed bar is: centered, taking 80% of the icon height, and
		// right-justified taking a 10% of the icon width.

		// X
		int Bar_xSize = 4;
		int Bar_xPos = CurrentIcon.GetWidth() - 5;

		IconWithSpeed.SetBrush(*(wxTheBrushList->FindOrCreateBrush(CStatisticsDlg::getColors(11))));
		IconWithSpeed.SetPen(*wxTRANSPARENT_PEN);

		IconWithSpeed.DrawRectangle(Bar_xPos + 1, Bar_ySize - NewSize, Bar_xSize -2 , NewSize);

		// Unselect the icon.
		IconWithSpeed.SelectObject(wxNullBitmap);

		// Do transparency

		// Set a new mask with transparency set to red.
		wxMask* new_mask = new wxMask(TempBMP, wxColour(0xFF, 0x00, 0x00));

		TempBMP.SetMask(new_mask);
		CurrentIcon.CopyFromBitmap(TempBMP);

		UpdateTray();
	}
}

void CMuleTrayIcon::SetTrayToolTip(const wxString& Tip)
{
	CurrentTip = Tip;
	UpdateTray();
}


/****************************************************/
/**************** Private Functions *****************/
/****************************************************/

void CMuleTrayIcon::UpdateTray()
{
	// Icon update and Tip update
#ifndef __WXCOCOA__
	if (IsOk())
#endif
	{
		SetIcon(CurrentIcon, CurrentTip);
	}
}


wxMenu* CMuleTrayIcon::CreatePopupMenu()
{
	float kBpsUp = theStats::GetUploadRate() / 1024.0;
	float kBpsDown = theStats::GetDownloadRate() / 1024.0;
	float MBpsUp = kBpsUp / 1024.0;
	float MBpsDown = kBpsDown / 1024.0;
	bool showMBpsUp = (MBpsUp >= 1);
	bool showMBpsDown = (MBpsDown >= 1);

	// Dynamically creates the menu to show the user.
	wxMenu *traymenu = new wxMenu();
	traymenu->SetTitle(_("aMule Tray Menu"));

	// Build the Top string name
	wxString label = MOD_VERSION_LONG;
	traymenu->Append(TRAY_MENU_INFO, label);
	traymenu->AppendSeparator();
	label = wxString(_("Speed limits:")) + " ";

	// Check for upload limits
	unsigned int max_upload = thePrefs::GetMaxUpload();
	if ( max_upload == UNLIMITED ) {
		label += _("UL: None");
	}
	else {
		label += CFormat(_("UL: %u")) % max_upload;
	}
	label += ", ";

	// Check for download limits
	unsigned int max_download = thePrefs::GetMaxDownload();
	if ( max_download == UNLIMITED ) {
		label += _("DL: None");
	}
	else {
		label += CFormat(_("DL: %u")) % max_download;
	}

	traymenu->Append(TRAY_MENU_INFO, label);
	label = CFormat(_("Download speed: %.1f%s"))
			% (showMBpsDown ? MBpsDown : kBpsDown) % (showMBpsDown ? _(" MB/s") : ((kBpsDown > 0) ? _(" kB/s") : ""));
	traymenu->Append(TRAY_MENU_INFO, label);
	label = CFormat(_("Upload speed: %.1f%s"))
			% (showMBpsUp ? MBpsUp : kBpsUp) % (showMBpsUp ? _(" MB/s") : ((kBpsUp > 0) ? _(" kB/s") : ""));
	traymenu->Append(TRAY_MENU_INFO, label);
	traymenu->AppendSeparator();

	// Client Info
	wxMenu* ClientInfoMenu = new wxMenu();
	ClientInfoMenu->SetTitle(_("Client Information"));

	// User nick-name
	{
		wxString temp = CFormat(_("Nickname: %s")) % ( thePrefs::GetUserNick().IsEmpty() ? wxString(_("No Nickname Selected!")) : thePrefs::GetUserNick() );

		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Client ID
	{
		wxString temp = _("ClientID: ");

		if (theApp->IsConnectedED2K()) {
			temp += CFormat("%u") % theApp->GetED2KID();
		} else {
			temp += _("Not connected");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Current Server and Server IP
	{
		wxString temp_name = _("ServerName: ");
		wxString temp_ip   = _("ServerIP: ");

		if ( theApp->serverconnect->GetCurrentServer() ) {
			temp_name += theApp->serverconnect->GetCurrentServer()->GetListName();
			temp_ip   += theApp->serverconnect->GetCurrentServer()->GetFullIP();
		} else {
			temp_name += _("Not connected");
			temp_ip   += _("Not Connected");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp_name);
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp_ip);
	}

	// IP Address
	{
		wxString temp = CFormat(_("IP: %s")) % ( (theApp->GetPublicIP()) ? Uint32toStringIP(theApp->GetPublicIP()) : wxString(_("Unknown")) );

		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// TCP PORT
	{
		wxString temp;
		if (thePrefs::GetPort()) {
			temp = CFormat(_("TCP port: %d")) % thePrefs::GetPort();
		} else {
			temp=_("TCP port: Not ready");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// UDP PORT
	{
		wxString temp;
		if (thePrefs::GetEffectiveUDPPort()) {
			temp = CFormat(_("UDP port: %d")) % thePrefs::GetEffectiveUDPPort();
		} else {
			temp=_("UDP port: Not ready");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Online Signature
	{
		wxString temp;
		if (thePrefs::IsOnlineSignatureEnabled()) {
			temp=_("Online Signature: Enabled");
		}
		else {
			temp=_("Online Signature: Disabled");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Uptime
	{
		wxString temp = CFormat(_("Uptime: %s")) % CastSecondsToHM(theStats::GetUptimeSeconds());
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Number of shared files
	{
		wxString temp = CFormat(_("Shared files: %d")) % theStats::GetSharedFileCount();
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Number of queued clients
	{
		wxString temp = CFormat(_("Queued clients: %d")) % theStats::GetWaitingUserCount();
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Total Downloaded
	{
		wxString temp = CastItoXBytes(theStats::GetTotalReceivedBytes());
		temp = CFormat(_("Total DL: %s")) % temp;
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Total Uploaded
	{
		wxString temp = CastItoXBytes(theStats::GetTotalSentBytes());
		temp = CFormat(_("Total UL: %s")) % temp;
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	traymenu->Append(TRAY_MENU_CLIENTINFO,ClientInfoMenu->GetTitle(),ClientInfoMenu);

	// Separator
	traymenu->AppendSeparator();

	// Upload Speed sub-menu
	wxMenu* UploadSpeedMenu = new wxMenu();
	UploadSpeedMenu->SetTitle(_("Upload limit"));

	// Download Speed sub-menu
	wxMenu* DownloadSpeedMenu = new wxMenu();
	DownloadSpeedMenu->SetTitle(_("Download limit"));

	// Upload Speed sub-menu
	{
		UploadSpeedMenu->Append(UPLOAD_ITEM1, _("Unlimited"));

		uint32 max_ul_speed = thePrefs::GetMaxGraphUploadRate();

		if ( max_ul_speed == UNLIMITED ) {
			max_ul_speed = 100;
		}
		else if ( max_ul_speed < 10 ) {
			max_ul_speed = 10;
		}

		for ( int i = 0; i < 5; i++ ) {
			unsigned int tempspeed = (unsigned int)((double)max_ul_speed / 5) * (5 - i);
			wxString temp = CFormat("%u %s") % tempspeed % _("kB/s");
			UploadSpeedMenu->Append((int)UPLOAD_ITEM1+i+1,temp);
		}
	}
	traymenu->Append(0,UploadSpeedMenu->GetTitle(),UploadSpeedMenu);

	// Download Speed sub-menu
	{
		DownloadSpeedMenu->Append(DOWNLOAD_ITEM1, _("Unlimited"));

		uint32 max_dl_speed = thePrefs::GetMaxGraphDownloadRate();

		if ( max_dl_speed == UNLIMITED ) {
			max_dl_speed = 100;
		}
		else if ( max_dl_speed < 10 ) {
			max_dl_speed = 10;
		}

		for ( int i = 0; i < 5; i++ ) {
			unsigned int tempspeed = (unsigned int)((double)max_dl_speed / 5) * (5 - i);
			wxString temp = CFormat("%d %s") % tempspeed % _("kB/s");
			DownloadSpeedMenu->Append((int)DOWNLOAD_ITEM1+i+1,temp);
		}
	}

	traymenu->Append(0,DownloadSpeedMenu->GetTitle(),DownloadSpeedMenu);
	// Separator
	traymenu->AppendSeparator();

	if (theApp->IsConnected()) {
		//Disconnection Speed item
		traymenu->Append(TRAY_MENU_DISCONNECT, _("Disconnect"));
	} else {
		//Connect item
		traymenu->Append(TRAY_MENU_CONNECT, _("Connect"));
	}

	// Separator
	traymenu->AppendSeparator();

	if (theApp->amuledlg->IsShown()) {
		//hide item
		traymenu->Append(TRAY_MENU_HIDE, _("Hide aMule"));
	} else {
		//show item
		traymenu->Append(TRAY_MENU_SHOW, _("Show aMule"));
	}

	// Separator
	traymenu->AppendSeparator();

	// Exit item
	traymenu->Append(TRAY_MENU_EXIT, _("Exit"));

	return traymenu;
}

void CMuleTrayIcon::SwitchShow(wxTaskBarIconEvent&)
{
	DoShowHide();
}

#endif  // !WITH_LIBAYATANA_APPINDICATOR

// File_checked_for_headers

//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
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

#ifndef MULETRAYICON_H
#define MULETRAYICON_H

#ifndef AMULE_DAEMON

#include "config.h"

enum TaskbarNotifier
{
	TBN_NULL = 0,
	TBN_CHAT,
	TBN_DLOAD,
	TBN_LOG,
	TBN_IMPORTANTEVENT,
	TBN_NEWVERSION
};

#include "Types.h"	// Needed for uint32

class wxString;

enum {
	TRAY_ICON_DISCONNECTED,
	TRAY_ICON_LOWID,
	TRAY_ICON_HIGHID
};

// Backend selection:
//
// Linux with libayatana-appindicator3 → SNI (StatusNotifierItem)
//   backend that talks D-Bus directly. This is what GNOME Shell (with
//   the AppIndicators extension Ubuntu ships by default), KDE Plasma,
//   Sway/wlroots, and every other modern desktop actually consume.
//
// Everywhere else (Windows, macOS, Linux build without the dep) →
//   wxTaskBarIcon, which on those platforms hits a working native API
//   (Win32 NOTIFYICONDATA, NSStatusItem). On Linux without the dep
//   wxTaskBarIcon falls through to the legacy GtkStatusIcon API which
//   GNOME 3.26+ silently dropped — distros really should ship the dep.
#ifdef WITH_LIBAYATANA_APPINDICATOR
struct _AppIndicator;
struct _GtkWidget;
typedef struct _AppIndicator AppIndicator;
typedef struct _GtkWidget GtkWidget;
#else
#include <wx/taskbar.h>
#include <wx/icon.h>
#include <wx/dcmemory.h>
class wxMenu;
#endif


/**
 * The mule tray icon class is responsible for drawing the mule systray icon
 * and reacting to the user input on it.
 */
class CMuleTrayIcon
#ifndef WITH_LIBAYATANA_APPINDICATOR
	: public wxTaskBarIcon
#endif
{
public:
	CMuleTrayIcon();
	~CMuleTrayIcon();

	/**
	 * Set the Tray icon.
	 * @param Icon  TRAY_ICON_HIGHID / LOWID / DISCONNECTED
	 * @param percent  download-speed bar percentage (legacy backend only;
	 *                 ignored by the SNI backend, which switches between
	 *                 three static state icons instead)
	 */
	void SetTrayIcon(int Icon, uint32 percent);

	/**
	 * Set the Tray tooltip.
	 */
	void SetTrayToolTip(const wxString& Tip);

	// Action handlers — invoked by the GTK menu (Ayatana backend) or by
	// the wxMenu event table (wxTaskBarIcon backend).
	void DoConnectDisconnect();
	void DoShowHide();
	void DoExit();
	void DoSetUploadLimit(long kBytesPerSec);   // UNLIMITED for no cap
	void DoSetDownloadLimit(long kBytesPerSec);

private:

#ifdef WITH_LIBAYATANA_APPINDICATOR
	AppIndicator* m_indicator;
	GtkWidget*    m_menu;
	int           m_lastIconState;

	void          RebuildMenu();
#else
	virtual wxMenu* CreatePopupMenu();
	void UpdateTray();

	void SwitchShow(wxTaskBarIconEvent&);
	void SetUploadSpeed(wxCommandEvent&);
	void SetDownloadSpeed(wxCommandEvent&);
	void ServerConnection(wxCommandEvent&);
	void ShowHide(wxCommandEvent&);
	void Close(wxCommandEvent&);

	int Old_Icon;
	int Old_SpeedSize;

	int Disconnected_Icon_size;
	int LowId_Icon_size;
	int HighId_Icon_size;

	wxIcon CurrentIcon;
	wxMemoryDC IconWithSpeed;
	wxString CurrentTip;

	wxDECLARE_EVENT_TABLE();
#endif
};

#endif // DAEMON

#endif //MULETRAYICON_H
// File_checked_for_headers

// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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
//

#ifndef MULETRAYICON_H
#define MULETRAYICON_H

#ifndef AMULE_DAEMON

#include <wx/defs.h>	// Needed before any other wx/*.h

#if !wxCHECK_VERSION(2, 5, 3)
 #define USE_WX_TRAY 0
#endif

#ifdef USE_WX_TRAY 

#include <wx/taskbar.h>
#include <wx/icon.h>
#include <wx/dcmemory.h>

#include "types.h"	// Needed for uint32

class wxString;
class wxMenu;

enum {
	TRAY_ICON_DISCONNECTED,
	TRAY_ICON_LOWID,
	TRAY_ICON_HIGHID
};

/**
 * The mule tray icon class is responsible for drawing the mule systray icon 
 * and reacting to the user input on it.
 */
class CMuleTrayIcon : public wxTaskBarIcon {

	public:
		/**
		 * Contructor.
		 */
		CMuleTrayIcon();
	
		/**
		 * Destructor.
		 */
		~CMuleTrayIcon();
	
		/**
		 * Set the Tray icon. 
		 * @param Icon The wxIcon object with the new tray icon
		 */
		void SetTrayIcon(int Icon, uint32 percent);
		
		/**
		 * Set the Tray tooltip
		 * @param Tip The wxString object with the new tray tooltip
		 */
		void SetTrayToolTip(const wxString& Tip);
		
	private:

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
	
		wxIcon Disconnected_Icon;
		wxIcon LowId_Icon;
		wxIcon HighId_Icon;
	
		wxIcon CurrentIcon;
		wxMemoryDC IconWithSpeed;
		wxString CurrentTip;
	
		DECLARE_EVENT_TABLE()
};

#endif // USE_WX_TRAY

#endif // DAEMON

#endif //MULETRAYICON_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


enum TaskbarNotifier
{
	TBN_NULL = 0,
	TBN_CHAT,
	TBN_DLOAD,
	TBN_LOG,
	TBN_IMPORTANTEVENT,
	TBN_NEWVERSION
};

#include <wx/taskbar.h>
#include <wx/icon.h>
#include <wx/dcmemory.h>

#include "Types.h"	// Needed for uint32

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
	
		int Disconnected_Icon_size;
		int LowId_Icon_size;
		int HighId_Icon_size;
	
		wxIcon CurrentIcon;
		wxMemoryDC IconWithSpeed;
		wxString CurrentTip;
	
		DECLARE_EVENT_TABLE()
};

#endif // DAEMON

#endif //MULETRAYICON_H
// File_checked_for_headers

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef COMMONMENUIDS_H
#define COMMONMENUIDS_H

#include <wx/defs.h>	// To access wxID_HIGHEST menu item constant

enum {
	MP_MESSAGE = wxID_HIGHEST + 1, // Start ID at next free value (defined by wx)
	MP_DETAIL,
	MP_ADDFRIEND,
	MP_REMOVEFRIEND,
	MP_SHOWLIST,
	MP_FRIENDSLOT,
	MP_CHANGE2FILE,
	MP_CANCEL,
	MP_STOP,
	MP_RESUME,
	MP_PAUSE,
	MP_CLEARCOMPLETED,
	MP_VIEW,
	MP_SENDMESSAGE,
	MP_WS,
	MP_RAZORSTATS,
	MP_ADDCOLLECTION,
	MP_GETCOMMENTS,
	MP_SEARCHRELATED,
	MP_MARK_AS_KNOWN,
//For comments
	MP_CMT,

	MP_PRIOVERYLOW,
	MP_PRIOLOW,
	MP_PRIONORMAL,
	MP_PRIOHIGH,
	MP_PRIOVERYHIGH,
	MP_POWERSHARE,
	MP_PRIOAUTO,
	MP_GETMAGNETLINK,
	MP_GETED2KLINK,
	MP_GETSOURCEED2KLINK,
	MP_GETCRYPTSOURCEDED2KLINK,
	MP_GETHOSTNAMESOURCEED2KLINK,
	MP_GETHOSTNAMECRYPTSOURCEED2KLINK,
	MP_GETAICHED2KLINK,
	MP_GETAICHED2KLINKSRC,
	MP_METINFO,
	MP_CONNECTTO,
	MP_REMOVE,
	MP_REMOVEALL,
	MP_UNBAN,
	MP_ADDTOSTATIC,
	MP_REMOVEFROMSTATIC,
	MP_VIEWFILECOMMENTS,
	MP_CAT_ADD,
	MP_CAT_EDIT,
	MP_CAT_REMOVE,
	MP_TOOGLELIST,
	MP_CLOSE_TAB,
	MP_CLOSE_ALL_TABS,
	MP_CLOSE_OTHER_TABS,
	MP_RENAME,

/* Razor 1a - Modif by MikaelB
     Opcodes for :
      - Drop No Needed Sources now
      - Drop Full Queue Sources now
      - Drop High Queue Rating Sources now
      - Clean Up Sources now ( drop NNS, FQS and HQRS )
      - Swap every A4AF to this file now
      - Swap every A4AF to this file ( AUTO )
      - Swap every A4AF to any other file now   */
	MP_DROP_NO_NEEDED_SOURCES,
	MP_DROP_FULL_QUEUE_SOURCES,
	MP_DROP_HIGH_QUEUE_RATING_SOURCES,
	MP_CLEAN_UP_SOURCES,
	MP_SWAP_A4AF_TO_THIS,
	MP_SWAP_A4AF_TO_THIS_AUTO,
	MP_SWAP_A4AF_TO_ANY_OTHER,

//menus
	MP_MENU_PRIO,
	MP_MENU_EXTD,
	MP_MENU_CATS,

// CMuleListCtrl tabs.
	MP_LISTCOL_1,
	MP_LISTCOL_2,
	MP_LISTCOL_3,
	MP_LISTCOL_4,
	MP_LISTCOL_5,
	MP_LISTCOL_6,
	MP_LISTCOL_7,
	MP_LISTCOL_8,
	MP_LISTCOL_9,
	MP_LISTCOL_10,
	MP_LISTCOL_11,
	MP_LISTCOL_12,
	MP_LISTCOL_13,
	MP_LISTCOL_14,
	MP_LISTCOL_15,

	MP_ASSIGNCAT	= MP_LISTCOL_15   + 1,   // reserve some for categories (about 100)
	MP_CAT_SET0		= MP_ASSIGNCAT    + 100, // reserve some for change all-cats (about 50)
	MP_SWITCHCTRL_0 = MP_CAT_SET0     + 50,
	MP_SWITCHCTRL_9	= MP_SWITCHCTRL_0 + 9,

// Pop-up menu clickable entries
	TRAY_MENU_INFO = 0,
	TRAY_MENU_CLIENTINFO = 0,
	TRAY_MENU_CLIENTINFO_ITEM = MP_SWITCHCTRL_9 + 1,  // continue from MP_SWITCHCTRL_9
	TRAY_MENU_DISCONNECT,
	TRAY_MENU_CONNECT,
	TRAY_MENU_HIDE,
	TRAY_MENU_SHOW,
	TRAY_MENU_EXIT,
	UPLOAD_ITEM1,
	UPLOAD_ITEM2,
	UPLOAD_ITEM3,
	UPLOAD_ITEM4,
	UPLOAD_ITEM5,
	UPLOAD_ITEM6,
	DOWNLOAD_ITEM1,
	DOWNLOAD_ITEM2,
	DOWNLOAD_ITEM3,
	DOWNLOAD_ITEM4,
	DOWNLOAD_ITEM5,
	DOWNLOAD_ITEM6
};

#endif // COMMONMENUIDS_H

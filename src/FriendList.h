// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <wx/list.h>		// Needed for WX_DECLARE_LIST

#include "types.h"		// Needed for uint16 and uint32
#include "CTypedPtrList.h"  // Needed for list.

class CFriend;
class CFriendListCtrl;
class CUpDownClient;



class CFriendList {
public:
	CFriendList();
	~CFriendList();

	bool		IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort ); 
	void		SaveList();
	bool		LoadList();
	void		RefreshFriend(CFriend* torefresh);
	void		SetWindow(CFriendListCtrl* NewWnd)	{m_wndOutput = NewWnd;}
	void		ShowFriends();
	CFriend*	SearchFriend(const uchar* achUserHash, uint32 dwIP, uint16 nPort) const;	
	void		AddFriend(CUpDownClient* toadd);
	void		RemoveFriend(CUpDownClient* todel);
	void		AddFriend( unsigned char tm_abyUserhash[16], uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, wxString tm_strName, uint32 tm_dwHasHash);
	void		RemoveFriend(CFriend* todel);
	void		RemoveAllFriendSlots();
	void		Process();
private:
	CTypedPtrList<CPtrList, CFriend*>	m_listFriends;
	CFriendListCtrl*	m_wndOutput;
	uint32			m_nLastSaved;
};

#endif // FRIENDLIST_H

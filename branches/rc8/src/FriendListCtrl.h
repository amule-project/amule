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

#ifndef FRIENDLISTCTRL_H
#define FRIENDLISTCTRL_H

#include "MuleListCtrl.h"


class wxString;
class CFriend;
class CUpDownClient;
class CMD4Hash;

class CFriendListCtrl : public CMuleListCtrl
{
public:
	CFriendListCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize siz, int flags);
	~CFriendListCtrl();
	
	bool		IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort ); 
	void		SaveList();
	bool		LoadList();
	CFriend*	FindFriend( const CMD4Hash& userhash, uint32 dwIP, uint16 nPort) const;	
	void 		AddFriend(CFriend* toadd);
	void		AddFriend( CUpDownClient* toadd );
	void		AddFriend( const CMD4Hash& userhash, uint32 lastSeen, uint32 lastUsedIP, uint32 lastUsedPort, uint32 lastChatted, wxString name, uint32 hasHash);
	void		RemoveFriend(CFriend* todel);
	void		RemoveFriend(CUpDownClient* todel);
	void		RefreshFriend(CFriend* toupdate);
	
protected:
	DECLARE_EVENT_TABLE()

	void	OnNMRclick(wxMouseEvent& evt);
	void	OnPopupMenu(wxCommandEvent& evt);
		
private:
	void 	OnItemSelected(wxListEvent& evt);
	void	OnItemActivated(wxListEvent& evt);
	
	void	RemoveAllFriendSlots();
};

#endif

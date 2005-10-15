//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef FRIENDLIST_H
#define FRIENDLIST_H

#include <list>

#include "Types.h"	// Needed for uint32

class wxString;
class CFriend;
class CUpDownClient;
class CMD4Hash;

typedef std::list<CFriend*> FriendList;

class CFriendList 
{
public:
	CFriendList();
	~CFriendList();
	
	bool		IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort ); 
	void		SaveList();
	void		LoadList();
	CFriend*	FindFriend( const CMD4Hash& userhash, uint32 dwIP, uint16 nPort);	
	void 		AddFriend(CFriend* toadd);
	void		AddFriend( CUpDownClient* toadd );
	void		AddFriend( const CMD4Hash& userhash, uint32 lastSeen, uint32 lastUsedIP, uint32 lastUsedPort, uint32 lastChatted, const wxString& name);
	void		RemoveFriend( const CMD4Hash& userhash, uint32 lastUsedIP, uint32 lastUsedPort);
	void		RequestSharedFileList(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort);
	void		UpdateFriendName(const CMD4Hash& userhash, const wxString& name, uint32 dwIP, uint16 nPort);

	void		SetFriendSlot(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort, bool new_state);
	void		StartChatSession(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort);
	void		RemoveAllFriendSlots();

//private:

	#warning THIS MUST BE MADE PRIVATE AFTER EC IS CODED
	FriendList m_FriendList;
};

#endif

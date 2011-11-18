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

#ifndef FRIENDLIST_H
#define FRIENDLIST_H


#include "Types.h"	// Needed for uint32

class wxString;
class CFriend;
class CMD4Hash;
class CClientRef;

class CFriendList
{
	typedef std::list<CFriend*> FriendList;
public:
	CFriendList();
	~CFriendList();
	
	bool		IsAlreadyFriend(uint32 dwLastUsedIP, uint32 nLastUsedPort);
	void		SaveList();
	void		LoadList();
	CFriend*	FindFriend(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort);
	CFriend*	FindFriend(uint32 ecid);
	void 		AddFriend(CFriend* toadd, bool notify = true);
	void		AddFriend(const CClientRef& toadd);
	void		AddFriend(const CMD4Hash& userhash, uint32 lastUsedIP, uint32 lastUsedPort, const wxString& name, uint32 lastSeen = 0, uint32 lastChatted = 0);
	void		RemoveFriend(CFriend* toremove);
	void		RequestSharedFileList(CFriend* Friend);

	void		SetFriendSlot(CFriend* Friend, bool new_state);
	void		StartChatSession(CFriend* Friend);
	void		RemoveAllFriendSlots();

	// Iterator class
	class const_iterator {
		// iterator for internal list
		FriendList::const_iterator m_it;
	public:
		// constructs
		const_iterator() {};
		const_iterator(const FriendList::const_iterator& it) { m_it = it; };
		// operators
		bool operator != (const const_iterator& it) const { return m_it != it.m_it; }
		const_iterator& operator ++ ()	{ ++ m_it; return *this; }	// prefix  (assignable)
		void operator ++ (int)			{ ++ m_it; }				// postfix (not assignable)
		const CFriend* operator * () { return *m_it; }
	};
	// begin/end iterators for looping
	const_iterator begin() const { return const_iterator(m_FriendList.begin()); }
	const_iterator end() const { return const_iterator(m_FriendList.end()); }


private:
	FriendList m_FriendList;
};

#endif
// File_checked_for_headers

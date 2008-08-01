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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "FriendList.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "FriendList.h" // Interface

#include "amule.h"			// Needed for theApp: let it first or fail under win32
#include "ClientList.h"		// Needed for CClientList
#include "OPCodes.h"		// Needed for MP_DETAIL
#include "updownclient.h"	// Needed for CUpDownClient
#include "Friend.h"		// Needed for CFriend
#include "ArchSpecific.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "Logger.h"

CFriendList::CFriendList()
{ 
	LoadList();
}

CFriendList::~CFriendList()
{
	SaveList();

	while ( m_FriendList.size() ) {
		delete m_FriendList.front();
		m_FriendList.pop_front();
	}
}


void CFriendList::AddFriend(CFriend* toadd)
{
	m_FriendList.push_back(toadd);
	SaveList();
}


void CFriendList::AddFriend(const CMD4Hash& userhash, uint32 lastSeen, uint32 lastUsedIP, uint32 lastUsedPort, uint32 lastChatted, const wxString& name)
{
	CFriend* NewFriend = new CFriend( userhash, lastSeen, lastUsedIP, lastUsedPort, lastChatted, name);

	AddFriend( NewFriend );
}


void CFriendList::AddFriend(CUpDownClient* toadd)
{
	if ( toadd->IsFriend() ) {
		return;
	}
	
	CFriend* NewFriend = new CFriend( toadd );
	toadd->m_Friend = NewFriend;
	
	AddFriend( NewFriend );
}


void CFriendList::RemoveFriend(const CMD4Hash& userhash, uint32 lastUsedIP, uint32 lastUsedPort)
{
	CFriend* toremove = FindFriend(userhash, lastUsedIP, lastUsedPort);
	if (toremove) {
		if ( toremove->GetLinkedClient() ){
			toremove->GetLinkedClient()->SetFriendSlot(false);
			toremove->GetLinkedClient()->m_Friend = NULL;
			toremove->UnLinkClient();
		}

		m_FriendList.remove(toremove);
	
		SaveList();
	
		delete toremove;
	}
}

void CFriendList::LoadList()
{
  	wxString metfile = theApp.ConfigDir + wxT("emfriends.met"); 
	
	if ( !wxFileExists(metfile) ) {
		return;
	}
	
	CSafeFile file;
	try {
		if ( file.Open(metfile) ) {
			if ( file.ReadUInt8() /*header*/ == MET_HEADER ) {
				uint32 nRecordsNumber = file.ReadUInt32();
				for (uint32 i = 0; i < nRecordsNumber; i++) {
					CFriend* Record = new CFriend();
					Record->LoadFromFile(&file);
					m_FriendList.push_back(Record);
				}				
			}
			file.Close();
		} else if ( wxFileExists(metfile) ) {
			AddLogLineM(false, _("Failed to open friendlist file 'emfriends.met' for reading!"));
		}
	} catch (...) {
		AddLogLineM(false, _("Failed to read corrupted friendlist file 'emfriends.met'!"));
	}
	
}


void CFriendList::SaveList()
{
	CSafeFile file;
	if( file.Create(theApp.ConfigDir + wxT("emfriends.met"), true) ) {
		uint8 header = MET_HEADER;
		file.WriteUInt8(header);
		
		file.WriteUInt32(m_FriendList.size());
		
		for(FriendList::iterator it = m_FriendList.begin(); it != m_FriendList.end(); ++it) {
			(*it)->WriteToFile(&file);
		}
		
		file.Close();	
	} else {
		AddLogLineM(false, _("Failed to open friendlist file 'emfriends.met' for writing!"));
	}
}


CFriend* CFriendList::FindFriend(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort) 
{
	
	for(FriendList::iterator it = m_FriendList.begin(); it != m_FriendList.end(); ++it) {
		
		CFriend* cur_friend = *it;
		// to avoid that unwanted clients become a friend, we have to distinguish between friends with
		// a userhash and of friends which are identified by IP+port only.
		if ( !userhash.IsEmpty() && cur_friend->HasHash() ) {
			// check for a friend which has the same userhash as the specified one
			if (cur_friend->GetUserHash() == userhash) {
				return cur_friend;
			}
		} else if (cur_friend->GetIP() == dwIP && cur_friend->GetPort() == nPort) {
				return cur_friend;
		}
	}

	return NULL;
}


bool CFriendList::IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort )
{
	return FindFriend( NULL, dwLastUsedIP, nLastUsedPort );
}


void CFriendList::RemoveAllFriendSlots()
{
	for(FriendList::iterator it = m_FriendList.begin(); it != m_FriendList.end(); ++it) {		
		CFriend* cur_friend = *it;
		if ( cur_friend->GetLinkedClient() ) {
				cur_friend->GetLinkedClient()->SetFriendSlot(false);
		}
	}
}

void	CFriendList::RequestSharedFileList(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort) {
	CFriend* cur_friend = FindFriend(userhash, dwIP, nPort);
	if (cur_friend) {
		CUpDownClient* client = cur_friend->GetLinkedClient();
		if (!client) {
			client = new CUpDownClient(cur_friend->GetPort(), cur_friend->GetIP(), 0, 0, 0);
			client->SetUserName(cur_friend->GetName());
			theApp.clientlist->AddClient(client);
		}
		client->RequestSharedFileList();
	}
}

void CFriendList::SetFriendSlot(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort, bool new_state) {
	CFriend* cur_friend = FindFriend(userhash, dwIP, nPort);
	if (cur_friend && cur_friend->GetLinkedClient()) {
		RemoveAllFriendSlots();
		cur_friend->GetLinkedClient()->SetFriendSlot(new_state);
	}
}

void CFriendList::StartChatSession(const CMD4Hash& userhash, uint32 dwIP, uint16 nPort) {
	CFriend* friend_client = FindFriend(userhash, dwIP, nPort);
	if (friend_client) {
		CUpDownClient* client = friend_client->GetLinkedClient();
		if (!client) {
			client = new CUpDownClient(friend_client->GetPort(), friend_client->GetIP(), 0, 0, 0);
			client->SetIP(friend_client->GetIP());
			client->SetUserName(friend_client->GetName());
			theApp.clientlist->AddClient(client);
			friend_client->LinkClient(client);
		}
	} else {
		printf("CRITICAL - no client on StartChatSession\n");
	}
	
}
	
void CFriendList::UpdateFriendName(const CMD4Hash& userhash, const wxString& name, uint32 dwIP, uint16 nPort) {
	CFriend* friend_client = FindFriend(userhash, dwIP, nPort);
	friend_client->SetName(name);
	SaveList();
}
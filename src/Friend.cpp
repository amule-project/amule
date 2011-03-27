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


#include "Friend.h"			// Interface declarations.
#include "SafeFile.h"		// Needed for CFileDataIO
#include "GuiEvents.h"		// Needed for Notify_*


void CFriend::Init()
{
	m_dwLastSeen = 0;
	m_dwLastUsedIP = 0;
	m_nLastUsedPort = 0;
	m_dwLastChatted = 0;
}


CFriend::CFriend( const CMD4Hash& userhash, uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, const wxString& tm_strName)
{
	m_dwLastSeen = tm_dwLastSeen;
	m_dwLastUsedIP = tm_dwLastUsedIP;
	m_nLastUsedPort = tm_nLastUsedPort;
	m_dwLastChatted = tm_dwLastChatted;
	m_UserHash = userhash;

	if (tm_strName.IsEmpty()) {
		m_strName = wxT("?");
	} else {
		m_strName = tm_strName;
	}	
}


CFriend::CFriend(CClientRef client)
{
	LinkClient(client);
	
	m_dwLastChatted = 0;
}


void CFriend::LinkClient(CClientRef client)
{
	wxASSERT(client.IsLinked());

	// Link the friend to that client
	if (m_LinkedClient != client) {		// do nothing if already linked to this client
		bool hadFriendslot = false;
		if (m_LinkedClient.IsLinked()) { // What, is already linked?
			hadFriendslot = m_LinkedClient.GetFriendSlot();
			UnLinkClient(false);
		}
		m_LinkedClient = client;
		m_LinkedClient.SetFriend(this);
		if (hadFriendslot) {
			// move an assigned friend slot between different client instances which are/were also friends
			m_LinkedClient.SetFriendSlot(true);
		}
	}

	// always update (even if client stays the same)
	if ( !client.GetUserName().IsEmpty() ) {
		m_strName = client.GetUserName();
	} else {
		m_strName = wxT("?");
	}	
	m_UserHash = client.GetUserHash();
	m_dwLastUsedIP = client.GetIP();
	m_nLastUsedPort = client.GetUserPort();
	m_dwLastSeen = time(NULL);
	// This will update the Link status also on GUI.
	Notify_ChatUpdateFriend(this);
}


void CFriend::UnLinkClient(bool notify)
{
	if (m_LinkedClient.IsLinked()) {
		// avoid that an unwanted client instance keeps a friend slot
		if (m_LinkedClient.GetFriendSlot()) {
			m_LinkedClient.SetFriendSlot(false);
			CoreNotify_Upload_Resort_Queue();
		}
		m_LinkedClient.SetFriend(NULL);
		m_LinkedClient.Unlink();
		if (notify) {
			Notify_ChatUpdateFriend(this);
		}
	}
}


void CFriend::LoadFromFile(CFileDataIO* file)
{
	wxASSERT( file );

	m_UserHash = file->ReadHash();
	m_dwLastUsedIP = file->ReadUInt32();
	m_nLastUsedPort = file->ReadUInt16();
	m_dwLastSeen = file->ReadUInt32();
	m_dwLastChatted = file->ReadUInt32();

	uint32 tagcount = file->ReadUInt32();
	for ( uint32 j = 0; j != tagcount; j++) {
		CTag newtag(*file, true);
		switch ( newtag.GetNameID() ) {
			case FF_NAME:
				if (m_strName.IsEmpty()) {
					m_strName = newtag.GetStr();
				}
				break;
		}
	}
}


void CFriend::WriteToFile(CFileDataIO* file)
{
	wxASSERT( file );
	file->WriteHash(m_UserHash);
	file->WriteUInt32(m_dwLastUsedIP);
	file->WriteUInt16(m_nLastUsedPort);
	file->WriteUInt32(m_dwLastSeen);
	file->WriteUInt32(m_dwLastChatted);
	
	uint32 tagcount = ( m_strName.IsEmpty() ? 0 : 2 );
	file->WriteUInt32(tagcount);			
	if ( !m_strName.IsEmpty() ) {
		CTagString nametag(FF_NAME, m_strName);
		nametag.WriteTagToFile(file, utf8strOptBOM);
		nametag.WriteTagToFile(file);
	}
}


bool CFriend::HasFriendSlot() {
	if (m_LinkedClient.IsLinked()) {
		return m_LinkedClient.GetFriendSlot();
	} else {
		return false;
	}
}
// File_checked_for_headers

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

#include <ctime>			// Needed for time(2)

#include "Friend.h"			// Interface declarations.
#include "PartFile.h"		// Needed for CPartFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "amule.h" 			// Needed for theApp / Notify_ChatRefreshFriend
#include "OtherFunctions.h"
#include "Tag.h"			// Needed for CTag
#include "Logger.h"			// Needed for AddDebugLogLineM


CFriend::CFriend()
{
	m_dwLastSeen = 0;
	m_dwLastUsedIP = 0;
	m_nLastUsedPort = 0;
	m_dwLastChatted = 0;
	m_LinkedClient = NULL;
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
	
	m_LinkedClient = NULL;
}


CFriend::CFriend(CUpDownClient* client)
{
	wxASSERT( client );
	
	LinkClient(client, false); // On init, no need to unlink. BETTER not to unlink.	
	
	m_dwLastChatted = 0;
}


void	CFriend::LinkClient(CUpDownClient* client, bool unlink) {
	// Link the friend to that client
	if (unlink && m_LinkedClient) { // What, is already linked?
		if (m_LinkedClient != client){
			bool bFriendSlot = m_LinkedClient->GetFriendSlot();
			// avoid that an unwanted client instance keeps a friend slot
			m_LinkedClient->SetFriendSlot(false);
			m_LinkedClient->SetFriend(NULL);
			m_LinkedClient = client;
			// move an assigned friend slot between different client instances which are/were also friends
			m_LinkedClient->SetFriendSlot(bFriendSlot);
		}
	} else {
		m_LinkedClient = client;
	}

	if ( !client->GetUserName().IsEmpty() ) {
		m_strName = client->GetUserName();
	} else {
		m_strName = wxT("?");
	}	
	m_UserHash = client->GetUserHash();
	m_dwLastUsedIP = client->GetIP();
	m_nLastUsedPort = client->GetUserPort();
	m_dwLastSeen = time(NULL);
	// This will update the Link status also on GUI.
	Notify_ChatRefreshFriend(m_dwLastUsedIP, m_nLastUsedPort, m_strName);
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
				#if wxUSE_UNICODE
				if (m_strName.IsEmpty()) 
				#endif
					m_strName = newtag.GetStr();
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
	
	uint32 tagcount = ( m_strName.IsEmpty() ? 0 : 
	#if wxUSE_UNICODE
		2 );
	#else
		1 );
	#endif
	file->WriteUInt32(tagcount);			
	if ( !m_strName.IsEmpty() ) {
		CTag nametag(FF_NAME, m_strName);
		#if wxUSE_UNICODE
			nametag.WriteTagToFile(file, utf8strOptBOM);
		#endif		
		nametag.WriteTagToFile(file);
	}
}


bool CFriend::HasFriendSlot() {
	if (GetLinkedClient()) {
		return GetLinkedClient()->GetFriendSlot();
	} else {
		return false;
	}
}

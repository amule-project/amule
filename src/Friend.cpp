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


#include <ctime>		// Needed for time(2)

#include "Friend.h"		// Interface declarations.
#include "packets.h"		// Needed for CInvalidPacket
#include "PartFile.h"		// Needed for CPartFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "otherfunctions.h"


const char CFriend::sm_abyNullHash[16] = {0};


CFriend::CFriend()
{
	m_dwLastSeen = 0;
	m_dwLastUsedIP = 0;
	m_nLastUsedPort = 0;
	m_dwLastChatted = 0;
	m_strName = wxT("");
	m_LinkedClient = NULL;
	m_dwHasHash = 0;
	md4clr( m_abyUserhash );
}


CFriend::CFriend(uchar tm_abyUserhash[16], uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, wxString tm_strName, uint32 tm_dwHasHash)
{
	m_dwLastSeen = tm_dwLastSeen;
	m_dwLastUsedIP = tm_dwLastUsedIP;
	m_nLastUsedPort = tm_nLastUsedPort;
	m_dwLastChatted = tm_dwLastChatted;
	if( tm_dwHasHash ) {
		md4cpy(m_abyUserhash, tm_abyUserhash);
		m_dwHasHash = 1;
	} else {
		m_dwHasHash = 0;
	}
	m_strName = tm_strName;
	m_LinkedClient = NULL;
}


CFriend::CFriend(CUpDownClient* client)
{
	wxASSERT( client );
	
	m_dwLastSeen = time(NULL);
	m_dwLastUsedIP = client->GetIP();
	m_nLastUsedPort = client->GetUserPort();
	m_dwLastChatted = 0;
	
	if ( client->GetUserName() ) {
		m_strName = char2unicode(client->GetUserName());
	}
	
	md4cpy(m_abyUserhash, client->GetUserHash());
	m_LinkedClient = client;
	m_dwHasHash = 1;
}


void CFriend::LoadFromFile(CFile* file)
{
	wxASSERT( file );

	file->Read(m_abyUserhash, 16);
	m_dwHasHash = md4cmp(m_abyUserhash, sm_abyNullHash) ? 1 : 0;
	file->Read(&m_dwLastUsedIP, 4);
	file->Read(&m_nLastUsedPort, 2);
	file->Read(&m_dwLastSeen, 4);
	file->Read(&m_dwLastChatted, 4);
	
	uint32 tagcount;
	file->Read(&tagcount, 4);
	for ( uint32 j = 0; j != tagcount; j++) {
		CTag* newtag = new CTag(file);
		switch ( newtag->tag.specialtag ) {
			case FF_NAME:
				m_strName = char2unicode(newtag->tag.stringvalue);
				break;
		}
		
		delete newtag;
	}
}


void CFriend::WriteToFile(CFile* file)
{
	wxASSERT( file );
	
	file->Write(m_abyUserhash, 16);
	file->Write(&m_dwLastUsedIP, 4);
	file->Write(&m_nLastUsedPort, 2);
	file->Write(&m_dwLastSeen, 4);
	file->Write(&m_dwLastChatted, 4);
	uint32 tagcount = ( m_strName.IsEmpty() ? 0 : 1 );

	file->Write(&tagcount, 4);
	if ( !m_strName.IsEmpty() ) {
		CTag nametag(FF_NAME, m_strName);
		nametag.WriteTagToFile(file);
	}
}

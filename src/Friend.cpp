//this file is part of aMule
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

CFriend::CFriend(void)
{
	m_dwLastSeen = 0;
	m_dwLastUsedIP = 0;
	m_nLastUsedPort = 0;
	m_dwLastChatted = 0;
	m_strName = "";
	m_LinkedClient = 0;
	m_dwHasHash = 0;
	memset( m_abyUserhash, 0, 16);
}

CFriend::CFriend( uchar tm_abyUserhash[16], uint32 tm_dwLastSeen, uint32 tm_dwLastUsedIP, uint32 tm_nLastUsedPort, uint32 tm_dwLastChatted, wxString tm_strName, uint32 tm_dwHasHash){
	m_dwLastSeen = tm_dwLastSeen;
	m_dwLastUsedIP = tm_dwLastUsedIP;
	m_nLastUsedPort = tm_nLastUsedPort;
	m_dwLastChatted = tm_dwLastChatted;
	if( tm_dwHasHash ){
		memcpy(m_abyUserhash,tm_abyUserhash,16);
		m_dwHasHash = 1;
	} else {
		m_dwHasHash = 0;
	}
	m_strName = tm_strName;
	m_LinkedClient = 0;
}

CFriend::CFriend(CUpDownClient* client) {
	assert ( client );
	m_dwLastSeen = time(NULL);//mktime(lwtime.GetLocalTm());
	m_dwLastUsedIP = client->GetIP();
	m_nLastUsedPort = client->GetUserPort();
	m_dwLastChatted = 0;
	if (client->GetUserName()) {
		m_strName = client->GetUserName();
	} else {
		m_strName = "";
	}
	memcpy(m_abyUserhash,client->GetUserHash(),16);
	m_LinkedClient = client;
	m_dwHasHash = 1;
}

CFriend::~CFriend(void)
{
}



void CFriend::LoadFromFile(CFile* file){
	file->Read(m_abyUserhash,16);
	m_dwHasHash = md4cmp(m_abyUserhash, sm_abyNullHash) ? 1 : 0;
	file->Read(&m_dwLastUsedIP,4);
	file->Read(&m_nLastUsedPort,2);
	file->Read(&m_dwLastSeen,4);
	file->Read(&m_dwLastChatted,4);
	uint32 tagcount;
	file->Read(&tagcount,4);
	for (uint32 j = 0; j != tagcount;j++){
		CTag* newtag = new CTag(file);
		switch(newtag->tag.specialtag){
			case FF_NAME:{
				m_strName = newtag->tag.stringvalue;
				break;
			}
		}	
		delete newtag;
	}
}

#if 0
void CFriend::LoadFromFile(CFile* file)
{
	try {
		if ( 16 != file->Read(m_abyUserhash,16) ) {
			throw CInvalidPacket();
		}
		if ( 4 != file->Read(&m_dwLastUsedIP,4) ) {
			throw CInvalidPacket();
		}
		if ( 2 != file->Read(&m_nLastUsedPort,2) ) {
			throw CInvalidPacket();
		}
		if ( 4 != file->Read(&m_dwLastSeen,4) ) {
			throw CInvalidPacket();
		}
		if ( 4 != file->Read(&m_dwLastChatted,4) ) {
			throw CInvalidPacket();
		}
		m_dwHasHash = 1;
		uint32 tagcount;
		if ( 4 != file->Read(&tagcount,4) ) {
			throw CInvalidPacket();
		}
		for (uint32 j = 0; j < tagcount;j++) {
			try {
				CTag* newtag = new CTag(file);
				switch(newtag->tag->specialtag) {
					case FF_NAME:
						if ( newtag->GetType() != TAG_STRING ) {
							throw CInvalidPacket("Invalid data type for friend name");
						}
						m_strName= newtag->tag->stringvalue;
						delete newtag;
						break;
					}
				}
			catch ( CStrangePacket ) {
			}
		}	
	}
	catch ( CInvalidPacket ) {
		m_dwLastSeen = 0;
		m_dwLastUsedIP = 0;
		m_nLastUsedPort = 0;
		m_dwLastChatted = 0;
		m_strName = "";
		m_LinkedClient = 0;
		m_dwHasHash = 0;
		memset(m_abyUserhash, 0, 16);
		throw;
	}
}
#endif

void CFriend::WriteToFile(CFile* file)
{
	file->Write(m_abyUserhash,16);
	file->Write(&m_dwLastUsedIP,4);
	file->Write(&m_nLastUsedPort,2);
	file->Write(&m_dwLastSeen,4);
	file->Write(&m_dwLastChatted,4);
	uint32 tagcount = 0;
	if (!m_strName.IsEmpty()) {
		tagcount++;
	}
	file->Write(&tagcount,4);
	if (!m_strName.IsEmpty()) {
		CTag nametag(FF_NAME,(char*)m_strName.GetData());
		nametag.WriteTagToFile(file);
	}
}

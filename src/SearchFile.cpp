//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "SearchFile.h"			// Interface declarations.
#include "NetworkFunctions.h" 	// Needed for IsGoodIP
#include "MemFile.h"			// Needed for CMemFile
#include "Preferences.h"		// Needed for thePrefs
#include "Tag.h"				// Needed for CTag

#include <algorithm>			// Needed for std::max




CSearchFile::CSearchFile(const CMemFile& data, bool bOptUTF8, long searchID, uint32 WXUNUSED(serverIP), uint16 WXUNUSED(serverPort), const wxString& directory, bool kademlia)
	: m_searchID(searchID),
	  m_sourceCount(0),
	  m_completeSourceCount(0),
	  m_kademlia(kademlia),
	  m_directory(directory)	
{
	m_abyFileHash = data.ReadHash();
	m_clientID = data.ReadUInt32();
	m_clientPort = data.ReadUInt16();

	if ((not m_clientID) or (not m_clientPort)) {
		m_clientID = m_clientPort = 0;
	} else if (not IsGoodIP(m_clientID, thePrefs::FilterLanIPs())) {
		m_clientID = m_clientPort = 0;
	}
	
	
	uint32 tagcount = data.ReadUInt32();
	for (unsigned int i = 0; i < tagcount; ++i) {
		std::auto_ptr<CTag> tag(new CTag(data, bOptUTF8));

		switch (tag->GetNameID()) {
			case FT_FILENAME:			SetFileName(tag->GetStr());	break;
			case FT_FILESIZE:			SetFileSize(tag->GetInt());	break;
			case FT_FILERATING:			m_iUserRating = (tag->GetInt() & 0xF) / 3;	break;
			case FT_SOURCES:			m_sourceCount = tag->GetInt(); break;
			case FT_COMPLETE_SOURCES:	m_completeSourceCount = tag->GetInt(); break;
			
			default:
				AddTagUnique(tag.release());
		}
	}

	if (GetFileName().IsEmpty()) {
		throw CInvalidPacket(wxT("No filename in search result"));
	}
}


CSearchFile::CSearchFile(const CSearchFile& other)
	: CAbstractFile(other),
	  m_searchID(other.m_searchID),
	  m_sourceCount(other.m_sourceCount),
	  m_completeSourceCount(other.m_completeSourceCount),
	  m_kademlia(other.m_kademlia),
	  m_clientID(other.m_clientID),
	  m_clientPort(other.m_clientPort),
	  m_directory(other.m_directory)
{
}


CSearchFile::~CSearchFile()
{	
}


void CSearchFile::AddSources(uint32 count, uint32 count_complete)
{
	if (m_kademlia) {
		m_sourceCount = std::max(m_sourceCount, count);
		m_completeSourceCount = std::max(m_completeSourceCount, count_complete);
	} else {
		m_sourceCount += count;
		m_completeSourceCount += count_complete;
	}
}


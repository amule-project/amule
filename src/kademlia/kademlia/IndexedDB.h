//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Stu Redman ( sturedman@amule.org / http://www.amule.org )
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


#ifndef __INDEXEDDB_H__
#define __INDEXEDDB_H__

#include "Entry.h"

class CMemFile;
class CDatabase;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CKadUDPKey;
class CIndexedDB
{

public:
	CIndexedDB();
	//~CIndexedDB();

	void SetDatabase(CDatabase* db) { database = db; }

	void StartupCleanup();
	bool AddKeyword(const CUInt128& keyWordID, const CUInt128& sourceID, Kademlia::CKeyEntry* entry, uint8_t& load);

	//bool AddSources(const CUInt128& keyWordID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8_t& load);
	//bool AddNotes(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8_t& load);
	//bool AddLoad(const CUInt128& keyID, uint32_t time);
	//size_t GetFileKeyCount() const throw() { return m_Keyword_map.size(); }
	void SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, uint32_t ip, uint16_t port, bool oldClient, uint16_t startPosition, const CKadUDPKey& senderKey, CMemFile & mirror);
	//void SendValidSourceResult(const CUInt128& keyID, uint32_t ip, uint16_t port, uint16_t startPosition, uint64_t fileSize, const CKadUDPKey& senderKey);
	//void SendValidNoteResult(const CUInt128& keyID, uint32_t ip, uint16_t port, uint64_t fileSize, const CKadUDPKey& senderKey);
	//bool SendStoreRequest(const CUInt128& keyID);

	uint32_t m_totalIndexSource;
	uint32_t m_totalIndexKeyword;
	uint32_t m_totalIndexNotes;
	uint32_t m_totalIndexLoad;


private:
	void Clean();
	void MergeIPsAndFilenames(uint64 sourceIndex, const wxString & filename, uint32 uIP);

	CDatabase* database;
	time_t m_lastClean;
};

} // End namespace

#endif //__INDEXEDDB_H__
// File_checked_for_headers

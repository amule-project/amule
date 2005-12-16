//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#ifndef __INDEXED_H__
#define __INDEXED_H__

#include <list>
#include <map>

#include "../../Types.h"
#include "SearchManager.h"
#include "../routing/Maps.h"
#include "../utils/UInt128.h"
#include "Entry.h"

class wxArrayString;


typedef std::list<Kademlia::CEntry*> CKadEntryPtrList;

struct Source
{
	Kademlia::CUInt128 sourceID;
	CKadEntryPtrList entryList;
};

typedef std::list<Source*> CKadSourcePtrList;
typedef std::map<Kademlia::CUInt128,Source*> CSourceKeyMap;

struct KeyHash
{
	Kademlia::CUInt128 keyID;
	CSourceKeyMap m_Source_map;
};


struct SrcHash
{
	Kademlia::CUInt128 keyID;
	CKadSourcePtrList m_Source_map;
};

struct Load
{
	Kademlia::CUInt128 keyID;
	uint32 time;
};

struct SSearchTerm
{
	SSearchTerm();
	~SSearchTerm();
	
	enum ESearchTermType {
		AND,
		OR,
		NAND,
		String,
		MetaTag,
		OpGreaterEqual,
		OpLessEqual,
		OpGreater,
		OpLess,
		OpEqual,
		OpNotEqual
	} type;
	
	Kademlia::CTag* tag;
	wxArrayString* astr;

	SSearchTerm* left;
	SSearchTerm* right;
};

typedef std::map<Kademlia::CUInt128,KeyHash*> KeyHashMap;
typedef std::map<Kademlia::CUInt128,SrcHash*> SrcHashMap;
typedef std::map<Kademlia::CUInt128,Load*> LoadMap;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CIndexed
{

public:
	CIndexed();
	~CIndexed();

	bool AddKeyword(const CUInt128& keyWordID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load);
	bool AddSources(const CUInt128& keyWordID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load);
	bool AddNotes(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load);
	bool AddLoad(const CUInt128& keyID, uint32 time);
	uint32 GetIndexedCount() {return m_Keyword_map.size();}
	uint32 GetFileKeyCount() {return m_Keyword_map.size();}
	void SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, uint32 ip, uint16 port);
	void SendValidSourceResult(const CUInt128& keyID, uint32 ip, uint16 port);
	void SendValidNoteResult(const CUInt128& keyID, const CUInt128& CheckID, uint32 ip, uint16 port);
	bool SendStoreRequest(const CUInt128& keyID);
	uint32 m_totalIndexSource;
	uint32 m_totalIndexKeyword;

private:
	time_t m_lastClean;
	KeyHashMap m_Keyword_map;
	SrcHashMap m_Sources_map;
	SrcHashMap m_Notes_map;
	LoadMap m_Load_map;
	static wxString m_sfilename;
	static wxString m_kfilename;
	static wxString m_loadfilename;
	void readFile(void);
	void clean(void);
};

} // End namespace

#endif //__INDEXED_H__

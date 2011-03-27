//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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

#ifndef __SEARCHMANAGER_H__
#define __SEARCHMANAGER_H__

#include "../utils/UInt128.h"
#include "../routing/Maps.h"
#include "../../Tag.h"

class CMemFile;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CSearch;
class CRoutingZone;
class CKadClientSearcher;

typedef std::list<wxString> WordList;
typedef std::map<CUInt128, CSearch*> SearchMap;

class CSearchManager
{
	friend class CRoutingZone;
	friend class CKademlia;

public:

	static bool IsSearching(uint32_t searchID) throw();
	static void StopSearch(uint32_t searchID, bool delayDelete);
	static void StopAllSearches();

	// Search for a particular file
	// Will return unique search id, returns zero if already searching for this file.
	static CSearch* PrepareLookup(uint32_t type, bool start, const CUInt128& id);

	// Will return unique search id, returns zero if already searching for this keyword.
	static CSearch* PrepareFindKeywords(const wxString& keyword, uint32_t searchTermsDataSize, const uint8_t *searchTermsData, uint32_t searchid);

	static bool StartSearch(CSearch* search);

	static void ProcessResponse(const CUInt128& target, uint32_t fromIP, uint16_t fromPort, ContactList *results);
	static void ProcessResult(const CUInt128& target, const CUInt128& answer, TagPtrList *info);
	static void ProcessPublishResult(const CUInt128& target, const uint8_t load, const bool loadResponse);

	static void GetWords(const wxString& str, WordList *words, bool allowDuplicates = false);

	static void UpdateStats() throw();

	static bool AlreadySearchingFor(const CUInt128& target) throw() { return m_searches.count(target) > 0; }

	static const wxChar* GetInvalidKeywordChars() { return wxT(" ()[]{}<>,._-!?:;\\/\""); }

	static void CancelNodeFWCheckUDPSearch();
	static bool FindNodeFWCheckUDP();
	static bool IsFWCheckUDPSearch(const CUInt128& target);
	static void SetNextSearchID(uint32_t nextID) throw()	{ m_nextID = nextID; }

private:

	static void FindNode(const CUInt128& id, bool complete);
	static bool FindNodeSpecial(const CUInt128& id, CKadClientSearcher *requester);
	static void CancelNodeSpecial(CKadClientSearcher *requester);

	static void JumpStart();

	static uint32_t  m_nextID;
	static SearchMap m_searches;
};

} // End namespace

#endif // __SEARCHMANAGER_H__
// File_checked_for_headers

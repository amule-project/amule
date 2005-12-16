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

#ifndef __SEARCHMANAGER_H__
#define __SEARCHMANAGER_H__

#include <map>
#include <list>
#include "../utils/UInt128.h"
#include "../routing/Maps.h"

class CMemFile;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CSearch;
class CRoutingZone;
class CTag;
typedef std::list<CTag*> TagList;
void deleteTagListEntries(TagList* taglist);

// If type is unknown it will be an empty string
// If there are any properties about the file to report, there will follow LPCSTR key/value pairs.
//typedef void (CALLBACK *SEARCH_KEYWORD_CALLBACK)(uint32 searchID, CUInt128 fileID, wxString name, uint32 size, wxString type, uint16 numProperties, ...);
//typedef void (CALLBACK *SEARCH_ID_CALLBACK)(uint32 searchID, CUInt128 contactID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 port);

typedef std::list<wxString> WordList;
typedef std::map<CUInt128, CSearch*> SearchMap;

#define SEARCH_IMAGE	"-image"
#define SEARCH_AUDIO	"-audio"
#define SEARCH_VIDEO	"-video"
#define SEARCH_DOC		"-doc"
#define SEARCH_PRO		"-pro"

class CSearchManager
{
	friend class CRoutingZone;
	friend class CKademlia;

public:

	static void stopSearch(uint32 searchID, bool delayDelete);
	static void stopAllSearches(void);

	// Search for a particular file
	// Will return unique search id, returns zero if already searching for this file.
	static CSearch* prepareLookup(uint32 type, bool start, const CUInt128 &id);

	// Will return unique search id, returns zero if already searching for this keyword.
	static CSearch* prepareFindKeywords(const wxString& keyword, CMemFile* ed2k_packet, uint32 searchid);

	static bool startSearch(CSearch* pSearch);
	static void deleteSearch(CSearch* pSearch);

	static void processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results);
	static void processResult(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info);
	static void processPublishResult(const CUInt128 &target, const uint8 load, const bool loadResponse);

	static void getWords(const wxString& str, WordList *words);

	static void updateStats(void);

	static bool isNodeSearch(const CUInt128 &target);

	static bool alreadySearchingFor(const CUInt128 &target);
private:

	static void findNode(const CUInt128 &id);
	static void findNodeComplete(const CUInt128 &id);

	static uint32 m_nextID;
	static SearchMap m_searches;

	static void jumpStart(void);
};

} // End namespace

#endif // __SEARCHMANAGER_H__

//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __KAD_ENTRY_H__
#define __KAD_ENTRY_H__

#include <ctime>

#include "../utils/UInt128.h"
#include "../../Tag.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CEntry
{
public:
	CEntry()
	{
		ip = 0;
		tcpport = 0;
		udpport = 0;
		size = 0;
		lifetime = time(NULL);
		source = false;
	}
	~CEntry()
	{
		TagPtrList::const_iterator it;
		for (it = taglist.begin(); it != taglist.end(); ++it) {
			delete *it;
		}
	}
	
	uint32 GetIntTagValue(const wxString& tagname) const
	{
		TagPtrList::const_iterator it;
		CTag* tag;
		for (it = taglist.begin(); it != taglist.end(); ++it) {
			tag = *it;
			if ((tag->GetName() == tagname) && tag->IsInt()) {
				return tag->GetInt();
			}
		}
		return 0;
	}

	wxString GetStrTagValue(const wxString& tagname) const
	{
		TagPtrList::const_iterator it;
		CTag* tag;
		for (it = taglist.begin(); it != taglist.end(); ++it) {
			tag = *it;
			if ((tag->GetName() == tagname) && tag->IsStr()) {
				return tag->GetStr();
			}
		}
		return wxEmptyString;
	}	
	
	uint32 ip;
	uint16 tcpport;
	uint16 udpport;
	CUInt128 keyID;
	CUInt128 sourceID;
	wxString fileName; // NOTE: this always holds the string in LOWERCASE!!!
	uint32	size;
	TagPtrList taglist;
	time_t lifetime;
	bool source;
};

}

#endif // __KAD_ENTRY_H__

//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2008 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __ROUTING_BIN__
#define __ROUTING_BIN__

#include "Maps.h"
#include "../../Types.h"
#include "../kademlia/Defines.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128;
class CContact;

class CRoutingBin
{
	friend class CRoutingZone;

public:

	~CRoutingBin();

private:

	CRoutingBin()
		: m_dontDeleteContacts(false)
	{}

	bool AddContact(CContact *contact);
	void SetAlive(CContact *contact);
	void SetTCPPort(uint32_t ip, uint16_t port, uint16_t tcpPort);
	void RemoveContact(CContact *contact)		{ m_entries.remove(contact); }
	CContact *GetContact(const CUInt128 &id) const throw();
	CContact *GetOldest() const throw()		{ return m_entries.size() ? m_entries.front() : NULL; }

	uint32_t GetSize() const throw()		{ return m_entries.size(); }
	uint32_t GetRemaining() const throw()		{ return K - m_entries.size(); }
	void GetEntries(ContactList *result, bool emptyFirst = true) const;

	void GetClosestTo(uint32_t maxType, const CUInt128 &target, uint32_t maxRequired, ContactMap *result, bool emptyFirst = true, bool setInUse = false) const;

	// Debug purposes.
//	void dumpContents(void);

	bool m_dontDeleteContacts;
	ContactList m_entries;
};

} // End namespace

#endif // __ROUTING_BIN__
// File_checked_for_headers

//
// This file is part of aMule Project
//
// Copyright (c) 2004-2008 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2008 aMule Project ( http://www.amule-project.net )
// Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

// This work is based on the java implementation of the Kademlia protocol.
// Kademlia: Peer-to-peer routing based on the XOR metric
// Copyright (C) 2002  Petar Maymounkov [petar@post.harvard.edu]
// http://kademlia.scs.cs.nyu.edu

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

#include "RoutingBin.h"
#include "Contact.h"


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CRoutingBin::~CRoutingBin()
{
	if (!m_dontDeleteContacts) {
		for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
			delete *it;
		}
	}

	m_entries.clear();
}

bool CRoutingBin::AddContact(CContact *contact)
{
	wxASSERT(contact != NULL);

	// Check if we already have a contact with this ID in the list.
	CContact *test = GetContact(contact->GetClientID());
	if (test == NULL) {
		// If not full, add to end of list
		if (m_entries.size() < K) {
			m_entries.push_back(contact);
			return true;
		}
	}
	return false;
}

void CRoutingBin::SetAlive(CContact *contact)
{
	wxASSERT(contact != NULL);
	// Check if we already have a contact with this ID in the list.
	CContact *test = GetContact(contact->GetClientID());
	wxASSERT(contact == test);
	if (test) {
		// Mark contact as being alive.
		test->UpdateType();
		// Move to the end of the list
		RemoveContact(test);
		m_entries.push_back(test);
	}
}

void CRoutingBin::SetTCPPort(uint32_t ip, uint16_t port, uint16_t tcpPort)
{
	// Find contact with IP/Port
	for (ContactList::iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
		CContact *c = *it;
		if ((ip == c->GetIPAddress()) && (port == c->GetUDPPort())) {
			// Set TCPPort and mark as alive.
			c->SetTCPPort(tcpPort);
			c->UpdateType();
			// Move to the end of the list
			RemoveContact(c);
			m_entries.push_back(c);
			break;
		}
	}
}

CContact *CRoutingBin::GetContact(const CUInt128 &id) const throw()
{
	for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
		if ((*it)->GetClientID() == id) {
			return *it;
		}
	}
	return NULL;
}

void CRoutingBin::GetEntries(ContactList *result, bool emptyFirst) const
{
	// Clear results if requested first.
	if (emptyFirst) {
		result->clear();
	}

	// Append all entries to the results.
	if (m_entries.size() > 0) {
		result->insert(result->end(), m_entries.begin(), m_entries.end());
	}
}

void CRoutingBin::GetClosestTo(uint32_t maxType, const CUInt128 &target, uint32_t maxRequired, ContactMap *result, bool emptyFirst, bool inUse) const
{
	// Empty list if requested.
	if (emptyFirst) {
		result->clear();
	}

	// No entries, no closest.
	if (m_entries.size() == 0) {
		return;
	}

	// First put results in sort order for target so we can insert them correctly.
	// We don't care about max results at this time.
	for (ContactList::const_iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
		if ((*it)->GetType() <= maxType) {
			CUInt128 targetDistance((*it)->GetClientID() ^ target);
			(*result)[targetDistance] = *it;
			// This list will be used for an unknown time, Inc in use so it's not deleted.
			if (inUse) {
				(*it)->IncUse();
			}
		}
	}

	// Remove any extra results by least wanted first.
	while (result->size() > maxRequired) {
		// Dec in use count.
 		if (inUse) {
  			(--result->end())->second->DecUse();
		}
 		// Remove from results
 		result->erase(--result->end());
	}
}
// File_checked_for_headers

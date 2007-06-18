//
// This file is part of aMule Project
//
// Copyright (c) 2004-2007 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2007 aMule Project ( http://www.amule-project.net )
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
#include "../kademlia/Defines.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CRoutingBin::CRoutingBin()
{
	m_dontDeleteContacts = false;
}

CRoutingBin::~CRoutingBin()
{
	ContactList::const_iterator it;
	if (!m_dontDeleteContacts) {
		for (it = m_entries.begin(); it != m_entries.end(); ++it) {
			delete *it;
		}
	}
	
	m_entries.clear();
}

bool CRoutingBin::Add(CContact *contact, bool check)
{
	wxASSERT(contact != NULL);
	bool retVal = false;
	// If this is already in the entries list
	CContact *c = NULL;
	if (check)
		c=GetContact(contact->GetClientID());

	if (c != NULL) {
		// Move to the end of the list
		Remove(c);
		m_entries.push_back(c);
		retVal = false;
	} else {
		// If not full, add to end of list
		if ( m_entries.size() < K) {
			m_entries.push_back(contact);
			retVal = true;
		} else {
			retVal = false;
		}
	}
	return retVal;
}

void CRoutingBin::SetAlive(uint32 ip, uint16 port)
{
	if (m_entries.empty()) {
		return;
	}

	CContact *c;
	ContactList::iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		c = *it;
		if ((ip == c->GetIPAddress()) && (port == c->GetUDPPort())) {
			c->UpdateType();
			break;
		}
	}
}

void CRoutingBin::SetTCPPort(uint32 ip, uint16 port, uint16 tcpPort)
{
	if (m_entries.empty()) {
		return;
	}

	CContact *c;
	ContactList::iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		c = *it;
		if ((ip == c->GetIPAddress()) && (port == c->GetUDPPort())) {
			c->SetTCPPort(tcpPort);
			c->UpdateType();
			// Move to the end of the list
			Remove(c);
			m_entries.push_back(c);
			break;
		}
	}
}

void CRoutingBin::Remove(CContact *contact)
{
	m_entries.remove(contact);
}

CContact *CRoutingBin::GetContact(const CUInt128 &id) 
{
	CContact *retVal = NULL;
	ContactList::const_iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		if ((*it)->GetClientID() == id) {
			retVal = *it;
			break;
		}
	}
	return retVal;
}

uint32 CRoutingBin::GetSize(void) const
{
	return (uint32)m_entries.size();
}

uint32 CRoutingBin::GetRemaining(void) const
{
	return (uint32)K - m_entries.size();
}

void CRoutingBin::GetEntries(ContactList *result, bool emptyFirst) 
{
	if (emptyFirst) {
		result->clear();
	}
	if (m_entries.size() > 0) {
		result->insert(result->end(), m_entries.begin(), m_entries.end());
	}
}

CContact *CRoutingBin::GetOldest(void) 
{
	if (m_entries.size() > 0) {
		return m_entries.front();
	}
	return NULL;
}

void CRoutingBin::GetClosestTo(uint32 maxType, const CUInt128 &target, uint32 maxRequired, ContactMap *result, bool emptyFirst, bool inUse)
{
	// If we have to clear the bin, do it now.
	if (emptyFirst) {
		result->clear();
	}

	// No entries, no closest.
	if (m_entries.size() == 0) {
		return;
	}

	// First put results in sort order for target so we can insert them correctly.
	// We don't care about max results at this time.
	
	ContactList::const_iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		if((*it)->GetType() <= maxType) {
			CUInt128 targetDistance((*it)->GetClientID());
			targetDistance.XOR(target);
			(*result)[targetDistance] = *it;
			// This list will be used for an unknown time, Inc in use so it's not deleted.
			if( inUse ) {
				(*it)->IncUse();
			}
		}
	}

	// Remove any extra results by least wanted first.
	while (result->size() > maxRequired) {
 	// Dec in use count.
 		if( inUse ) {
  			(--result->end())->second->DecUse();
		}
 		// Remove from results
 		result->erase(--result->end());
	}
}
// File_checked_for_headers

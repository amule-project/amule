//
// This file is part of aMule Project
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Project ( http://www.amule-project.net )
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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

//#include "stdafx.h"
#include "RoutingBin.h"
#include "Contact.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Defines.h"
#include "../routing/RoutingZone.h"
#include "Logger.h"

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
	try {
		if (!m_dontDeleteContacts)
		{
			for (it = m_entries.begin(); it != m_entries.end(); ++it) {
				delete *it;
			}
		}
		m_entries.clear();
	} catch (...)  {
		AddDebugLogLineM(false, logKadRouting, wxT("Exception in ~CRoutingBin"));
	}
}

bool CRoutingBin::add(CContact *contact)
{
	wxASSERT(contact != NULL);
	bool retVal = false;
	// If this is already in the entries list
	CUInt128 id;
	contact->getClientID(&id);
	CContact *c = getContact(id);
	if (c != NULL) {
		// Move to the end of the list
		remove(c);
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

void CRoutingBin::setAlive(uint32 ip, uint16 port)
{
	if (m_entries.empty()) {
		return;
	}

	CContact *c;
	ContactList::iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		c = *it;
		if ((ip == c->getIPAddress()) && (port == c->getUDPPort())) {
			c->madeContact(true);
			break;
		}
	}
}

void CRoutingBin::setTCPPort(uint32 ip, uint16 port, uint16 tcpPort)
{
	if (m_entries.empty()) {
		return;
	}

	CContact *c;
	ContactList::iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		c = *it;
		if ((ip == c->getIPAddress()) && (port == c->getUDPPort())) {
			c->setTCPPort(tcpPort);
			c->madeContact(true);
			// Move to the end of the list
			remove(c);
			m_entries.push_back(c);
			break;
		}
	}
}

void CRoutingBin::remove(CContact *contact)
{
	m_entries.remove(contact);
}

CContact *CRoutingBin::getContact(const CUInt128 &id) 
{
	CContact *retVal = NULL;
	ContactList::const_iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		if (id == (*it)->m_clientID) {
			retVal = *it;
			break;
		}
	}
	return retVal;
}

uint32 CRoutingBin::getSize(void) const
{
	return (uint32)m_entries.size();
}

uint32 CRoutingBin::getRemaining(void) const
{
	return (uint32)K - m_entries.size();
}

void CRoutingBin::getEntries(ContactList *result, bool emptyFirst) 
{
	if (emptyFirst) {
		result->clear();
	}
	if (m_entries.size() > 0) {
		result->insert(result->end(), m_entries.begin(), m_entries.end());
	}
}

CContact *CRoutingBin::getOldest(void) 
{
	if (m_entries.size() > 0) {
		return m_entries.front();
	}
	return NULL;
}

int CRoutingBin::getClosestTo(int maxType, const CUInt128 &target, int maxRequired, ContactMap *result, bool emptyFirst, bool inUse)
{
	if (m_entries.size() == 0) {
		return 0;
	}

	if (emptyFirst) {
		result->clear();
	}

	int count = 0;
	ContactList::const_iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); ++it) {
		if((*it)->getType() <= maxType) {
			CUInt128 distance((*it)->m_clientID);
			distance.XOR(target);
			(*result)[distance] = *it;
			if( inUse ) {
				(*it)->incUse();
			}
			if (++count == maxRequired) {
				break;
			}
		}
	}
	return count;
}

/*
//Keep just in case needed at sometime.
void CRoutingBin::dumpContents(void)
{
	CString line;
	CString hex;
	CString ipStr;
	CString distance;
	CContact *c;
	ContactList::const_iterator it;
	for (it = m_entries.begin(); it != m_entries.end(); it++)
	{
		c = *it;
		c->m_clientID.toHexString(&hex);
		c->getIPAddress(&ipStr);
		c->getDistance(&distance);
		line.Format(_T("\t%s\t%s (%ld)\tDistance: %s\r\n"), hex, ipStr, c->getUDPPort(), distance);
		OutputDebugString(line);
	}
}
*/

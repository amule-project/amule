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

//#include "../../stdafx.h"

#include "Ping.h"

#if 0 // This class is, huh, completely unused!

#include "RoutingZone.h"
#include "Contact.h"

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CPing::CPing(CRoutingZone *zone, const ContactList &test, const ContactList &replacements)
{
	m_zone = zone;
	m_test.insert(m_test.end(), test.begin(), test.end());
	m_replacements.insert(m_replacements.end(), replacements.begin(), replacements.end());
	// Send hello requests and add to notification list
}

void CPing::responded(const byte *key)
{
	CUInt128 r(key);
	ContactList::const_iterator it;
	for (it = m_test.begin(); it != m_test.end(); it++) {
		if (r == (*it)->getClientID()) {
			m_zone->add(*it);
		}
	}
}

void CPing::onTimer(void)
{
	// remove from notification list

	int replaceCount = (int)m_replacements.size();
	ContactList::const_iterator it;
	for (it = m_test.begin(); it != m_test.end(); it++) {
		m_zone->remove(*it);
		if (replaceCount-- > 0) {
			m_zone->add(m_replacements.front());
			m_replacements.pop_front();
		}
	}
	for (it = m_replacements.begin(); it != m_replacements.end(); it++) {
		m_zone->removePending(*it);
	}
}

#endif //0

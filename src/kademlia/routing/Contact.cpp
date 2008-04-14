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

#include "Contact.h"

#include <common/Macros.h>

#include "../../Statistics.h"

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CContact::~CContact()
{
	theStats::RemoveKadNode();
}

CContact::CContact(const CUInt128 &clientID, uint32 ip, uint16 udpPort, uint16 tcpPort, uint8 version, const CUInt128 &target)
	: m_clientID(clientID),
	  m_distance(target ^ clientID),
	  m_ip(ip),
	  m_tcpPort(tcpPort),
	  m_udpPort(udpPort),
	  m_type(3),
	  m_lastTypeSet(time(NULL)),
	  m_expires(0),
	  m_created(m_lastTypeSet),
	  m_inUse(0),
	  m_version(version),
	  m_checkKad2(true)
{
	wxASSERT(udpPort);
	theStats::AddKadNode();
}

#ifndef CLIENT_GUI
void CContact::SetClientID(const CUInt128 &clientID) throw()
{
	m_clientID = clientID;
	m_distance = CKademlia::GetPrefs()->GetKadID() ^ clientID;
}
#endif

void CContact::CheckingType() throw()
{
	time_t now = time(NULL);

	if(now - m_lastTypeSet < 10 || m_type == 4) {
		return;
	}

	m_lastTypeSet = now;

	m_expires = now + MIN2S(2);
	m_type++;
}

void CContact::UpdateType() throw()
{
	time_t now = time(NULL);
	uint32_t hours = (now - m_created) / HR2S(1);
	switch (hours) {
		case 0:
			m_type = 2;
			m_expires = now + HR2S(1);
			break;
		case 1:
			m_type = 1;
			m_expires = now + MIN2S(90); //HR2S(1.5)
			break;
		default:
			m_type = 0;
			m_expires = now + HR2S(2);
	}
}
// File_checked_for_headers

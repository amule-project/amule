//
// This file is part of aMule Project
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Project ( http://www.amule-project.net )
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
#include "../../OPCodes.h" // Neededf for MIN2MS and such stuff.
#include "../../Statistics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CContact::~CContact()
{
	theStats::RemoveKadNode();
}

CContact::CContact(const CUInt128 &clientID, uint32 ip, uint16 udpPort, uint16 tcpPort, const CUInt128 &target)
:
m_clientID(clientID),
m_distance(target),
m_ip(ip),
m_tcpPort(tcpPort),
m_udpPort(udpPort),
m_type(3),
m_lastTypeSet(time(NULL)),
m_expires(0),
m_created(time(NULL)),
m_inUse(0)
{
	m_distance.XOR(clientID);
	wxASSERT(udpPort);
	theStats::AddKadNode();
}

const wxString CContact::GetClientIDString(void) const
{
	return m_clientID.ToHexString();
}

#ifndef CLIENT_GUI
void CContact::SetClientID(const CUInt128 &clientID)
{
	m_clientID = clientID;
	m_distance = CKademlia::GetPrefs()->GetKadID();
	m_distance.XOR(clientID);
}
#endif

const wxString CContact::GetDistanceString(void) const
{
	return m_distance.ToBinaryString();
}

uint32 CContact::GetIPAddress(void) const
{
	return m_ip;
}

void CContact::SetIPAddress(uint32 ip)
{
	m_ip = ip;
}

uint16 CContact::GetTCPPort(void) const
{
	return m_tcpPort;
}

void CContact::SetTCPPort(uint16 port)
{
	m_tcpPort = port;
}

uint16 CContact::GetUDPPort(void) const
{
	return m_udpPort;
}

void CContact::SetUDPPort(uint16 port)
{
	wxASSERT(port);
	m_udpPort = port;
}

byte CContact::GetType(void) const
{
	return m_type;
}

void CContact::CheckingType()
{
	if(time(NULL) - m_lastTypeSet < 10 || m_type == 4) {
		return;
	}

	m_lastTypeSet = time(NULL);

	m_expires = time(NULL) + MIN2S(2);
	m_type++;

}

void CContact::UpdateType()
{
	uint32 hours = (time(NULL)-m_created)/HR2S(1);
	switch(hours) {
		case 0:
			m_type = 2;
			m_expires = time(NULL) + HR2S(1);
			break;
		case 1:
			m_type = 1;
			m_expires = time(NULL) + (int)HR2S(1.5);
			break;
		default:
			m_type = 0;
			m_expires = time(NULL) + HR2S(2);
	}
}
// File_checked_for_headers

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

#include "Contact.h"
#include "../../amule.h"
#include "../../OPCodes.h" // Neededf for MIN2MS and such stuff.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#warning EC
#ifndef AMULE_DAEMON
#include "../../amuleDlg.h"
#include "../../KadDlg.h"
#endif

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CContact::~CContact()
{
	#warning TODO: EC
	#ifndef AMULE_DAEMON
	theApp.amuledlg->kademliawnd->RemoveNode(this);
	#endif
}

CContact::CContact()
{
	m_clientID = 0;
	m_ip = 0;
	m_udpPort = 0;
	m_tcpPort = 0;
	initContact();
}

CContact::CContact(const CUInt128 &clientID, uint32 ip, uint16 udpPort, uint16 tcpPort, const CUInt128 &target)
{
	m_clientID = clientID;
	m_distance.setValue(target);
	m_distance.XOR(clientID);
	m_ip = ip;
	m_udpPort = udpPort;
	m_tcpPort = tcpPort;
	initContact();
}

void CContact::initContact() 
{
	m_type = 3;
	m_expires = 0;
	m_lastTypeSet = time(NULL);
	m_created = time(NULL);
	m_inUse = 0;	
}

const wxString CContact::getClientIDString(void) const
{
	return m_clientID.toHexString();
}

#ifndef CLIENT_GUI
void CContact::setClientID(const CUInt128 &clientID)
{
	m_clientID = clientID;
	m_distance = CKademlia::getPrefs()->getKadID();
	m_distance.XOR(clientID);
}
#endif

const wxString CContact::getDistanceString(void) const
{
	return m_distance.toBinaryString();
}

uint32 CContact::getIPAddress(void) const
{
	return m_ip;
}

void CContact::setIPAddress(uint32 ip)
{
	m_ip = ip;
}

uint16 CContact::getTCPPort(void) const
{
	return m_tcpPort;
}

void CContact::setTCPPort(uint16 port)
{
	m_tcpPort = port;
}

uint16 CContact::getUDPPort(void) const
{
	return m_udpPort;
}

void CContact::setUDPPort(uint16 port)
{
	m_udpPort = port;
}

byte CContact::getType(void) const
{
	return m_type;
}

void CContact::checkingType()
{
	if(time(NULL) - m_lastTypeSet < 10 || m_type == 4) {
		return;
	}

	m_lastTypeSet = time(NULL);

	m_expires = time(NULL) + MIN2S(2);
	m_type++;

	#warning TODO: EC
	#ifndef AMULE_DAEMON
		theApp.amuledlg->kademliawnd->RefreshNode(this);
	#endif
}

void CContact::updateType()
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
	#warning TODO: EC
	#ifndef AMULE_DAEMON
		theApp.amuledlg->kademliawnd->RefreshNode(this);
	#endif
}

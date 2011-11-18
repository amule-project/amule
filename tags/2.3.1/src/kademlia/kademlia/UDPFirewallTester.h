//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef KADEMLIA_KADEMLIA_UDPFIREWALLTESTER_H
#define KADEMLIA_KADEMLIA_UDPFIREWALLTESTER_H

#include "Kademlia.h"
#include "../routing/Contact.h"
#include <list>

namespace Kademlia
{

class CUInt128;
class CKadUDPKey;

struct UsedClient_Struct {
	CContact	contact;
	bool		answered;
};

#define UDP_FIREWALLTEST_CLIENTSTOASK	2	// more clients increase the chance of a false positive, while less the chance of a false negative

class CUDPFirewallTester
{
      public:
	static bool	IsFirewalledUDP(bool lastStateIfTesting); // Are we UDP firewalled - if unknown open is assumed unless onlyVerified == true
	static void	SetUDPFWCheckResult(bool succeeded, bool testCancelled, uint32_t fromIP, uint16_t incomingPort);
	static void	ReCheckFirewallUDP(bool setUnverified);
	static bool	IsFWCheckUDPRunning() throw()		{ return m_fwChecksFinishedUDP < UDP_FIREWALLTEST_CLIENTSTOASK && !CKademlia::IsRunningInLANMode(); }
	static bool	IsVerified() throw()			{ return m_isFWVerifiedUDP || CKademlia::IsRunningInLANMode(); }

	static void	AddPossibleTestContact(const CUInt128& clientID, uint32_t ip, uint16_t port, uint16_t tport, const CUInt128& target, uint8_t version, const CKadUDPKey& udpKey, bool ipVerified)
	{
		if (!IsFWCheckUDPRunning()) {
			return;
		}
		// add the possible contact to our list - no checks in advance
		m_possibleTestClients.push_front(CContact(clientID, ip, port, tport, version, udpKey, ipVerified, target));
		QueryNextClient();
	}

	static void	Reset(); // when stopping Kad
	static void	Connected();
	static void	QueryNextClient(); // try the next available client for the firewallcheck

      private:
	// are we in search for testclients
	static bool	GetUDPCheckClientsNeeded() throw()	{ return (m_fwChecksRunningUDP + m_fwChecksFinishedUDP) < UDP_FIREWALLTEST_CLIENTSTOASK; }
	static bool	m_firewalledUDP;
	static bool	m_firewalledLastStateUDP;
	static bool	m_isFWVerifiedUDP;
	static bool	m_nodeSearchStarted;
	static bool	m_timedOut;
	static uint8_t	m_fwChecksRunningUDP;
	static uint8_t	m_fwChecksFinishedUDP;
	static uint32_t	m_testStart;
	static uint32_t	m_lastSucceededTime;
	typedef std::list<CContact> PossibleClientList;
	typedef std::list<UsedClient_Struct> UsedClientList;
	static PossibleClientList m_possibleTestClients;
	static UsedClientList	m_usedTestClients;
};

} // namespace Kademlia

#endif /* KADEMLIA_KADEMLIA_UDPFIREWALLTESTER */

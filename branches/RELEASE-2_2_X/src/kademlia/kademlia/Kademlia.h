//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2009 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __KAD_KADEMLIA_H__
#define __KAD_KADEMLIA_H__

#include <map>
#include "../utils/UInt128.h"
#include "Prefs.h"
#include "../routing/Maps.h"
#include "../net/KademliaUDPListener.h"
#include <common/Macros.h>


////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CRoutingZone;
class CIndexed;
class CKadUDPKey;
class CKadClientSearcher;

typedef std::map<CRoutingZone*, CRoutingZone*> EventMap;

class CKademlia
{
public:
	static void Start() 		{ Start(new CPrefs); }
	static void Start(CPrefs *prefs);
	static void Stop();
 
	static CPrefs *			GetPrefs() throw()		{ if (instance == NULL || instance->m_prefs == NULL) return NULL; else return instance->m_prefs; }
	static CRoutingZone *		GetRoutingZone() throw()	{ wxCHECK(instance && instance->m_routingZone, NULL); return instance->m_routingZone; }
	static CKademliaUDPListener *	GetUDPListener() throw()	{ wxCHECK(instance && instance->m_udpListener, NULL); return instance->m_udpListener; }
	static CIndexed *		GetIndexed() throw()		{ wxCHECK(instance && instance->m_indexed, NULL); return instance->m_indexed; }
	static bool			IsRunning() throw()		{ return m_running; }
	static bool			IsConnected() throw()		{ return instance && instance->m_prefs ? instance->m_prefs->HasHadContact() : false; }
	static bool			IsFirewalled() throw()		{ return instance && instance->m_prefs ? instance->m_prefs->GetFirewalled() : true; }
	static void			RecheckFirewalled();
	static uint32_t			GetKademliaUsers() throw()	{ return instance && instance->m_prefs ? instance->m_prefs->GetKademliaUsers() : 0; }
	static uint32_t			GetKademliaFiles() throw()	{ return instance && instance->m_prefs ? instance->m_prefs->GetKademliaFiles() : 0; }
	static uint32_t			GetTotalStoreKey() throw()	{ return instance && instance->m_prefs ? instance->m_prefs->GetTotalStoreKey() : 0; }
	static uint32_t			GetTotalStoreSrc() throw()	{ return instance && instance->m_prefs ? instance->m_prefs->GetTotalStoreSrc() : 0; }
	static uint32_t			GetTotalStoreNotes() throw()	{ return instance && instance->m_prefs ? instance->m_prefs->GetTotalStoreNotes() : 0; }
	static uint32_t			GetTotalFile() throw()		{ return instance && instance->m_prefs ? instance->m_prefs->GetTotalFile() : 0; }
	static bool			GetPublish() throw()		{ return instance && instance->m_prefs ? instance->m_prefs->GetPublish() : false; }
	static uint32_t			GetIPAddress() throw()		{ return instance && instance->m_prefs ? instance->m_prefs->GetIPAddress() : 0; }
	static void Bootstrap(uint32_t ip, uint16_t port, bool kad2)
	{
		time_t now = time(NULL);
		if (instance && instance->m_udpListener && !IsConnected() && now - m_bootstrap > 10) {
			m_bootstrap = now;
			instance->m_udpListener->Bootstrap(ip, port, kad2);
		}
	}

	static void ProcessPacket(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port, bool validReceiverKey, const CKadUDPKey& senderKey);

	static void AddEvent(CRoutingZone *zone) throw()		{ m_events[zone] = zone; }
	static void RemoveEvent(CRoutingZone *zone)			{ m_events.erase(zone); }
	static void Process();
	static bool FindNodeIDByIP(CKadClientSearcher& requester, uint32_t ip, uint16_t tcpPort, uint16_t udpPort);
	static bool FindIPByNodeID(CKadClientSearcher& requester, const uint8_t *nodeID);
	static void CancelClientSearch(CKadClientSearcher& fromRequester);

	static ContactList	s_bootstrapList;

private:
	CKademlia() {}

	static CKademlia *instance;
	static EventMap	m_events;
	static time_t	m_nextSearchJumpStart;
	static time_t	m_nextSelfLookup;
	static time_t	m_nextFirewallCheck;
	static time_t	m_nextFindBuddy;
	static time_t	m_statusUpdate;
	static time_t	m_bigTimer;
	static time_t	m_bootstrap;
	static time_t	m_consolidate;
	static time_t	m_externPortLookup;
	static bool	m_running;

	CPrefs *		m_prefs;
	CRoutingZone *		m_routingZone;
	CKademliaUDPListener *	m_udpListener;
	CIndexed *		m_indexed;
};

} // End namespace

void KadGetKeywordHash(const wxString& rstrKeyword, Kademlia::CUInt128* pKadID);
wxString KadGetKeywordBytes(const wxString& rstrKeywordW);

#endif // __KAD_KADEMLIA_H__

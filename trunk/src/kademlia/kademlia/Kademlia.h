//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include "../../Types.h"
#include "../utils/UInt128.h"

class CSharedFileList;
struct Status;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs;
class CRoutingZone;
class CKademliaUDPListener;
class CKademliaError;
class CSearch;
class CContact;
class CIndexed;
class CEntry;

typedef std::map<CRoutingZone*, CRoutingZone*> EventMap;

#define KADEMLIA_VERSION 0.1

class CKademlia
{
public:
	static void start(void);
	static void start(CPrefs *prefs);
	static void stop();
 
	static CPrefs				*getPrefs(void);
	static CRoutingZone			*getRoutingZone(void);
	static CKademliaUDPListener	*getUDPListener(void);
	static CIndexed				*getIndexed(void);
	static bool					isRunning(void) {return m_running;}
	static bool					isConnected(void);
	static bool					IsFirewalled(void);
	static void					RecheckFirewalled(void);
	static uint32				getKademliaUsers(void);
	static uint32				getKademliaFiles(void);
	static uint32				getTotalStoreKey(void);
	static uint32				getTotalStoreSrc(void);
	static uint32				getTotalStoreNotes(void);
	static uint32				getTotalFile(void);
	static bool					getPublish(void);
	static uint32				getIPAddress(void);
	static void					bootstrap(uint32 ip, uint16 port);
	static void					processPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port);

	static void addEvent(CRoutingZone *zone);
	static void removeEvent(CRoutingZone *zone);
	static void process();

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
	static bool		m_running;

	CPrefs					*m_prefs;
	CRoutingZone			*m_routingZone;
	CKademliaUDPListener	*m_udpListener;
	CIndexed				*m_indexed;
};

} // End namespace

void KadGetKeywordHash(const wxString& rstrKeyword, Kademlia::CUInt128* pKadID);
wxString KadGetKeywordBytes(const wxString& rstrKeywordW);

#endif // __KAD_KADEMLIA_H__

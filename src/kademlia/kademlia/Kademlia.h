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
	static void Start(void);
	static void Start(CPrefs *prefs);
	static void Stop();
 
	static CPrefs				*GetPrefs(void);
	static CRoutingZone			*GetRoutingZone(void);
	static CKademliaUDPListener	*GetUDPListener(void);
	static CIndexed				*GetIndexed(void);
	static bool					IsRunning(void) {return m_running;}
	static bool					IsConnected(void);
	static bool					IsFirewalled(void);
	static void					RecheckFirewalled(void);
	static uint32				GetKademliaUsers(void);
	static uint32				GetKademliaFiles(void);
	static uint32				GetTotalStoreKey(void);
	static uint32				GetTotalStoreSrc(void);
	static uint32				GetTotalStoreNotes(void);
	static uint32				GetTotalFile(void);
	static bool					GetPublish(void);
	static uint32				GetIPAddress(void);
	static void					Bootstrap(uint32 ip, uint16 port);
	static void					ProcessPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port);

	static void AddEvent(CRoutingZone *zone);
	static void RemoveEvent(CRoutingZone *zone);
	static void Process();

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
// File_checked_for_headers

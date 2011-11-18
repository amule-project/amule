//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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

#include "Kademlia.h" // Interface declarations

#include <protocol/kad/Constants.h>

#include "Defines.h"
#include "Indexed.h"
#include "UDPFirewallTester.h"
#include "../routing/RoutingZone.h"
#include "../utils/KadUDPKey.h"
#include "../utils/KadClientSearcher.h"
#include "../../amule.h"
#include "../../Logger.h"
#include <protocol/kad2/Client2Client/UDP.h>

#ifdef _MSC_VER  // silly warnings about deprecated functions
#pragma warning(disable:4996)
#endif

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CKademlia *	CKademlia::instance = NULL;
EventMap	CKademlia::m_events;
time_t		CKademlia::m_nextSearchJumpStart;
time_t		CKademlia::m_nextSelfLookup;
time_t		CKademlia::m_statusUpdate;
time_t		CKademlia::m_bigTimer;
time_t		CKademlia::m_nextFirewallCheck;
time_t		CKademlia::m_nextFindBuddy;
time_t		CKademlia::m_bootstrap;
time_t		CKademlia::m_consolidate;
time_t		CKademlia::m_externPortLookup;
time_t		CKademlia::m_lanModeCheck = 0;
bool		CKademlia::m_running = false;
bool		CKademlia::m_lanMode = false;
ContactList	CKademlia::s_bootstrapList;
std::list<uint32_t>	CKademlia::m_statsEstUsersProbes;

void CKademlia::Start(CPrefs *prefs)
{
	if (instance) {
		// If we already have an instance, something is wrong.
		delete prefs;
		wxASSERT(instance->m_running);
		wxASSERT(instance->m_prefs);
		return;
	}

	// Make sure a prefs was passed in..
	if (!prefs) {
		return;
	}

	AddDebugLogLineN(logKadMain, wxT("Starting Kademlia"));

	// Init jump start timer.
	m_nextSearchJumpStart = time(NULL);
	// Force a FindNodeComplete within the first 3 minutes.
	m_nextSelfLookup = time(NULL) + MIN2S(3);
	// Init status timer.
	m_statusUpdate = time(NULL);
	// Init big timer for Zones
	m_bigTimer = time(NULL);
	// First Firewall check is done on connect, init next check.
	m_nextFirewallCheck = time(NULL) + (HR2S(1));
	// Find a buddy after the first 5mins of starting the client.
	// We wait just in case it takes a bit for the client to determine firewall status..
	m_nextFindBuddy = time(NULL) + (MIN2S(5));
	// Init contact consolidate timer;
	m_consolidate = time(NULL) + (MIN2S(45));
	// Look up our extern port
	m_externPortLookup = time(NULL);
	// Init bootstrap time.
	m_bootstrap = 0;
	// Init our random seed.
	srand((uint32_t)time(NULL));
	// Create our Kad objects.
	instance = new CKademlia();
	instance->m_prefs = prefs;
	instance->m_indexed = new CIndexed();
	instance->m_routingZone = new CRoutingZone();
	instance->m_udpListener = new CKademliaUDPListener();
	// Mark Kad as running state.
	m_running = true;
}


void CKademlia::Stop()
{
	// Make sure we are running to begin with.
	if (!m_running) {
		return;
	}

	AddDebugLogLineN(logKadMain, wxT("Stopping Kademlia"));

	// Mark Kad as being in the stop state to make sure nothing else is used.
	m_running = false;

	// Reset Firewallstate
	CUDPFirewallTester::Reset();

	// Remove all active searches.
	CSearchManager::StopAllSearches();

	// Delete all Kad Objects.
	delete instance->m_udpListener;
	instance->m_udpListener = NULL;

	delete instance->m_routingZone;
	instance->m_routingZone = NULL;

	delete instance->m_indexed;
	instance->m_indexed = NULL;

	delete instance->m_prefs;
	instance->m_prefs = NULL;

	delete instance;
	instance = NULL;

	for (ContactList::iterator it = s_bootstrapList.begin(); it != s_bootstrapList.end(); ++it) {
		delete *it;
	}
	s_bootstrapList.clear();

	// Make sure all zones are removed.
	m_events.clear();

//	theApp->ShowConnectionState();
}

void CKademlia::Process()
{

	if (instance == NULL || !m_running) {
		return;
	}

	time_t now = time(NULL);
	uint32_t maxUsers = 0;
	uint32_t tempUsers = 0;
	uint32_t lastContact = 0;
	bool updateUserFile = false;

	wxASSERT(instance->m_prefs != NULL);
	lastContact = instance->m_prefs->GetLastContact();
	CSearchManager::UpdateStats();

	if (m_statusUpdate <= now) {
		updateUserFile = true;
		m_statusUpdate = MIN2S(1) + now;
	}

	if (m_nextFirewallCheck <= now) {
		RecheckFirewalled();
	}

	if (m_nextSelfLookup <= now) {
		CSearchManager::FindNode(instance->m_prefs->GetKadID(), true);
		m_nextSelfLookup = HR2S(4) + now;
	}

	if (m_nextFindBuddy <= now) {
		instance->m_prefs->SetFindBuddy();
		m_nextFindBuddy = MIN2S(20) + now;
	}

	if (m_externPortLookup <= now && CUDPFirewallTester::IsFWCheckUDPRunning() && GetPrefs()->FindExternKadPort(false)) {
		// If our UDP firewallcheck is running and we don't know our external port, we send a request every 15 seconds
		CContact *contact = GetRoutingZone()->GetRandomContact(3, 6);
		if (contact != NULL) {
			AddDebugLogLineN(logKadPrefs, wxT("Requesting our external port from ") + KadIPToString(contact->GetIPAddress()));
			DebugSend(Kad2Ping, contact->GetIPAddress(), contact->GetUDPPort());
			GetUDPListener()->SendNullPacket(KADEMLIA2_PING, contact->GetIPAddress(), contact->GetUDPPort(), contact->GetUDPKey(), &contact->GetClientID());
		} else {
			AddDebugLogLineN(logKadPrefs, wxT("No valid client for requesting external port available"));
		}
		m_externPortLookup = 15 + now;
	}

	for (EventMap::const_iterator it = m_events.begin(); it != m_events.end(); ++it) {
		CRoutingZone *zone = it->first;
		if (updateUserFile) {
			// The EstimateCount function is not made for really small networks, if we are in LAN mode, it is actually
			// better to assume that all users of the network are in our routing table and use the real count function
			if (IsRunningInLANMode()) {
				tempUsers = zone->GetNumContacts();
			} else {
				tempUsers = zone->EstimateCount();
			}
			if (maxUsers < tempUsers) {
				maxUsers = tempUsers;
			}
		}

		if (m_bigTimer <= now) {
			if (zone->m_nextBigTimer <= now) {
				if(zone->OnBigTimer()) {
					zone->m_nextBigTimer = HR2S(1) + now;
					m_bigTimer = SEC(10) + now;
				}
			} else {
				if (lastContact && (now - lastContact > KADEMLIADISCONNECTDELAY - MIN2S(5))) {
					if(zone->OnBigTimer()) {
						zone->m_nextBigTimer = HR2S(1) + now;
						m_bigTimer = SEC(10) + now;
					}
				} 
			}
		}

		if (zone->m_nextSmallTimer <= now) {
			zone->OnSmallTimer();
			zone->m_nextSmallTimer = MIN2S(1) + now;
		}
	}
	
	// This is a convenient place to add this, although not related to routing
	if (m_nextSearchJumpStart <= now) {
		CSearchManager::JumpStart();
		m_nextSearchJumpStart = SEARCH_JUMPSTART + now;
	}

	// Try to consolidate any zones that are close to empty.
	if (m_consolidate <= now) {
		uint32_t mergedCount = instance->m_routingZone->Consolidate();
		if (mergedCount) {
			AddDebugLogLineN(logKadRouting, CFormat(wxT("Kad merged %u zones")) % mergedCount);
		}
		m_consolidate = MIN2S(45) + now;
	}

	// Update user count only if changed.
	if (updateUserFile) {
		if (maxUsers != instance->m_prefs->GetKademliaUsers()) {
			instance->m_prefs->SetKademliaUsers(maxUsers);
			instance->m_prefs->SetKademliaFiles();
			theApp->ShowUserCount();
		}
	}

	if (!IsConnected() && !s_bootstrapList.empty() && (now - m_bootstrap > 15 || (GetRoutingZone()->GetNumContacts() == 0 && now - m_bootstrap >= 2))) {
		CContact *contact = s_bootstrapList.front();
		s_bootstrapList.pop_front();
		m_bootstrap = now;
		AddDebugLogLineN(logKadMain, CFormat(wxT("Trying to bootstrap Kad from %s, Distance: %s Version: %u, %u contacts left"))
			% KadIPToString(contact->GetIPAddress()) % contact->GetDistance().ToHexString() % contact->GetVersion() % s_bootstrapList.size());
		instance->m_udpListener->Bootstrap(contact->GetIPAddress(), contact->GetUDPPort(), contact->GetVersion(), &contact->GetClientID());
		delete contact;
	}

	if (GetUDPListener() != NULL) {
		GetUDPListener()->ExpireClientSearch();	// function does only one compare in most cases, so no real need for a timer
	}
}

void CKademlia::ProcessPacket(const uint8_t *data, uint32_t lenData, uint32_t ip, uint16_t port, bool validReceiverKey, const CKadUDPKey& senderKey)
{
	try {
		if( instance && instance->m_udpListener ) {
			instance->m_udpListener->ProcessPacket(data, lenData, ip, port, validReceiverKey, senderKey);
		}
	} catch (const wxString& DEBUG_ONLY(error)) {
		AddDebugLogLineN(logKadMain, CFormat(wxT("Exception on Kad ProcessPacket while processing packet (length = %u) from %s:"))
			% lenData % KadIPPortToString(ip, port));
		AddDebugLogLineN(logKadMain, error);
		throw;
	} catch (...) {
		AddDebugLogLineN(logKadMain, wxT("Unhandled exception on Kad ProcessPacket"));
		throw;
	}
}

void CKademlia::RecheckFirewalled()
{
	if (instance && instance->m_prefs && !IsRunningInLANMode()) {
		// Something is forcing a new firewall check
		// Stop any new buddy requests, and tell the client
		// to recheck it's IP which in turns rechecks firewall.
		instance->m_prefs->SetFindBuddy(false);
		instance->m_prefs->SetRecheckIP();
		// also UDP check
		CUDPFirewallTester::ReCheckFirewallUDP(false);
		time_t now = time(NULL);
		// Delay the next buddy search to at least 5 minutes after our firewallcheck so we are sure to be still firewalled
		m_nextFindBuddy = (m_nextFindBuddy < MIN2S(5) + now) ? MIN2S(5) + now : m_nextFindBuddy;
		m_nextFirewallCheck = HR2S(1) + now;
	}
}

bool CKademlia::FindNodeIDByIP(CKadClientSearcher& requester, uint32_t ip, uint16_t tcpPort, uint16_t udpPort)
{
	wxCHECK(IsRunning() && instance && GetUDPListener() && GetRoutingZone(), false);

	// first search our known contacts if we can deliver a result without asking, otherwise forward the request
	CContact* contact;
	if ((contact = GetRoutingZone()->GetContact(wxUINT32_SWAP_ALWAYS(ip), tcpPort, true)) != NULL) {
		uint8_t nodeID[16];
		contact->GetClientID().ToByteArray(nodeID);
		requester.KadSearchNodeIDByIPResult(KCSR_SUCCEEDED, nodeID);
		return true;
	} else {
		return GetUDPListener()->FindNodeIDByIP(&requester, wxUINT32_SWAP_ALWAYS(ip), tcpPort, udpPort);
	}
}

bool CKademlia::FindIPByNodeID(CKadClientSearcher& requester, const uint8_t* nodeID)
{
	wxCHECK(IsRunning() && instance && GetUDPListener(), false);

	// first search our known contacts if we can deliver a result without asking, otherwise forward the request
	CContact* contact;
	if ((contact = GetRoutingZone()->GetContact(CUInt128(nodeID))) != NULL) {
		// make sure that this entry is not too old, otherwise just do a search to be sure
		if (contact->GetLastSeen() != 0 && time(NULL) - contact->GetLastSeen() < 1800) {
			requester.KadSearchIPByNodeIDResult(KCSR_SUCCEEDED, wxUINT32_SWAP_ALWAYS(contact->GetIPAddress()), contact->GetTCPPort());
			return true;
		}
	}
	return CSearchManager::FindNodeSpecial(CUInt128(nodeID), &requester);
}

void CKademlia::CancelClientSearch(CKadClientSearcher& fromRequester)
{
	wxCHECK_RET(instance && GetUDPListener(), wxT("Something is really bad"));

	GetUDPListener()->ExpireClientSearch(&fromRequester);
	CSearchManager::CancelNodeSpecial(&fromRequester);
}

void CKademlia::StatsAddClosestDistance(const CUInt128& distance)
{
	if (distance.Get32BitChunk(0) > 0) {
		uint32_t toAdd = (0xFFFFFFFF / distance.Get32BitChunk(0)) / 2;
		std::list<uint32_t>::iterator it = m_statsEstUsersProbes.begin();
		for (; it != m_statsEstUsersProbes.end(); ++it) {
			if (*it == toAdd) {
				break;
			}
		}
		if (it == m_statsEstUsersProbes.end()) {
			m_statsEstUsersProbes.push_front(toAdd);
		}
	}
	if (m_statsEstUsersProbes.size() > 100) {
		m_statsEstUsersProbes.pop_back();
	}
}

uint32_t CKademlia::CalculateKadUsersNew()
{
	// the idea of calculating the user count with this method is simple:
	// whenever we do a search for any NodeID (except in certain cases where the result is not usable),
	// we remember the distance of the closest node we found. Because we assume all NodeIDs are distributed
	// equally, we can calculate based on this distance how "filled" the possible NodesID room is and by this
	// calculate how many users there are. Of course this only works if we have enough samples, because
	// each single sample will be wrong, but the average of them should produce a usable number. To avoid
	// drifts caused by a a single (or more) really close or really far away hits, we do use median-average instead through

	// doesn't work well if we have no files to index and nothing to download and the numbers seems to be a bit too low
	// compared to our other method. So let's stay with the old one for now, but keep this here as an alternative

	if (m_statsEstUsersProbes.size() < 10) {
		return 0;
	}
	uint32_t median = 0;

	std::list<uint32_t> medianList;
	for (std::list<uint32_t>::iterator it1 = m_statsEstUsersProbes.begin(); it1 != m_statsEstUsersProbes.end(); ++it1) {
		uint32_t probe = *it1;
		bool inserted = false;
		for (std::list<uint32_t>::iterator it2 = medianList.begin(); it2 != medianList.end(); ++it2) {
			if (*it2 > probe) {
				medianList.insert(it2, probe);
				inserted = true;
				break;
			}
		}
		if (!inserted) {
			medianList.push_back(probe);
		}
	}
	// cut away 1/3 of the values - 1/6 of the top and 1/6 of the bottom  to avoid spikes having too much influence, build the average of the rest
	std::list<uint32_t>::size_type cut = medianList.size() / 6;
	for (std::list<uint32_t>::size_type i = 0; i != cut; ++i) {
		medianList.pop_front();
		medianList.pop_back();
	}
	uint64_t average = 0;
	for (std::list<uint32_t>::iterator it = medianList.begin(); it != medianList.end(); ++it) {
		average += *it;
	}
	median = (uint32_t)(average / medianList.size());

	// LowIDModififier
	// Modify count by assuming 20% of the users are firewalled and can't be a contact for < 0.49b nodes
	// Modify count by actual statistics of Firewalled ratio for >= 0.49b if we are not firewalled ourself
	// Modify count by 40% for >= 0.49b if we are firewalled ourself (the actual Firewalled count at this date on kad is 35-55%)
	const float firewalledModifyOld = 1.20f;
	float firewalledModifyNew = 0.0;
	if (CUDPFirewallTester::IsFirewalledUDP(true)) {
		firewalledModifyNew = 1.40f; // we are firewalled and can't get the real statistics, assume 40% firewalled >=0.49b nodes
	} else if (GetPrefs()->StatsGetFirewalledRatio(true) > 0) {
		firewalledModifyNew = 1.0 + (CKademlia::GetPrefs()->StatsGetFirewalledRatio(true)); // apply the firewalled ratio to the modify
		wxASSERT(firewalledModifyNew > 1.0 && firewalledModifyNew < 1.90);
	}
	float newRatio = CKademlia::GetPrefs()->StatsGetKadV8Ratio();
	float firewalledModifyTotal = 0.0;
	if (newRatio > 0 && firewalledModifyNew > 0) { // weigth the old and the new modifier based on how many new contacts we have
		firewalledModifyTotal = (newRatio * firewalledModifyNew) + ((1 - newRatio) * firewalledModifyOld);
	} else {
		firewalledModifyTotal = firewalledModifyOld;
	}
	wxASSERT(firewalledModifyTotal > 1.0 && firewalledModifyTotal < 1.90);

	return (uint32_t)((float)median * firewalledModifyTotal);
}

bool CKademlia::IsRunningInLANMode()
{
	if (thePrefs::FilterLanIPs() || !IsRunning()) {
		return false;
	}

	time_t now = time(NULL);
	if (m_lanModeCheck + 10 <= now) {
		m_lanModeCheck = now;
		uint32_t count = GetRoutingZone()->GetNumContacts();
		// Limit to 256 nodes, if we have more we don't want to use the LAN mode which is assuming we use a small home LAN
		// (otherwise we might need to do firewallcheck, external port requests etc after all)
		if (count == 0 || count > 256) {
			m_lanMode = false;
		} else {
			if (GetRoutingZone()->HasOnlyLANNodes()) {
				if (!m_lanMode) {
					m_lanMode = true;
					theApp->ShowConnectionState();
					AddDebugLogLineN(logKadMain, wxT("Activating LAN mode"));
				}
			} else {
				if (m_lanMode) {
					m_lanMode = false;
					theApp->ShowConnectionState();
					AddDebugLogLineN(logKadMain, wxT("Deactivating LAN mode"));
				}
			}
		}
	}
	return m_lanMode;
}


// Global function.

#include "../../CryptoPP_Inc.h"
void KadGetKeywordHash(const wxString& rstrKeyword, Kademlia::CUInt128* pKadID)
{
	byte Output[16];

	#ifdef __WEAK_CRYPTO__
		CryptoPP::Weak::MD4 md4_hasher;
	#else
		CryptoPP::MD4 md4_hasher;
	#endif

	// This should be safe - we assume rstrKeyword is ANSI anyway.
	char* ansi_buffer = strdup(unicode2UTF8(rstrKeyword));
	
	//printf("Kad keyword hash: UTF8 %s\n",ansi_buffer);
	md4_hasher.CalculateDigest(Output,(const unsigned char*)ansi_buffer,strlen(ansi_buffer));
	//DumpMem(Output,16);
	free(ansi_buffer);
	
	pKadID->SetValueBE(Output);
}

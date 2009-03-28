//
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
bool		CKademlia::m_running = false;
ContactList	CKademlia::s_bootstrapList;

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

	AddDebugLogLineM(false, logKadMain, wxT("Starting Kademlia"));

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

	AddDebugLogLineM(false, logKadMain, wxT("Stopping Kademlia"));

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
			AddDebugLogLineM(false, logKadMain, wxT("Requesting our external port from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(contact->GetIPAddress())));
			DebugSend(Kad2Ping, contact->GetIPAddress(), contact->GetUDPPort());
			GetUDPListener()->SendNullPacket(KADEMLIA2_PING, contact->GetIPAddress(), contact->GetUDPPort(), contact->GetUDPKey(), &contact->GetClientID());
		} else {
			AddDebugLogLineM(false, logKadMain, wxT("No valid client for requesting external port available"));
		}
		m_externPortLookup = 15 + now;
	}

	for (EventMap::const_iterator it = m_events.begin(); it != m_events.end(); ++it) {
		CRoutingZone *zone = it->first;
		if (updateUserFile) {
			tempUsers = zone->EstimateCount();
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
			AddDebugLogLineM(false, logKadRouting, wxString::Format(wxT("Kad merged %u zones"), mergedCount));
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
		AddDebugLogLineM(false, logKadMain, wxT("Trying to bootstrap Kad from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(contact->GetIPAddress())) + wxT(", Distance: ") + contact->GetDistance().ToHexString() + wxString::Format(wxT(" Version: %u, %u contacts left"), contact->GetVersion(), s_bootstrapList.size()));
		instance->m_udpListener->Bootstrap(contact->GetIPAddress(), contact->GetUDPPort(), contact->GetVersion() > 1, contact->GetVersion(), &contact->GetClientID());
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
	} catch (const wxString& error) {
		AddDebugLogLineM(false, logKadMain, wxString::Format(wxT("Exception on Kad ProcessPacket while processing packet (length = %u) from "), lenData) + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port) + wxT(':'));
		AddDebugLogLineM(false, logKadMain, error);
		throw;
	} catch (...) {
		AddDebugLogLineM(false, logKadMain, wxT("Unhandled exception on Kad ProcessPacket"));
		throw;
	}
}

void CKademlia::RecheckFirewalled()
{
	if (instance && instance->m_prefs) {
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
// File_checked_for_headers

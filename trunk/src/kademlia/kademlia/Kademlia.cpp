//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
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

//#include "stdafx.h"
//#include "../utils/MiscUtils.h"
//#include "StringConversion.h"
//#include "MD4.h"

#include "Kademlia.h"
#include "Defines.h"
#include "Prefs.h"
#include "Error.h"
#include "SearchManager.h"
#include "Indexed.h"
#include "../net/KademliaUDPListener.h"
#include "../routing/RoutingZone.h"
#include "../../SharedFileList.h"
#include "../routing/Contact.h"
#include "amule.h"
#include "OPCodes.h"
#include "Preferences.h"
#include "Logger.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CKademlia	*CKademlia::instance = NULL;
EventMap	CKademlia::m_events;
time_t		CKademlia::m_nextSearchJumpStart;
time_t		CKademlia::m_nextSelfLookup;
time_t		CKademlia::m_statusUpdate;
time_t		CKademlia::m_bigTimer;
time_t		CKademlia::m_nextFirewallCheck;
time_t		CKademlia::m_nextFindBuddy;
time_t		CKademlia::m_bootstrap;
bool		CKademlia::m_running = false;

void CKademlia::start(void)
{
	if (instance != NULL) {
		return;
	}
	start(new CPrefs());
}

void CKademlia::start(CPrefs *prefs)
{
	if( m_running ) {
		delete prefs;
		return;
	}

	AddDebugLogLineM(false, logKadMain, wxT("Starting Kademlia"));

	m_nextSearchJumpStart = time(NULL);
	m_nextSelfLookup = time(NULL) + MIN2S(3);
	m_statusUpdate = time(NULL);
	m_bigTimer = time(NULL);
	m_nextFirewallCheck = time(NULL) + (HR2S(1));
	// First Firewall check is done on connect, find a buddy after the first 10min of starting
	// the client to try to allow it to settle down..
	m_nextFindBuddy = time(NULL) + (MIN2S(5));
	m_bootstrap = 0;

	srand((uint32)time(NULL));
	instance = new CKademlia();	
	instance->m_prefs = prefs;
	instance->m_udpListener = NULL;
	instance->m_routingZone = NULL;
	instance->m_indexed = new CIndexed();
	instance->m_routingZone = new CRoutingZone();
	instance->m_udpListener = new CKademliaUDPListener();
	m_running = true;
}


void CKademlia::stop()
{	
	if( !m_running ) {
		return;
	}
	
	AddDebugLogLineM(false, logKadMain, wxT("Stopping Kademlia"));
	m_running = false;

	CSearchManager::stopAllSearches();
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

	m_events.clear();
	
	theApp.ShowConnectionState();
}

void CKademlia::process()
{

	if( instance == NULL || !m_running) {
		return;
	}
	wxASSERT(instance != NULL);
	time_t now;
	CRoutingZone *zone;
	EventMap::const_iterator it;
	uint32 maxUsers = 0;
	uint32 tempUsers = 0;
	uint32 lastContact = 0;
	bool updateUserFile = false;
		
	now = time(NULL);
	wxASSERT(instance->m_prefs != NULL);
	lastContact = instance->m_prefs->getLastContact();
	CSearchManager::updateStats();
	if( m_statusUpdate <= now ) {
		updateUserFile = true;
		m_statusUpdate = MIN2S(1) + now;
	}
	if( m_nextFirewallCheck <= now) {
		RecheckFirewalled();
	}
	if (m_nextSelfLookup <= now) {
		CUInt128 me;
		instance->m_prefs->getKadID(&me);
		CSearchManager::findNodeComplete(me);
		m_nextSelfLookup = HR2S(4) + now;
	}
	if (m_nextFindBuddy <= now) {
		instance->m_prefs->setFindBuddy();
		m_nextFindBuddy = MIN2S(5) + m_nextFirewallCheck;
	}
	for (it = m_events.begin(); it != m_events.end(); it++) {
		zone = it->first;
		if( updateUserFile ) {
			tempUsers = zone->estimateCount();
			if( maxUsers < tempUsers ) {
				maxUsers = tempUsers;
			}
		}
		if (m_bigTimer <= now) {
			if( zone->m_nextBigTimer <= now ) {
				if(zone->onBigTimer()) {
					zone->m_nextBigTimer = HR2S(1) + now;
					m_bigTimer = SEC(10) + now;
				}
			} else {
				if( lastContact && ( (now - lastContact) > (KADEMLIADISCONNECTDELAY-MIN2S(5)))) {
					if(zone->onBigTimer()) {
						zone->m_nextBigTimer = HR2S(1) + now;
						m_bigTimer = SEC(10) + now;
					}
				} 
			}
		}

		if (zone->m_nextSmallTimer <= now) {
			zone->onSmallTimer();
			zone->m_nextSmallTimer = MIN2S(1) + now;
		}
			// This is a convenient place to add this, although not related to routing
		if (m_nextSearchJumpStart <= now) {
			CSearchManager::jumpStart();
			m_nextSearchJumpStart += SEARCH_JUMPSTART;
		}
	}

	//Update user count only if changed.
	if( updateUserFile ) {
		if( maxUsers != instance->m_prefs->getKademliaUsers()) {
			instance->m_prefs->setKademliaUsers(maxUsers);
			instance->m_prefs->setKademliaFiles();
			theApp.ShowUserCount();
		}
	}
}

void CKademlia::addEvent(CRoutingZone *zone)
{
	m_events[zone] = zone;
}

void CKademlia::removeEvent(CRoutingZone *zone)
{
	m_events.erase(zone);
}

bool CKademlia::isConnected(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->hasHadContact();
	}
	return false;
}

bool CKademlia::isFirewalled(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getFirewalled();
	}
	return true;
}

uint32 CKademlia::getKademliaUsers(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getKademliaUsers();
	}
	return 0;
}

uint32 CKademlia::getKademliaFiles(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getKademliaFiles();
	}
	return 0;
}

uint32 CKademlia::getTotalStoreKey(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getTotalStoreKey();
	}
	return 0;
}

uint32 CKademlia::getTotalStoreSrc(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getTotalStoreSrc();
	}
	return 0;
}

uint32 CKademlia::getTotalStoreNotes(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getTotalStoreNotes();
	}
	return 0;
}

uint32 CKademlia::getTotalFile(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getTotalFile();
	}
	return 0;
}

uint32 CKademlia::getIPAddress(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getIPAddress();
	}
	return 0;
}

void CKademlia::processPacket(const byte *data, uint32 lenData, uint32 ip, uint16 port)
{
	try {
		if( instance && instance->m_udpListener ) {
			instance->m_udpListener->processPacket( data, lenData, ip, port);
		}
	} catch (const wxString& error) {
		AddDebugLogLineM(false, logKadMain, wxT("Exception on Kad processPacket: ") + error);
		throw;		
	} catch (...) {
		AddDebugLogLineM(false, logKadMain, wxT("Unhandled exception on Kad processPacket"));
		throw;
	}
}

bool CKademlia::getPublish(void)
{
	if( instance && instance->m_prefs ) {
		return instance->m_prefs->getPublish();
	}
	return 0;
}

void CKademlia::bootstrap(const wxString& host, uint16 port)
{
	if( instance && instance->m_udpListener && !isConnected() && time(NULL) - m_bootstrap > MIN2S(1) ) {
		instance->m_udpListener->bootstrap( host, port);
	}
}

void CKademlia::bootstrap(uint32 ip, uint16 port)
{
	if( instance && instance->m_udpListener && !isConnected() && time(NULL) - m_bootstrap > MIN2S(1) ) {
		instance->m_udpListener->bootstrap( ip, port);
	}
}

void CKademlia::RecheckFirewalled()
{
	if( instance && instance->getPrefs() ) {
		instance->m_prefs->setFindBuddy(false);
		instance->m_prefs->setRecheckIP();
		m_nextFindBuddy = MIN2S(5) + m_nextFirewallCheck;
		m_nextFirewallCheck = HR2S(1) + time(NULL);
	}
}

CPrefs *CKademlia::getPrefs(void)
{
	if (instance == NULL || instance->m_prefs == NULL) {
		wxASSERT(0);
		return NULL;
	}
	return instance->m_prefs;
}

CKademliaUDPListener *CKademlia::getUDPListener(void)
{
	if (instance == NULL || instance->m_udpListener == NULL) {
		wxASSERT(0);
		return NULL;
	}
	return instance->m_udpListener;
}

CRoutingZone *CKademlia::getRoutingZone(void)
{
	if (instance == NULL || instance->m_routingZone == NULL) {
		wxASSERT(0);
		return NULL;
	}
	return instance->m_routingZone;
}

CIndexed *CKademlia::getIndexed(void)
{
	if ( instance == NULL || instance->m_indexed == NULL) {
		wxASSERT(0);
		return NULL;
	}
	return instance->m_indexed;
}

// Global function.

#include "../../CryptoPP_Inc.h"
#include "StringFunctions.h"
void KadGetKeywordHash(const wxString& rstrKeyword, Kademlia::CUInt128* pKadID)
{
	byte Output[16];
	
	CryptoPP::MD4 md4_hasher; 	
	
	// This should be safe - we assume rstrKeyword is ANSI anyway.
	char* ansi_buffer = strdup(unicode2UTF8(rstrKeyword));
	
	//printf("Kad keyword hash: UTF8 %s\n",ansi_buffer);
	md4_hasher.CalculateDigest(Output,(const unsigned char*)ansi_buffer,strlen(ansi_buffer));
	//DumpMem(Output,16);
	free(ansi_buffer);
	
	pKadID->setValueBE(Output);
}

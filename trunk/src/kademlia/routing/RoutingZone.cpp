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

/**
 * The *Zone* is just a node in a binary tree of *Zone*s.
 * Each zone is either an internal node or a leaf node.
 * Internal nodes have "bin == null" and "subZones[i] != null",
 * leaf nodes have "subZones[i] == null" and "bin != null".
 * 
 * All key unique id's are relative to the center (self), which
 * is considered to be 000..000
 */
//#include "stdafx.h"
#include "RoutingZone.h"
#include "Contact.h"
#include "RoutingBin.h"
#include "../utils/UInt128.h"
//#include "../utils/MiscUtils.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Prefs.h"
#include "../kademlia/SearchManager.h"
#include "../kademlia/Defines.h"
#include "../kademlia/Error.h"
#include "../net/KademliaUDPListener.h"
#include "../../OtherFunctions.h"
#include "../../OPCodes.h"
#include "../../amule.h"
#include "../../CFile.h"
#include "../../Logger.h"
#include "../../NetworkFunctions.h"
#include "../../ArchSpecific.h"

#warning EC
#ifndef AMULE_DAEMON
	#include "../../amuleDlg.h"
	#include "../../KadDlg.h"
#endif

#include <cmath>
#include <algorithm>		// Needed for std::min

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

// This is just a safety precaution
#define CONTACT_FILE_LIMIT 5000

wxString CRoutingZone::m_filename;
CUInt128 CRoutingZone::me((uint32)0);

CRoutingZone::CRoutingZone()
{
	// Can only create routing zone after prefs
	me = CKademlia::getPrefs()->getKadID();
	m_filename = theApp.ConfigDir + wxT("nodes.dat");
	CUInt128 zero((uint32)0);
	init(NULL, 0, zero);
}

CRoutingZone::CRoutingZone(CRoutingZone *super_zone, int level, const CUInt128 &zone_index)
{
	init(super_zone, level, zone_index);
}

void CRoutingZone::init(CRoutingZone *super_zone, int level, const CUInt128 &zone_index)
{
	m_superZone = super_zone;
	m_level = level;
	m_zoneIndex = zone_index;
	m_subZones[0] = NULL;
	m_subZones[1] = NULL;
	m_bin = new CRoutingBin();

	m_nextSmallTimer = time(NULL) + m_zoneIndex.get32BitChunk(3);

	if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
		readFile();
	}

	startTimer();
}

CRoutingZone::~CRoutingZone()
{
	if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
		#warning EC
		#ifndef AMULE_DAEMON
		theApp.amuledlg->kademliawnd->HideNodes();
		#endif
		writeFile();
	}
	if (isLeaf()) {
		delete m_bin;
	} else {
		delete m_subZones[0];
		delete m_subZones[1];
	}
	#warning EC
	#ifndef AMULE_DAEMON
	if (m_superZone == NULL) {
		theApp.amuledlg->kademliawnd->ShowNodes();
	}
	#endif
}

void CRoutingZone::readFile(void)
{
	try {
		#warning EC
		#ifndef AMULE_DAEMON
		theApp.amuledlg->kademliawnd->HideNodes();
		#endif
		uint32 numContacts = 0;
		CFile file;
		if (file.Open(m_filename, CFile::read)) {

			numContacts = file.ReadUInt32();
			for (uint32 i = 0; i < numContacts; i++) {
				CUInt128 id = file.ReadUInt128();
				uint32 ip = file.ReadUInt32();
				uint16 udpPort = file.ReadUInt16();
				uint16 tcpPort = file.ReadUInt16();
				byte type = file.ReadUInt8();
				if(IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip),udpPort)) {
					if( type < 4) {
						add(id, ip, udpPort, tcpPort, type);
					}
				}
			}
			AddLogLineM( false, wxString::Format(_("Read %u Kad contacts"), numContacts));
		}
		if (numContacts == 0) {
			AddDebugLogLineM( false, logKadRouting, _("Error while reading Kad contacts - 0 entries"));
		}
	} catch (const CSafeIOException& e) {
		AddDebugLogLineM(false, logKadRouting, wxT("IO error in CRoutingZone::readFile: ") + e.what());
	}
	#warning EC
	#ifndef AMULE_DAEMON	
	theApp.amuledlg->kademliawnd->ShowNodes();
	#endif
}

void CRoutingZone::writeFile(void)
{
	try {
		unsigned int count = 0;
		CContact *c;
		CFile file;
		if (file.Open(m_filename, CFile::write)) {
			ContactList contacts;
			getBootstrapContacts(&contacts, 200);
			file.WriteUInt32((uint32)std::min((int)contacts.size(), CONTACT_FILE_LIMIT));
			ContactList::const_iterator it;
			for (it = contacts.begin(); it != contacts.end(); ++it) {
				count++;
				c = *it;
				file.WriteUInt128(c->getClientID());
				file.WriteUInt32(c->getIPAddress());
				file.WriteUInt16(c->getUDPPort());
				file.WriteUInt16(c->getTCPPort());
				file.WriteUInt8(c->getType());
				if (count == CONTACT_FILE_LIMIT) {
					break;
				}
			}
		}
		AddDebugLogLineM( false, logKadRouting, wxString::Format(wxT("Wrote %d contacts to file."), count));
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(false, logKadRouting, wxT("IO failure in CRoutingZone::writeFile: ") + e.what());
	}
}

bool CRoutingZone::canSplit(void) const
{
	if (m_level >= 127) {
		return false;
	}
		
	/* Check if we are close to the center */
	if ( (m_zoneIndex < KK || m_level < KBASE) && m_bin->getSize() == K) {
		return true;
	}
	return false;
}

bool CRoutingZone::add(const CUInt128 &id, uint32 ip, uint16 port, uint16 tport, byte type)
{
	
	//AddDebugLogLineM(false, logKadMain, wxT("Adding a contact (routing) with ip ") + Uint32_16toStringIP_Port(ip,port));
	
	if (id == me) {
		return false;
	}

	bool retVal = false;
	CUInt128 distance(me);
	distance.XOR(id);
	CContact *c = NULL;

	if (!isLeaf()) {
		retVal = m_subZones[distance.getBitNumber(m_level)]->add(id, ip, port, tport, type);
	} else {
		c = m_bin->getContact(id);
		if (c != NULL) {
			c->setIPAddress(ip);
			c->setUDPPort(port);
			c->setTCPPort(tport);
			retVal = true;
			#warning TODO: EC
			#ifndef AMULE_DAEMON
			theApp.amuledlg->kademliawnd->RefreshNode(c);
			#endif
		} else if (m_bin->getRemaining() > 0) {
			c = new CContact(id, ip, port, tport);
			retVal = m_bin->add(c);
			if(retVal) {
				#warning TODO: EC
				#ifndef AMULE_DAEMON
				theApp.amuledlg->kademliawnd->AddNode(c);
				#endif
			}
		} else if (canSplit()) {
			split();
			retVal = m_subZones[distance.getBitNumber(m_level)]->add(id, ip, port, tport, type);
		} else {
			merge();
			c = new CContact(id, ip, port, tport);
			retVal = m_bin->add(c);
			#warning TODO: EC
			#ifndef AMULE_DAEMON
			theApp.amuledlg->kademliawnd->AddNode(c);
			#endif
		}

		if (!retVal) {
			if (c != NULL) {
				delete c;
			}
		}
	}
	
	return retVal;
}

void CRoutingZone::remove(const CUInt128 &id) {
	CUInt128 distance(me);
	distance.XOR(id);
	if (!isLeaf()) {
		m_subZones[distance.getBitNumber(m_level)]->remove(id);
	} else {
		CContact *c = m_bin->getContact(id);
		if (c) {
			m_bin->remove(c);
		}
	}
}

void CRoutingZone::setAlive(uint32 ip, uint16 port)
{
	if (isLeaf()) {
		m_bin->setAlive(ip, port);
	} else {
		m_subZones[0]->setAlive(ip, port);
		m_subZones[1]->setAlive(ip, port);
	}
}

CContact *CRoutingZone::getContact(const CUInt128 &id) const
{
	if (isLeaf()) {
		return m_bin->getContact(id);
	} else {
		return m_subZones[id.getBitNumber(m_level)]->getContact(id);
	}
}

uint32 CRoutingZone::getClosestTo(uint32 maxType, const CUInt128 &target, const CUInt128 &distance, uint32 maxRequired, ContactMap *result, bool emptyFirst, bool inUse) const
{
	// If leaf zone, do it here
	if (isLeaf()) {
		return m_bin->getClosestTo(maxType, target, distance, maxRequired, result, emptyFirst, inUse);
	}
	
	// otherwise, recurse in the closer-to-the-target subzone first
	int closer = distance.getBitNumber(m_level);
	uint32 found = m_subZones[closer]->getClosestTo(maxType, target, distance, maxRequired, result, emptyFirst, inUse);
	
	// if still not enough tokens found, recurse in the other subzone too
	if (found < maxRequired) {
		found += m_subZones[1-closer]->getClosestTo(maxType, target, distance, maxRequired-found, result, false, inUse);
	}
	
	return found;
}

void CRoutingZone::getAllEntries(ContactList *result, bool emptyFirst)
{
	if (isLeaf()) {
		m_bin->getEntries(result, emptyFirst);
	} else {
		m_subZones[0]->getAllEntries(result, emptyFirst);
		m_subZones[1]->getAllEntries(result, false);			
	}
}

void CRoutingZone::topDepth(int depth, ContactList *result, bool emptyFirst)
{
	if (isLeaf()) {
		m_bin->getEntries(result, emptyFirst);
	} else if (depth <= 0) {
		randomBin(result, emptyFirst);
	} else {
		m_subZones[0]->topDepth(depth-1, result, emptyFirst);
		m_subZones[1]->topDepth(depth-1, result, false);
	}
}

void CRoutingZone::randomBin(ContactList *result, bool emptyFirst)
{
	if (isLeaf()) {
		m_bin->getEntries(result, emptyFirst);
	} else {
		m_subZones[rand()&1]->randomBin(result, emptyFirst);
	}
}

uint32 CRoutingZone::getMaxDepth(void) const
{
	if (isLeaf()) {
		return 0;
	}
	return 1 + std::max(m_subZones[0]->getMaxDepth(), m_subZones[1]->getMaxDepth());
}

void CRoutingZone::split(void)
{
	stopTimer();
		
	m_subZones[0] = genSubZone(0);
	m_subZones[1] = genSubZone(1);

	ContactList entries;
	m_bin->getEntries(&entries);
	ContactList::const_iterator it;
	for (it = entries.begin(); it != entries.end(); ++it) {
		int sz = (*it)->getDistance().getBitNumber(m_level);
		m_subZones[sz]->m_bin->add(*it);
	}
	m_bin->m_dontDeleteContacts = true;
	delete m_bin;
	m_bin = NULL;
}

void CRoutingZone::merge(void)
{
	if (isLeaf() && m_superZone != NULL) {
		m_superZone->merge();
	} else if ((!isLeaf())
		&& (m_subZones[0]->isLeaf() && m_subZones[1]->isLeaf()) 
		&&	(getNumContacts()) < (K/2) ) 
		{
		m_bin = new CRoutingBin();
		
		m_subZones[0]->stopTimer();
		m_subZones[1]->stopTimer();

		if (getNumContacts() > 0) {
			ContactList list0;
			ContactList list1;
			m_subZones[0]->m_bin->getEntries(&list0);
			m_subZones[1]->m_bin->getEntries(&list1);
			ContactList::const_iterator it;
			for (it = list0.begin(); it != list0.end(); ++it) {
				m_bin->add(*it);
			}
			for (it = list1.begin(); it != list1.end(); ++it) {
				m_bin->add(*it);
			}
		}

		m_subZones[0]->m_superZone = NULL;
		m_subZones[1]->m_superZone = NULL;

		delete m_subZones[0];
		delete m_subZones[1];

		m_subZones[0] = NULL;
		m_subZones[1] = NULL;

		startTimer();
		
		if (m_superZone != NULL) {
			m_superZone->merge();
		}
	}
}

bool CRoutingZone::isLeaf(void) const
{
	return (m_bin != NULL);
}

CRoutingZone *CRoutingZone::genSubZone(int side) 
{
	CUInt128 newIndex(m_zoneIndex);
	newIndex.shiftLeft(1);
	if (side != 0) {
		newIndex.add(1);
	}
	CRoutingZone *retVal = new CRoutingZone(this, m_level+1, newIndex);
	return retVal;
}

void CRoutingZone::startTimer(void)
{
	time_t now = time(NULL);
	// Start filling the tree, closest bins first.
	m_nextBigTimer = now + (MIN2S(1)*m_zoneIndex.get32BitChunk(3)) + SEC(10);
	CKademlia::addEvent(this);
}

void CRoutingZone::stopTimer(void)
{
	CKademlia::removeEvent(this);
}

bool CRoutingZone::onBigTimer(void)
{
	if (!isLeaf()) {
		return false;
	}

	if ( (m_zoneIndex < KK || m_level < KBASE || m_bin->getRemaining() >= (K*.4))) {
		randomLookup();
		return true;
	}

	return false;
}

//This is used when we find a leaf and want to know what this sample looks like.
//We fall back two levels and take a sample to try to minimize any areas of the 
//tree that will give very bad results.
uint32 CRoutingZone::estimateCount()
{
	if( !isLeaf() ) {
		return 0;
	}
	if( m_level < KBASE ) {
		return (uint32)(pow(2, m_level)*10);
	}
	CRoutingZone* curZone = m_superZone->m_superZone->m_superZone;

	float modify = ((float)curZone->getNumContacts())/20.0F;
	return (uint32)(pow( 2, m_level-2)*10*(modify));
}

void CRoutingZone::onSmallTimer(void)
{
	if (!isLeaf()) {
		return;
	}

	wxString test(m_zoneIndex.toBinaryString());

	CContact *c = NULL;
	time_t now = time(NULL);
	ContactList entries;
	ContactList::iterator it;

	// Remove dead entries
	m_bin->getEntries(&entries);
	for (it = entries.begin(); it != entries.end(); ++it) {
		c = *it;
		if ( c->getType() == 4) {
			if (((c->getExpireTime() > 0) && (c->getExpireTime() <= now))) {
				if(!c->inUse()) {
					m_bin->remove(c);
					delete c;
				}
				continue;
			}
		}
		if(c->getExpireTime() == 0) {
			c->setExpireTime(now);
		}
	}
	c = NULL;
	//Ping only contacts that are in the branches that meet the set level and are not close to our ID.
	//The other contacts are checked with the big timer.
	if( m_bin->getRemaining() < (K*(.4)) ) {
		c = m_bin->getOldest();
	} 
	
	if( c != NULL ) {
		if ( c->getExpireTime() >= now || c->getType() == 4) {
			m_bin->remove(c);
			m_bin->m_entries.push_back(c);
			c = NULL;
		}
	}
	
	if(c != NULL) {
		c->checkingType();
		AddDebugLogLineM(false, logClientKadUDP, wxT("KadHelloReq to ") + Uint32_16toStringIP_Port(c->getIPAddress(), c->getUDPPort()));
		CKademlia::getUDPListener()->sendMyDetails(KADEMLIA_HELLO_REQ, c->getIPAddress(), c->getUDPPort());
	}
}

void CRoutingZone::randomLookup(void) 
{
	// Look-up a random client in this zone
	CUInt128 prefix(m_zoneIndex);
	prefix.shiftLeft(128 - m_level);
	CUInt128 random(prefix, m_level);
	random.XOR(me);
	CSearchManager::findNode(random);
}

uint32 CRoutingZone::getNumContacts(void) const
{
	if (isLeaf()) {
		return m_bin->getSize();
	} else {
		return m_subZones[0]->getNumContacts() + m_subZones[1]->getNumContacts();
	}
}

uint32 CRoutingZone::getBootstrapContacts(ContactList *results, uint32 maxRequired)
{
	wxASSERT(m_superZone == NULL);

	results->clear();

	uint32 retVal = 0;
	ContactList top;
	topDepth(LOG_BASE_EXPONENT, &top);
	if (top.size() > 0) {
		ContactList::const_iterator it;
		for (it = top.begin(); it != top.end(); ++it) {
			results->push_back(*it);
			retVal++;
			if (retVal == maxRequired) {
				break;
			}
		}
	}
	
	return retVal;
}

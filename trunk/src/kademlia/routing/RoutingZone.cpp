//
// This file is part of aMule Project
//
// Copyright (c) 2004-2008 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2008 aMule Project ( http://www.amule-project.net )
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

/**
 * The *Zone* is just a node in a binary tree of *Zone*s.
 * Each zone is either an internal node or a leaf node.
 * Internal nodes have "bin == null" and "subZones[i] != null",
 * leaf nodes have "subZones[i] == null" and "bin != null".
 * 
 * All key unique id's are relative to the center (self), which
 * is considered to be 000..000
 */
#include "RoutingZone.h"

#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <common/Macros.h>

#include "Contact.h"
#include "RoutingBin.h"
#include "../kademlia/SearchManager.h"
#include "../kademlia/Defines.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/KadUDPKey.h"
#include "../../amule.h"
#include "../../CFile.h"
#include "../../Logger.h"
#include "../../NetworkFunctions.h"
#include "../../IPFilter.h"
#include "../../RandomFunctions.h"

#include <cmath>

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

// This is just a safety precaution
#define CONTACT_FILE_LIMIT	500

wxString CRoutingZone::m_filename;
CUInt128 CRoutingZone::me((uint32_t)0);

CRoutingZone::CRoutingZone()
{
	// Can only create routing zone after prefs
	// Set our KadID for creating the contact tree
	me = CKademlia::GetPrefs()->GetKadID();
	// Set the preference file name.
	m_filename = theApp->ConfigDir + wxT("nodes.dat");
	Init(NULL, 0, CUInt128((uint32_t)0));
}

void CRoutingZone::Init(CRoutingZone *super_zone, int level, const CUInt128& zone_index)
{
	// Init all Zone vars
	// Set this zone's parent
	m_superZone = super_zone;
	// Set this zone's level
	m_level = level;
	// Set this zone's CUInt128 index
	m_zoneIndex = zone_index;
	// Mark this zone as having no leafs.
	m_subZones[0] = NULL;
	m_subZones[1] = NULL;
	// Create a new contact bin as this is a leaf.
	m_bin = new CRoutingBin();

	// Set timer so that zones closer to the root are processed earlier.
	m_nextSmallTimer = time(NULL) + m_zoneIndex.Get32BitChunk(3);

	// Start this zone.
	StartTimer();

	// If we are initializing the root node, read in our saved contact list.
	if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
		ReadFile();
	}
}

CRoutingZone::~CRoutingZone()
{
	// Root node is processed first so that we can write our contact list and delete all branches.
	if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
		WriteFile();
	}

	// If this zone is a leaf, delete our contact bin.
	if (IsLeaf()) {
		delete m_bin;
	} else {
		// If this zone is a branch, delete its leafs.
		delete m_subZones[0];
		delete m_subZones[1];
	}
}

void CRoutingZone::ReadFile(const wxString& specialNodesdat)
{
	if (m_superZone != NULL || (m_filename.IsEmpty() && specialNodesdat.IsEmpty())) {
		wxFAIL;
		return;
	}

	// Read in the saved contact list
	try {
		uint32_t numContacts = 0;
		CFile file;
		if (CPath::FileExists(specialNodesdat.IsEmpty() ? m_filename : specialNodesdat) && file.Open(m_filename, CFile::read)) {
			// Get how many contacts in the saved list.
			// NOTE: Older clients put the number of contacts here...
			//       Newer clients always have 0 here to prevent older clients from reading it.
			numContacts = file.ReadUInt32();
			uint32_t fileVersion = 0;
			if (numContacts == 0) {
				fileVersion = file.ReadUInt32();
				if (fileVersion >= 1) {
					numContacts = file.ReadUInt32();
				}
			}
			uint32_t validContacts = 0;
			for (uint32_t i = 0; i < numContacts; i++) {
				CUInt128 id = file.ReadUInt128();
				uint32_t ip = file.ReadUInt32();
				uint16_t udpPort = file.ReadUInt16();
				uint16_t tcpPort = file.ReadUInt16();
				uint8_t type = 0;
				uint8_t contactVersion = 0;
				if (fileVersion >= 1) {
					contactVersion = file.ReadUInt8();
				} else {
					type = file.ReadUInt8();
				}
				CKadUDPKey kadUDPKey;
				bool verified = false;
				if (fileVersion >= 2) {
					kadUDPKey.ReadFromFile(file);
					verified = file.ReadUInt8() != 0;
				}
				// IP appears valid
				if (type < 4) {
					if(IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip),udpPort)) {
						if (!theApp->ipfilter->IsFiltered(wxUINT32_SWAP_ALWAYS(ip)) &&
						    !(udpPort == 53 && contactVersion <= 5 /*No DNS Port without encryption*/)) {
							// This was not a dead contact, inc counter if add was successful
							if (AddUnfiltered(id, ip, udpPort, tcpPort, contactVersion, kadUDPKey, verified, false)) {
								validContacts++;
							}
						}
					}
				}
			}
			AddLogLineM(false, wxString::Format(wxPLURAL("Read %u Kad contact", "Read %u Kad contacts", validContacts), validContacts));
		}
		if (numContacts == 0) {
			AddDebugLogLineM(false, logKadRouting, wxT("Error while reading Kad contacts - 0 entries"));
		}
	} catch (const CSafeIOException& e) {
		AddDebugLogLineM(false, logKadRouting, wxT("IO error in CRoutingZone::readFile: ") + e.what());
	}
}

void CRoutingZone::WriteFile()
{
	// The bootstrap method gets a very nice sample of contacts to save.
	ContactList contacts;
	GetBootstrapContacts(&contacts, 200);
	ContactList::size_type numContacts = contacts.size();
	numContacts = std::min<ContactList::size_type>(numContacts, CONTACT_FILE_LIMIT); // safety precaution, should not be above
	if (numContacts < 25) {
		AddLogLineM(false, wxString::Format(wxPLURAL("Only %d Kad contact available, nodes.dat not written", "Only %d Kad contacts available, nodes.dat not written", numContacts), numContacts));
		return;
	}
	try {
		unsigned int count = 0;
		CFile file;
		if (file.Open(m_filename, CFile::write)) {
			// Start file with 0 to prevent older clients from reading it.
			file.WriteUInt32(0);
			// Now tag it with a version which happens to be 2.
			file.WriteUInt32(2);
			file.WriteUInt32(numContacts);
			for (ContactList::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
				CContact *c = *it;
				count++;
				if (count > CONTACT_FILE_LIMIT) {
					// This should never happen
					wxFAIL;
					break;
				}
				file.WriteUInt128(c->GetClientID());
				file.WriteUInt32(c->GetIPAddress());
				file.WriteUInt16(c->GetUDPPort());
				file.WriteUInt16(c->GetTCPPort());
				file.WriteUInt8(c->GetVersion());
				c->GetUDPKey().StoreToFile(file);
				file.WriteUInt8(c->IsIPVerified() ? 1 : 0);
			}
		}
		AddLogLineM(false, wxString::Format(wxPLURAL("Wrote %d Kad contact", "Wrote %d Kad contacts", count), count));
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logKadRouting, wxT("IO failure in CRoutingZone::writeFile: ") + e.what());
	}
}

bool CRoutingZone::CanSplit() const throw()
{
	// Max levels allowed.
	if (m_level >= 127) {
		return false;
	}

	// Check if this zone is allowed to split.
	return ((m_zoneIndex < KK || m_level < KBASE) && m_bin->GetSize() == K);
}

bool CRoutingZone::Add(const CUInt128& id, uint32_t ip, uint16_t port, uint16_t tport, uint8_t version, const CKadUDPKey& key, bool ipVerified, bool update)
{
	if (IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip), port)) {
		if (!theApp->ipfilter->IsFiltered(wxUINT32_SWAP_ALWAYS(ip)) && !(port == 53 && version <= 5) /*No DNS Port without encryption*/) {
			return AddUnfiltered(id, ip, port, tport, version, key, ipVerified, update);
		}
	}
	return false;
}

bool CRoutingZone::AddUnfiltered(const CUInt128& id, uint32_t ip, uint16_t port, uint16_t tport, uint8_t version, const CKadUDPKey& key, bool ipVerified, bool update)
{
	if (id != me) {
		CContact *contact = new CContact(id, ip, port, tport, version, key, ipVerified);
		if (Add(contact, update)) {
			return true;
		} else {
			delete contact;
		}
	}
	return false;
}

bool CRoutingZone::Add(CContact *contact, bool update)
{
	// If we're not a leaf, call add on the correct branch.
	if (!IsLeaf()) {
		return m_subZones[contact->GetDistance().GetBitNumber(m_level)]->Add(contact, update);
	} else {
		// Do we already have a contact with this KadID?
		CContact *contactUpdate = m_bin->GetContact(contact->GetClientID());
		if (contactUpdate) {
			if (update) {
				if (m_bin->ChangeContactIPAddress(contactUpdate, contact->GetIPAddress())) {
					contactUpdate->SetUDPPort(contact->GetUDPPort());
					contactUpdate->SetTCPPort(contact->GetTCPPort());
					contactUpdate->SetVersion(contact->GetVersion());
					contactUpdate->SetUDPKey(contact->GetUDPKey());
					if (!contactUpdate->IsIPVerified()) { // don't unset the verified flag (will clear itself on ipchanges)
						contactUpdate->SetIPVerified(contact->IsIPVerified());
					}
					m_bin->SetAlive(contactUpdate);
				}
			}
			return false;
		} else if (m_bin->GetRemaining()) {
			// This bin is not full, so add the new contact
			return m_bin->AddContact(contact);
		} else if (CanSplit()) {
			// This bin was full and split, call add on the correct branch.
			Split();
			return m_subZones[contact->GetDistance().GetBitNumber(m_level)]->Add(contact, update);
		} else {
			return false;
		}
	}
}

CContact *CRoutingZone::GetContact(const CUInt128& id) const throw()
{
	if (IsLeaf()) {
		return m_bin->GetContact(id);
	} else {
		return m_subZones[id.GetBitNumber(m_level)]->GetContact(id);
	}
}

CContact *CRoutingZone::GetContact(uint32_t ip, uint16_t port, bool tcpPort) const throw()
{
	if (IsLeaf()) {
		return m_bin->GetContact(ip, port, tcpPort);
	} else {
		CContact *contact = m_subZones[0]->GetContact(ip, port, tcpPort);
		return (contact != NULL) ? contact : m_subZones[1]->GetContact(ip, port, tcpPort);
	}
}

CContact *CRoutingZone::GetRandomContact(uint32_t maxType, uint32_t minKadVersion) const throw()
{
	if (IsLeaf()) {
		return m_bin->GetRandomContact(maxType, minKadVersion);
	} else {
		unsigned zone = GetRandomUint16() & 1 /* GetRandomUint16() % 2 */;
		CContact *contact = m_subZones[zone]->GetRandomContact(maxType, minKadVersion);
		return (contact != NULL) ? contact : m_subZones[1 - zone]->GetRandomContact(maxType, minKadVersion);
	}
}

void CRoutingZone::GetClosestTo(uint32_t maxType, const CUInt128& target, const CUInt128& distance, uint32_t maxRequired, ContactMap *result, bool emptyFirst, bool inUse) const
{
	// If leaf zone, do it here
	if (IsLeaf()) {
		m_bin->GetClosestTo(maxType, target, maxRequired, result, emptyFirst, inUse);
		return;
	}

	// otherwise, recurse in the closer-to-the-target subzone first
	int closer = distance.GetBitNumber(m_level);
	m_subZones[closer]->GetClosestTo(maxType, target, distance, maxRequired, result, emptyFirst, inUse);

	// if still not enough tokens found, recurse in the other subzone too
	if (result->size() < maxRequired) {
		m_subZones[1-closer]->GetClosestTo(maxType, target, distance, maxRequired, result, false, inUse);
	}
}

void CRoutingZone::GetAllEntries(ContactList *result, bool emptyFirst) const
{
	if (IsLeaf()) {
		m_bin->GetEntries(result, emptyFirst);
	} else {
		m_subZones[0]->GetAllEntries(result, emptyFirst);
		m_subZones[1]->GetAllEntries(result, false);			
	}
}

void CRoutingZone::TopDepth(int depth, ContactList *result, bool emptyFirst) const
{
	if (IsLeaf()) {
		m_bin->GetEntries(result, emptyFirst);
	} else if (depth <= 0) {
		RandomBin(result, emptyFirst);
	} else {
		m_subZones[0]->TopDepth(depth-1, result, emptyFirst);
		m_subZones[1]->TopDepth(depth-1, result, false);
	}
}

void CRoutingZone::RandomBin(ContactList *result, bool emptyFirst) const
{
	if (IsLeaf()) {
		m_bin->GetEntries(result, emptyFirst);
	} else {
		m_subZones[rand()&1]->RandomBin(result, emptyFirst);
	}
}

uint32_t CRoutingZone::GetMaxDepth() const throw()
{
	if (IsLeaf()) {
		return 0;
	}
	return 1 + std::max(m_subZones[0]->GetMaxDepth(), m_subZones[1]->GetMaxDepth());
}

void CRoutingZone::Split()
{
	StopTimer();
		
	m_subZones[0] = GenSubZone(0);
	m_subZones[1] = GenSubZone(1);

	ContactList entries;
	m_bin->GetEntries(&entries);
	m_bin->m_dontDeleteContacts = true;
	delete m_bin;
	m_bin = NULL;

	for (ContactList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
		if (!m_subZones[(*it)->GetDistance().GetBitNumber(m_level)]->m_bin->AddContact(*it)) {
			delete *it;
		}
	}
}

uint32_t CRoutingZone::Consolidate()
{
	uint32_t mergeCount = 0;

	if (IsLeaf()) {
		return mergeCount;
	}

	wxASSERT(m_bin == NULL);

	if (!m_subZones[0]->IsLeaf()) {
		mergeCount += m_subZones[0]->Consolidate();
	}
	if (!m_subZones[1]->IsLeaf()) {
		mergeCount += m_subZones[1]->Consolidate();
	}

	if (m_subZones[0]->IsLeaf() && m_subZones[1]->IsLeaf() && GetNumContacts() < K / 2) {
		m_bin = new CRoutingBin();

		m_subZones[0]->StopTimer();
		m_subZones[1]->StopTimer();

		ContactList list0;
		ContactList list1;
		m_subZones[0]->m_bin->GetEntries(&list0);
		m_subZones[1]->m_bin->GetEntries(&list1);

		m_subZones[0]->m_bin->m_dontDeleteContacts = true;
		m_subZones[1]->m_bin->m_dontDeleteContacts = true;

		delete m_subZones[0];
		delete m_subZones[1];

		m_subZones[0] = NULL;
		m_subZones[1] = NULL;

		for (ContactList::const_iterator it = list0.begin(); it != list0.end(); ++it) {
			m_bin->AddContact(*it);
		}
		for (ContactList::const_iterator it = list1.begin(); it != list1.end(); ++it) {
			m_bin->AddContact(*it);
		}

		StartTimer();

		mergeCount++;
	}
	return mergeCount;
}

CRoutingZone *CRoutingZone::GenSubZone(unsigned side)
{
	wxASSERT(side <= 1);

	CUInt128 newIndex(m_zoneIndex);
	newIndex <<= 1;
	newIndex += side;
	return new CRoutingZone(this, m_level + 1, newIndex);
}

void CRoutingZone::StartTimer()
{
	// Start filling the tree, closest bins first.
	m_nextBigTimer = time(NULL) + SEC(10);
	CKademlia::AddEvent(this);
}

void CRoutingZone::StopTimer()
{
	CKademlia::RemoveEvent(this);
}

bool CRoutingZone::OnBigTimer() const
{
	if (IsLeaf() && (m_zoneIndex < KK || m_level < KBASE || m_bin->GetRemaining() >= (K * 0.8))) {
		RandomLookup();
		return true;
	}

	return false;
}

//This is used when we find a leaf and want to know what this sample looks like.
//We fall back two levels and take a sample to try to minimize any areas of the 
//tree that will give very bad results.
uint32_t CRoutingZone::EstimateCount() const throw()
{
	if (!IsLeaf()) {
		return 0;
	}

	if (m_level < KBASE) {
		return (uint32_t)(pow(2.0, (int)m_level) * K);
	}

	CRoutingZone* curZone = m_superZone->m_superZone->m_superZone;

	float modify = ((float)curZone->GetNumContacts()) / (float)(K * 2);
	return (uint32_t)(pow(2.0, (int)m_level - 2) * (float)K * modify * 1.20);
}

void CRoutingZone::OnSmallTimer()
{
	if (!IsLeaf()) {
		return;
	}
	
	CContact *c = NULL;
	time_t now = time(NULL);
	ContactList entries;

	// Remove dead entries
	m_bin->GetEntries(&entries);
	for (ContactList::iterator it = entries.begin(); it != entries.end(); ++it) {
		c = *it;
		if (c->GetType() == 4) {
			if ((c->GetExpireTime() > 0) && (c->GetExpireTime() <= now)) {
				if (!c->InUse()) {
					m_bin->RemoveContact(c);
					delete c;
				}
				continue;
			}
		}
		if(c->GetExpireTime() == 0) {
			c->SetExpireTime(now);
		}
	}

	c = m_bin->GetOldest();
	if (c != NULL) {
		if (c->GetExpireTime() >= now || c->GetType() == 4) {
			m_bin->PushToBottom(c);
			c = NULL;
		}
	}

	if (c != NULL) {
		c->CheckingType();
		if (c->GetVersion() >= 6) {
			DebugSend(Kad2HelloReq, c->GetIPAddress(), c->GetUDPPort());
			CUInt128 clientID = c->GetClientID();
			CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA2_HELLO_REQ, c->GetIPAddress(), c->GetUDPPort(), true, c->GetUDPKey(), &clientID);
		} else if (c->GetVersion() >= 2) {
			DebugSend(Kad2HelloReq, c->GetIPAddress(), c->GetUDPPort());
			CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA2_HELLO_REQ, c->GetIPAddress(), c->GetUDPPort(), true, 0, NULL);
			wxASSERT(c->GetUDPKey() == CKadUDPKey(0));
		} else {
			DebugSend(KadHelloReq, c->GetIPAddress(), c->GetUDPPort());
			CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA_HELLO_REQ, c->GetIPAddress(), c->GetUDPPort(), false, 0, NULL);
			if (c->CheckIfKad2()) {
				DebugSend(Kad2HelloReq, c->GetIPAddress(), c->GetUDPPort());
				CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA2_HELLO_REQ, c->GetIPAddress(), c->GetUDPPort(), true, 0, NULL);
			}
		}
	}
}

void CRoutingZone::RandomLookup() const
{
	// Look-up a random client in this zone
	CUInt128 prefix(m_zoneIndex);
	prefix <<= 128 - m_level;
	CUInt128 random(prefix, m_level);
	random ^= me;
	CSearchManager::FindNode(random, false);
}

uint32_t CRoutingZone::GetNumContacts() const throw()
{
	if (IsLeaf()) {
		return m_bin->GetSize();
	} else {
		return m_subZones[0]->GetNumContacts() + m_subZones[1]->GetNumContacts();
	}
}

uint32_t CRoutingZone::GetBootstrapContacts(ContactList *results, uint32_t maxRequired) const
{
	wxASSERT(m_superZone == NULL);

	results->clear();

	uint32_t count = 0;
	ContactList top;
	TopDepth(LOG_BASE_EXPONENT, &top);
	if (top.size() > 0) {
		for (ContactList::const_iterator it = top.begin(); it != top.end(); ++it) {
			results->push_back(*it);
			count++;
			if (count == maxRequired) {
				break;
			}
		}
	}

	return count;
}
// File_checked_for_headers

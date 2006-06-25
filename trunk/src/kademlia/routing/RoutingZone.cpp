//
// This file is part of aMule Project
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Project ( http://www.amule-project.net )
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

#include <include/protocol/kad/Client2Client/UDP.h>
#include <include/common/Macros.h>

#include "Contact.h"
#include "RoutingBin.h"
#include "../kademlia/SearchManager.h"
#include "../kademlia/Defines.h"
#include "../net/KademliaUDPListener.h"
#include "../../amule.h"
#include "../../CFile.h"
#include "../../Logger.h"
#include "../../NetworkFunctions.h"


#include <cmath>

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
	me = CKademlia::GetPrefs()->GetKadID();
	m_filename = theApp.ConfigDir + wxT("nodes.dat");
	CUInt128 zero((uint32)0);
	Init(NULL, 0, zero);
}

CRoutingZone::CRoutingZone(CRoutingZone *super_zone, int level, const CUInt128 &zone_index)
{
	Init(super_zone, level, zone_index);
}

void CRoutingZone::Init(CRoutingZone *super_zone, int level, const CUInt128 &zone_index)
{
	m_superZone = super_zone;
	m_level = level;
	m_zoneIndex = zone_index;
	m_subZones[0] = NULL;
	m_subZones[1] = NULL;
	m_bin = new CRoutingBin();

	m_nextSmallTimer = time(NULL) + m_zoneIndex.Get32BitChunk(3);

	if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
		ReadFile();
	}

	dirty = false;
	
	StartTimer();
}

CRoutingZone::~CRoutingZone()
{
	if ((m_superZone == NULL) && (m_filename.Length() > 0)) {
		WriteFile();
	}
	if (IsLeaf()) {
		delete m_bin;
	} else {
		delete m_subZones[0];
		delete m_subZones[1];
	}
}

void CRoutingZone::ReadFile(void)
{
	try {
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
						Add(id, ip, udpPort, tcpPort, type);
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
}

void CRoutingZone::WriteFile(void)
{
	try {
		unsigned int count = 0;
		CContact *c;
		CFile file;
		if (file.Open(m_filename, CFile::write)) {
			ContactList contacts;
			GetBootstrapContacts(&contacts, 200);
			file.WriteUInt32((uint32)std::min((int)contacts.size(), CONTACT_FILE_LIMIT));
			ContactList::const_iterator it;
			for (it = contacts.begin(); it != contacts.end(); ++it) {
				count++;
				c = *it;
				file.WriteUInt128(c->GetClientID());
				file.WriteUInt32(c->GetIPAddress());
				file.WriteUInt16(c->GetUDPPort());
				file.WriteUInt16(c->GetTCPPort());
				file.WriteUInt8(c->GetType());
				if (count == CONTACT_FILE_LIMIT) {
					break;
				}
			}
		}
		AddDebugLogLineM( false, logKadRouting, wxString::Format(wxT("Wrote %d contacts to file."), count));
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logKadRouting, wxT("IO failure in CRoutingZone::writeFile: ") + e.what());
	}
}

bool CRoutingZone::CanSplit(void) const
{
	if (m_level >= 127) {
		return false;
	}
		
	/* Check if we are close to the center */
	if ( (m_zoneIndex < KK || m_level < KBASE) && m_bin->GetSize() == K) {
		return true;
	}
	return false;
}

bool CRoutingZone::Add(const CUInt128 &id, uint32 ip, uint16 port, uint16 tport, byte type)
{
	
	//AddDebugLogLineM(false, logKadMain, wxT("Adding a contact (routing) with ip ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip),port));
	
	if (id == me) {
		return false;
	}

	CUInt128 distance(me);
	distance.XOR(id);

	return AddByDistance(distance,id,ip,port,tport,type);
}

bool CRoutingZone::AddByDistance(const CUInt128 &distance, const CUInt128 &id, uint32 ip, uint16 port, uint16 tport, byte type) {

	bool retVal = false;
	CContact *c = NULL;

	if (!IsLeaf()) {
		retVal = m_subZones[distance.GetBitNumber(m_level)]->AddByDistance(distance, id, ip, port, tport, type);
	} else {
		c = m_bin->GetContact(id);
		if (c != NULL) {
			c->SetIPAddress(ip);
			c->SetUDPPort(port);
			c->SetTCPPort(tport);
			retVal = true;
		} else if (m_bin->GetRemaining() > 0) {
			c = new CContact(id, ip, port, tport);
			retVal = m_bin->Add(c,false);
		} else if (CanSplit()) {
			Split();
			retVal = m_subZones[distance.GetBitNumber(m_level)]->AddByDistance(distance, id, ip, port, tport, type);
		}/* else {
			Merge();
			c = new CContact(id, ip, port, tport);
			retVal = m_bin->Add(c,false);
		}*/

		if (!retVal) {
			if (c != NULL) {
				delete c;
			}
		}
	}
	
	return retVal;
}

void CRoutingZone::Remove(const CUInt128 &id) {
	CUInt128 distance(me);
	distance.XOR(id);
	if (!IsLeaf()) {
		m_subZones[distance.GetBitNumber(m_level)]->Remove(id);
	} else {
		CContact *c = m_bin->GetContact(id);
		if (c) {
			m_bin->Remove(c);
		}
	}
}

void CRoutingZone::SetAlive(uint32 ip, uint16 port)
{
	if (IsLeaf()) {
		m_bin->SetAlive(ip, port);
	} else {
		m_subZones[0]->SetAlive(ip, port);
		m_subZones[1]->SetAlive(ip, port);
	}
}

CContact *CRoutingZone::GetContact(const CUInt128 &id) const
{
	if (IsLeaf()) {
		return m_bin->GetContact(id);
	} else {
		return m_subZones[id.GetBitNumber(m_level)]->GetContact(id);
	}
}

void CRoutingZone::GetClosestTo(uint32 maxType, const CUInt128 &target, const CUInt128 &distance, uint32 maxRequired, ContactMap *result, bool emptyFirst, bool inUse) const
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

void CRoutingZone::GetAllEntries(ContactList *result, bool emptyFirst)
{
	if (IsLeaf()) {
		m_bin->GetEntries(result, emptyFirst);
	} else {
		m_subZones[0]->GetAllEntries(result, emptyFirst);
		m_subZones[1]->GetAllEntries(result, false);			
	}
}

void CRoutingZone::TopDepth(int depth, ContactList *result, bool emptyFirst)
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

void CRoutingZone::RandomBin(ContactList *result, bool emptyFirst)
{
	if (IsLeaf()) {
		m_bin->GetEntries(result, emptyFirst);
	} else {
		m_subZones[rand()&1]->RandomBin(result, emptyFirst);
	}
}

uint32 CRoutingZone::GetMaxDepth(void) const
{
	if (IsLeaf()) {
		return 0;
	}
	return 1 + std::max(m_subZones[0]->GetMaxDepth(), m_subZones[1]->GetMaxDepth());
}

void CRoutingZone::Split(void)
{
	StopTimer();
		
	m_subZones[0] = GenSubZone(0);
	m_subZones[1] = GenSubZone(1);

	ContactList entries;
	m_bin->GetEntries(&entries);
	ContactList::const_iterator it;
	for (it = entries.begin(); it != entries.end(); ++it) {
		int sz = (*it)->GetDistance().GetBitNumber(m_level);
		m_subZones[sz]->m_bin->Add(*it);
	}
	m_bin->m_dontDeleteContacts = true;
	delete m_bin;
	m_bin = NULL;
}

void CRoutingZone::Merge(void)
{
	dirty = false; /* This subzone/superzone won't be re-checked */
	
	AddDebugLogLineM( false, logKadRouting, _("Merge attempt"));
	
	if (IsLeaf() && m_superZone != NULL) {
		AddDebugLogLineM( false, logKadRouting, _("Recursive merge"));
		m_superZone->Merge();
	} else if ((!IsLeaf())
		&& (m_subZones[0]->IsLeaf() && m_subZones[1]->IsLeaf()) 
		&&	(GetNumContacts()) < (K/2) ) 
		{
		m_bin = new CRoutingBin();
		
		m_subZones[0]->StopTimer();
		m_subZones[1]->StopTimer();

		if (GetNumContacts() > 0) {
			ContactList list0;
			ContactList list1;
			m_subZones[0]->m_bin->GetEntries(&list0);
			m_subZones[1]->m_bin->GetEntries(&list1);
			ContactList::const_iterator it;
			for (it = list0.begin(); it != list0.end(); ++it) {
				m_bin->Add(*it);
			}
			for (it = list1.begin(); it != list1.end(); ++it) {
				m_bin->Add(*it);
			}
		}

		m_subZones[0]->m_superZone = NULL;
		m_subZones[1]->m_superZone = NULL;

		m_subZones[0]->m_bin->m_dontDeleteContacts = true;
		m_subZones[1]->m_bin->m_dontDeleteContacts = true;	
		
		delete m_subZones[0];
		delete m_subZones[1];

		m_subZones[0] = NULL;
		m_subZones[1] = NULL;

		StartTimer();
			
		AddDebugLogLineM( false, logKadRouting, _("Sucessful merge!"));
		
		if (m_superZone != NULL) {
			m_superZone->Merge();
		}
	} else {
		AddDebugLogLineM( false, logKadRouting, _("No merge possible"));
	}
}

bool CRoutingZone::IsLeaf(void) const
{
	return (m_bin != NULL);
}

CRoutingZone *CRoutingZone::GenSubZone(int side) 
{
	CUInt128 newIndex(m_zoneIndex);
	newIndex.ShiftLeft(1);
	if (side != 0) {
		newIndex.Add(1);
	}
	CRoutingZone *retVal = new CRoutingZone(this, m_level+1, newIndex);
	return retVal;
}

void CRoutingZone::StartTimer(void)
{
	time_t now = time(NULL);
	// Start filling the tree, closest bins first.
	m_nextBigTimer = now + (MIN2S(1)*m_zoneIndex.Get32BitChunk(3)) + SEC(10);
	CKademlia::AddEvent(this);
}

void CRoutingZone::StopTimer(void)
{
	CKademlia::RemoveEvent(this);
}

bool CRoutingZone::OnBigTimer(void)
{
	if (!IsLeaf()) {
		return false;
	}

	if ( (m_zoneIndex < KK || m_level < KBASE || m_bin->GetRemaining() >= (K*.4))) {
		RandomLookup();
		return true;
	}

	return false;
}

//This is used when we find a leaf and want to know what this sample looks like.
//We fall back two levels and take a sample to try to minimize any areas of the 
//tree that will give very bad results.
uint32 CRoutingZone::EstimateCount()
{
	if( !IsLeaf() ) {
		return 0;
	}
	if( m_level < KBASE ) {
		return (uint32)(pow(2, m_level)*10);
	}
	CRoutingZone* curZone = m_superZone->m_superZone->m_superZone;

	float modify = ((float)curZone->GetNumContacts())/20.0F;
	return (uint32)(pow( 2, m_level-2)*10*(modify));
}

void CRoutingZone::OnSmallTimer(void)
{
	if (!IsLeaf()) {
		return;
	}
	
	dirty = false;
	
	wxString test(m_zoneIndex.ToBinaryString());

	CContact *c = NULL;
	time_t now = time(NULL);
	ContactList entries;
	ContactList::iterator it;

	// Remove dead entries
	m_bin->GetEntries(&entries);
	for (it = entries.begin(); it != entries.end(); ++it) {
		c = *it;
		if ( c->GetType() == 4) {
			if (((c->GetExpireTime() > 0) && (c->GetExpireTime() <= now))) {
				if(!c->InUse()) {
					m_bin->Remove(c);
					delete c;
					dirty = true;
				}
				continue;
			}
		}
		if(c->GetExpireTime() == 0) {
			c->SetExpireTime(now);
		}
	}
	c = NULL;
	//Ping only contacts that are in the branches that meet the set level and are not close to our ID.
	//The other contacts are checked with the big timer.
	if( m_bin->GetRemaining() < (K*(.4)) ) {
		c = m_bin->GetOldest();
	} 
	
	if( c != NULL ) {
		if ( c->GetExpireTime() >= now || c->GetType() == 4) {
			dirty = true;
			m_bin->Remove(c);
			m_bin->m_entries.push_back(c);
			c = NULL;
		}
	}
	
	if(c != NULL) {
		c->CheckingType();
		AddDebugLogLineM(false, logClientKadUDP, wxT("KadHelloReq to ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(c->GetIPAddress()), c->GetUDPPort()));
		CKademlia::GetUDPListener()->SendMyDetails(KADEMLIA_HELLO_REQ, c->GetIPAddress(), c->GetUDPPort());
	}
}

void CRoutingZone::RandomLookup(void) 
{
	// Look-up a random client in this zone
	CUInt128 prefix(m_zoneIndex);
	prefix.ShiftLeft(128 - m_level);
	CUInt128 random(prefix, m_level);
	random.XOR(me);
	CSearchManager::FindNode(random);
}

uint32 CRoutingZone::GetNumContacts(void) const
{
	if (IsLeaf()) {
		return m_bin->GetSize();
	} else {
		return m_subZones[0]->GetNumContacts() + m_subZones[1]->GetNumContacts();
	}
}

uint32 CRoutingZone::GetBootstrapContacts(ContactList *results, uint32 maxRequired)
{
	wxASSERT(m_superZone == NULL);

	results->clear();

	uint32 retVal = 0;
	ContactList top;
	TopDepth(LOG_BASE_EXPONENT, &top);
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
// File_checked_for_headers

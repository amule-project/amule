//								-*- C++ -*-
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

#ifndef __ROUTING_ZONE__
#define __ROUTING_ZONE__

#include "Maps.h"
#include "../utils/UInt128.h"

class CFileDataIO;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CRoutingBin;
class CContact;
class CKadUDPKey;

/**
 * The *Zone* is just a node in a binary tree of *Zone*s.
 * Each zone is either an internal node or a leaf node.
 * Internal nodes have "bin == null" and "subZones[i] != null",
 * leaf nodes have "subZones[i] == null" and "bin != null".
 * 
 * All key pseudoaddresses are relative to the center (self), which
 * is considered to be 000..000
 */
class CRoutingZone
{
public:

	CRoutingZone();
	~CRoutingZone();

	bool	 OnBigTimer() const;
	void	 OnSmallTimer();
	uint32_t Consolidate();

	bool	 Add(const CUInt128 &id, uint32_t ip, uint16_t port, uint16_t tport, uint8_t version, const CKadUDPKey& key, bool& ipVerified, bool update, bool fromHello);
	bool	 AddUnfiltered(const CUInt128 &id, uint32_t ip, uint16_t port, uint16_t tport, uint8_t version, const CKadUDPKey& key, bool& ipVerified, bool update, bool fromHello);
	bool	 Add(CContact *contact, bool& update, bool& outIpVerified);

	void	 ReadFile(const wxString& specialNodesdat = wxEmptyString);

	bool	 VerifyContact(const CUInt128& id, uint32_t ip);
	CContact *GetContact(const CUInt128& id) const throw();
	CContact *GetContact(uint32_t ip, uint16_t port, bool tcpPort) const throw();
	CContact *GetRandomContact(uint32_t maxType, uint32_t minKadVersion) const;
	uint32_t GetNumContacts() const throw();
	void	 GetNumContacts(uint32_t& nInOutContacts, uint32_t& nInOutFilteredContacts, uint8_t minVersion) const throw();

	// Check if we know a contact with the same IP or ID but not matching IP/ID and other limitations, similar checks like when adding a node to the table except allowing duplicates
	bool	IsAcceptableContact(const CContact *toCheck) const;

	// Returns a list of all contacts in all leafs of this zone.
	void	 GetAllEntries(ContactList *result, bool emptyFirst = true) const;

	// Returns the *maxRequired* tokens that are closest to the target within this zone's subtree.
	void	 GetClosestTo(uint32_t maxType, const CUInt128& target, const CUInt128& distance, uint32_t maxRequired, ContactMap *result, bool emptyFirst = true, bool setInUse = false) const;

	// Ideally: Returns all contacts that are in buckets of common range between us and the asker.
	// In practice: returns the contacts from the top (2^{logBase+1}) buckets.
	uint32_t GetBootstrapContacts(ContactList *results, uint32_t maxRequired) const;

	uint32_t EstimateCount() const;
	bool	 HasOnlyLANNodes() const throw();

	time_t	 m_nextBigTimer;
	time_t	 m_nextSmallTimer;

private:

	CRoutingZone(CRoutingZone *super_zone, int level, const CUInt128& zone_index) { Init(super_zone, level, zone_index); }
	void Init(CRoutingZone *super_zone, int level, const CUInt128& zone_index);
	void ReadBootstrapNodesDat(CFileDataIO& file);
#if 0
	void WriteBootstrapFile();
#endif

	void WriteFile();

	bool IsLeaf() const throw() { return m_bin != NULL; }
	bool CanSplit() const throw();

	// Returns all contacts from this zone tree that are no deeper than *depth* from the current zone.
	void TopDepth(int depth, ContactList *result, bool emptyFirst = true) const;

	// Returns the maximum depth of the tree as the number of edges of the longest path to a leaf.
	uint32_t GetMaxDepth() const throw();

	void RandomBin(ContactList *result, bool emptyFirst = true) const;

	void Split();

	void StartTimer();
	void StopTimer();

	void RandomLookup() const;

	void SetAllContactsVerified();

	/**
	 * Generates a new TokenBin for this zone. Used when the current zone is becoming a leaf zone.
	 * Must be deleted by caller
	 */
	CRoutingZone *GenSubZone(unsigned side);

	/**
	 * Zone pair is an array of two. Either both entries are null, which
	 * means that *this* is a leaf zone, or both are non-null which means
	 * that *this* has been split into equally sized finer zones.
	 * The zone with index 0 is the one closer to our *self* token.
	 */
	CRoutingZone *m_subZones[2];
	CRoutingZone *m_superZone;

	static wxString m_filename;
	static CUInt128 me;

	/**
	 * The level indicates what size chunk of the address space
	 * this zone is representing. Level 0 is the whole space,
	 * level 1 is 1/2 of the space, level 2 is 1/4, etc.
	 */
	uint32_t m_level;

	/**
	 * This is the distance in number of zones from the zone at this level
	 * that contains the center of the system; distance is wrt the XOR metric.
	 */
	CUInt128 m_zoneIndex;

	/** List of contacts, if this zone is a leaf zone. */
	CRoutingBin *m_bin;
};

} // End namespace

#endif // __ROUTING_ZONE__
// File_checked_for_headers

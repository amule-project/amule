//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef KADEMLIA_NET_PACKETTRACKING_H
#define KADEMLIA_NET_PACKETTRACKING_H

#include <map>
#include <list>
#include "../utils/UInt128.h"
#include "../../Types.h"

namespace Kademlia
{

struct TrackPackets_Struct {
	uint32_t ip;
	uint32_t inserted;
	uint8_t  opcode;
};

struct TrackChallenge_Struct {
	uint32_t	ip;
	uint32_t	inserted;
	uint8_t		opcode;
	CUInt128	contactID;
	CUInt128	challenge;
};

struct TrackPacketsIn_Struct {
	struct TrackedRequestIn_Struct {
		uint32_t m_count;
		uint32_t m_firstAdded;
		uint8_t	 m_opcode;
		bool	 m_dbgLogged;
	};

	TrackPacketsIn_Struct()
	{
		m_lastExpire = 0;
		m_ip = 0;
	}

	uint32_t m_ip;
	uint32_t m_lastExpire;
	typedef std::list<TrackedRequestIn_Struct>	TrackedRequestList;
	TrackedRequestList	m_trackedRequests;
};

class CPacketTracking
{
      public:
	CPacketTracking() throw() { lastTrackInCleanup = 0; }
	virtual ~CPacketTracking();

      protected:
	void AddTrackedOutPacket(uint32_t ip, uint8_t opcode);
	bool IsOnOutTrackList(uint32_t ip, uint8_t opcode, bool dontRemove = false);
	bool InTrackListIsAllowedPacket(uint32_t ip, uint8_t opcode, bool validReceiverkey);
	void InTrackListCleanup();
	void AddLegacyChallenge(const CUInt128& contactID, const CUInt128& challengeID, uint32_t ip, uint8_t opcode);
	bool IsLegacyChallenge(const CUInt128& challengeID, uint32_t ip, uint8_t opcode, CUInt128& contactID);
	bool HasActiveLegacyChallenge(uint32_t ip) const;

      private:
	static bool IsTrackedOutListRequestPacket(uint8_t opcode) throw();
	typedef std::list<TrackPackets_Struct>		TrackedPacketList;
	typedef std::list<TrackChallenge_Struct>	TrackChallengeList;
	typedef std::map<uint32_t, TrackPacketsIn_Struct*>	TrackedPacketInMap;
	TrackedPacketList	listTrackedRequests;
	TrackChallengeList	listChallengeRequests;
	TrackedPacketInMap	m_mapTrackPacketsIn;
	uint32_t		lastTrackInCleanup;
};

} // namespace Kademlia

#endif /* KADEMLIA_NET_PACKETTRACKING_H */

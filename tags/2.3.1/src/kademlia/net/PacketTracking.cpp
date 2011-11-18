//
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

#include "PacketTracking.h"
#include "../kademlia/Kademlia.h"
#include "../../amule.h"
#include "../../Logger.h"
#include "../../OtherFunctions.h"
#include "../../NetworkFunctions.h"
#include "../../ClientList.h"
#include "../../GetTickCount.h"
#include <common/Macros.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/UDP.h>


using namespace Kademlia;


CPacketTracking::~CPacketTracking()
{
	DeleteContents(m_mapTrackPacketsIn);
}

void CPacketTracking::AddTrackedOutPacket(uint32_t ip, uint8_t opcode)
{
	// this tracklist tacks _outgoing_ request packets, to make sure incoming answer packets were requested
	// only track packets which we actually check for later
	if (!IsTrackedOutListRequestPacket(opcode)) {
		return;
	}
	uint32_t now = ::GetTickCount();
	TrackPackets_Struct track = { ip, now, opcode };
	listTrackedRequests.push_front(track);
	while (!listTrackedRequests.empty()) {
		if (now - listTrackedRequests.back().inserted > SEC2MS(180)) {
			listTrackedRequests.pop_back();
		} else {
			break;
		}
	}
}

bool CPacketTracking::IsTrackedOutListRequestPacket(uint8_t opcode) throw()
{
	switch (opcode) {
	 case KADEMLIA2_BOOTSTRAP_REQ:
	 case KADEMLIA2_HELLO_REQ:
	 case KADEMLIA2_HELLO_RES:
	 case KADEMLIA2_REQ:
	 case KADEMLIA_SEARCH_NOTES_REQ:
	 case KADEMLIA2_SEARCH_NOTES_REQ:
	 case KADEMLIA_PUBLISH_REQ:
	 case KADEMLIA2_PUBLISH_KEY_REQ:
	 case KADEMLIA2_PUBLISH_SOURCE_REQ:
	 case KADEMLIA2_PUBLISH_NOTES_REQ:
	 case KADEMLIA_FINDBUDDY_REQ:
	 case KADEMLIA_CALLBACK_REQ:
	 case KADEMLIA2_PING:
		 return true;
	 default:
		 return false;
	}
}

bool CPacketTracking::IsOnOutTrackList(uint32_t ip, uint8_t opcode, bool dontRemove)
{
#ifdef __DEBUG__
	if (!IsTrackedOutListRequestPacket(opcode)) {
		wxFAIL;	// code error / bug
	}
#endif
	uint32_t now = ::GetTickCount();
	for (TrackedPacketList::iterator it = listTrackedRequests.begin(); it != listTrackedRequests.end(); ++it) {
		if (it->ip == ip && it->opcode == opcode && now - it->inserted < SEC2MS(180)) {
			if (!dontRemove) {
				listTrackedRequests.erase(it);
			}
			return true;
		}
	}
	return false;
}

bool CPacketTracking::InTrackListIsAllowedPacket(uint32_t ip, uint8_t opcode, bool /*bValidSenderkey*/)
{
	// this tracklist tacks _incoming_ request packets and acts as a general flood protection by dropping
	// too frequent requests from a single IP, avoiding response floods, processing time DOS attacks and slowing down
	// other possible attacks/behavior (scanning indexed files, fake publish floods, etc)

	// first figure out if this is a request packet to be tracked and its timelimits
	// timelimits are chosen by estimating the max. frequency of such packets on normal operation (+ buffer)
	// (those limits are not meant to be fine to be used by normal usage, but only supposed to be a flood detection)

	uint32_t allowedPacketsPerMinute;
	DEBUG_ONLY( const uint8_t dbgOrgOpcode = opcode; )

	switch (opcode) {
		case KADEMLIA2_BOOTSTRAP_REQ:
			allowedPacketsPerMinute = 2;
			break;
		case KADEMLIA2_HELLO_REQ:
			allowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_REQ:
			allowedPacketsPerMinute = 10;
			break;
		case KADEMLIA2_SEARCH_NOTES_REQ:
			allowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_SEARCH_KEY_REQ:
			allowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_SEARCH_SOURCE_REQ:
			allowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_PUBLISH_KEY_REQ:
			allowedPacketsPerMinute = 3;
			break;
		case KADEMLIA2_PUBLISH_SOURCE_REQ:
			allowedPacketsPerMinute = 2;
			break;
		case KADEMLIA2_PUBLISH_NOTES_REQ:
			allowedPacketsPerMinute = 2;
			break;
		case KADEMLIA_FIREWALLED2_REQ:
			opcode = KADEMLIA_FIREWALLED_REQ;
		case KADEMLIA_FIREWALLED_REQ:
			allowedPacketsPerMinute = 2;
			break;
		case KADEMLIA_FINDBUDDY_REQ:
			allowedPacketsPerMinute = 2;
			break;
		case KADEMLIA_CALLBACK_REQ:
			allowedPacketsPerMinute = 1;
			break;
		case KADEMLIA2_PING:
			allowedPacketsPerMinute = 2;
			break;
		default:
			// not any request packets, so it's a response packet - no further checks on this point
			return true;
	}

	const uint32_t secondsPerPacket = 60 / allowedPacketsPerMinute;
	const uint32_t currentTick = ::GetTickCount();

	// time for cleaning up?
	if (currentTick - lastTrackInCleanup > MIN2MS(12)) {
		InTrackListCleanup();
	}

	// check for existing entries
	TrackedPacketInMap::iterator it2 = m_mapTrackPacketsIn.find(ip);
	TrackPacketsIn_Struct *trackEntry;
	if (it2 == m_mapTrackPacketsIn.end()) {
		trackEntry = new TrackPacketsIn_Struct();
		trackEntry->m_ip = ip;
		m_mapTrackPacketsIn[ip] = trackEntry;
	} else {
		trackEntry = it2->second;
	}

	// search specific request tracks
	for (TrackPacketsIn_Struct::TrackedRequestList::iterator it = trackEntry->m_trackedRequests.begin(); it != trackEntry->m_trackedRequests.end(); ++it) {
		if (it->m_opcode == opcode) {
			// already tracked requests with this opcode, remove already expired request counts
			if (it->m_count > 0 && currentTick - it->m_firstAdded > SEC2MS(secondsPerPacket)) {
				uint32_t removeCount = (currentTick - it->m_firstAdded) / SEC2MS(secondsPerPacket);
				if (removeCount > it->m_count) {
					it->m_count = 0;
					it->m_firstAdded = currentTick; // for the packet we just process
				} else {
					it->m_count -= removeCount;
					it->m_firstAdded += SEC2MS(secondsPerPacket) * removeCount;
				}
			}
			// we increase the counter in any case, even if we drop the packet later
			it->m_count++;
			// remember only for easier cleanup
			trackEntry->m_lastExpire = std::max(trackEntry->m_lastExpire, it->m_firstAdded + SEC2MS(secondsPerPacket) * it->m_count);

			if (CKademlia::IsRunningInLANMode() && ::IsLanIP(wxUINT32_SWAP_ALWAYS(ip))) {
				return true;	// no flood detection in LAN mode
			}

			// now the actual check if this request is allowed
			if (it->m_count > allowedPacketsPerMinute * 5) {
				// this is so far above the limit that it has to be an intentional flood / misuse in any case
				// so we take the next higher punishment and ban the IP
				AddDebugLogLineN(logKadPacketTracking, CFormat(wxT("Massive request flood detected for opcode 0x%X (0x%X) from IP %s - Banning IP")) % opcode % dbgOrgOpcode % KadIPToString(ip));
				theApp->clientlist->AddBannedClient(wxUINT32_SWAP_ALWAYS(ip));
				return false; // drop packet
			} else if (it->m_count > allowedPacketsPerMinute) {
				// over the limit, drop the packet but do nothing else
				if (!it->m_dbgLogged) {
					it->m_dbgLogged = true;
					AddDebugLogLineN(logKadPacketTracking, CFormat(wxT("Request flood detected for opcode 0x%X (0x%X) from IP %s - Dropping packets with this opcode")) % opcode % dbgOrgOpcode % KadIPToString(ip));
				}
				return false; // drop packet
			} else {
				it->m_dbgLogged = false;
			}
			return true;
		}	
	}

	// add a new entry for this request, no checks needed since 1 is always ok
	TrackPacketsIn_Struct::TrackedRequestIn_Struct curTrackedRequest;
	curTrackedRequest.m_opcode = opcode;
	curTrackedRequest.m_dbgLogged = false;
	curTrackedRequest.m_firstAdded = currentTick;
	curTrackedRequest.m_count = 1;
	// remember only for easier cleanup
	trackEntry->m_lastExpire = std::max(trackEntry->m_lastExpire, currentTick + SEC2MS(secondsPerPacket));
	trackEntry->m_trackedRequests.push_back(curTrackedRequest);
	return true;
}

void CPacketTracking::InTrackListCleanup()
{
	const uint32_t currentTick = ::GetTickCount();
	DEBUG_ONLY( const uint32_t dbgOldSize = m_mapTrackPacketsIn.size(); )
	lastTrackInCleanup = currentTick;
	for (TrackedPacketInMap::iterator it = m_mapTrackPacketsIn.begin(); it != m_mapTrackPacketsIn.end();) {
		TrackedPacketInMap::iterator it2 = it++;
		if (it2->second->m_lastExpire < currentTick) {
			delete it2->second;
			m_mapTrackPacketsIn.erase(it2);
		}
	}
	AddDebugLogLineN(logKadPacketTracking, CFormat(wxT("Cleaned up Kad Incoming Requests Tracklist, entries before: %u, after %u")) % dbgOldSize % m_mapTrackPacketsIn.size());
}

void CPacketTracking::AddLegacyChallenge(const CUInt128& contactID, const CUInt128& challengeID, uint32_t ip, uint8_t opcode)
{
	uint32_t now = ::GetTickCount();
	TrackChallenge_Struct sTrack = { ip, now, opcode, contactID, challengeID };
	listChallengeRequests.push_front(sTrack);
	while (!listChallengeRequests.empty()) {
		if (now - listChallengeRequests.back().inserted > SEC2MS(180)) {
			AddDebugLogLineN(logKadPacketTracking, wxT("Challenge timed out, client not verified - ") + KadIPToString(listChallengeRequests.back().ip));
			listChallengeRequests.pop_back();
		} else {
			break;
		}
	}
}

bool CPacketTracking::IsLegacyChallenge(const CUInt128& challengeID, uint32_t ip, uint8_t opcode, CUInt128& contactID)
{
	uint32_t now = ::GetTickCount();
	DEBUG_ONLY( bool warning = false; )
	for (TrackChallengeList::iterator it = listChallengeRequests.begin(); it != listChallengeRequests.end();) {
		TrackChallengeList::iterator it2 = it++;
		if (it2->ip == ip && it2->opcode == opcode && now - it2->inserted < SEC2MS(180)) {
			wxASSERT(it2->challenge != 0 || opcode == KADEMLIA2_PING);
			if (it2->challenge == 0 || it2->challenge == challengeID) {
				contactID = it2->contactID;
				listChallengeRequests.erase(it2);
				return true;
			} else {
				DEBUG_ONLY( warning = true; )
			}
		}
	}
#ifdef __DEBUG__
	if (warning) {
		AddDebugLogLineN(logKadPacketTracking, wxT("Wrong challenge answer received, client not verified (") + KadIPToString(ip) + wxT(")"));
	}
#endif
	return false;
}

bool CPacketTracking::HasActiveLegacyChallenge(uint32_t ip) const
{
	uint32_t now = ::GetTickCount();
	for (TrackChallengeList::const_iterator it = listChallengeRequests.begin(); it != listChallengeRequests.end(); ++it) {
		if (it->ip == ip && now - it->inserted <= SEC2MS(180)) {
			return true;
		}
	}
	return false;
}

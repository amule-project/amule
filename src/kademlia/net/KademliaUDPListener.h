//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2008 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __KAD_UDP_LISTENER_H__
#define __KAD_UDP_LISTENER_H__

#include "../utils/UInt128.h"
#include "../../Tag.h"

#include <list>


class CMemFile;
struct SSearchTerm;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CContact; 

struct TrackedPacket {
	uint32_t ip;
	uint32_t inserted;
	uint8_t  opcode;
};

typedef std::list<TrackedPacket> TrackedPacketList;


#ifdef __DEBUG__
#	define DebugSendF(what, ip, port)	AddDebugLogLineM(false, logClientKadUDP, what + wxString(wxT(" to ")) + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port))
#	define DebugRecvF(what, ip, port)	AddDebugLogLineM(false, logClientKadUDP, what + wxString(wxT(" from ")) + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port))
#else
#	define DebugSendF(what, ip, port)
#	define DebugRecvF(what, ip, port)
#endif

#define DebugSend(what, ip, port)	DebugSendF(wxSTRINGIZE_T(what), ip, port)
#define DebugRecv(what, ip, port)	DebugRecvF(wxSTRINGIZE_T(what), ip, port)


class CKademliaUDPListener
{
public:
	virtual ~CKademliaUDPListener() {};
	void Bootstrap(uint32_t ip, uint16_t port, bool kad2);
	void FirewalledCheck(uint32_t ip, uint16_t port);
	void SendMyDetails(uint8_t opcode, uint32_t ip, uint16_t port, bool kad2);
	void SendNullPacket(uint8_t opcode, uint32_t ip, uint16_t port);
	void SendPublishSourcePacket(const CContact& contact, const CUInt128& targetID, const CUInt128& contactID, const TagPtrList& tags);
	virtual void ProcessPacket(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port);
	void SendPacket(const CMemFile& data, uint8_t opcode, uint32_t destinationHost, uint16_t destinationPort);

private:
	static SSearchTerm* CreateSearchExpressionTree(CMemFile& bio, int iLevel);
	static void Free(SSearchTerm* pSearchTerms);

	// Kad1.0
	void AddContact (const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port, uint16_t tport, bool update);
	void AddContacts(const uint8_t* data, uint32_t lenData, uint16_t numContacts, bool update);

	void ProcessBootstrapRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessBootstrapResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessHelloRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessHelloResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessKademliaRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessKademliaResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessSearchRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessSearchResponse		(const uint8_t* packetData, uint32_t lenPacket);
	void ProcessPublishRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessPublishResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessSearchNotesRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessSearchNotesResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessPublishNotesRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessPublishNotesResponse	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessFirewalledRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessFirewalledResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessFirewalledAckResponse	(uint32_t lenPacket);
	void ProcessFindBuddyRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessFindBuddyResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessCallbackRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);

	// Kad2.0
	void AddContact2(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port, bool update);

	void Process2BootstrapRequest		(uint32_t ip, uint16_t port);
	void Process2BootstrapResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2HelloRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2HelloResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessKademlia2Request		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void ProcessKademlia2Response		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2SearchNotesRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2SearchKeyRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2SearchSourceRequest	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2SearchResponse		(const uint8_t* packetData, uint32_t lenPacket);
	void Process2PublishNotesRequest	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2PublishKeyRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2PublishSourceRequest	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port);
	void Process2PublishResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void Process2Ping			(uint32_t ip, uint16_t port);
	void Process2Pong			();

	// Debug
	void DebugClientOutput(const wxString& place, uint32_t kad_ip, uint32_t port, const uint8_t* data = NULL, int len = 0);

	// Tracking
	void AddTrackedPacket(uint32_t ip, uint8_t opcode);
	bool IsOnTrackList(uint32_t ip, uint8_t opcode, bool dontRemove = false);
	TrackedPacketList m_trackedRequests;
};

} // End namespace

#endif //__KAD_UDP_LISTENER_H__
// File_checked_for_headers

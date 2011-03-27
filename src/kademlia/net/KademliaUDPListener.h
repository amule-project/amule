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

#ifndef __KAD_UDP_LISTENER_H__
#define __KAD_UDP_LISTENER_H__

#include "../utils/UInt128.h"
#include "../../Tag.h"
#include "PacketTracking.h"

#include <list>


class CMemFile;
struct SSearchTerm;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CContact; 
class CKadUDPKey;
class CKadClientSearcher;

struct FetchNodeID_Struct {
	uint32_t ip;
	uint32_t tcpPort;
	uint32_t expire;
	CKadClientSearcher* requester;
};

#define DebugSendF(what, ip, port)	AddDebugLogLineN(logClientKadUDP, what + wxString(wxT(" to ")) + KadIPPortToString(ip, port))
#define DebugRecvF(what, ip, port)	AddDebugLogLineN(logClientKadUDP, what + wxString(wxT(" from ")) + KadIPPortToString(ip, port))

#define DebugSend(what, ip, port)	DebugSendF(wxSTRINGIZE_T(what), ip, port)
#define DebugRecv(what, ip, port)	DebugRecvF(wxSTRINGIZE_T(what), ip, port)


class CKademliaUDPListener : public CPacketTracking
{
public:
	~CKademliaUDPListener();
	void Bootstrap(uint32_t ip, uint16_t port, uint8_t kadVersion = 0, const CUInt128* cryptTargetID = NULL);
	void FirewalledCheck(uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, uint8_t kadVersion);
	void SendMyDetails(uint8_t opcode, uint32_t ip, uint16_t port, uint8_t kadVersion, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID, bool requestAckPacket);
	void SendNullPacket(uint8_t opcode, uint32_t ip, uint16_t port, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID);
	void SendPublishSourcePacket(const CContact& contact, const CUInt128& targetID, const CUInt128& contactID, const TagPtrList& tags);
	virtual void ProcessPacket(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port, bool validReceiverKey, const CKadUDPKey& senderKey);
	void SendPacket(const CMemFile& data, uint8_t opcode, uint32_t destinationHost, uint16_t destinationPort, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID);

	bool FindNodeIDByIP(CKadClientSearcher *requester, uint32_t ip, uint16_t tcpPort, uint16_t udpPort);
	void ExpireClientSearch(CKadClientSearcher *expireImmediately = NULL);
private:
	static SSearchTerm* CreateSearchExpressionTree(CMemFile& bio, int iLevel);
	static void Free(SSearchTerm* pSearchTerms);

	// Kad1.0
	void SendLegacyChallenge(uint32_t ip, uint16_t port, const CUInt128& contactID);

	void ProcessSearchResponse		(CMemFile &bio);
	void ProcessSearchResponse		(const uint8_t* packetData, uint32_t lenPacket);
	void ProcessPublishResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessSearchNotesResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessFirewalledRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void ProcessFirewalledResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, const CKadUDPKey& senderKey);
	void ProcessFirewalledAckResponse	(uint32_t lenPacket);
	void ProcessFindBuddyRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void ProcessFindBuddyResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void ProcessCallbackRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);

	// Kad2.0
	bool AddContact2(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t& port, uint8_t* outVersion, const CKadUDPKey& key, bool& ipVerified, bool update, bool fromHelloReq, bool* outRequestsACK, CUInt128* outContactID);

	void Process2BootstrapRequest		(uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2BootstrapResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey);
	void Process2HelloRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey);
	void Process2HelloResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey);
	void Process2HelloResponseAck		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, bool validReceiverKey);
	void ProcessKademlia2Request		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void ProcessKademlia2Response		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2SearchNotesRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2SearchKeyRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2SearchSourceRequest	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2SearchResponse		(const uint8_t* packetData, uint32_t lenPacket, const CKadUDPKey& senderKey);
	void Process2PublishNotesRequest	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2PublishKeyRequest		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2PublishSourceRequest	(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2PublishResponse		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2Ping			(uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);
	void Process2Pong			(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void Process2FirewallUDP		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip);
	void ProcessFirewalled2Request		(const uint8_t* packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey);

	// Debug
	void DebugClientOutput(const wxString& place, uint32_t kad_ip, uint32_t port, const uint8_t* data = NULL, int len = 0);

	typedef std::list<FetchNodeID_Struct>	FetchNodeIDList;
	FetchNodeIDList m_fetchNodeIDRequests;
};

} // End namespace

#endif //__KAD_UDP_LISTENER_H__
// File_checked_for_headers

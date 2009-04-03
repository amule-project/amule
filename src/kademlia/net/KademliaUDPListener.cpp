//
// This file is part of aMule Project
//
// Copyright (c) 2004-2009 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2009 aMule Project ( http://www.amule-project.net )
// Copyright (C)2003 Barry Dunne (http://www.emule-project.net)
// Copyright (C)2007-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )

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

#include <wx/wx.h>

#include "KademliaUDPListener.h"

#include <protocol/Protocols.h>
#include <protocol/kad/Constants.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Constants.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <protocol/ed2k/Client2Client/TCP.h> // OP_CALLBACK is sent in some cases.
#include <common/Macros.h>
#include <common/Format.h>
#include <tags/FileTags.h>

#include "../routing/Contact.h"
#include "../routing/RoutingZone.h"
#include "../kademlia/Indexed.h"
#include "../kademlia/Defines.h"
#include "../kademlia/UDPFirewallTester.h"
#include "../utils/KadUDPKey.h"
#include "../utils/KadClientSearcher.h"
#include "../../amule.h"
#include "../../ClientUDPSocket.h"
#include "../../Packet.h"
#include "../../ClientList.h"
#include "../../Statistics.h"
#include "../../MemFile.h"
#include "../../updownclient.h"
#include "../../ClientTCPSocket.h"
#include "../../Logger.h"
#include "../../Preferences.h"
#include "../../ScopedPtr.h"
#include "../../IPFilter.h"

#include <wx/tokenzr.h>

#define THIS_DEBUG_IS_JUST_FOR_KRY_DONT_TOUCH_IT_KTHX 0

#if defined(__SUNPRO_CC)
#define __FUNCTION__ __FILE__+__LINE__
#endif

#define CHECK_PACKET_SIZE(OP, SIZE) \
	if (lenPacket OP (uint32_t)(SIZE)) \
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__)

#define CHECK_PACKET_MIN_SIZE(SIZE)	CHECK_PACKET_SIZE(<, SIZE)
#define CHECK_PACKET_EXACT_SIZE(SIZE)	CHECK_PACKET_SIZE(!=, SIZE)

#define CHECK_TRACKED_PACKET(OPCODE) \
	if (!IsOnOutTrackList(ip, OPCODE)) \
		throw wxString::Format(wxT("***NOTE: Received unrequested response packet, size (%u) in "), lenPacket) + wxString::FromAscii(__FUNCTION__)


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CKademliaUDPListener::~CKademliaUDPListener()
{
	// report timeout to all pending FetchNodeIDRequests
	for (FetchNodeIDList::iterator it = m_fetchNodeIDRequests.begin(); it != m_fetchNodeIDRequests.end(); ++it) {
		it->requester->KadSearchNodeIDByIPResult(KCSR_TIMEOUT, NULL);
	}
}

// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::Bootstrap(uint32_t ip, uint16_t port, bool kad2, uint8_t kadVersion, const CUInt128* cryptTargetID)
{
	wxASSERT(ip);

	if (kad2) {
		DebugSend(Kad2BootstrapReq, ip, port);
		CMemFile bio(0);
		if (kadVersion >= 6) {
			SendPacket(bio, KADEMLIA2_BOOTSTRAP_REQ, ip, port, 0, cryptTargetID);
		} else {
			SendPacket(bio, KADEMLIA2_BOOTSTRAP_REQ, ip, port, 0, NULL);
		}
	} else {
		DebugSend(KadBootstrapReq, ip, port);
		SendMyDetails(KADEMLIA_BOOTSTRAP_REQ, ip, port, 0, 0, NULL, false);
	}
}

// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::SendMyDetails(uint8_t opcode, uint32_t ip, uint16_t port, uint8_t kadVersion, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID, bool requestAckPacket)
{
	CMemFile packetdata;
	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
	
	if (kadVersion > 0) {
		packetdata.WriteUInt16(thePrefs::GetPort());
		packetdata.WriteUInt8(KADEMLIA_VERSION);
		// Tag Count.
		uint8_t tagCount = 0;
		if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
			tagCount++;
		}
		if (kadVersion >= 8 && (requestAckPacket || CKademlia::GetPrefs()->GetFirewalled() || CUDPFirewallTester::IsFirewalledUDP(true))) {
			tagCount++;
		}
		packetdata.WriteUInt8(tagCount);
		if (!CKademlia::GetPrefs()->GetUseExternKadPort()) {
			packetdata.WriteTag(CTagVarInt(TAG_SOURCEUPORT, CKademlia::GetPrefs()->GetInternKadPort()));
		}
		if (kadVersion >= 8 && (requestAckPacket || CKademlia::GetPrefs()->GetFirewalled() || CUDPFirewallTester::IsFirewalledUDP(true))) {
			// if we're firewalled we send this tag, so the other client doesn't add us to his routing table (if UDP firewalled) and for statistics reasons (TCP firewalled)
			// 5 - reserved (!)
			// 1 - requesting HELLO_RES_ACK
			// 1 - TCP firewalled
			// 1 - UDP firewalled
			packetdata.WriteTag(CTagVarInt(TAG_KADMISCOPTIONS, (uint8_t)(
				(requestAckPacket ? 1 : 0) << 2 |
				(CKademlia::GetPrefs()->GetFirewalled() ? 1 : 0) << 1 |
				(CUDPFirewallTester::IsFirewalledUDP(true) ? 1 : 0)
			)));
		}
		// packetdata.WriteTag(CKadTagUInt(TAG_USER_COUNT, CKademlia::GetPrefs()->GetKademliaUsers()));
		// packetdata.WriteTag(CKadTagUInt(TAG_FILE_COUNT, CKademlia::GetPrefs()->GetKademliaFiles()));
		if (kadVersion >= 6) {
			if (cryptTargetID == NULL || *cryptTargetID == 0) {
				AddDebugLogLineM(false, logClientKadUDP, wxT("Sending hello response to crypt enabled Kad Node which provided an empty NodeID: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxString::Format(wxT(" (%u)"), kadVersion));
				SendPacket(packetdata, opcode, ip, port, targetKey, NULL);
			} else {
				SendPacket(packetdata, opcode, ip, port, targetKey, cryptTargetID);
			}
		} else {
			SendPacket(packetdata, opcode, ip, port, 0, NULL);
			wxASSERT(targetKey.IsEmpty());
		}
	} else {
		wxASSERT(!requestAckPacket);
		packetdata.WriteUInt32(CKademlia::GetPrefs()->GetIPAddress());
		packetdata.WriteUInt16(thePrefs::GetEffectiveUDPPort());
		packetdata.WriteUInt16(thePrefs::GetPort());
		packetdata.WriteUInt8(0);
		SendPacket(packetdata, opcode, ip, port, 0, NULL);
	}
}

// Kad1.0 and Kad2.0 currently.
void CKademliaUDPListener::FirewalledCheck(uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, uint8_t kadVersion)
{
	if (kadVersion > 6) {
		// new opcode since 0.49a with extended informations to support obfuscated connections properly
		CMemFile packetdata(19);
		packetdata.WriteUInt16(thePrefs::GetPort());
		packetdata.WriteUInt128(CKademlia::GetPrefs()->GetClientHash());
		packetdata.WriteUInt8(CPrefs::GetMyConnectOptions(true, false));
		DebugSend(KadFirewalled2Req, ip, port);
		SendPacket(packetdata, KADEMLIA_FIREWALLED2_REQ, ip, port, senderKey, NULL);
	} else {
		CMemFile packetdata(2);
		packetdata.WriteUInt16(thePrefs::GetPort());
		DebugSend(KadFirewalledReq, ip, port);
		SendPacket(packetdata, KADEMLIA_FIREWALLED_REQ, ip, port, senderKey, NULL);
	}
	theApp->clientlist->AddKadFirewallRequest(wxUINT32_SWAP_ALWAYS(ip));
}

void CKademliaUDPListener::SendNullPacket(uint8_t opcode, uint32_t ip, uint16_t port, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID)
{
	CMemFile packetdata(0);
	SendPacket(packetdata, opcode, ip, port, targetKey, cryptTargetID);
}

void CKademliaUDPListener::SendPublishSourcePacket(const CContact& contact, const CUInt128 &targetID, const CUInt128 &contactID, const TagPtrList& tags)
{
	uint8_t opcode;
	CMemFile packetdata;
	packetdata.WriteUInt128(targetID);
	if (contact.GetVersion() >= 4/*47c*/) {
		opcode = KADEMLIA2_PUBLISH_SOURCE_REQ;
		packetdata.WriteUInt128(contactID);
		packetdata.WriteTagPtrList(tags);
		DebugSend(Kad2PublishSrcReq, contact.GetIPAddress(), contact.GetUDPPort());
	} else {
		opcode = KADEMLIA_PUBLISH_REQ;
		//We only use this for publishing sources now.. So we always send one here..
		packetdata.WriteUInt16(1);
		packetdata.WriteUInt128(contactID);
		packetdata.WriteTagPtrList(tags);
		DebugSend(KadPublishReq, contact.GetIPAddress(), contact.GetUDPPort());
	}
	if (contact.GetVersion() >= 6) {	// obfuscated ?
		CUInt128 clientID = contact.GetClientID();
		SendPacket(packetdata, opcode, contact.GetIPAddress(), contact.GetUDPPort(), contact.GetUDPKey(), &clientID);
	} else {
		SendPacket(packetdata, opcode, contact.GetIPAddress(), contact.GetUDPPort(), 0, NULL);
	}
}

void CKademliaUDPListener::ProcessPacket(const uint8_t* data, uint32_t lenData, uint32_t ip, uint16_t port, bool validReceiverKey, const CKadUDPKey& senderKey)
{
	// we do not accept (<= 0.48a) unencrypted incoming packets from port 53 (DNS) to avoid attacks based on DNS protocol confusion
	if (port == 53 && senderKey.IsEmpty()) {
#ifdef __DEBUG__
		AddDebugLogLineM(false, logKadPacketTracking, wxT("Dropping incoming unencrypted packet on port 53 (DNS), IP: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
#endif
		return;
	}

	//Update connection state only when it changes.
	bool curCon = CKademlia::GetPrefs()->HasHadContact();
	CKademlia::GetPrefs()->SetLastContact();
	CUDPFirewallTester::Connected();
	if( curCon != CKademlia::GetPrefs()->HasHadContact()) {
		theApp->ShowConnectionState();
	}

	uint8_t opcode = data[1];
	const uint8_t *packetData = data + 2;
	uint32_t lenPacket = lenData - 2;

	if (!InTrackListIsAllowedPacket(ip, opcode, validReceiverKey)) {
		return;
	}

	switch (opcode) {
		case KADEMLIA_BOOTSTRAP_REQ:
			DebugRecv(KadBootstrapReq, ip, port);
			ProcessBootstrapRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_BOOTSTRAP_REQ:
			DebugRecv(Kad2BootstrapReq, ip, port);
			Process2BootstrapRequest(ip, port, senderKey);
			break;
		case KADEMLIA_BOOTSTRAP_RES:
			DebugRecv(KadBootstrapRes, ip, port);
			ProcessBootstrapResponse(packetData, lenPacket, ip);
			break;
		case KADEMLIA2_BOOTSTRAP_RES:
			DebugRecv(Kad2BootstrapRes, ip, port);
			Process2BootstrapResponse(packetData, lenPacket, ip, port, senderKey, validReceiverKey);
			break;
		case KADEMLIA_HELLO_REQ:
			DebugRecv(KadHelloReq, ip, port);
			ProcessHelloRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_HELLO_REQ:
			DebugRecv(Kad2HelloReq, ip, port);
			Process2HelloRequest(packetData, lenPacket, ip, port, senderKey, validReceiverKey);
			break;
		case KADEMLIA_HELLO_RES:
			DebugRecv(KadHelloRes, ip, port);
			ProcessHelloResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_HELLO_RES:
			DebugRecv(Kad2HelloRes, ip, port);
			Process2HelloResponse(packetData, lenPacket, ip, port, senderKey, validReceiverKey);
			break;
		case KADEMLIA2_HELLO_RES_ACK:
			DebugRecv(Kad2HelloResAck, ip, port);
			Process2HelloResponseAck(packetData, lenPacket, ip, validReceiverKey);
			break;
		case KADEMLIA_REQ:
			DebugRecv(KadReq, ip, port);
			ProcessKademliaRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_REQ:
			DebugRecv(Kad2Req, ip, port);
			ProcessKademlia2Request(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_RES:
			DebugRecv(KadRes, ip, port);
			ProcessKademliaResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_RES:
			DebugRecv(Kad2Res, ip, port);
			ProcessKademlia2Response(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_SEARCH_REQ:
			DebugRecv(KadSearchReq, ip, port);
			ProcessSearchRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_NOTES_REQ:
			DebugRecv(KadSearchNotesReq, ip, port);
			ProcessSearchNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_SEARCH_NOTES_REQ:
			DebugRecv(Kad2SearchNotesReq, ip, port);
			Process2SearchNotesRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA2_SEARCH_KEY_REQ:
			DebugRecv(Kad2SearchKeyReq, ip, port);
			Process2SearchKeyRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA2_SEARCH_SOURCE_REQ:
			DebugRecv(Kad2SearchSourceReq, ip, port);
			Process2SearchSourceRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_SEARCH_RES:
			DebugRecv(KadSearchRes, ip, port);
			ProcessSearchResponse(packetData, lenPacket);
			break;
		case KADEMLIA_SEARCH_NOTES_RES:
			DebugRecv(KadSearchNotesRes, ip, port);
			ProcessSearchNotesResponse(packetData, lenPacket, ip);
			break;
		case KADEMLIA2_SEARCH_RES:
			DebugRecv(Kad2SearchRes, ip, port);
			Process2SearchResponse(packetData, lenPacket, senderKey);
			break;
		case KADEMLIA_PUBLISH_REQ:
			DebugRecv(KadPublishReq, ip, port);
			ProcessPublishRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_NOTES_REQ:
			DebugRecv(KadPublishNotesReq, ip, port);
			ProcessPublishNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA2_PUBLISH_NOTES_REQ:
			DebugRecv(Kad2PublishNotesReq, ip, port);
			Process2PublishNotesRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA2_PUBLISH_KEY_REQ:
			DebugRecv(Kad2PublishKeyReq, ip, port);
			Process2PublishKeyRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA2_PUBLISH_SOURCE_REQ:
			DebugRecv(Kad2PublishSourceReq, ip, port);
			Process2PublishSourceRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_PUBLISH_RES:
			DebugRecv(KadPublishRes, ip, port);
			ProcessPublishResponse(packetData, lenPacket, ip);
			break;
		case KADEMLIA_PUBLISH_NOTES_RES:
			DebugRecv(KadPublishNotesRes, ip, port);
			ProcessPublishNotesResponse(packetData, lenPacket, ip);
			break;
		case KADEMLIA2_PUBLISH_RES:
			DebugRecv(Kad2PublishRes, ip, port);
			Process2PublishResponse(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_FIREWALLED_REQ:
			DebugRecv(KadFirewalledReq, ip, port);
			ProcessFirewalledRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_FIREWALLED2_REQ:
			DebugRecv(KadFirewalled2Req, ip, port);
			ProcessFirewalled2Request(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_FIREWALLED_RES:
			DebugRecv(KadFirewalledRes, ip, port);
			ProcessFirewalledResponse(packetData, lenPacket, ip, senderKey);
			break;
		case KADEMLIA_FIREWALLED_ACK_RES:
			DebugRecv(KadFirewalledAck, ip, port);
			ProcessFirewalledAckResponse(lenPacket);
			break;
		case KADEMLIA_FINDBUDDY_REQ:
			DebugRecv(KadFindBuddyReq, ip, port);
			ProcessFindBuddyRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_FINDBUDDY_RES:
			DebugRecv(KadFindBuddyRes, ip, port);
			ProcessFindBuddyResponse(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA_CALLBACK_REQ:
			DebugRecv(KadCallbackReq, ip, port);
			ProcessCallbackRequest(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA2_PING:
			DebugRecv(Kad2Ping, ip, port);
			Process2Ping(ip, port, senderKey);
			break;
		case KADEMLIA2_PONG:
			DebugRecv(Kad2Pong, ip, port);
			Process2Pong(packetData, lenPacket, ip);
			break;
		case KADEMLIA2_FIREWALLUDP:
			DebugRecv(Kad2FirewallUDP, ip, port);
			Process2FirewallUDP(packetData, lenPacket, ip);
			break;
		default: {
			throw wxString::Format(wxT("Unknown opcode %02x on CKademliaUDPListener::ProcessPacket()"), opcode);
		}
	}
}

// Used only for Kad1.0
bool CKademliaUDPListener::AddContact(const uint8_t *data, uint32_t lenData, uint32_t ip, uint16_t port, uint16_t tport, const CKadUDPKey& udpKey, bool& ipVerified, bool update, CUInt128* outContactID)
{
	CMemFile bio(data, lenData);
	CUInt128 id = bio.ReadUInt128();
	if (outContactID != NULL) {
		*outContactID = id;
	}
	bio.ReadUInt32();
	bio.ReadUInt16();
	if (tport) {
		bio.ReadUInt16();
	} else {
		tport = bio.ReadUInt16();
	}
	bio.ReadUInt8();
	//AddDebugLogLineM(false, logKadMain, wxT("Adding a contact with ip ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip),port));
	// Ignore stated ip and port, use the address the packet came from
	return CKademlia::GetRoutingZone()->Add(id, ip, port, tport, 0, udpKey, ipVerified, update, false, true);
}

// Used only for Kad2.0
bool CKademliaUDPListener::AddContact2(const uint8_t *data, uint32_t lenData, uint32_t ip, uint16_t& port, uint8_t *outVersion, const CKadUDPKey& udpKey, bool& ipVerified, bool update, bool fromHelloReq, bool* outRequestsACK, CUInt128* outContactID)
{
	if (outRequestsACK != 0) {
		*outRequestsACK = false;
	}

	CMemFile bio(data, lenData);
	CUInt128 id = bio.ReadUInt128();
	if (outContactID != NULL) {
		*outContactID = id;
	}
	uint16_t tport = bio.ReadUInt16();
	uint8_t version = bio.ReadUInt8();
	if (version == 0) {
		throw wxString::Format(wxT("***NOTE: Received invalid Kademlia2 version (%u) in "), version) + wxString::FromAscii(__FUNCTION__);
	}
	if (outVersion != NULL) {
		*outVersion = version;
	}
	bool udpFirewalled = false;
	bool tcpFirewalled = false;
	uint8_t tags = bio.ReadUInt8();
	while (tags) {
		CTag *tag = bio.ReadTag();
		if (!tag->GetName().Cmp(TAG_SOURCEUPORT)) {
			if (tag->IsInt() && (uint16_t)tag->GetInt() > 0) {
				port = tag->GetInt();
			}
		} else if (!tag->GetName().Cmp(TAG_KADMISCOPTIONS)) {
			if (tag->IsInt() && tag->GetInt() > 0) {
				udpFirewalled = (tag->GetInt() & 0x01) > 0;
				tcpFirewalled = (tag->GetInt() & 0x02) > 0;
				if ((tag->GetInt() & 0x04) > 0) {
					if (outRequestsACK != NULL) {
						if (version >= 8) {
							*outRequestsACK = true;
						}
					} else {
						wxFAIL;
					}
				}
			}
		}
		delete tag;
		--tags;
	}

	// check if we are waiting for informations (nodeid) about this client and if so inform the requester
	for (FetchNodeIDList::iterator it = m_fetchNodeIDRequests.begin(); it != m_fetchNodeIDRequests.end(); ++it) {
		if (it->ip == ip && it->tcpPort == tport) {
			//AddDebugLogLineM(false, logKadMain, wxT("Result Addcontact: ") + id.ToHexString());
			uint8_t uchID[16];
			id.ToByteArray(uchID);
			it->requester->KadSearchNodeIDByIPResult(KCSR_SUCCEEDED, uchID);
			m_fetchNodeIDRequests.erase(it);
			break;
		}
	}

	if (fromHelloReq && version >= 8) {
		// this is just for statistic calculations. We try to determine the ratio of (UDP) firewalled users,
		// by counting how many of all nodes which have us in their routing table (our own routing table is supposed
		// to have no UDP firewalled nodes at all) and support the firewalled tag are firewalled themself.
		// Obviously this only works if we are not firewalled ourself
		CKademlia::GetPrefs()->StatsIncUDPFirewalledNodes(udpFirewalled);
		CKademlia::GetPrefs()->StatsIncTCPFirewalledNodes(tcpFirewalled);
	}

	if (!udpFirewalled) {	// do not add (or update) UDP firewalled sources to our routing table
		return CKademlia::GetRoutingZone()->Add(id, ip, port, tport, version, udpKey, ipVerified, update, false, true);
	} else {
		AddDebugLogLineM(false, logKadRouting, wxT("Not adding firewalled client to routing table (") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
		return false;
	}
}

// Used only for Kad1.0
void CKademliaUDPListener::AddContacts(const uint8_t *data, uint32_t lenData, uint16_t numContacts, bool update)
{
	CMemFile bio(data, lenData);
	CRoutingZone *routingZone = CKademlia::GetRoutingZone();
	for (uint16_t i = 0; i < numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32_t ip = bio.ReadUInt32();
		ip = wxUINT32_SWAP_ALWAYS(ip);
		uint16_t port = bio.ReadUInt16();
		uint16_t tport = bio.ReadUInt16();
		bio.ReadUInt8();
		bool verified = false;
		//AddDebugLogLineM(false, logKadMain, wxT("Adding contact(s) with ip ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip),port));
		routingZone->Add(id, ip, port, tport, 0, 0, verified, update, false, false);
	}
}

// KADEMLIA_BOOTSTRAP_REQ
// Used only for Kad1.0
void CKademliaUDPListener::ProcessBootstrapRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(25);

	// Add the sender to the list of contacts
	bool verified = false;
	AddContact(packetData, lenPacket, ip, port, 0, 0, verified, true, NULL);

	// Get some contacts to return
	ContactList contacts;
	uint16_t numContacts = 1 + (uint16_t)CKademlia::GetRoutingZone()->GetBootstrapContacts(&contacts, 20);

	// Create response packet
	//We only collect a max of 20 contacts here.. Max size is 527.
	//2 + 25(20) + 25
	CMemFile packetdata(527);

	// Write packet info
	packetdata.WriteUInt16(numContacts);
	CContact *contact;
	for (ContactList::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
		contact = *it;
		packetdata.WriteUInt128(contact->GetClientID());
		packetdata.WriteUInt32(contact->GetIPAddress());
		packetdata.WriteUInt16(contact->GetUDPPort());
		packetdata.WriteUInt16(contact->GetTCPPort());
		packetdata.WriteUInt8(contact->GetType());
	}

	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
	packetdata.WriteUInt32(CKademlia::GetPrefs()->GetIPAddress());
	packetdata.WriteUInt16(thePrefs::GetEffectiveUDPPort());
	packetdata.WriteUInt16(thePrefs::GetPort());
	packetdata.WriteUInt8(0);

	// Send response
	DebugSend(KadBootstrapRes, ip, port);
	SendPacket(packetdata, KADEMLIA_BOOTSTRAP_RES, ip, port, 0, NULL);
}

// KADEMLIA2_BOOTSTRAP_REQ
// Used only for Kad2.0
void CKademliaUDPListener::Process2BootstrapRequest(uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// Get some contacts to return
	ContactList contacts;
	uint16_t numContacts = (uint16_t)CKademlia::GetRoutingZone()->GetBootstrapContacts(&contacts, 20);

	// Create response packet
	//We only collect a max of 20 contacts here.. Max size is 521.
	//2 + 25(20) + 19
	CMemFile packetdata(521);

	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
	packetdata.WriteUInt16(thePrefs::GetPort());
	packetdata.WriteUInt8(KADEMLIA_VERSION);

	// Write packet info
	packetdata.WriteUInt16(numContacts);
	CContact *contact;
	for (ContactList::const_iterator it = contacts.begin(); it != contacts.end(); ++it) {
		contact = *it;
		packetdata.WriteUInt128(contact->GetClientID());
		packetdata.WriteUInt32(contact->GetIPAddress());
		packetdata.WriteUInt16(contact->GetUDPPort());
		packetdata.WriteUInt16(contact->GetTCPPort());
		packetdata.WriteUInt8(contact->GetVersion());
	}

	// Send response
	DebugSend(Kad2BootstrapRes, ip, port);
	SendPacket(packetdata, KADEMLIA2_BOOTSTRAP_RES, ip, port, senderKey, NULL);
}

// KADEMLIA_BOOTSTRAP_RES
// Used only for Kad1.0
void CKademliaUDPListener::ProcessBootstrapResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(27);
	CHECK_TRACKED_PACKET(KADEMLIA_BOOTSTRAP_REQ);

	// How many contacts were given
	CMemFile bio(packetData, lenPacket);
	uint16_t numContacts = bio.ReadUInt16();

	// Verify packet is expected size
	if (lenPacket != (uint32_t)(2 + 25 * numContacts)) {
		return;
	}

	// Add these contacts to the list.
	AddContacts(packetData + 2, lenPacket - 2, numContacts, false);
}

// KADEMLIA2_BOOTSTRAP_RES
// Used only for Kad2.0
void CKademliaUDPListener::Process2BootstrapResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey)
{
	CHECK_TRACKED_PACKET(KADEMLIA2_BOOTSTRAP_REQ);

	CRoutingZone *routingZone = CKademlia::GetRoutingZone();

	// How many contacts were given
	CMemFile bio(packetData, lenPacket);
	CUInt128 contactID = bio.ReadUInt128();
	uint16_t tport = bio.ReadUInt16();
	uint8_t version = bio.ReadUInt8();
	// if we don't know any Contacts yet and try to Bootstrap, we assume that all contacts are verified,
	// in order to speed up the connecting process. The attackvectors to exploit this are very small with no
	// major effects, so that's a good trade
	bool assumeVerified = CKademlia::GetRoutingZone()->GetNumContacts() == 0;

	if (CKademlia::s_bootstrapList.empty()) {
		routingZone->Add(contactID, ip, port, tport, version, senderKey, validReceiverKey, true, false, false);
	}

	uint16_t numContacts = bio.ReadUInt16();
	while (numContacts) {
		contactID = bio.ReadUInt128();
		ip = bio.ReadUInt32();
		port = bio.ReadUInt16();
		tport = bio.ReadUInt16();
		version = bio.ReadUInt8();
		bool verified = assumeVerified;
		routingZone->Add(contactID, ip, port, tport, version, 0, verified, false, false, false);
		numContacts--;
	}
}

// KADEMLIA_HELLO_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessHelloRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(25);

	// Add the sender to the list of contacts
	bool validReceiverKey = false;
	CUInt128 contactID;
	bool addedOrUpdated = AddContact(packetData, lenPacket, ip, port, 0, 0, validReceiverKey, true, &contactID);

	// Send response
	DebugSend(KadHelloRes, ip, port);	
	SendMyDetails(KADEMLIA_HELLO_RES, ip, port, 0, 0, NULL, false);

	if (addedOrUpdated && !validReceiverKey) {
		// we need to verify this contact but it doesn't support HELLO_RES_ACK nor keys, do a little workaround
		SendLegacyChallenge(ip, port, contactID, false);
	}

	// Check if firewalled
	if (CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port, 0, 0);
	}
}

// KADEMLIA2_HELLO_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey)
{
	uint16_t dbgOldUDPPort = port;
	uint8_t contactVersion = 0;
	CUInt128 contactID;
	bool addedOrUpdated = AddContact2(packetData, lenPacket, ip, port, &contactVersion, senderKey, validReceiverKey, true, true, NULL, &contactID); // might change (udp)port, validReceiverKey
	wxASSERT(contactVersion >= 1);
#ifdef __DEBUG__
	if (dbgOldUDPPort != port) {
		AddDebugLogLineM(false, logClientKadUDP, wxT("KadContact ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxString::Format(wxT(" uses his internal (%u) instead external (%u) UDP Port"), port, dbgOldUDPPort));
	}
#endif

	DebugSend(Kad2HelloRes, ip, port);
	// if this contact was added or updated (so with other words not filtered or invalid) to our routing table and did not already send a valid
	// receiver key or is already verified in the routing table, we request an additional ACK package to complete a three-way-handshake and
	// verify the remote IP
	SendMyDetails(KADEMLIA2_HELLO_RES, ip, port, contactVersion, senderKey, &contactID, addedOrUpdated && !validReceiverKey);

	if (addedOrUpdated && !validReceiverKey && contactVersion == 7 && !HasActiveLegacyChallenge(ip)) {
		// Kad Version 7 doesn't support HELLO_RES_ACK but sender/receiver keys, so send a ping to validate
		DebugSend(Kad2Ping, ip, port);
		SendNullPacket(KADEMLIA2_PING, ip, port, senderKey, NULL);
#ifdef __DEBUG__
		CContact* contact = CKademlia::GetRoutingZone()->GetContact(contactID);
		if (contact != NULL) {
			if (contact->GetType() < 2) {
				AddDebugLogLineM(false, logClientKadUDP, wxT("Sending (ping) challenge to a long known contact (should be verified already) - ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
			}
		} else {
			wxFAIL;
		}
#endif
	} else if (CKademlia::GetPrefs()->FindExternKadPort(false) && contactVersion > 5) {	// do we need to find out our extern port?
		DebugSend(Kad2Ping, ip, port);
		SendNullPacket(KADEMLIA2_PING, ip, port, senderKey, NULL);
	}

	if (addedOrUpdated && !validReceiverKey && contactVersion < 7 && !HasActiveLegacyChallenge(ip)) {
		// we need to verify this contact but it doesn't support HELLO_RES_ACK nor keys, do a little workaround
		SendLegacyChallenge(ip, port, contactID, true);
	}

	// Check if firewalled
	if (CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port, senderKey, contactVersion);
	}
}

// KADEMLIA_HELLO_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessHelloResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	CHECK_TRACKED_PACKET(KADEMLIA_HELLO_REQ);

	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(25);

	// Add or Update contact.
	bool validReceiverKey = false;
	CUInt128 contactID;
	bool addedOrUpdated = AddContact(packetData, lenPacket, ip, port, 0, 0, validReceiverKey, true, &contactID);
	if (addedOrUpdated && !validReceiverKey) {
		// even though this is supposably an answer to a request from us, there are still possibilities to spoof
		// it, as long as the attacker knows that we would send a HELLO_REQ (which in this case is quite often),
		// so for old Kad Version which doesn't support keys, we need
		SendLegacyChallenge(ip, port, contactID, false);
	}
}

// KADEMLIA2_HELLO_RES_ACK
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloResponseAck(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, bool validReceiverKey)
{
	CHECK_PACKET_MIN_SIZE(17);
	CHECK_TRACKED_PACKET(KADEMLIA2_HELLO_RES);

	if (!validReceiverKey) {
		AddDebugLogLineM(false, logClientKadUDP, wxT("Receiver key is invalid! (sender: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
		return;
	}

	// Additional packet to complete a three-way-handshake, making sure the remote contact is not using a spoofed ip.
	CMemFile bio(packetData, lenPacket);
	CUInt128 remoteID = bio.ReadUInt128();
	if (!CKademlia::GetRoutingZone()->VerifyContact(remoteID, ip)) {
		AddDebugLogLineM(false, logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
	}
#ifdef __DEBUG__
	else {
		AddDebugLogLineM(false, logKadRouting, wxT("Verified contact (") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(") by HELLO_RES_ACK"));
	}
#endif
}

// KADEMLIA2_HELLO_RES
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey)
{
	CHECK_TRACKED_PACKET(KADEMLIA2_HELLO_REQ);

	// Add or Update contact.
	uint8_t contactVersion;
	CUInt128 contactID;
	bool sendACK = false;
	bool addedOrUpdated = AddContact2(packetData, lenPacket, ip, port, &contactVersion, senderKey, validReceiverKey, true, false, &sendACK, &contactID);

	if (sendACK) {
		// the client requested us to send an ACK packet, which proves that we're not a spoofed fake contact
		// fulfill his wish
		if (senderKey.IsEmpty()) {
			// but we don't have a valid sender key - there is no point to reply in this case
			// most likely a bug in the remote client
			AddDebugLogLineM(false, logClientKadUDP, wxT("Remote client demands ACK, but didn't send any sender key! (sender: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
		} else {
			CMemFile packet(17);
			packet.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
			packet.WriteUInt8(0);	// no tags at this time
			DebugSend(Kad2HelloResAck, ip, port);
			SendPacket(packet, KADEMLIA2_HELLO_RES_ACK, ip, port, senderKey, NULL);
		}
	} else if (addedOrUpdated && !validReceiverKey && contactVersion < 7) {
		// even though this is supposably an answer to a request from us, there are still possibilities to spoof
		// it, as long as the attacker knows that we would send a HELLO_REQ (which in this case is quite often),
		// so for old Kad Version which doesn't support keys, we need
		SendLegacyChallenge(ip, port, contactID, true);
	}

	// do we need to find out our extern port?
	if (CKademlia::GetPrefs()->FindExternKadPort(false) && contactVersion > 5) {
		DebugSend(Kad2Ping, ip, port);
		SendNullPacket(KADEMLIA2_PING, ip, port, senderKey, NULL);
	}

	// Check if firewalled
	if (CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port, senderKey, contactVersion);
	}
}

// KADEMLIA_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessKademliaRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(33);

	// RecheckIP and firewall status
	if (CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port, 0, 0);
		DebugSend(KadHelloReq, ip, port);
		SendMyDetails(KADEMLIA_HELLO_REQ, ip, port, 0, 0, NULL, false);
	}

	// Get target and type
	CMemFile bio(packetData, lenPacket);
	uint8_t type = bio.ReadUInt8();
//		bool flag1 = (type >> 6); //Reserved
//		bool flag2 = (type >> 7); //Reserved
//		bool flag3 = (type >> 8); //Reserved

	type &= 0x1F;
	if( type == 0 ) {
		throw wxString::Format(wxT("***NOTE: Received wrong type (0x%02x) in "), type) + wxString::FromAscii(__FUNCTION__);
	}

	// This is the target node trying to be found.
	CUInt128 target = bio.ReadUInt128();
	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(target);

	// This makes sure we are not mistaken identify. Some client may have fresh installed and have a new hash.
	CUInt128 check = bio.ReadUInt128();
	if (CKademlia::GetPrefs()->GetKadID().CompareTo(check)) {
		return;
	}

	// Get required number closest to target
	ContactMap results;
	CKademlia::GetRoutingZone()->GetClosestTo(2, target, distance, type, &results);
	uint16_t count = (uint16_t)results.size();

	// Write response
	// Max count is 32. size 817.. 
	// 16 + 1 + 25(32)
	CMemFile packetdata(817);
	packetdata.WriteUInt128(target);
	packetdata.WriteUInt8((uint8_t)count);
	CContact *c;
	for (ContactMap::const_iterator it = results.begin(); it != results.end(); ++it) {
		c = it->second;
		packetdata.WriteUInt128(c->GetClientID());
		packetdata.WriteUInt32(c->GetIPAddress());
		packetdata.WriteUInt16(c->GetUDPPort());
		packetdata.WriteUInt16(c->GetTCPPort());
		packetdata.WriteUInt8(c->GetType());
	}

	DebugSendF(wxString::Format(wxT("KadRes (count=%u)"), count), ip, port);
	SendPacket(packetdata, KADEMLIA_RES, ip, port, 0, NULL);
}

// KADEMLIA2_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::ProcessKademlia2Request(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// Get target and type
	CMemFile bio(packetData, lenPacket);
	uint8_t type = bio.ReadUInt8();
	type &= 0x1F;
	if (type == 0) {
		throw wxString::Format(wxT("***NOTE: Received wrong type (0x%02x) in "), type) + wxString::FromAscii(__FUNCTION__);
	}

	// This is the target node trying to be found.
	CUInt128 target = bio.ReadUInt128();
	// Convert Target to Distance as this is how we store contacts.
	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(target);

	// This makes sure we are not mistaken identify. Some client may have fresh installed and have a new KadID.
	CUInt128 check = bio.ReadUInt128();
	if (CKademlia::GetPrefs()->GetKadID() == check) {
		// Get required number closest to target
		ContactMap results;
		CKademlia::GetRoutingZone()->GetClosestTo(2, target, distance, type, &results);
		uint8_t count = (uint8_t)results.size();

		// Write response
		// Max count is 32. size 817..
		// 16 + 1 + 25(32)
		CMemFile packetdata(817);
		packetdata.WriteUInt128(target);
		packetdata.WriteUInt8(count);
		CContact *c;
		for (ContactMap::const_iterator it = results.begin(); it != results.end(); ++it) {
			c = it->second;
			packetdata.WriteUInt128(c->GetClientID());
			packetdata.WriteUInt32(c->GetIPAddress());
			packetdata.WriteUInt16(c->GetUDPPort());
			packetdata.WriteUInt16(c->GetTCPPort());
			packetdata.WriteUInt8(c->GetVersion()); //<- Kad Version inserted to allow backward compatibility.
		}

		DebugSendF(wxString::Format(wxT("Kad2Res (count=%u)"), count), ip, port);
		SendPacket(packetdata, KADEMLIA2_RES, ip, port, senderKey, NULL);
	}
}

// KADEMLIA_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessKademliaResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(17);
	CHECK_TRACKED_PACKET(KADEMLIA_REQ);

	// Used Pointers
	CRoutingZone *routingZone = CKademlia::GetRoutingZone();

	if (CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port, 0, 0);
		DebugSend(KadHelloReq, ip, port);
		SendMyDetails(KADEMLIA_HELLO_REQ, ip, port, 0, 0, NULL, false);
	}

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint8_t numContacts = bio.ReadUInt8();

	// Is this one of our legacy challenge packets?
	CUInt128 contactID;
	if (IsLegacyChallenge(target, ip, KADEMLIA_REQ, contactID)) {
		// yup it is, set the contact as verified
		if (!routingZone->VerifyContact(contactID, ip)) {
			AddDebugLogLineM(false, logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
		}
#ifdef __DEBUG__
		else {
			AddDebugLogLineM(false, logClientKadUDP, wxT("Verified contact with legacy challenge (KadReq) - ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
		}
#endif
		return;	// we do not actually care for its other content
	}

	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(16+1 + (16+4+2+2+1)*numContacts);

	uint32_t ignoredCount = 0;
	CScopedPtr<ContactList> results(new ContactList);
	
	for (uint16_t i = 0; i < numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32_t contactIP = bio.ReadUInt32();
		uint16_t contactPort = bio.ReadUInt16();
		uint16_t tport = bio.ReadUInt16();
		bio.ReadUInt8();
		uint32_t hostIP = wxUINT32_SWAP_ALWAYS(contactIP);
		if(::IsGoodIPPort(hostIP, contactPort) && contactPort != 53 /*No DNS Port without encryption*/) {
			if (!theApp->ipfilter->IsFiltered(hostIP)) {
				bool verified = false;
				// we are now setting all version for received contact to "2" which means we assume full Kad2 when adding
				// the contact to the routing table. If this should be an old Kad1 contact, we won't be able to keep it, but
				// we avoid having to send double hello packets to the 90% Kad2 nodes
				// This is the first step of dropping Kad1 support
				bool wasAdded = routingZone->AddUnfiltered(id, contactIP, contactPort, tport, 2, 0, verified, false, false, false);
				CContact *temp = new CContact(id, contactIP, contactPort, tport, 0, 0, false, target);
				if (wasAdded || routingZone->IsAcceptableContact(temp)) {
					results->push_back(temp);
				} else {
					ignoredCount++;
					delete temp;
				}
			}
		}
	}

	if (ignoredCount > 0) {
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("Ignored %u bad contacts in routing answer from "), ignoredCount) + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
	}

	CSearchManager::ProcessResponse(target, ip, port, results.release());
}

// KADEMLIA2_RES
// Used in Kad2.0 only
void CKademliaUDPListener::ProcessKademlia2Response(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& WXUNUSED(senderKey))
{
	CHECK_TRACKED_PACKET(KADEMLIA2_REQ);

	// Used Pointers
	CRoutingZone *routingZone = CKademlia::GetRoutingZone();

	// don't do firewallchecks on this opcode anymore, since we need the contacts kad version - hello opcodes are good enough
// 	if (CKademlia::GetPrefs()->GetRecheckIP()) {
// 		FirewalledCheck(ip, port, senderKey);
// 		DebugSend(Kad2HelloReq, ip, port);
// 		SendMyDetails(KADEMLIA2_HELLO_REQ, ip, port, true, senderKey, NULL);
// 	}

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint8_t numContacts = bio.ReadUInt8();

	// Is this one of our legacy challenge packets?
	CUInt128 contactID;
	if (IsLegacyChallenge(target, ip, KADEMLIA2_REQ, contactID)) {
		// yup it is, set the contact as verified
		if (!routingZone->VerifyContact(contactID, ip)) {
			AddDebugLogLineM(false, logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
		}
#ifdef __DEBUG__
		else {
			AddDebugLogLineM(false, logClientKadUDP, wxT("Verified contact with legacy challenge (Kad2Req) - ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
		}
#endif
		return;	// we do not actually care for its other content
	}
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(16+1 + (16+4+2+2+1)*numContacts);

	// is this a search for firewallcheck ips?
	bool isFirewallUDPCheckSearch = false;
	if (CUDPFirewallTester::IsFWCheckUDPRunning() && CSearchManager::IsFWCheckUDPSearch(target)) {
		isFirewallUDPCheckSearch = true;
	}

	uint32_t ignoredCount = 0;
	CScopedPtr<ContactList> results(new ContactList);
	for (uint8_t i = 0; i < numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32_t contactIP = bio.ReadUInt32();
		uint16_t contactPort = bio.ReadUInt16();
		uint16_t tport = bio.ReadUInt16();
		uint8_t version = bio.ReadUInt8();
		uint32_t hostIP = wxUINT32_SWAP_ALWAYS(contactIP);
		if (::IsGoodIPPort(hostIP, contactPort)) {
			if (!theApp->ipfilter->IsFiltered(hostIP) && !(contactPort == 53 && version <= 5) /*No DNS Port without encryption*/) {
				if (isFirewallUDPCheckSearch) {
					// UDP FirewallCheck searches are special. The point is we need an IP which we didn't sent an UDP message yet
					// (or in the near future), so we do not try to add those contacts to our routingzone and we also don't
					// deliver them back to the searchmanager (because he would UDP-ask them for further results), but only report
					// them to FirewallChecker - this will of course cripple the search but thats not the point, since we only 
					// care for IPs and not the random set target
					CUDPFirewallTester::AddPossibleTestContact(id, contactIP, contactPort, tport, target, version, 0, false);
				} else {
					bool verified = false;
					bool wasAdded = routingZone->AddUnfiltered(id, contactIP, contactPort, tport, version, 0, verified, false, false, false);
					CContact *temp = new CContact(id, contactIP, contactPort, tport, version, 0, false, target);
					if (wasAdded || routingZone->IsAcceptableContact(temp)) {
						results->push_back(temp);
					} else {
						ignoredCount++;
						delete temp;
					}
				}
			}
		}
	}

	if (ignoredCount > 0) {
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("Ignored %u bad contacts in routing answer from "), ignoredCount) + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
	}

	CSearchManager::ProcessResponse(target, ip, port, results.release());
}

void CKademliaUDPListener::Free(SSearchTerm* pSearchTerms)
{
	if (pSearchTerms) {
		Free(pSearchTerms->left);
		Free(pSearchTerms->right);
		delete pSearchTerms;
	}
}

SSearchTerm* CKademliaUDPListener::CreateSearchExpressionTree(CMemFile& bio, int iLevel)
{
	// the max. depth has to match our own limit for creating the search expression 
	// (see also 'ParsedSearchExpression' and 'GetSearchPacket')
	if (iLevel >= 24){
		AddDebugLogLineM(false, logKadSearch, wxT("***NOTE: Search expression tree exceeds depth limit!"));
		return NULL;
	}
	iLevel++;

	uint8_t op = bio.ReadUInt8();
	if (op == 0x00) {
		uint8_t boolop = bio.ReadUInt8();
		if (boolop == 0x00) { // AND
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::AND;
			//TRACE(" AND");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxFAIL;
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxFAIL;
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		} else if (boolop == 0x01) { // OR
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::OR;
			//TRACE(" OR");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxFAIL;
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxFAIL;
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		} else if (boolop == 0x02) { // NOT
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::NOT;
			//TRACE(" NOT");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxFAIL;
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxFAIL;
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		} else{
			AddDebugLogLineM(false, logKadSearch, wxString::Format(wxT("*** Unknown boolean search operator 0x%02x (CreateSearchExpressionTree)"), boolop));
			return NULL;
		}
	} else if (op == 0x01) { // String
		wxString str(bio.ReadString(true));
		
		// Make lowercase, the search code expects lower case strings!
		str.MakeLower(); 
		
		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::String;
		pSearchTerm->astr = new wxArrayString;

		// pre-tokenize the string term
		//#warning TODO: TokenizeOptQuotedSearchTerm
		wxStringTokenizer token(str, CSearchManager::GetInvalidKeywordChars(),wxTOKEN_DEFAULT );
		while (token.HasMoreTokens()) {
			wxString strTok(token.GetNextToken());
			if (!strTok.IsEmpty()) {
				pSearchTerm->astr->Add(strTok);
			}
		}

		return pSearchTerm;
	} else if (op == 0x02) { // Meta tag
		// read tag value
		wxString strValue(bio.ReadString(true));
		// Make lowercase, the search code expects lower case strings!
		strValue.MakeLower();

		// read tag name
		wxString strTagName =  bio.ReadString(false);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::MetaTag;
		pSearchTerm->tag = new CTagString(strTagName, strValue);
		return pSearchTerm;
	}
	else if (op == 0x03 || op == 0x08) { // Numeric relation (0x03=32-bit or 0x08=64-bit)
		static const struct {
			SSearchTerm::ESearchTermType eSearchTermOp;
			wxString pszOp;
		} _aOps[] =
		{
			{ SSearchTerm::OpEqual,		wxT("=")	}, // mmop=0x00
			{ SSearchTerm::OpGreater,	wxT(">")	}, // mmop=0x01
			{ SSearchTerm::OpLess,		wxT("<")	}, // mmop=0x02
			{ SSearchTerm::OpGreaterEqual,	wxT(">=")	}, // mmop=0x03
			{ SSearchTerm::OpLessEqual,	wxT("<=")	}, // mmop=0x04
			{ SSearchTerm::OpNotEqual,	wxT("<>")	}  // mmop=0x05
		};

		// read tag value
		uint64_t ullValue = (op == 0x03) ? bio.ReadUInt32() : bio.ReadUInt64();

		// read integer operator
		uint8_t mmop = bio.ReadUInt8();
		if (mmop >= itemsof(_aOps)){
			AddDebugLogLineM(false, logKadSearch, wxString::Format(wxT("*** Unknown integer search op=0x%02x (CreateSearchExpressionTree)"), mmop));
			return NULL;
		}

		// read tag name
		wxString strTagName = bio.ReadString(false);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = _aOps[mmop].eSearchTermOp;
		pSearchTerm->tag = new CTagVarInt(strTagName, ullValue);

		return pSearchTerm;
	} else {
		AddDebugLogLineM(false, logKadSearch, wxString::Format(wxT("*** Unknown search op=0x%02x (CreateSearchExpressionTree)"), op));
		return NULL;
	}
}

// KADEMLIA_SEARCH_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessSearchRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(17);

	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint8_t restrictive = bio.ReadUInt8();

	if (lenPacket == 17) {
		if (restrictive) {
			// Source request
			CKademlia::GetIndexed()->SendValidSourceResult(target, ip, port, false, 0, 0, 0);
		} else {
			// Single keyword request
			CKademlia::GetIndexed()->SendValidKeywordResult(target, NULL, ip, port, true, false, 0, 0);
		}
	} else if (lenPacket > 17) {
		SSearchTerm* pSearchTerms = NULL;
		bool oldClient = true;
		if (restrictive) {
			pSearchTerms = CreateSearchExpressionTree(bio, 0);
			if (pSearchTerms == NULL) {
				throw wxString(wxT("Invalid search expression"));
			}
			if (restrictive > 1) {
				oldClient = false;
			}
		} else {
			oldClient = false;
		}

		// Keyword request with added options.
		CKademlia::GetIndexed()->SendValidKeywordResult(target, pSearchTerms, ip, port, oldClient, false, 0, 0); 
		Free(pSearchTerms);
	}
}

// KADEMLIA2_SEARCH_KEY_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchKeyRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint16_t startPosition = bio.ReadUInt16();
	bool restrictive = ((startPosition & 0x8000) == 0x8000);
	startPosition &= 0x7FFF;
	SSearchTerm* pSearchTerms = NULL;
	if (restrictive) {
		pSearchTerms = CreateSearchExpressionTree(bio, 0);
		if (pSearchTerms == NULL) {
			throw wxString(wxT("Invalid search expression"));
		}
	}
	CKademlia::GetIndexed()->SendValidKeywordResult(target, pSearchTerms, ip, port, false, true, startPosition, senderKey);
	if (pSearchTerms) {
		Free(pSearchTerms);
	}
}

// KADEMLIA2_SEARCH_SOURCE_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchSourceRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint16_t startPosition = (bio.ReadUInt16() & 0x7FFF);
	uint64_t fileSize = bio.ReadUInt64();
	CKademlia::GetIndexed()->SendValidSourceResult(target, ip, port, true, startPosition, fileSize, senderKey);
}

// KADEMLIA_SEARCH_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessSearchResponse(const uint8_t *packetData, uint32_t lenPacket)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(37);

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();

	// How many results.. Not supported yet..
	uint16_t count = bio.ReadUInt16();
	while (count > 0) {
		// What is the answer
		CUInt128 answer = bio.ReadUInt128();

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagPtrList* tags = new TagPtrList;
		try {
			bio.ReadTagPtrList(tags, true/*bOptACP*/);
		} catch(...) {
			//DebugClientOutput(wxT("CKademliaUDPListener::processSearchResponse"),ip,port,packetData,lenPacket);
			deleteTagPtrListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::ProcessResult(target, answer, tags);
		count--;
	}
}

// KADEMLIA2_SEARCH_RES
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchResponse(const uint8_t *packetData, uint32_t lenPacket, const CKadUDPKey& WXUNUSED(senderKey))
{
	CMemFile bio(packetData, lenPacket);

	// Who sent this packet.
	CUInt128 source = bio.ReadUInt128();

	// What search does this relate to
	CUInt128 target = bio.ReadUInt128();

	// Total results.
	uint16_t count = bio.ReadUInt16();
	while (count > 0) {
		// What is the answer
		CUInt128 answer = bio.ReadUInt128();

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagPtrList* tags = new TagPtrList;
		try {
			bio.ReadTagPtrList(tags, true);
		} catch(...) {
			deleteTagPtrListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::ProcessResult(target, answer, tags);
		count--;
	}
}

// KADEMLIA_PUBLISH_REQ
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessPublishRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	//There are different types of publishing..
	//Keyword and File are Stored..
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(37);

	//Used Pointers
	CIndexed *indexed = CKademlia::GetIndexed();

	// check if we are UDP firewalled
	if (CUDPFirewallTester::IsFirewalledUDP(true)) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(file);

	if (thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	wxString strInfo;
	uint16_t count = bio.ReadUInt16();
	bool flag = false;
	uint8_t load = 0;
	while (count > 0) {
		strInfo.Clear();

		CUInt128 target = bio.ReadUInt128();

		Kademlia::CKeyEntry* entry = new Kademlia::CKeyEntry();
		try {
			entry->m_uIP = ip;
			entry->m_uUDPport = port;
			entry->m_uKeyID.SetValue(file);
			entry->m_uSourceID.SetValue(target);
			uint32_t tags = bio.ReadUInt8();
			while (tags > 0) {
				CTag* tag = bio.ReadTag();
				if (tag) {
					if (!tag->GetName().Cmp(TAG_SOURCETYPE)) {
						if (entry->m_bSource == false) {
							entry->AddTag(new CTagVarInt(TAG_SOURCEIP, entry->m_uIP));
							entry->AddTag(new CTagVarInt(TAG_SOURCEUPORT, entry->m_uUDPport));
							entry->AddTag(tag);
							entry->m_bSource = true;
						} else {
							//More than one sourcetype tag found.
							delete tag;
						}
					} else if (!tag->GetName().Cmp(TAG_FILENAME)) {
						if (entry->GetCommonFileName().IsEmpty()) {
							entry->SetFileName(tag->GetStr());
							strInfo += CFormat(wxT("  Name=\"%s\"")) % tag->GetStr();
						}
						delete tag;
					} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
						if (entry->m_uSize == 0) {
							if (tag->IsBsob() && (tag->GetBsobSize() == 8)) {
								// Kad1.0 uint64 type using a BSOB.
								entry->m_uSize = PeekUInt64(tag->GetBsob());
							} else {
								wxASSERT(tag->IsInt());
								entry->m_uSize = tag->GetInt();	
							}
							strInfo += wxString::Format(wxT("  Size=%") wxLongLongFmtSpec wxT("u"), entry->m_uSize);
						}
						delete tag;
					} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
						if (entry->m_uTCPport == 0) {
							entry->m_uTCPport = tag->GetInt();
							entry->AddTag(tag);
						} else {
							//More than one port tag found
							delete tag;
						}
					} else {
						//TODO: Filter tags
						entry->AddTag(tag);
					}
				}
				tags--;
			}
			if (!strInfo.IsEmpty()) {
				AddDebugLogLineM(false, logClientKadUDP, strInfo);
			}
		} catch(...) {
			//printf("Error on count %i tag %i\n",totalcount-count, totaltags-tags);
			//DebugClientOutput(wxT("CKademliaUDPListener::processPublishRequest"),ip,port,packetData,lenPacket);
			delete entry;
			throw;
		}

		if (entry->m_bSource == true) {
			entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMES;
			CEntry *sourceEntry = entry->Copy(); // "downcast" since we didnt knew before if this was a keyword, Kad1 support gets removed soon anyway no need for a beautiful solution here
			delete entry;
			entry = NULL;
			if (indexed->AddSources(file, target, sourceEntry, load)) {
				flag = true;
			} else {
				delete sourceEntry;
				sourceEntry = NULL;
			}
		} else {
			entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMEK;
			if (indexed->AddKeyword(file, target, entry, load)) {
				// This makes sure we send a publish response.. 
				// This also makes sure we index all the files for this keyword.
				flag = true;
			} else {
				//We already indexed the maximum number of keywords.
				//We do not index anymore but we still send a success..
				//Reason: Because if a VERY busy node tells the publisher it failed,
				//this busy node will spread to all the surrounding nodes causing popular
				//keywords to be stored on MANY nodes..
				//So, once we are full, we will periodically clean our list until we can
				//begin storing again..
				flag = true;
				delete entry;
				entry = NULL;
			}
		}
		count--;
	}
	if (flag) {
		CMemFile packetdata(17);
		packetdata.WriteUInt128(file);
		packetdata.WriteUInt8(load);

		DebugSend(KadPublishRes, ip, port);
		SendPacket(packetdata, KADEMLIA_PUBLISH_RES, ip, port, 0, NULL);
	}
}

// KADEMLIA2_PUBLISH_KEY_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2PublishKeyRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	//Used Pointers
	CIndexed *indexed = CKademlia::GetIndexed();

	// check if we are UDP firewalled
	if (CUDPFirewallTester::IsFirewalledUDP(true)) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(file);

	// Shouldn't LAN IPs already be filtered?
	if (thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	wxString strInfo;
	uint16_t count = bio.ReadUInt16();
	uint8_t load = 0;
	while (count > 0) {
		strInfo.Clear();

		CUInt128 target = bio.ReadUInt128();

		Kademlia::CKeyEntry* entry = new Kademlia::CKeyEntry();
		try
		{
			entry->m_uIP = ip;
			entry->m_uUDPport = port;
			entry->m_uKeyID.SetValue(file);
			entry->m_uSourceID.SetValue(target);
			entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMEK;
			entry->m_bSource = false;
			uint32_t tags = bio.ReadUInt8();
			while (tags > 0) {
				CTag* tag = bio.ReadTag();
				if (tag) {
					if (!tag->GetName().Cmp(TAG_FILENAME)) {
						if (entry->GetCommonFileName().IsEmpty()) {
							entry->SetFileName(tag->GetStr());
							strInfo += CFormat(wxT("  Name=\"%s\"")) % entry->GetCommonFileName();
						}
						delete tag; // tag is no longer stored, but membervar is used
					} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
						if (entry->m_uSize == 0) {
							if (tag->IsBsob() && tag->GetBsobSize() == 8) {
								entry->m_uSize = PeekUInt64(tag->GetBsob());
							} else {
								entry->m_uSize = tag->GetInt();
							}
							strInfo += wxString::Format(wxT("  Size=%") wxLongLongFmtSpec wxT("u"), entry->m_uSize);
						}
						delete tag; // tag is no longer stored, but membervar is used
					} else {
						//TODO: Filter tags
						entry->AddTag(tag);
					}
				}
				tags--;
			}
			if (!strInfo.IsEmpty()) {
				AddDebugLogLineM(false, logClientKadUDP, strInfo);
			}
		} catch(...) {
			//DebugClientOutput(wxT("CKademliaUDPListener::Process2PublishKeyRequest"),ip,port,packetData,lenPacket);
			delete entry;
			throw;
		}

		if (!indexed->AddKeyword(file, target, entry, load)) {
			//We already indexed the maximum number of keywords.
			//We do not index anymore but we still send a success..
			//Reason: Because if a VERY busy node tells the publisher it failed,
			//this busy node will spread to all the surrounding nodes causing popular
			//keywords to be stored on MANY nodes..
			//So, once we are full, we will periodically clean our list until we can
			//begin storing again..
			delete entry;
			entry = NULL;
		}
		count--;
	}
	CMemFile packetdata(17);
	packetdata.WriteUInt128(file);
	packetdata.WriteUInt8(load);
	DebugSend(Kad2PublishRes, ip, port);
	SendPacket(packetdata, KADEMLIA2_PUBLISH_RES, ip, port, senderKey, NULL);
}

// KADEMLIA2_PUBLISH_SOURCE_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2PublishSourceRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	//Used Pointers
	CIndexed *indexed = CKademlia::GetIndexed();

	// check if we are UDP firewalled
	if (CUDPFirewallTester::IsFirewalledUDP(true)) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(file);

	if (thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	wxString strInfo;
	uint8_t load = 0;
	bool flag = false;
	CUInt128 target = bio.ReadUInt128();
	Kademlia::CEntry* entry = new Kademlia::CEntry();
	try {
		entry->m_uIP = ip;
		entry->m_uUDPport = port;
		entry->m_uKeyID.SetValue(file);
		entry->m_uSourceID.SetValue(target);
		entry->m_bSource = false;
		entry->m_tLifeTime = (uint32_t)time(NULL) + KADEMLIAREPUBLISHTIMES;
		bool addUDPPortTag = true;
		uint32_t tags = bio.ReadUInt8();
		while (tags > 0) {
			CTag* tag = bio.ReadTag();
			if (tag) {
				if (!tag->GetName().Cmp(TAG_SOURCETYPE)) {
					if (entry->m_bSource == false) {
						entry->AddTag(new CTagVarInt(TAG_SOURCEIP, entry->m_uIP));
						entry->AddTag(tag);
						entry->m_bSource = true;
					} else {
						//More than one sourcetype tag found.
						delete tag;
					}
				} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
					if (entry->m_uSize == 0) {
						if (tag->IsBsob() && tag->GetBsobSize() == 8) {
							entry->m_uSize = PeekUInt64(tag->GetBsob());
						} else {
							entry->m_uSize = tag->GetInt();
						}
						strInfo += wxString::Format(wxT("  Size=%") wxLongLongFmtSpec wxT("u"), entry->m_uSize);
					}
					delete tag;
				} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
					if (entry->m_uTCPport == 0) {
						entry->m_uTCPport = (uint16_t)tag->GetInt();
						entry->AddTag(tag);
					} else {
						//More than one port tag found
						delete tag;
					}
				} else if (!tag->GetName().Cmp(TAG_SOURCEUPORT)) {
					if (addUDPPortTag && tag->IsInt() && tag->GetInt() != 0) {
						entry->m_uUDPport = (uint16_t)tag->GetInt();
						entry->AddTag(tag);
						addUDPPortTag = false;
					} else {
						//More than one udp port tag found
						delete tag;
					}
				} else {
					//TODO: Filter tags
					entry->AddTag(tag);
				}
			}
			tags--;
		}
		if (addUDPPortTag) {
			entry->AddTag(new CTagVarInt(TAG_SOURCEUPORT, entry->m_uUDPport));
		}
		if (!strInfo.IsEmpty()) {
			AddDebugLogLineM(false, logClientKadUDP, strInfo);
		}
	} catch(...) {
		//DebugClientOutput(wxT("CKademliaUDPListener::Process2PublishSourceRequest"),ip,port,packetData,lenPacket);
		delete entry;
		throw;
	}

	if (entry->m_bSource == true) {
		if (indexed->AddSources(file, target, entry, load)) {
			flag = true;
		} else {
			delete entry;
			entry = NULL;
		}
	} else {
		delete entry;
		entry = NULL;
	}
	if (flag) {
		CMemFile packetdata(17);
		packetdata.WriteUInt128(file);
		packetdata.WriteUInt8(load);
		DebugSend(Kad2PublishRes, ip, port);
		SendPacket(packetdata, KADEMLIA2_PUBLISH_RES, ip, port, senderKey, NULL);
	}
}

// KADEMLIA_PUBLISH_RES
// Used only by Kad1.0
void CKademliaUDPListener::ProcessPublishResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(16);
	CHECK_TRACKED_PACKET(KADEMLIA_PUBLISH_REQ);

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	bool loadResponse = false;
	uint8_t load = 0;
	if (bio.GetLength() > bio.GetPosition()) {
		loadResponse = true;
		load = bio.ReadUInt8();
	}

	CSearchManager::ProcessPublishResult(file, load, loadResponse);
}

// KADEMLIA2_PUBLISH_RES
// Used only by Kad2.0
void CKademliaUDPListener::Process2PublishResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	if (!IsOnOutTrackList(ip, KADEMLIA2_PUBLISH_KEY_REQ) && !IsOnOutTrackList(ip, KADEMLIA2_PUBLISH_SOURCE_REQ) && !IsOnOutTrackList(ip, KADEMLIA2_PUBLISH_NOTES_REQ)) {
		throw wxString::Format(wxT("***NOTE: Received unrequested response packet, size (%u) in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}
	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();
	uint8_t load = bio.ReadUInt8();
	CSearchManager::ProcessPublishResult(file, load, true);
	if (bio.GetLength() > bio.GetPosition()) {
		// for future use
		uint8_t options = bio.ReadUInt8();
		bool requestACK = (options & 0x01) > 0;
		if (requestACK && !senderKey.IsEmpty()) {
			DebugSend(Kad2PublishResAck, ip, port);
			SendNullPacket(KADEMLIA2_PUBLISH_RES_ACK, ip, port, senderKey, NULL);
		}
	}
}

// KADEMLIA_SEARCH_NOTES_REQ
// Used only by Kad1.0
void CKademliaUDPListener::ProcessSearchNotesRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(32);

	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	// This info is currently not used.
	// CUInt128 source = bio.ReadUInt128();

	CKademlia::GetIndexed()->SendValidNoteResult(target, ip, port, false, 0, 0);
}

// KADEMLIA2_SEARCH_NOTES_REQ
// Used only by Kad2.0
void CKademliaUDPListener::Process2SearchNotesRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint64_t fileSize = bio.ReadUInt64();
	CKademlia::GetIndexed()->SendValidNoteResult(target, ip, port, true, fileSize, senderKey);
}

// KADEMLIA_SEARCH_NOTES_RES
// Used only by Kad1.0
void CKademliaUDPListener::ProcessSearchNotesResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(37);
	CHECK_TRACKED_PACKET(KADEMLIA_SEARCH_NOTES_REQ);

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();

	uint16_t count = bio.ReadUInt16();
	while (count > 0) {
		// What is the answer
		CUInt128 answer = bio.ReadUInt128();

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagPtrList* tags = new TagPtrList;
		try {
			bio.ReadTagPtrList(tags, true/*bOptACP*/);
		} catch(...){
			//DebugClientOutput(wxT("CKademliaUDPListener::processSearchNotesResponse"),ip,port,packetData,lenPacket);
			deleteTagPtrListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::ProcessResult(target, answer, tags);
		count--;
	}
}

// KADEMLIA_PUBLISH_NOTES_REQ
// Used only by Kad1.0
void CKademliaUDPListener::ProcessPublishNotesRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(37);

	// check if we are UDP firewalled
	if (CUDPFirewallTester::IsFirewalledUDP(true)) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();

	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(target);

	if( thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	CUInt128 source = bio.ReadUInt128();

	Kademlia::CEntry* entry = new Kademlia::CEntry();
	try {
		entry->m_uIP = ip;
		entry->m_uUDPport = port;
		entry->m_uKeyID.SetValue(target);
		entry->m_uSourceID.SetValue(source);
		uint32_t tagCount = bio.ReadUInt8();
		while (tagCount > 0) {
			CTag *tag = bio.ReadTag();
			if (tag) {
				if (!tag->GetName().Cmp(TAG_FILENAME)) {
					if (entry->GetCommonFileName().IsEmpty()) {
						entry->SetFileName(tag->GetStr());
					}
					delete tag;
				} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
					if (entry->m_uSize == 0) {
						entry->m_uSize = tag->GetInt();
					}
					delete tag;
				} else {
					//TODO: Filter tags
					entry->AddTag(tag);
				}
			}
			tagCount--;
		}
		entry->m_bSource = false;
	} catch(...) {
		//DebugClientOutput(wxT("CKademliaUDPListener::processPublishNotesRequest"),ip,port,packetData,lenPacket);
		delete entry;
		entry = NULL;
		throw;
	}

	if (entry == NULL) {
		throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry == NULL"));
	} else if (entry->GetTagCount() == 0 || entry->GetTagCount() > 5) {
		delete entry;
		throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry->GetTagCount() == 0 || entry->GetTagCount() > 5"));
	}

	uint8_t load = 0;
	if (CKademlia::GetIndexed()->AddNotes(target, source, entry, load)) {
		CMemFile packetdata(17);
		packetdata.WriteUInt128(target);
		packetdata.WriteUInt8(load);
		DebugSend(KadPublishNotesRes, ip, port);
		SendPacket(packetdata, KADEMLIA_PUBLISH_NOTES_RES, ip, port, 0, NULL);
	} else {
		delete entry;
	}
}

// KADEMLIA2_PUBLISH_NOTES_REQ
// Used only by Kad2.0
void CKademliaUDPListener::Process2PublishNotesRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// check if we are UDP firewalled
	if (CUDPFirewallTester::IsFirewalledUDP(true)) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();

	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(target);

	// Shouldn't LAN IPs already be filtered?
	if (thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	CUInt128 source = bio.ReadUInt128();

	Kademlia::CEntry* entry = new Kademlia::CEntry();
	try {
		entry->m_uIP = ip;
		entry->m_uUDPport = port;
		entry->m_uKeyID.SetValue(target);
		entry->m_uSourceID.SetValue(source);
		entry->m_bSource = false;
		uint32_t tags = bio.ReadUInt8();
		while (tags > 0) {
			CTag* tag = bio.ReadTag();
			if(tag) {
				if (!tag->GetName().Cmp(TAG_FILENAME)) {
					if (entry->GetCommonFileName().IsEmpty()) {
						entry->SetFileName(tag->GetStr());
					}
					delete tag;
				} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
					if (entry->m_uSize == 0) {
						entry->m_uSize = tag->GetInt();
					}
					delete tag;
				} else {
					//TODO: Filter tags
					entry->AddTag(tag);
				}
			}
			tags--;
		}
	} catch(...) {
		//DebugClientOutput(wxT("CKademliaUDPListener::Process2PublishNotesRequest"),ip,port,packetData,lenPacket);
		delete entry;
		entry = NULL;
		throw;
	}

	uint8_t load = 0;
	if (CKademlia::GetIndexed()->AddNotes(target, source, entry, load)) {
		CMemFile packetdata(17);
		packetdata.WriteUInt128(target);
		packetdata.WriteUInt8(load);
		DebugSend(Kad2PublishRes, ip, port);
		SendPacket(packetdata, KADEMLIA2_PUBLISH_RES, ip, port, senderKey, NULL);
	} else {
		delete entry;
	}
}

// KADEMLIA_PUBLISH_NOTES_RES
// Used only by Kad1.0
void CKademliaUDPListener::ProcessPublishNotesResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(16);
	CHECK_TRACKED_PACKET(KADEMLIA_PUBLISH_NOTES_REQ);

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	bool loadResponse = false;
	uint8_t load = 0;
	if( bio.GetLength() > bio.GetPosition() ) {
		loadResponse = true;
		load = bio.ReadUInt8();
	}

	CSearchManager::ProcessPublishResult(file, load, loadResponse);
}

// KADEMLIA_FIREWALLED_REQ
// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::ProcessFirewalledRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(2);

	CMemFile bio(packetData, lenPacket);
	uint16_t tcpport = bio.ReadUInt16();

	CUInt128 zero;
	CContact contact(zero, ip, port, tcpport, 0, 0, false, zero);
	if (!theApp->clientlist->RequestTCP(&contact, 0)) {
		return; // cancelled for some reason, don't send a response
	}

	// Send response
	CMemFile packetdata(4);
	packetdata.WriteUInt32(ip);
	DebugSend(KadFirewalledRes, ip, port);
	SendPacket(packetdata, KADEMLIA_FIREWALLED_RES, ip, port, senderKey, NULL);
}

// KADEMLIA_FIREWALLED2_REQ
// Used by Kad2.0 Prot.Version 7+
void CKademliaUDPListener::ProcessFirewalled2Request(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(19);

	CMemFile bio(packetData, lenPacket);
	uint16_t tcpPort = bio.ReadUInt16();
	CUInt128 userID = bio.ReadUInt128();
	uint8_t connectOptions = bio.ReadUInt8();

	CUInt128 zero;
	CContact contact(userID, ip, port, tcpPort, 0, 0, false, zero);
	if (!theApp->clientlist->RequestTCP(&contact, connectOptions)) {
		return; // cancelled for some reason, don't send a response
	}

	// Send response
	CMemFile packetdata(4);
	packetdata.WriteUInt32(ip);
	DebugSend(KadFirewalledRes, ip, port);
	SendPacket(packetdata, KADEMLIA_FIREWALLED_RES, ip, port, senderKey, NULL);
}

// KADEMLIA_FIREWALLED_RES
// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::ProcessFirewalledResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, const CKadUDPKey& WXUNUSED(senderKey))
{
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(4);

	if (!theApp->clientlist->IsKadFirewallCheckIP(wxUINT32_SWAP_ALWAYS(ip))) { /* KADEMLIA_FIREWALLED2_REQ + KADEMLIA_FIREWALLED_REQ */
		throw wxString(wxT("Received unrequested firewall response packet in ")) + wxString::FromAscii(__FUNCTION__);
	}

	CMemFile bio(packetData, lenPacket);
	uint32_t firewalledIP = bio.ReadUInt32();

	// Update con state only if something changes.
	if (CKademlia::GetPrefs()->GetIPAddress() != firewalledIP) {
		CKademlia::GetPrefs()->SetIPAddress(firewalledIP);
		theApp->ShowConnectionState();
	}
	CKademlia::GetPrefs()->IncRecheckIP();
}

// KADEMLIA_FIREWALLED_ACK_RES
// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::ProcessFirewalledAckResponse(uint32_t lenPacket)
{
	// Deprecated since KadVersion 7+, the result is now sent per TCP instead of UDP, because this will fail if our intern UDP port is unreachable.
	// But we want the TCP testresult regardless if UDP is firewalled, the new UDP state and test takes care of the rest.

	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(0);

	CKademlia::GetPrefs()->IncFirewalled();
}

// KADEMLIA_FINDBUDDY_REQ
// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::ProcessFindBuddyRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(34);

	if (CKademlia::GetPrefs()->GetFirewalled() || CUDPFirewallTester::IsFirewalledUDP(true) || !CUDPFirewallTester::IsVerified()) {
		// We are firewalled but somehow we still got this packet.. Don't send a response..
		return;
	} else if (theApp->clientlist->GetBuddyStatus() == Connected) {
		// we already have a buddy
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 BuddyID = bio.ReadUInt128();
	CUInt128 userID = bio.ReadUInt128();
	uint16_t tcpport = bio.ReadUInt16();

	CUInt128 zero;
	CContact contact(userID, ip, port, tcpport, 0, 0, false, zero);
	if (!theApp->clientlist->IncomingBuddy(&contact, &BuddyID)) {
		return; // cancelled for some reason, don't send a response
	}

	CMemFile packetdata(34);
	packetdata.WriteUInt128(BuddyID);
	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetClientHash());
	packetdata.WriteUInt16(thePrefs::GetPort());
	if (!senderKey.IsEmpty()) { // remove check for later versions
		packetdata.WriteUInt8(CPrefs::GetMyConnectOptions(true, false)); // new since 0.49a, old mules will ignore it  (hopefully ;) )
	}

	DebugSend(KadFindBuddyRes, ip, port);
	SendPacket(packetdata, KADEMLIA_FINDBUDDY_RES, ip, port, senderKey, NULL);
}

// KADEMLIA_FINDBUDDY_RES
// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::ProcessFindBuddyResponse(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& WXUNUSED(senderKey))
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(34);
	CHECK_TRACKED_PACKET(KADEMLIA_FINDBUDDY_REQ);

	CMemFile bio(packetData, lenPacket);
	CUInt128 check = bio.ReadUInt128();
	check.XOR(CUInt128(true));
	if (CKademlia::GetPrefs()->GetKadID() == check) {
		CUInt128 userID = bio.ReadUInt128();
		uint16_t tcpport = bio.ReadUInt16();
		uint8_t connectOptions = 0;
		if (lenPacket > 34) {
			// 0.49+ (kad version 7) sends additionally its connect options so we know whether to use an obfuscated connection
			connectOptions = bio.ReadUInt8();
		}

		CUInt128 zero;
		CContact contact(userID, ip, port, tcpport, 0, 0, false, zero);
		theApp->clientlist->RequestBuddy(&contact, connectOptions);
	}
}

// KADEMLIA_CALLBACK_REQ
// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::ProcessCallbackRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& WXUNUSED(senderKey))
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(34);

	CUpDownClient* buddy = theApp->clientlist->GetBuddy();
	if (buddy != NULL) {
		CMemFile bio(packetData, lenPacket);
		CUInt128 check = bio.ReadUInt128();
		// JOHNTODO: Begin filtering bad buddy ID's..
		// CUInt128 bud(buddy->GetBuddyID());
		CUInt128 file = bio.ReadUInt128();
		uint16_t tcp = bio.ReadUInt16();

		if (buddy->GetSocket()) {
			CMemFile packetdata(lenPacket + 6);
			packetdata.WriteUInt128(check);
			packetdata.WriteUInt128(file);
			packetdata.WriteUInt32(ip);
			packetdata.WriteUInt16(tcp);
			CPacket* packet = new CPacket(packetdata, OP_EMULEPROT, OP_CALLBACK);
			DebugSend(KadCallbackReq, ip, port);
			buddy->GetSocket()->SendPacket(packet);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		} else {
			throw wxString::FromAscii(__FUNCTION__) + wxT(": Buddy has no valid socket");
		}
	}
}

// KADEMLIA2_PING
void CKademliaUDPListener::Process2Ping(uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// can be used just as PING, currently it is however only used to determine one's external port
	CMemFile packetdata(2);
	packetdata.WriteUInt16(port);
	DebugSend(Kad2Pong, ip, port);
	SendPacket(packetdata, KADEMLIA2_PONG, ip, port, senderKey, NULL); 
}

// KADEMLIA2_PONG
void CKademliaUDPListener::Process2Pong(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip)
{
	CHECK_PACKET_MIN_SIZE(2);
	CHECK_TRACKED_PACKET(KADEMLIA2_PING);

	// Is this one of our legacy challenge packets?
	CUInt128 contactID;
	if (IsLegacyChallenge(CUInt128((uint32_t)0), ip, KADEMLIA2_PING, contactID)) {
		// yup it is, set the contact as verified
		if (!CKademlia::GetRoutingZone()->VerifyContact(contactID, ip)) {
			AddDebugLogLineM(false, logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"));
		}
#ifdef __DEBUG__
		else {
			AddDebugLogLineM(false, logClientKadUDP, wxT("Verified contact with legacy challenge (Kad2Ping) - ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
		}
#endif
		return;	// we do not actually care for its other content
	}

	if (CKademlia::GetPrefs()->FindExternKadPort(false)) {
		// the reported port doesn't always have to be our true external port, esp. if we used our intern port
		// and communicated recently with the client some routers might remember this and assign the intern port as source
		// but this shouldn't be a problem because we prefer intern ports anyway.
		// might have to be reviewed in later versions when more data is available
		CKademlia::GetPrefs()->SetExternKadPort(PeekUInt16(packetData), ip);

		if (CUDPFirewallTester::IsFWCheckUDPRunning()) {
			CUDPFirewallTester::QueryNextClient();
		}
	}
	theApp->ShowConnectionState();
}

// KADEMLIA2_FIREWALLUDP
void CKademliaUDPListener::Process2FirewallUDP(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(3);

	uint8_t errorCode = PeekUInt8(packetData);
	uint16_t incomingPort = PeekUInt16(packetData + 1);
	if (incomingPort != CKademlia::GetPrefs()->GetExternalKadPort() && incomingPort != CKademlia::GetPrefs()->GetInternKadPort() || incomingPort == 0) {
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("Received UDP FirewallCheck on unexpected incoming port %u (") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(")"), incomingPort));
		CUDPFirewallTester::SetUDPFWCheckResult(false, true, ip, 0);
	} else if (errorCode == 0) {
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("Received UDP FirewallCheck packet from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(" with incoming port %u"), incomingPort));
		CUDPFirewallTester::SetUDPFWCheckResult(true, false, ip, incomingPort);
	} else {
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("Received UDP FirewallCheck packet from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(" with incoming port %u with remote errorcode %u - ignoring result"), incomingPort, errorCode));
		CUDPFirewallTester::SetUDPFWCheckResult(false, true, ip, 0);
	}
}

void CKademliaUDPListener::SendPacket(const CMemFile &data, uint8_t opcode, uint32_t destinationHost, uint16_t destinationPort, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID)
{
	AddTrackedOutPacket(destinationHost, opcode);
	CPacket* packet = new CPacket(data, OP_KADEMLIAHEADER, opcode);
	if (packet->GetPacketSize() > 200) {
		packet->PackPacket();
	}
	uint8_t cryptData[16];
	uint8_t *cryptKey;
	if (cryptTargetID != NULL) {
		cryptKey = (uint8_t *)&cryptData;
		cryptTargetID->StoreCryptValue(cryptKey);
	} else {
		cryptKey = NULL;
	}
	theStats::AddUpOverheadKad(packet->GetPacketSize());
	theApp->clientudp->SendPacket(packet, wxUINT32_SWAP_ALWAYS(destinationHost), destinationPort, true, cryptKey, true, targetKey.GetKeyValue(theApp->GetPublicIP(false)));
}

bool CKademliaUDPListener::FindNodeIDByIP(CKadClientSearcher* requester, uint32_t ip, uint16_t tcpPort, uint16_t udpPort)
{
	// send a hello packet to the given IP in order to get a HELLO_RES with the NodeID

	// we will drop support for Kad1 soon, so don't bother sending two packets in case we don't know if kad2 is supported
	// (if we know that it's not, this function isn't called in the first place)
	AddDebugLogLineM(false, logClientKadUDP, wxT("FindNodeIDByIP: Requesting NodeID from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) + wxT(" by sending Kad2HelloReq"));
	DebugSend(Kad2HelloReq, ip, udpPort);
	SendMyDetails(KADEMLIA2_HELLO_REQ, ip, udpPort, 1, 0, NULL, false); // todo: we send this unobfuscated, which is not perfect, see this can be avoided in the future
	FetchNodeID_Struct sRequest = { ip, tcpPort, ::GetTickCount() + SEC2MS(60), requester };
	m_fetchNodeIDRequests.push_back(sRequest);
	return true;
}

void CKademliaUDPListener::ExpireClientSearch(CKadClientSearcher* expireImmediately)
{
	uint32_t now = ::GetTickCount();
	for (FetchNodeIDList::iterator it = m_fetchNodeIDRequests.begin(); it != m_fetchNodeIDRequests.end();) {
		FetchNodeIDList::iterator it2 = it++;
		if (it2->requester == expireImmediately) {
			m_fetchNodeIDRequests.erase(it2);
		}
		else if (it2->expire < now) {
			it2->requester->KadSearchNodeIDByIPResult(KCSR_TIMEOUT, NULL);
			m_fetchNodeIDRequests.erase(it2);
		}
	}
}

void CKademliaUDPListener::SendLegacyChallenge(uint32_t ip, uint16_t port, const CUInt128& contactID, bool kad2)
{
	// We want to verify that a pre-0.49a contact is valid and not sent from a spoofed IP.
	// Because those versions don't support any direct validating, we send a KAD_REQ with a random ID,
	// which is our challenge. If we receive an answer packet for this request, we can be sure the
	// contact is not spoofed
#ifdef __DEBUG__
	CContact* contact = CKademlia::GetRoutingZone()->GetContact(contactID);
	if (contact != NULL) {
		if (contact->GetType() < 2) {
			AddDebugLogLineM(false, logClientKadUDP, wxT("Sending challenge to a long known contact (should be verified already) - ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)));
		}
	} else {
		wxFAIL;
	}
#endif

	if (HasActiveLegacyChallenge(ip)) {
		// don't send more than one challenge at a time
		return;
	}
	CMemFile packetdata(33);
	packetdata.WriteUInt8(KADEMLIA_FIND_VALUE);
	CUInt128 challenge;
	challenge.SetValueRandom();
	if (challenge == 0) {
		// hey there is a 2^128 chance that this happens ;)
		wxFAIL;
		challenge = 1;
	}
	// Put the target we want into the packet. This is our challenge
	packetdata.WriteUInt128(challenge);
	// Add the ID of the contact we are contacting for sanity checks on the other end.
	packetdata.WriteUInt128(contactID);
	if (kad2) {
		DebugSendF(wxT("Kad2Req(SendLegacyChallenge)"), ip, port);
		// those versions we send those requests to don't support encryption / obfuscation
		SendPacket(packetdata, KADEMLIA2_REQ, ip, port, 0, NULL);
		AddLegacyChallenge(contactID, challenge, ip, KADEMLIA2_REQ);
	} else {
		DebugSendF(wxT("KadReq(SendLegacyChallenge)"), ip, port);
		SendPacket(packetdata, KADEMLIA_REQ, ip, port, 0, NULL);
		AddLegacyChallenge(contactID, challenge, ip, KADEMLIA_REQ);
	}
}

void CKademliaUDPListener::DebugClientOutput(const wxString& place, uint32 kad_ip, uint32 port, const byte* data, int len)
{
#if THIS_DEBUG_IS_JUST_FOR_KRY_DONT_TOUCH_IT_KTHX
	uint32 ip = wxUINT32_SWAP_ALWAYS(kad_ip);
	printf("Error on %s received from: %s\n",(const char*)unicode2char(place),(const char*)unicode2char(Uint32_16toStringIP_Port(ip,port)));
	if (data) {
		printf("Packet dump:\n");
		DumpMem(data, len);
	}
	CClientList::SourceList clientslist = theApp->clientlist->GetClientsByIP(ip);
	if (!clientslist.empty()) {
		for (CClientList::SourceList::iterator it = clientslist.begin(); it != clientslist.end(); ++it) {
			printf("Ip Matches: %s\n",(const char*)unicode2char((*it)->GetClientFullInfo()));
		}
	} else {
		printf("No ip match, trying to create a client connection:\n");
		printf("Trying port %d\n", port - 10);
		CUpDownClient* client = new CUpDownClient(port-10,kad_ip,0,0,NULL,false,false);
		client->SetConnectionReason(wxT("Error on ") + place);
		client->TryToConnect(true);
	}
#else
	// No need for warnings for the rest of us.
	(void)place;
	(void)kad_ip;
	(void)port;
	(void)data;
	(void)len;
#endif
}
// File_checked_for_headers

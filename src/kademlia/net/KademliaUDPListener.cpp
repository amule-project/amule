//
// This file is part of aMule Project
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne ( http://www.emule-project.net )
// Copyright (C)2007-2011 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )

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
#include "../../RandomFunctions.h"		// Needed for GetRandomUint128()

#include <wx/tokenzr.h>

#define THIS_DEBUG_IS_JUST_FOR_KRY_DONT_TOUCH_IT_KTHX 0

#if defined(__SUNPRO_CC)
#define __FUNCTION__ __FILE__+__LINE__
#endif

#define CHECK_PACKET_SIZE(OP, SIZE) \
	if (lenPacket OP (uint32_t)(SIZE)) \
		throw wxString(CFormat(wxT("***NOTE: Received wrong size (%u) packet in %s")) % lenPacket % wxString::FromAscii(__FUNCTION__))

#define CHECK_PACKET_MIN_SIZE(SIZE)	CHECK_PACKET_SIZE(<, SIZE)
#define CHECK_PACKET_EXACT_SIZE(SIZE)	CHECK_PACKET_SIZE(!=, SIZE)

#define CHECK_TRACKED_PACKET(OPCODE) \
	if (!IsOnOutTrackList(ip, OPCODE)) \
		throw wxString(CFormat(wxT("***NOTE: Received unrequested response packet, size (%u) in %s")) % lenPacket % wxString::FromAscii(__FUNCTION__))


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
void CKademliaUDPListener::Bootstrap(uint32_t ip, uint16_t port, uint8_t kadVersion, const CUInt128* cryptTargetID)
{
	wxASSERT(ip);

	DebugSend(Kad2BootstrapReq, ip, port);
	CMemFile bio(0);
	if (kadVersion >= 6) {
		SendPacket(bio, KADEMLIA2_BOOTSTRAP_REQ, ip, port, 0, cryptTargetID);
	} else {
		SendPacket(bio, KADEMLIA2_BOOTSTRAP_REQ, ip, port, 0, NULL);
	}
}

// Used by Kad1.0 and Kad2.0
void CKademliaUDPListener::SendMyDetails(uint8_t opcode, uint32_t ip, uint16_t port, uint8_t kadVersion, const CKadUDPKey& targetKey, const CUInt128* cryptTargetID, bool requestAckPacket)
{
	CMemFile packetdata;
	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
	
	if (kadVersion > 1) {
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
		if (kadVersion >= 6) {
			if (cryptTargetID == NULL || *cryptTargetID == 0) {
				AddDebugLogLineN(logClientKadUDP, CFormat(wxT("Sending hello response to crypt enabled Kad Node which provided an empty NodeID: %s (%u)")) % KadIPToString(ip) % kadVersion);
				SendPacket(packetdata, opcode, ip, port, targetKey, NULL);
			} else {
				SendPacket(packetdata, opcode, ip, port, targetKey, cryptTargetID);
			}
		} else {
			SendPacket(packetdata, opcode, ip, port, 0, NULL);
			wxASSERT(targetKey.IsEmpty());
		}
	} else {
		wxFAIL;
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
		AddDebugLogLineN(logKadPacketTracking, wxT("Dropping incoming unencrypted packet on port 53 (DNS), IP: ") + KadIPToString(ip));
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
		case KADEMLIA2_BOOTSTRAP_REQ:
			DebugRecv(Kad2BootstrapReq, ip, port);
			Process2BootstrapRequest(ip, port, senderKey);
			break;
		case KADEMLIA2_BOOTSTRAP_RES:
			DebugRecv(Kad2BootstrapRes, ip, port);
			Process2BootstrapResponse(packetData, lenPacket, ip, port, senderKey, validReceiverKey);
			break;
		case KADEMLIA2_HELLO_REQ:
			DebugRecv(Kad2HelloReq, ip, port);
			Process2HelloRequest(packetData, lenPacket, ip, port, senderKey, validReceiverKey);
			break;
		case KADEMLIA2_HELLO_RES:
			DebugRecv(Kad2HelloRes, ip, port);
			Process2HelloResponse(packetData, lenPacket, ip, port, senderKey, validReceiverKey);
			break;
		case KADEMLIA2_HELLO_RES_ACK:
			DebugRecv(Kad2HelloResAck, ip, port);
			Process2HelloResponseAck(packetData, lenPacket, ip, validReceiverKey);
			break;
		case KADEMLIA2_REQ:
			DebugRecv(Kad2Req, ip, port);
			ProcessKademlia2Request(packetData, lenPacket, ip, port, senderKey);
			break;
		case KADEMLIA2_RES:
			DebugRecv(Kad2Res, ip, port);
			ProcessKademlia2Response(packetData, lenPacket, ip, port, senderKey);
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

		// old Kad1 opcodes which we don't handle anymore
		case KADEMLIA_BOOTSTRAP_REQ_DEPRECATED:
		case KADEMLIA_BOOTSTRAP_RES_DEPRECATED:
		case KADEMLIA_HELLO_REQ_DEPRECATED:
		case KADEMLIA_HELLO_RES_DEPRECATED:
		case KADEMLIA_REQ_DEPRECATED:
		case KADEMLIA_RES_DEPRECATED:
		case KADEMLIA_PUBLISH_NOTES_REQ_DEPRECATED:
		case KADEMLIA_PUBLISH_NOTES_RES_DEPRECATED:
		case KADEMLIA_SEARCH_REQ:
		case KADEMLIA_PUBLISH_REQ:
		case KADEMLIA_SEARCH_NOTES_REQ:
			break;

		default: {
			throw wxString(CFormat(wxT("Unknown opcode %02x on CKademliaUDPListener::ProcessPacket()")) % opcode);
		}
	}
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
		throw wxString(CFormat(wxT("***NOTE: Received invalid Kademlia2 version (%u) in %s")) % version % wxString::FromAscii(__FUNCTION__));
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
			//AddDebugLogLineN(logKadMain, wxT("Result Addcontact: ") + id.ToHexString());
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
		return CKademlia::GetRoutingZone()->Add(id, ip, port, tport, version, udpKey, ipVerified, update, true);
	} else {
		AddDebugLogLineN(logKadRouting, wxT("Not adding firewalled client to routing table (") + KadIPToString(ip) + wxT(")"));
		return false;
	}
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
		routingZone->Add(contactID, ip, port, tport, version, senderKey, validReceiverKey, true, false);
	}
	//AddDebugLogLineN(logClientKadUDP, wxT("Inc Kad2 Bootstrap packet from ") + KadIPToString(ip));

	uint16_t numContacts = bio.ReadUInt16();
	while (numContacts) {
		contactID = bio.ReadUInt128();
		ip = bio.ReadUInt32();
		port = bio.ReadUInt16();
		tport = bio.ReadUInt16();
		version = bio.ReadUInt8();
		bool verified = assumeVerified;
		routingZone->Add(contactID, ip, port, tport, version, 0, verified, false, false);
		numContacts--;
	}
}

// KADEMLIA2_HELLO_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey, bool validReceiverKey)
{
	DEBUG_ONLY( uint16_t dbgOldUDPPort = port; )
	uint8_t contactVersion = 0;
	CUInt128 contactID;
	bool addedOrUpdated = AddContact2(packetData, lenPacket, ip, port, &contactVersion, senderKey, validReceiverKey, true, true, NULL, &contactID); // might change (udp)port, validReceiverKey
	wxASSERT(contactVersion >= 2);
#ifdef __DEBUG__
	if (dbgOldUDPPort != port) {
		AddDebugLogLineN(logClientKadUDP, CFormat(wxT("KadContact %s uses his internal (%u) instead external (%u) UDP Port")) % KadIPToString(ip) % port % dbgOldUDPPort);
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
				AddDebugLogLineN(logKadRouting, wxT("Sending (ping) challenge to a long known contact (should be verified already) - ") + KadIPToString(ip));
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
		SendLegacyChallenge(ip, port, contactID);
	}

	// Check if firewalled
	if (CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port, senderKey, contactVersion);
	}
}

// KADEMLIA2_HELLO_RES_ACK
// Used in Kad2.0 only
void CKademliaUDPListener::Process2HelloResponseAck(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, bool validReceiverKey)
{
	CHECK_PACKET_MIN_SIZE(17);
	CHECK_TRACKED_PACKET(KADEMLIA2_HELLO_RES);

	if (!validReceiverKey) {
		AddDebugLogLineN(logClientKadUDP, wxT("Receiver key is invalid! (sender: ") + KadIPToString(ip) + wxT(")"));
		return;
	}

	// Additional packet to complete a three-way-handshake, making sure the remote contact is not using a spoofed ip.
	CMemFile bio(packetData, lenPacket);
	CUInt128 remoteID = bio.ReadUInt128();
	if (!CKademlia::GetRoutingZone()->VerifyContact(remoteID, ip)) {
		AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + KadIPToString(ip) + wxT(")"));
	} else {
		AddDebugLogLineN(logKadRouting, wxT("Verified contact (") + KadIPToString(ip) + wxT(") by HELLO_RES_ACK"));
	}
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
			AddDebugLogLineN(logClientKadUDP, wxT("Remote client demands ACK, but didn't send any sender key! (sender: ") + KadIPToString(ip) + wxT(")"));
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
		SendLegacyChallenge(ip, port, contactID);
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

// KADEMLIA2_REQ
// Used in Kad2.0 only
void CKademliaUDPListener::ProcessKademlia2Request(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	// Get target and type
	CMemFile bio(packetData, lenPacket);
	uint8_t type = bio.ReadUInt8();
	type &= 0x1F;
	if (type == 0) {
		throw wxString(CFormat(wxT("***NOTE: Received wrong type (0x%02x) in %s")) % type % wxString::FromAscii(__FUNCTION__));
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

		DebugSendF(CFormat(wxT("Kad2Res (count=%u)")) % count, ip, port);
		SendPacket(packetdata, KADEMLIA2_RES, ip, port, senderKey, NULL);
	}
}

// KADEMLIA2_RES
// Used in Kad2.0 only
void CKademliaUDPListener::ProcessKademlia2Response(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& WXUNUSED(senderKey))
{
	CHECK_TRACKED_PACKET(KADEMLIA2_REQ);

	// Used Pointers
	CRoutingZone *routingZone = CKademlia::GetRoutingZone();

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint8_t numContacts = bio.ReadUInt8();

	// Is this one of our legacy challenge packets?
	CUInt128 contactID;
	if (IsLegacyChallenge(target, ip, KADEMLIA2_REQ, contactID)) {
		// yup it is, set the contact as verified
		if (!routingZone->VerifyContact(contactID, ip)) {
			AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + KadIPToString(ip) + wxT(")"));
		} else {
			AddDebugLogLineN(logKadRouting, wxT("Verified contact with legacy challenge (Kad2Req) - ") + KadIPToString(ip));
		}
		return;	// we do not actually care for its other content
	}
	// Verify packet is expected size
	CHECK_PACKET_EXACT_SIZE(16+1 + (16+4+2+2+1)*numContacts);

	// is this a search for firewallcheck ips?
	bool isFirewallUDPCheckSearch = false;
	if (CUDPFirewallTester::IsFWCheckUDPRunning() && CSearchManager::IsFWCheckUDPSearch(target)) {
		isFirewallUDPCheckSearch = true;
	}

	DEBUG_ONLY( uint32_t ignoredCount = 0; )
	DEBUG_ONLY( uint32_t kad1Count = 0; )
	CScopedPtr<ContactList> results;
	for (uint8_t i = 0; i < numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32_t contactIP = bio.ReadUInt32();
		uint16_t contactPort = bio.ReadUInt16();
		uint16_t tport = bio.ReadUInt16();
		uint8_t version = bio.ReadUInt8();
		uint32_t hostIP = wxUINT32_SWAP_ALWAYS(contactIP);
		if (version > 1) {	// Kad1 nodes are no longer accepted and ignored
			if (::IsGoodIPPort(hostIP, contactPort)) {
				if (!theApp->ipfilter->IsFiltered(hostIP) && !(contactPort == 53 && version <= 5) /*No DNS Port without encryption*/) {
					if (isFirewallUDPCheckSearch) {
						// UDP FirewallCheck searches are special. The point is we need an IP which we didn't sent a UDP message yet
						// (or in the near future), so we do not try to add those contacts to our routingzone and we also don't
						// deliver them back to the searchmanager (because he would UDP-ask them for further results), but only report
						// them to FirewallChecker - this will of course cripple the search but thats not the point, since we only
						// care for IPs and not the random set target
						CUDPFirewallTester::AddPossibleTestContact(id, contactIP, contactPort, tport, target, version, 0, false);
					} else {
						bool verified = false;
						bool wasAdded = routingZone->AddUnfiltered(id, contactIP, contactPort, tport, version, 0, verified, false, false);
						CContact *temp = new CContact(id, contactIP, contactPort, tport, version, 0, false, target);
						if (wasAdded || routingZone->IsAcceptableContact(temp)) {
							results->push_back(temp);
						} else {
							DEBUG_ONLY( ignoredCount++; )
							delete temp;
						}
					}
				}
			}
		} else {
			DEBUG_ONLY( kad1Count++; )
		}
	}

#ifdef __DEBUG__
	if (ignoredCount > 0) {
		AddDebugLogLineN(logKadRouting, CFormat(wxT("Ignored %u bad %s in routing answer from %s")) % ignoredCount % (ignoredCount > 1 ? wxT("contacts") : wxT("contact")) % KadIPToString(ip));
	}
	if (kad1Count > 0) {
		AddDebugLogLineN(logKadRouting, CFormat(wxT("Ignored %u kad1 %s in routing answer from %s")) % kad1Count % (kad1Count > 1 ? wxT("contacts") : wxT("contact")) % KadIPToString(ip));
	}
#endif

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
		AddDebugLogLineN(logKadSearch, wxT("***NOTE: Search expression tree exceeds depth limit!"));
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
		} else {
			AddDebugLogLineN(logKadSearch, CFormat(wxT("*** Unknown boolean search operator 0x%02x (CreateSearchExpressionTree)")) % boolop);
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
			AddDebugLogLineN(logKadSearch, CFormat(wxT("*** Unknown integer search op=0x%02x (CreateSearchExpressionTree)")) % mmop);
			return NULL;
		}

		// read tag name
		wxString strTagName = bio.ReadString(false);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = _aOps[mmop].eSearchTermOp;
		pSearchTerm->tag = new CTagVarInt(strTagName, ullValue);

		return pSearchTerm;
	} else {
		AddDebugLogLineN(logKadSearch, CFormat(wxT("*** Unknown search op=0x%02x (CreateSearchExpressionTree)")) % op);
		return NULL;
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
	CKademlia::GetIndexed()->SendValidKeywordResult(target, pSearchTerms, ip, port, false, startPosition, senderKey);
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
	CKademlia::GetIndexed()->SendValidSourceResult(target, ip, port, startPosition, fileSize, senderKey);
}

void CKademliaUDPListener::ProcessSearchResponse(CMemFile& bio)
{
	// What search does this relate to
	CUInt128 target = bio.ReadUInt128();

	// How many results..
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
 		CScopedContainer<TagPtrList> tags;
		bio.ReadTagPtrList(tags.get(), true/*bOptACP*/);
		CSearchManager::ProcessResult(target, answer, tags.get());
		count--;
	}
}


// KADEMLIA_SEARCH_RES
// Used in Kad1.0 only
void CKademliaUDPListener::ProcessSearchResponse(const uint8_t *packetData, uint32_t lenPacket)
{
	// Verify packet is expected size
	CHECK_PACKET_MIN_SIZE(37);

	CMemFile bio(packetData, lenPacket);
	ProcessSearchResponse(bio);
}

// KADEMLIA2_SEARCH_RES
// Used in Kad2.0 only
void CKademliaUDPListener::Process2SearchResponse(const uint8_t *packetData, uint32_t lenPacket, const CKadUDPKey& WXUNUSED(senderKey))
{
	CMemFile bio(packetData, lenPacket);

	// Who sent this packet.
	CUInt128 source = bio.ReadUInt128();

	ProcessSearchResponse(bio);
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

	if (distance.Get32BitChunk(0) > SEARCHTOLERANCE && !::IsLanIP(wxUINT32_SWAP_ALWAYS(ip))) {
		return;
	}

	DEBUG_ONLY( wxString strInfo; )
	uint16_t count = bio.ReadUInt16();
	uint8_t load = 0;
	while (count > 0) {
		DEBUG_ONLY( strInfo.Clear(); )

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
							DEBUG_ONLY( strInfo += CFormat(wxT("  Name=\"%s\"")) % entry->GetCommonFileName(); )
						}
						delete tag; // tag is no longer stored, but membervar is used
					} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
						if (entry->m_uSize == 0) {
							if (tag->IsBsob() && tag->GetBsobSize() == 8) {
								entry->m_uSize = PeekUInt64(tag->GetBsob());
							} else {
								entry->m_uSize = tag->GetInt();
							}
							DEBUG_ONLY( strInfo += CFormat(wxT("  Size=%u")) % entry->m_uSize; )
						}
						delete tag; // tag is no longer stored, but membervar is used
					} else {
						//TODO: Filter tags
						entry->AddTag(tag);
					}
				}
				tags--;
			}
#ifdef __DEBUG__
			if (!strInfo.IsEmpty()) {
				AddDebugLogLineN(logClientKadUDP, strInfo);
			}
#endif
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

	if (distance.Get32BitChunk(0) > SEARCHTOLERANCE && !::IsLanIP(wxUINT32_SWAP_ALWAYS(ip))) {
		return;
	}

	DEBUG_ONLY( wxString strInfo; )
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
						DEBUG_ONLY( strInfo += CFormat(wxT("  Size=%u")) % entry->m_uSize; )
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
#ifdef __DEBUG__
		if (!strInfo.IsEmpty()) {
			AddDebugLogLineN(logClientKadUDP, strInfo);
		}
#endif
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
		throw wxString(CFormat(wxT("***NOTE: Received unrequested response packet, size (%u) in %s")) % lenPacket % wxString::FromAscii(__FUNCTION__));
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

// KADEMLIA2_SEARCH_NOTES_REQ
// Used only by Kad2.0
void CKademliaUDPListener::Process2SearchNotesRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t port, const CKadUDPKey& senderKey)
{
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint64_t fileSize = bio.ReadUInt64();
	CKademlia::GetIndexed()->SendValidNoteResult(target, ip, port, fileSize, senderKey);
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
	ProcessSearchResponse(bio);
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

	if (distance.Get32BitChunk(0) > SEARCHTOLERANCE && !::IsLanIP(wxUINT32_SWAP_ALWAYS(ip))) {
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
void CKademliaUDPListener::ProcessCallbackRequest(const uint8_t *packetData, uint32_t lenPacket, uint32_t ip, uint16_t WXUNUSED(port), const CKadUDPKey& WXUNUSED(senderKey))
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
			AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_CALLBACK to ") + KadIPToString(ip));
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
			AddDebugLogLineN(logKadRouting, wxT("Unable to find valid sender in routing table (sender: ") + KadIPToString(ip) + wxT(")"));
		} else {
			AddDebugLogLineN(logKadRouting, wxT("Verified contact with legacy challenge (Kad2Ping) - ") + KadIPToString(ip));
		}
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
	if ((incomingPort != CKademlia::GetPrefs()->GetExternalKadPort() && incomingPort != CKademlia::GetPrefs()->GetInternKadPort()) || incomingPort == 0) {
		AddDebugLogLineN(logClientKadUDP, CFormat(wxT("Received UDP FirewallCheck on unexpected incoming port %u from %s")) % incomingPort % KadIPToString(ip));
		CUDPFirewallTester::SetUDPFWCheckResult(false, true, ip, 0);
	} else if (errorCode == 0) {
		AddDebugLogLineN(logClientKadUDP, CFormat(wxT("Received UDP FirewallCheck packet from %s with incoming port %u")) % KadIPToString(ip) % incomingPort);
		CUDPFirewallTester::SetUDPFWCheckResult(true, false, ip, incomingPort);
	} else {
		AddDebugLogLineN(logClientKadUDP, CFormat(wxT("Received UDP FirewallCheck packet from %s with incoming port %u with remote errorcode %u - ignoring result"))
			% KadIPToString(ip) % incomingPort % errorCode);
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
	AddDebugLogLineN(logClientKadUDP, wxT("FindNodeIDByIP: Requesting NodeID from ") + KadIPToString(ip) + wxT(" by sending Kad2HelloReq"));
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

void CKademliaUDPListener::SendLegacyChallenge(uint32_t ip, uint16_t port, const CUInt128& contactID)
{
	// We want to verify that a pre-0.49a contact is valid and not sent from a spoofed IP.
	// Because those versions don't support any direct validating, we send a KAD_REQ with a random ID,
	// which is our challenge. If we receive an answer packet for this request, we can be sure the
	// contact is not spoofed
#ifdef __DEBUG__
	CContact* contact = CKademlia::GetRoutingZone()->GetContact(contactID);
	if (contact != NULL) {
		if (contact->GetType() < 2) {
			AddDebugLogLineN(logKadRouting, wxT("Sending challenge to a long known contact (should be verified already) - ") + KadIPToString(ip));
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
	CUInt128 challenge(GetRandomUint128());
	if (challenge == 0) {
		// hey there is a 2^128 chance that this happens ;)
		wxFAIL;
		challenge = 1;
	}
	// Put the target we want into the packet. This is our challenge
	packetdata.WriteUInt128(challenge);
	// Add the ID of the contact we are contacting for sanity checks on the other end.
	packetdata.WriteUInt128(contactID);
	DebugSendF(wxT("Kad2Req(SendLegacyChallenge)"), ip, port);
	// those versions we send those requests to don't support encryption / obfuscation
	SendPacket(packetdata, KADEMLIA2_REQ, ip, port, 0, NULL);
	AddLegacyChallenge(contactID, challenge, ip, KADEMLIA2_REQ);
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

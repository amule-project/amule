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

#include "KademliaUDPListener.h"

#include <include/protocol/Protocols.h>
#include <include/protocol/kad/Constants.h>
#include <include/protocol/kad/Client2Client/UDP.h>
#include <include/protocol/ed2k/Client2Client/TCP.h> // OP_CALLBACK is sent in some cases.
#include <include/common/Macros.h>
#include <include/tags/FileTags.h>

#include "../routing/Contact.h"
#include "../routing/RoutingZone.h"
#include "../kademlia/Indexed.h"
#include "../kademlia/Defines.h"
#include "../../amule.h"
#include "../../ClientUDPSocket.h"
#include "../../Packet.h"
#include "../../ClientList.h"
#include "../../Statistics.h"
#include "../../MemFile.h"
#include "../../updownclient.h"
#include "../../ClientTCPSocket.h"
#include "../../Logger.h"
#include <common/Format.h>
#include "../../Preferences.h"
#include "ScopedPtr.h"

#include <wx/tokenzr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define THIS_DEBUG_IS_JUST_FOR_KRY_DONT_TOUCH_IT_KTHX 1


extern wxChar* InvKadKeywordChars;

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void CKademliaUDPListener::Bootstrap(uint32 ip, uint16 port)
{
	wxASSERT(ip);
	SendMyDetails(KADEMLIA_BOOTSTRAP_REQ, ip, port);
}

void CKademliaUDPListener::SendMyDetails(byte opcode, uint32 ip, uint16 port)
{
	CMemFile packetdata(25);
	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetKadID());
	packetdata.WriteUInt32(CKademlia::GetPrefs()->GetIPAddress());
	packetdata.WriteUInt16(thePrefs::GetEffectiveUDPPort());
	packetdata.WriteUInt16(thePrefs::GetPort());
	packetdata.WriteUInt8(0);
	SendPacket(packetdata, opcode, ip, port);
}

void CKademliaUDPListener::FirewalledCheck(uint32 ip, uint16 port)
{
	CMemFile packetdata(2);
	packetdata.WriteUInt16(thePrefs::GetPort());
	SendPacket(packetdata, KADEMLIA_FIREWALLED_REQ, ip, port);
}

void CKademliaUDPListener::SendNullPacket(byte opcode,uint32 ip, uint16 port)
{
	CMemFile packetdata(0);
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadNullPacket %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
	SendPacket(packetdata, opcode, ip, port);
}

void CKademliaUDPListener::PublishPacket(uint32 ip, uint16 port, const CUInt128 &targetID, const CUInt128 &contactID, const TagPtrList& tags)
{
	CMemFile packetdata;
	packetdata.WriteUInt128(targetID);
	//We only use this for publishing sources now.. So we always send one here..
	packetdata.WriteUInt16(1);
	packetdata.WriteUInt128(contactID);
	packetdata.WriteTagPtrList(tags);
	SendPacket(packetdata, KADEMLIA_PUBLISH_REQ, ip, port);
}

void CKademliaUDPListener::ProcessPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port)
{
	//Update connection state only when it changes.
	bool curCon = CKademlia::GetPrefs()->HasHadContact();
	CKademlia::GetPrefs()->SetLastContact();
	if( curCon != CKademlia::GetPrefs()->HasHadContact()) {
		theApp.ShowConnectionState();
	}

	byte opcode = data[1];
	const byte *packetData = data + 2;
	uint32 lenPacket = lenData - 2;

	switch (opcode) {
		case KADEMLIA_BOOTSTRAP_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadBootstrapReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessBootstrapRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_BOOTSTRAP_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadBootstrapRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessBootstrapResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_HELLO_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessHelloRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_HELLO_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessHelloResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessKademliaRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessKademliaResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessSearchRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessSearchResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessPublishRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));			
			ProcessPublishResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SRC_NOTES_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchNotesReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessSearchNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SRC_NOTES_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchNotesRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessSearchNotesResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUB_NOTES_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishNotesReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessPublishNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUB_NOTES_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishNotesRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessPublishNotesResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessFirewalledRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessFirewalledResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_ACK:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledAck from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessFirewalledResponse2(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDBUDDY_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFindBuddyReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessFindBuddyRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDBUDDY_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFindBuddyRes from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessFindBuddyResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_CALLBACK_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadCallbackReq from %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			ProcessCallbackRequest(packetData, lenPacket, ip, port);
			break;
		default: {
			throw wxString::Format(wxT("Unknown opcode %02x on CKademliaUDPListener::processPacket"), opcode);
		}
	}
}

void CKademliaUDPListener::AddContact( const byte *data, uint32 lenData, uint32 ip, uint16 port, uint16 tport)
{
	CMemFile bio((byte*)data, lenData);
	CUInt128 id = bio.ReadUInt128();
	bio.ReadUInt32();
	bio.ReadUInt16();
	if( tport ) {
		bio.ReadUInt16();
	} else {
		tport = bio.ReadUInt16();
	}
	byte type = bio.ReadUInt8();
	//AddDebugLogLineM(false, logKadMain, wxT("Adding a contact with ip ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip),port));
	// Look for existing client
	CContact *contact = CKademlia::GetRoutingZone()->GetContact(id);
	if (contact != NULL) {
		contact->SetIPAddress(ip);
		contact->SetUDPPort(port);
		contact->SetTCPPort(tport);
	} else {
		if(IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip),port)) {
			// Ignore stated ip and port, use the address the packet came from
			CKademlia::GetRoutingZone()->Add(id, ip, port, tport, type);
		}
	}
}

void CKademliaUDPListener::AddContacts( const byte *data, uint32 lenData, uint16 numContacts)
{
	CMemFile bio((byte*)data, lenData );
	CRoutingZone *routingZone = CKademlia::GetRoutingZone();
	for (uint16 i=0; i<numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32 ip = bio.ReadUInt32();
		ip = wxUINT32_SWAP_ALWAYS(ip);
		uint16 port = bio.ReadUInt16();
		uint16 tport = bio.ReadUInt16();
		byte type = bio.ReadUInt8();
		//AddDebugLogLineM(false, logKadMain, wxT("Adding contact(s) with ip ") + Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip),port));
		if (IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip),port)) {
			routingZone->Add(id, ip, port, tport, type);
		}
	}
}

//KADEMLIA_BOOTSTRAP_REQ
void CKademliaUDPListener::ProcessBootstrapRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Add the sender to the list of contacts
	AddContact(packetData, lenPacket, ip, port);

	// Get some contacts to return
	ContactList contacts;
	uint16 numContacts = 1 + (uint16)CKademlia::GetRoutingZone()->GetBootstrapContacts(&contacts, 20);

	// Create response packet
	//We only collect a max of 20 contacts here.. Max size is 527.
	//2 + 25(20) + 15(1)
	CMemFile packetdata(527);

	// Write packet info
	packetdata.WriteUInt16(numContacts);
	CContact *contact;
	ContactList::const_iterator it;
	for (it = contacts.begin(); it != contacts.end(); it++) {
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
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadBootstrapRes %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));

	SendPacket(packetdata, KADEMLIA_BOOTSTRAP_RES, ip, port);
}

//KADEMLIA_BOOTSTRAP_RES
void CKademliaUDPListener::ProcessBootstrapResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 27){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// How many contacts were given
	CMemFile bio(packetData, lenPacket);
	uint16 numContacts = bio.ReadUInt16();

	// Verify packet is expected size
	if (lenPacket != (uint32)(2 + 25*numContacts)) {
		return;
	}

	// Add these contacts to the list.
	AddContacts(packetData+2, lenPacket-2, numContacts);
	// Send sender to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);
}

//KADEMLIA_HELLO_REQ
void CKademliaUDPListener::ProcessHelloRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Add the sender to the list of contacts
	AddContact(packetData, lenPacket, ip, port);

	// Send response
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloRes %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));	
	SendMyDetails(KADEMLIA_HELLO_RES, ip, port);

	// Check if firewalled
	if(CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port);
	}
}

//KADEMLIA_HELLO_RES
void CKademliaUDPListener::ProcessHelloResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Add or Update contact.
	AddContact(packetData, lenPacket, ip, port);

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);
}

//KADEMLIA_REQ
void CKademliaUDPListener::ProcessKademliaRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 33){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	//RecheckIP and firewall status
	if(CKademlia::GetPrefs()->GetRecheckIP())
	{
		FirewalledCheck(ip, port);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloReq %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
		SendMyDetails(KADEMLIA_HELLO_REQ, ip, port);
	}

	// Get target and type
	CMemFile bio(packetData, lenPacket);
	byte type = bio.ReadUInt8();
//		bool flag1 = (type >> 6); //Reserved
//		bool flag2 = (type >> 7); //Reserved
//		bool flag3 = (type >> 8); //Reserved

	type = type & 0x1F;
	if( type == 0 ) {
		throw wxString::Format(wxT("***NOTE: Received wrong type (0x%02x) in "), type) + wxString::FromAscii(__FUNCTION__);
	}

	//This is the target node trying to be found.
	CUInt128 target = bio.ReadUInt128();
	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(target);

	//This makes sure we are not mistaken identify. Some client may have fresh installed and have a new hash.
	CUInt128 check = bio.ReadUInt128();
	if( CKademlia::GetPrefs()->GetKadID().CompareTo(check)) {
		return;
	}

	// Get required number closest to target
	ContactMap results;
	CKademlia::GetRoutingZone()->GetClosestTo(2, target, distance, (int)type, &results);
	uint16 count = (uint16)results.size();

	// Write response
	// Max count is 32. size 817.. 
	// 16 + 1 + 25(32)
	CMemFile packetdata( 817 );
	packetdata.WriteUInt128(target);
	packetdata.WriteUInt8((byte)count);
	CContact *c;
	ContactMap::const_iterator it;
	for (it = results.begin(); it != results.end(); it++) {
		c = it->second;
		packetdata.WriteUInt128(c->GetClientID());
		packetdata.WriteUInt32(c->GetIPAddress());
		packetdata.WriteUInt16(c->GetUDPPort());
		packetdata.WriteUInt16(c->GetTCPPort());
		packetdata.WriteUInt8(c->GetType());
	}

	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadRes %s Count=%u")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port) % count);

	SendPacket(packetdata, KADEMLIA_RES, ip, port);
}

//KADEMLIA_RES
void CKademliaUDPListener::ProcessKademliaResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 17){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);	
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::GetRoutingZone();

	if(CKademlia::GetPrefs()->GetRecheckIP()) {
		FirewalledCheck(ip, port);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloReq %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
		SendMyDetails(KADEMLIA_HELLO_REQ, ip, port);
	}

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint16 numContacts = bio.ReadUInt8();

	// Verify packet is expected size
	if (lenPacket != (uint32)(16+1 + (16+4+2+2+1)*numContacts)) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Set contact to alive.
	routingZone->SetAlive(ip, port);

	
	CScopedPtr<ContactList> results(new ContactList);
	
	for (uint16 i=0; i<numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32 contactIP = bio.ReadUInt32();
		uint16 contactPort = bio.ReadUInt16();
		uint16 tport = bio.ReadUInt16();
		byte type = bio.ReadUInt8();
		if(::IsGoodIPPort(wxUINT32_SWAP_ALWAYS(contactIP),contactPort)) {
			routingZone->Add(id, contactIP, contactPort, tport, type);
			results->push_back(new CContact(id, contactIP, contactPort, tport, target));
		}
	}

	CSearchManager::ProcessResponse(target, ip, port, results.release());
}

void CKademliaUDPListener::Free(SSearchTerm* pSearchTerms)
{
	if (!pSearchTerms) {
		return;
	}

	Free(pSearchTerms->left);
	Free(pSearchTerms->right);
	delete pSearchTerms;
}

SSearchTerm* CKademliaUDPListener::CreateSearchExpressionTree(CMemFile& bio, int iLevel)
{
	// the max. depth has to match our own limit for creating the search expression 
	// (see also 'ParsedSearchExpression' and 'GetSearchPacket')
	if (iLevel >= 24){
		AddDebugLogLineM(false, logClientKadUDP, wxT("***NOTE: Search expression tree exceeds depth limit!"));
		return NULL;
	}
	iLevel++;

	uint8 op = bio.ReadUInt8();
	if (op == 0x00) {
		uint8 boolop = bio.ReadUInt8();
		if (boolop == 0x00) { // AND
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::AND;
			//TRACE(" AND");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxASSERT(0);
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxASSERT(0);
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
				wxASSERT(0);
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxASSERT(0);
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		} else if (boolop == 0x02) { // NAND
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::NAND;
			//TRACE(" NAND");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxASSERT(0);
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				wxASSERT(0);
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
		wxStringTokenizer token(str,InvKadKeywordChars,wxTOKEN_DEFAULT );
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
		wxString strTagName =  bio.ReadString(true);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::MetaTag;
		pSearchTerm->tag = new CTagString(strTagName, strValue);
		return pSearchTerm;
	}
	else if (op == 0x03) { // Min/Max
		static const struct {
			SSearchTerm::ESearchTermType eSearchTermOp;
			wxString pszOp;
		} _aOps[] =
		{
			{ SSearchTerm::OpEqual,			wxT("=")		}, // mmop=0x00
			{ SSearchTerm::OpGreaterEqual,	wxT(">=")	}, // mmop=0x01
			{ SSearchTerm::OpLessEqual,		wxT("<=")	}, // mmop=0x02
			{ SSearchTerm::OpGreater,		wxT(">")		}, // mmop=0x03
			{ SSearchTerm::OpLess,			wxT("<")		}, // mmop=0x04
			{ SSearchTerm::OpNotEqual,		wxT("!=")	}  // mmop=0x05
		};

		// read tag value
		uint32 uValue = bio.ReadUInt32();

		// read integer operator
		uint8 mmop = bio.ReadUInt8();
		if (mmop >= itemsof(_aOps)){
			AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("*** Unknown integer search op=0x%02x (CreateSearchExpressionTree)"), mmop));
			return NULL;
		}

		// read tag name
		wxString strTagName = bio.ReadString(false);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = _aOps[mmop].eSearchTermOp;
		pSearchTerm->tag = new CTagVarInt(strTagName, uValue);

		return pSearchTerm;
	} else{
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("*** Unknown search op=0x%02x (CreateSearchExpressionTree)"), op));
		return NULL;
	}
}

//KADEMLIA_SEARCH_REQ
void CKademliaUDPListener::ProcessSearchRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 17){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);		
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint8 restrictive = bio.ReadUInt8();

#ifdef _DEBUG
	//DWORD dwNow = GetTickCount();
#endif
	if(lenPacket == 17 ) {
		if(restrictive) {
			//Source request
			CKademlia::GetIndexed()->SendValidSourceResult(target, ip, port);
			//DEBUG_ONLY( Debug("SendValidSourceResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
		} else {
			//Single keyword request
			CKademlia::GetIndexed()->SendValidKeywordResult(target, NULL, ip, port );
			//DEBUG_ONLY( Debug("SendValidKeywordResult (Single): Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
		}
	} else if(lenPacket > 17) {
		SSearchTerm* pSearchTerms = NULL;
		if (restrictive) {
			pSearchTerms = CreateSearchExpressionTree(bio, 0);
		}
		//Keyword request with added options.
		CKademlia::GetIndexed()->SendValidKeywordResult(target, pSearchTerms, ip, port); 
		Free(pSearchTerms);
		//DEBUG_ONLY( Debug("SendValidKeywordResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
	}
}

//KADEMLIA_SEARCH_RES
void CKademliaUDPListener::ProcessSearchResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();

	// How many results.. Not supported yet..
	uint16 count = bio.ReadUInt16();
	while( count > 0 ) {
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
			DebugClientOutput(wxT("CKademliaUDPListener::processSearchResponse"),ip,port,packetData,lenPacket);
			deleteTagPtrListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::ProcessResult(target, ip, port, answer, tags);
		count--;
	}
}

//KADEMLIA_PUBLISH_REQ
void CKademliaUDPListener::ProcessPublishRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	//There are different types of publishing..
	//Keyword and File are Stored..
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	//Used Pointers
	CIndexed *indexed = CKademlia::GetIndexed();

	if( CKademlia::GetPrefs()->GetFirewalled() ) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	CUInt128 distance(CKademlia::GetPrefs()->GetKadID());
	distance.XOR(file);

	if( thePrefs::FilterLanIPs() && distance.Get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	wxString strInfo;
	uint16 count = bio.ReadUInt16();
	uint16 totalcount = count;
	bool flag = false;
	uint8 load = 0;
	while( count > 0 ) {
		strInfo.Clear();

		CUInt128 target = bio.ReadUInt128();

		Kademlia::CEntry* entry = new Kademlia::CEntry();
		uint32 tags = 0, totaltags = 0;
		try {
			entry->m_iIP = ip;
			entry->m_iUDPport = port;
			entry->m_iKeyID.SetValue(file);
			entry->m_iSourceID.SetValue(target);
			tags = bio.ReadUInt8();
			totaltags = tags;
			while(tags > 0) {
				CTag* tag = bio.ReadTag();
				totaltags = tags;
				if(tag) {
					if (!tag->GetName().Cmp(TAG_SOURCETYPE) && tag->GetType() == 9) {
						if( entry->m_bSource == false ) {
							entry->m_lTagList.push_back(new CTagVarInt(TAG_SOURCEIP, entry->m_iIP));
							entry->m_lTagList.push_back(new CTagVarInt(TAG_SOURCEUPORT, entry->m_iUDPport));
						} else {
							//More than one sourcetype tag found.
							delete tag;
						}
						entry->m_bSource = true;
					}
					
					if (!tag->GetName().Cmp(TAG_FILENAME)) {
						if ( entry->m_sFileName.IsEmpty() ) {
							entry->m_sFileName = tag->GetStr();
							// Make lowercase, the search code expects lower case strings!
							entry->m_sFileName.MakeLower();
							strInfo += CFormat(wxT("  Name=\"%s\"")) % entry->m_sFileName;
							// NOTE: always add the 'name' tag, even if it's stored separately in 'fileName'. the tag is still needed for answering search request
							entry->m_lTagList.push_back(tag);
						} else {
							//More then one Name tag found.
							delete tag;
						}
					} else if (!tag->GetName().Cmp(TAG_FILESIZE)) {
						if( entry->m_iSize == 0 ) {
							if (tag->IsBsob() && (tag->GetBsobSize() == 8)) {
								// Kad1.0 uint64 type using a BSOB.
								entry->m_iSize = PeekUInt64(tag->GetBsob());
							} else {
								wxASSERT(tag->IsInt());
								entry->m_iSize = tag->GetInt();	
							}
							strInfo += wxString::Format(wxT("  Size=%") wxLongLongFmtSpec wxT("u"), entry->m_iSize);
							// NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
							entry->m_lTagList.push_back(tag);
						} else {
							//More then one size tag found
							delete tag;
						}
					} else if (!tag->GetName().Cmp(TAG_SOURCEPORT)) {
						if( entry->m_iTCPport == 0 ) {
							entry->m_iTCPport = tag->GetInt();
							entry->m_lTagList.push_back(tag);
						} else {
							//More then one port tag found
							delete tag;
						}
					} else {
						//TODO: Filter tags
						entry->m_lTagList.push_back(tag);
					}
				}
				tags--;
			}
			if (!strInfo.IsEmpty()) {
				AddDebugLogLineM(false, logClientKadUDP, strInfo);
			}
		} catch(...) {
			printf("Error on count %i tag %i\n",totalcount-count, totaltags-tags);
			DebugClientOutput(wxT("CKademliaUDPListener::processPublishRequest"),ip,port,packetData,lenPacket);
			delete entry;
			throw;
		}

		if( entry->m_bSource == true ) {
			entry->m_tLifeTime = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES;
			if( indexed->AddSources(file, target, entry, load ) ) {
				flag = true;
			} else {
				delete entry;
				entry = NULL;
			}
		} else {
			entry->m_tLifeTime = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEK;
			if( indexed->AddKeyword(file, target, entry, load) ) {
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
	if( flag ) {
		CMemFile packetdata(17);
		packetdata.WriteUInt128(file);
		packetdata.WriteUInt8(load);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishRes %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));

		SendPacket( packetdata, KADEMLIA_PUBLISH_RES, ip, port);
	}
}

//KADEMLIA_PUBLISH_ACK
void CKademliaUDPListener::ProcessPublishResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 16) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	bool loadResponse = false;
	uint8 load = 0;
	if( bio.GetLength() > bio.GetPosition() ) {
		loadResponse = true;
		load = bio.ReadUInt8();
	}

	CSearchManager::ProcessPublishResult(file, load, loadResponse);
}

//KADEMLIA_SRC_NOTES_REQ
void CKademliaUDPListener::ProcessSearchNotesRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 32) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	CUInt128 source = bio.ReadUInt128();

	CKademlia::GetIndexed()->SendValidNoteResult(target, source, ip, port);
}

//KADEMLIA_SRC_NOTES_RES
void CKademliaUDPListener::ProcessSearchNotesResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);

	// What search does this relate to
	CMemFile bio(packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();

	uint16 count = bio.ReadUInt16();
	while( count > 0 ) {
		// What is the answer
		CUInt128 answer = bio.ReadUInt128();

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagPtrList* tags = new TagPtrList;
		try{
			bio.ReadTagPtrList(tags, true/*bOptACP*/);
		} catch(...){
			DebugClientOutput(wxT("CKademliaUDPListener::processSearchNotesResponse"),ip,port,packetData,lenPacket);
			deleteTagPtrListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::ProcessResult(target, ip, port, answer, tags);
		count--;
	}
}

//KADEMLIA_PUB_NOTES_REQ
void CKademliaUDPListener::ProcessPublishNotesRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	if( CKademlia::GetPrefs()->GetFirewalled() ) {
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
		entry->m_iIP = ip;
		entry->m_iUDPport = port;
		entry->m_iKeyID.SetValue(target);
		entry->m_iSourceID.SetValue(source);
		bio.ReadTagPtrList(&entry->m_lTagList);
		entry->m_bSource = false;
	} catch(...) {
		DebugClientOutput(wxT("CKademliaUDPListener::processPublishNotesRequest"),ip,port,packetData,lenPacket);
		delete entry;
		throw;
	}

	if( entry == NULL ) {
		throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry == NULL"));
	} else if( entry->m_lTagList.size() == 0 || entry->m_lTagList.size() > 5) {
		delete entry;
		throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry->m_lTagList.size() == 0 || entry->m_lTagList.size() > 5"));
	}

	uint8 load = 0;
	if( CKademlia::GetIndexed()->AddNotes(target, source, entry, load ) ) {
		CMemFile packetdata(17);
		packetdata.WriteUInt128(target);
		packetdata.WriteUInt8(load);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishNotesRes %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
		SendPacket( packetdata, KADEMLIA_PUB_NOTES_RES, ip, port);
	} else {
		delete entry;
	}
}

//KADEMLIA_PUB_NOTES_RES
void CKademliaUDPListener::ProcessPublishNotesResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 16){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);		
	}

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);

	CMemFile bio(packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	bool loadResponse = false;
	uint8 load = 0;
	if( bio.GetLength() > bio.GetPosition() ) {
		loadResponse = true;
		load = bio.ReadUInt8();
	}

	CSearchManager::ProcessPublishResult(file, load, loadResponse);
}

//KADEMLIA_FIREWALLED_REQ
void CKademliaUDPListener::ProcessFirewalledRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 2){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	CMemFile bio(packetData, lenPacket);
	uint16 tcpport = bio.ReadUInt16();

	CUInt128 zero((uint32)0);
	CContact contact(zero, ip, port, tcpport, zero);
	theApp.clientlist->RequestTCP(&contact);

	// Send response
	CMemFile packetdata(4);
	packetdata.WriteUInt32(ip);
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledRes %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));

	SendPacket(packetdata, KADEMLIA_FIREWALLED_RES, ip, port);
}

//KADEMLIA_FIREWALLED_RES
void CKademliaUDPListener::ProcessFirewalledResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 4){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);

	CMemFile bio(packetData, lenPacket);
	uint32 firewalledIP = bio.ReadUInt32();

	//Update con state only if something changes.
	if( CKademlia::GetPrefs()->GetIPAddress() != firewalledIP ) {
		CKademlia::GetPrefs()->SetIPAddress(firewalledIP);
		theApp.ShowConnectionState();
	}
	CKademlia::GetPrefs()->IncRecheckIP();
}

//KADEMLIA_FIREWALLED_ACK
void CKademliaUDPListener::ProcessFirewalledResponse2 (const byte *WXUNUSED(packetData), uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 0) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::GetRoutingZone()->SetAlive(ip, port);
	CKademlia::GetPrefs()->IncFirewalled();
}

//KADEMLIA_FINDBUDDY_REQ
void CKademliaUDPListener::ProcessFindBuddyRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	if( CKademlia::GetPrefs()->GetFirewalled() ) {
		//We are firewalled but somehow we still got this packet.. Don't send a response..
		return;
	}

	CMemFile bio(packetData, lenPacket);
	CUInt128 BuddyID = bio.ReadUInt128();
	CUInt128 userID = bio.ReadUInt128();
	uint16 tcpport = bio.ReadUInt16();

	CUInt128 zero((uint32)0);
	CContact contact(userID, ip, port, tcpport, zero);
	theApp.clientlist->IncomingBuddy(&contact, &BuddyID);

	CMemFile packetdata(34);
	packetdata.WriteUInt128(BuddyID);
	packetdata.WriteUInt128(CKademlia::GetPrefs()->GetClientHash());
	packetdata.WriteUInt16(thePrefs::GetPort());
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFindBuddyRes %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));

	SendPacket(packetdata, KADEMLIA_FINDBUDDY_RES, ip, port);
}

//KADEMLIA_FINDBUDDY_RES
void CKademliaUDPListener::ProcessFindBuddyResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}


	CMemFile bio(packetData, lenPacket);
	CUInt128 check = bio.ReadUInt128();
	check.XOR(CUInt128(true));
	if( CKademlia::GetPrefs()->GetKadID().CompareTo(check)) {
		return;
	}
	CUInt128 userID = bio.ReadUInt128();
	uint16 tcpport = bio.ReadUInt16();

	CUInt128 zero((uint32)0);
	CContact contact(userID, ip, port, tcpport, zero);
	theApp.clientlist->RequestBuddy(&contact);
}

//KADEMLIA_CALLBACK_REQ
void CKademliaUDPListener::ProcessCallbackRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in "), lenPacket) + wxString::FromAscii(__FUNCTION__);
	}

	CUpDownClient* buddy = theApp.clientlist->GetBuddy();
	if( buddy != NULL ) {
		CMemFile bio(packetData, lenPacket);
		CUInt128 check = bio.ReadUInt128();
		// JOHNTODO: Filter bad buddies
		//CUInt128 bud(buddy->GetBuddyID());
		CUInt128 file = bio.ReadUInt128();
		uint16 tcp = bio.ReadUInt16();
		CMemFile packetdata(16+16+4+2);
		packetdata.WriteUInt128(check);
		packetdata.WriteUInt128(file);
		packetdata.WriteUInt32(ip);
		packetdata.WriteUInt16(tcp);
		CPacket* packet = new CPacket(packetdata, OP_EMULEPROT, OP_CALLBACK);
		if( buddy->GetSocket() ) {
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadCallback %s")) % Uint32_16toStringIP_Port(wxUINT32_SWAP_ALWAYS(ip), port));
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
          	buddy->GetSocket()->SendPacket(packet);
		} else {
			wxASSERT(0);
		}
	}
}

void CKademliaUDPListener::SendPacket(const CMemFile &data, byte opcode, uint32 destinationHost, uint16 destinationPort)
{
	CPacket* packet = new CPacket(data, OP_KADEMLIAHEADER, opcode);
	if( packet->GetPacketSize() > 200 ) {
		packet->PackPacket();
	}
	theStats::AddUpOverheadKad(packet->GetPacketSize());
	theApp.clientudp->SendPacket(packet,wxUINT32_SWAP_ALWAYS(destinationHost), destinationPort);
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
	CClientList::SourceList clientslist = theApp.clientlist->GetClientsByIP(ip);
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

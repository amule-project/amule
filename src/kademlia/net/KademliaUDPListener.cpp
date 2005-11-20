//
// This file is part of aMule Project
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Project ( http://www.amule-project.net )
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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
#include "../kademlia/Prefs.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/SearchManager.h"
#include "../routing/Contact.h"
#include "../routing/RoutingZone.h"
#include "../io/ByteIO.h"
#include "../io/IOException.h"
#include "../../NetworkFunctions.h"
#include "../../KnownFile.h"
#include "../../KnownFileList.h"
#include "../../OtherFunctions.h"
#include "../kademlia/Indexed.h"
#include "../kademlia/Tag.h"
#include "../../OPCodes.h"
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

#include <wx/tokenzr.h>

#if wxCHECK_VERSION(2, 5, 0)
#include <wx/arrstr.h>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef AMULE_DAEMON
#include "amuleDlg.h"
#include "KadDlg.h"
#endif

extern wxChar* InvKadKeywordChars;

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void CKademliaUDPListener::bootstrap(uint32 ip, uint16 port)
{
	wxASSERT(ip);
	sendMyDetails(KADEMLIA_BOOTSTRAP_REQ, ip, port);
}

void CKademliaUDPListener::sendMyDetails(byte opcode, uint32 ip, uint16 port)
{
	CMemFile bio(25);
	bio.WriteUInt128(CKademlia::getPrefs()->getKadID());
	bio.WriteUInt32(CKademlia::getPrefs()->getIPAddress());
	bio.WriteUInt16(thePrefs::GetEffectiveUDPPort());
	bio.WriteUInt16(thePrefs::GetPort());
	bio.WriteUInt8(0);
	sendPacket(&bio, opcode, ip, port);
}

void CKademliaUDPListener::firewalledCheck(uint32 ip, uint16 port)
{
	CMemFile bio(2);
	bio.WriteUInt16(thePrefs::GetPort());
	sendPacket(&bio, KADEMLIA_FIREWALLED_REQ, ip, port);
}

void CKademliaUDPListener::sendNullPacket(byte opcode,uint32 ip, uint16 port)
{
	CMemFile bio(0);
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadNullPacket %s")) % Uint32_16toStringIP_Port(ip, port));
	sendPacket(&bio, opcode, ip, port);
}

void CKademliaUDPListener::publishPacket(uint32 ip, uint16 port, const CUInt128 &targetID, const CUInt128 &contactID, const TagList& tags)
{
	//We need to get the tag lists working with CMemFiles..
	byte packet[1024];
	CByteIO bio(packet, sizeof(packet));
	bio.writeByte(OP_KADEMLIAHEADER);
	bio.writeByte(KADEMLIA_PUBLISH_REQ);
	bio.writeUInt128(targetID);
	//We only use this for publishing sources now.. So we always send one here..
	bio.writeUInt16(1);
	bio.writeUInt128(contactID);
	bio.writeTagList(tags);
	uint32 len = sizeof(packet) - bio.getAvailable();
	sendPacket(packet, len,  ip, port);
}

void CKademliaUDPListener::processPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port)
{
	//Update connection state only when it changes.
	bool curCon = CKademlia::getPrefs()->hasHadContact();
	CKademlia::getPrefs()->setLastContact();
	if( curCon != CKademlia::getPrefs()->hasHadContact()) {
		theApp.ShowConnectionState();
	}

	byte opcode = data[1];
	const byte *packetData = data + 2;
	uint32 lenPacket = lenData - 2;

	switch (opcode) {
		case KADEMLIA_BOOTSTRAP_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadBootstrapReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processBootstrapRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_BOOTSTRAP_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadBootstrapRes from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processBootstrapResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_HELLO_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processHelloRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_HELLO_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloRes from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processHelloResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadReq from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processKademliaRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadRes from %s")) % Uint32_16toStringIP_Port(ip, port));
			processKademliaResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchReq from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processSearchRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchRes from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processSearchResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishReq from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processPublishRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishRes from %s")) % Uint32_16toStringIP_Port(ip, port));			
			processPublishResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SRC_NOTES_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchNotesReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processSearchNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SRC_NOTES_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadSearchNotesRes from %s")) % Uint32_16toStringIP_Port(ip, port));
			processSearchNotesResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUB_NOTES_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishNotesReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processPublishNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUB_NOTES_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishNotesRes from %s")) % Uint32_16toStringIP_Port(ip, port));
			processPublishNotesResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processFirewalledRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledRes from %s")) % Uint32_16toStringIP_Port(ip, port));
			processFirewalledResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_ACK:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledAck from %s")) % Uint32_16toStringIP_Port(ip, port));
			processFirewalledResponse2(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDBUDDY_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFindBuddyReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processFindBuddyRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDBUDDY_RES:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFindBuddyRes from %s")) % Uint32_16toStringIP_Port(ip, port));
			processFindBuddyResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_CALLBACK_REQ:
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadCallbackReq from %s")) % Uint32_16toStringIP_Port(ip, port));
			processCallbackRequest(packetData, lenPacket, ip, port);
			break;
		default: {
			throw wxString::Format(wxT("Unknown opcode %02x on CKademliaUDPListener::processPacket"), opcode);
		}
	}
}

void CKademliaUDPListener::addContact( const byte *data, uint32 lenData, uint32 ip, uint16 port, uint16 tport)
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
	//AddDebugLogLineM(false, logKadMain, wxT("Adding a contact with ip ") + Uint32_16toStringIP_Port(ip,port));
	// Look for existing client
	CContact *contact = CKademlia::getRoutingZone()->getContact(id);
	if (contact != NULL) {
		contact->setIPAddress(ip);
		contact->setUDPPort(port);
		contact->setTCPPort(tport);
	} else {
		if(IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip),port)) {
			// Ignore stated ip and port, use the address the packet came from
			CKademlia::getRoutingZone()->add(id, ip, port, tport, type);
		}
	}
}

void CKademliaUDPListener::addContacts( const byte *data, uint32 lenData, uint16 numContacts)
{
	CMemFile bio((byte*)data, lenData );
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	for (uint16 i=0; i<numContacts; i++) {
		CUInt128 id = bio.ReadUInt128();
		uint32 ip = bio.ReadUInt32();
		ip = wxUINT32_SWAP_ALWAYS(ip);
		uint16 port = bio.ReadUInt16();
		uint16 tport = bio.ReadUInt16();
		byte type = bio.ReadUInt8();
		//AddDebugLogLineM(false, logKadMain, wxT("Adding contact(s) with ip ") + Uint32_16toStringIP_Port(ip,port));
		if (IsGoodIPPort(wxUINT32_SWAP_ALWAYS(ip),port)) {
			routingZone->add(id, ip, port, tport, type);
		}
	}
}

//KADEMLIA_BOOTSTRAP_REQ
void CKademliaUDPListener::processBootstrapRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Add the sender to the list of contacts
	addContact(packetData, lenPacket, ip, port);

	// Get some contacts to return
	ContactList contacts;
	uint16 numContacts = 1 + (uint16)CKademlia::getRoutingZone()->getBootstrapContacts(&contacts, 20);

	// Create response packet
	//We only collect a max of 20 contacts here.. Max size is 527.
	//2 + 25(20) + 15(1)
	CMemFile bio(527);

	// Write packet info
	bio.WriteUInt16(numContacts);
	CContact *contact;
	ContactList::const_iterator it;
	for (it = contacts.begin(); it != contacts.end(); it++) {
		contact = *it;
		bio.WriteUInt128(contact->getClientID());
		bio.WriteUInt32(contact->getIPAddress());
		bio.WriteUInt16(contact->getUDPPort());
		bio.WriteUInt16(contact->getTCPPort());
		bio.WriteUInt8(contact->getType());
	}

	bio.WriteUInt128(CKademlia::getPrefs()->getKadID());
	bio.WriteUInt32(CKademlia::getPrefs()->getIPAddress());
	bio.WriteUInt16(thePrefs::GetEffectiveUDPPort());
	bio.WriteUInt16(thePrefs::GetPort());
	bio.WriteUInt8(0);

	// Send response
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadBootstrapRes %s")) % Uint32_16toStringIP_Port(ip, port));

	sendPacket(&bio, KADEMLIA_BOOTSTRAP_RES, ip, port);
}

//KADEMLIA_BOOTSTRAP_RES
void CKademliaUDPListener::processBootstrapResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 27){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// How many contacts were given
	CMemFile bio((byte*)packetData, lenPacket);
	uint16 numContacts = bio.ReadUInt16();

	// Verify packet is expected size
	if (lenPacket != (uint32)(2 + 25*numContacts)) {
		return;
	}

	// Add these contacts to the list.
	addContacts(packetData+2, lenPacket-2, numContacts);
	// Send sender to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);
}

//KADEMLIA_HELLO_REQ
void CKademliaUDPListener::processHelloRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Add the sender to the list of contacts
	addContact(packetData, lenPacket, ip, port);

	// Send response
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloRes %s")) % Uint32_16toStringIP_Port(ip, port));	
	sendMyDetails(KADEMLIA_HELLO_RES, ip, port);

	// Check if firewalled
	if(CKademlia::getPrefs()->getRecheckIP()) {
		firewalledCheck(ip, port);
	}
}

//KADEMLIA_HELLO_RES
void CKademliaUDPListener::processHelloResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Add or Update contact.
	addContact(packetData, lenPacket, ip, port);

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);
}

//KADEMLIA_REQ
void CKademliaUDPListener::processKademliaRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 33){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	//RecheckIP and firewall status
	if(CKademlia::getPrefs()->getRecheckIP())
	{
		firewalledCheck(ip, port);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloReq %s")) % Uint32_16toStringIP_Port(ip, port));
		sendMyDetails(KADEMLIA_HELLO_REQ, ip, port);
	}

	// Get target and type
	CMemFile bio((byte*)packetData, lenPacket);
	byte type = bio.ReadUInt8();
//		bool flag1 = (type >> 6); //Reserved
//		bool flag2 = (type >> 7); //Reserved
//		bool flag3 = (type >> 8); //Reserved

	type = type & 0x1F;
	if( type == 0 ) {
		throw wxString::Format(wxT("***NOTE: Received wrong type (0x%02x) in %s"), type, __FUNCTION__);
	}

	//This is the target node trying to be found.
	CUInt128 target = bio.ReadUInt128();
	CUInt128 distance(CKademlia::getPrefs()->getKadID());
	distance.XOR(target);

	//This makes sure we are not mistaken identify. Some client may have fresh installed and have a new hash.
	CUInt128 check = bio.ReadUInt128();
	if( CKademlia::getPrefs()->getKadID().compareTo(check)) {
		return;
	}

	// Get required number closest to target
	ContactMap results;
	CKademlia::getRoutingZone()->getClosestTo(2, target, distance, (int)type, &results);
	uint16 count = (uint16)results.size();

	// Write response
	// Max count is 32. size 817.. 
	// 16 + 1 + 25(32)
	CMemFile bio2( 817 );
	bio2.WriteUInt128(target);
	bio2.WriteUInt8((byte)count);
	CContact *c;
	ContactMap::const_iterator it;
	for (it = results.begin(); it != results.end(); it++) {
		c = it->second;
		bio2.WriteUInt128(c->getClientID());
		bio2.WriteUInt32(c->getIPAddress());
		bio2.WriteUInt16(c->getUDPPort());
		bio2.WriteUInt16(c->getTCPPort());
		bio2.WriteUInt8(c->getType());
	}

	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadRes %s Count=%u")) % Uint32_16toStringIP_Port(ip, port) % count);

	sendPacket(&bio2, KADEMLIA_RES, ip, port);
}

//KADEMLIA_RES
void CKademliaUDPListener::processKademliaResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 17){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);	
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();

	if(CKademlia::getPrefs()->getRecheckIP()) {
		firewalledCheck(ip, port);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadHelloReq %s")) % Uint32_16toStringIP_Port(ip, port));
		sendMyDetails(KADEMLIA_HELLO_REQ, ip, port);
	}

	// What search does this relate to
	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint16 numContacts = bio.ReadUInt8();

	// Verify packet is expected size
	if (lenPacket != (uint32)(16+1 + (16+4+2+2+1)*numContacts)) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	
	ContactList *results = new ContactList;
	try {
		for (uint16 i=0; i<numContacts; i++) {
			CUInt128 id = bio.ReadUInt128();
			uint32 contactIP = bio.ReadUInt32();
			uint16 contactPort = bio.ReadUInt16();
			uint16 tport = bio.ReadUInt16();
			byte type = bio.ReadUInt8();
			if(::IsGoodIPPort(wxUINT32_SWAP_ALWAYS(contactIP),contactPort)) {
				routingZone->add(id, contactIP, contactPort, tport, type);
				results->push_back(new CContact(id, contactIP, contactPort, tport, target));
			}
		}
	} catch(...) {
		delete results;
		throw;
	}
	CSearchManager::processResponse(target, ip, port, results);
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
		CTagValueString str(bio.ReadString(true));

		KadTagStrMakeLower(str); // make lowercase, the search code expects lower case strings!
		//TRACE(" \"%ls\"", str);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::String;
		pSearchTerm->astr = new wxArrayString;

		// pre-tokenize the string term
		wxStringTokenizer token(str,InvKadKeywordChars,wxTOKEN_DEFAULT );
		while (token.HasMoreTokens()) {
			CTagValueString strTok(token.GetNextToken());
			if (!strTok.IsEmpty()) {
				pSearchTerm->astr->Add(strTok);
			}
		}

		return pSearchTerm;
	} else if (op == 0x02) { // Meta tag 
		// read tag value
		CTagValueString strValue(bio.ReadString(true));

		KadTagStrMakeLower(strValue); // make lowercase, the search code expects lower case strings!

		// read tag name
		wxString strTagName =  bio.ReadString(true);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::MetaTag;
		pSearchTerm->tag = new Kademlia::CTagStr(strTagName, strValue);
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
		if (mmop >= ARRSIZE(_aOps)){
			AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("*** Unknown integer search op=0x%02x (CreateSearchExpressionTree)"), mmop));
			return NULL;
		}

		// read tag name
		wxString strTagName = bio.ReadString(false);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = _aOps[mmop].eSearchTermOp;
		pSearchTerm->tag = new Kademlia::CTagUInt32(strTagName, uValue);

		return pSearchTerm;
	} else{
		AddDebugLogLineM(false, logClientKadUDP, wxString::Format(wxT("*** Unknown search op=0x%02x (CreateSearchExpressionTree)"), op));
		return NULL;
	}
}

//KADEMLIA_SEARCH_REQ
void CKademliaUDPListener::processSearchRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 17){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);		
	}

	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	uint8 restrictive = bio.ReadUInt8();

#ifdef _DEBUG
	//DWORD dwNow = GetTickCount();
#endif
	if(lenPacket == 17 ) {
		if(restrictive) {
			//Source request
			CKademlia::getIndexed()->SendValidSourceResult(target, ip, port);
			//DEBUG_ONLY( Debug("SendValidSourceResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
		} else {
			//Single keyword request
			CKademlia::getIndexed()->SendValidKeywordResult(target, NULL, ip, port );
			//DEBUG_ONLY( Debug("SendValidKeywordResult (Single): Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
		}
	} else if(lenPacket > 17) {
		SSearchTerm* pSearchTerms = NULL;
		if (restrictive) {
			try {
				pSearchTerms = CreateSearchExpressionTree(bio, 0);
				//TRACE("\n");
			} catch(...) {
				Free(pSearchTerms);
				throw;
			}
		}
		//Keyword request with added options.
		CKademlia::getIndexed()->SendValidKeywordResult(target, pSearchTerms, ip, port); 
		Free(pSearchTerms);
		//DEBUG_ONLY( Debug("SendValidKeywordResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
	}
}

//KADEMLIA_SEARCH_RES
void CKademliaUDPListener::processSearchResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);

	// What search does this relate to
	CByteIO bio(packetData, lenPacket);
	CUInt128 target;
	bio.readUInt128(&target);

	// How many results.. Not supported yet..
	uint16 count = bio.readUInt16();
	while( count > 0 ) {
		// What is the answer
		CUInt128 answer;
		bio.readUInt128(&answer);

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagList* tags = new TagList;
		try {
			bio.readTagList(tags, true/*bOptACP*/);
		} catch(...){
			deleteTagListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::processResult(target, ip, port, answer, tags);
		count--;
	}
}

//KADEMLIA_PUBLISH_REQ
void CKademliaUDPListener::processPublishRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	//There are different types of publishing..
	//Keyword and File are Stored..
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	//Used Pointers
	CIndexed *indexed = CKademlia::getIndexed();

	if( CKademlia::getPrefs()->getFirewalled() ) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CByteIO bio(packetData, lenPacket);
	CUInt128 file;
	bio.readUInt128(&file);

	CUInt128 distance(CKademlia::getPrefs()->getKadID());
	distance.XOR(file);

	if( thePrefs::FilterLanIPs() && distance.get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	wxString strInfo;
	uint16 count = bio.readUInt16();
	bool flag = false;
	uint8 load = 0;
	while( count > 0 ) {
		strInfo.Clear();

		CUInt128 target;
		bio.readUInt128(&target);

		Kademlia::CEntry* entry = new Kademlia::CEntry();
		try {
			entry->ip = ip;
			entry->udpport = port;
			entry->keyID.setValue(file);
			entry->sourceID.setValue(target);
			uint32 tags = bio.readByte();
			while(tags > 0) {
				CTag* tag = bio.readTag();
				if(tag) {
					if (!tag->m_name.Cmp(wxT(TAG_SOURCETYPE)) && tag->m_type == 9) {
						if( entry->source == false ) {
							entry->taglist.push_back(new CTagUInt32(TAG_SOURCEIP, entry->ip));
							entry->taglist.push_back(new CTagUInt16(TAG_SOURCEUPORT, entry->udpport));
						} else {
							//More than one sourcetype tag found.
							delete tag;
						}
						entry->source = true;
					}
					
					if (!tag->m_name.Cmp(wxT(TAG_FILENAME))) {
						if ( entry->fileName.IsEmpty() ) {
							entry->fileName = tag->GetStr();
							KadTagStrMakeLower(entry->fileName); // make lowercase, the search code expects lower case strings!							
							strInfo += CFormat(wxT("  Name=\"%s\"")) % entry->fileName;
							// NOTE: always add the 'name' tag, even if it's stored separately in 'fileName'. the tag is still needed for answering search request
							entry->taglist.push_back(tag);
						} else {
							//More then one Name tag found.
							delete tag;
						}
					} else if (!tag->m_name.Cmp(wxT(TAG_FILESIZE))) {
						if( entry->size == 0 ) {
							entry->size = tag->GetInt();
							strInfo += wxString::Format(wxT("  Size=%u"), entry->size);
							// NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
							entry->taglist.push_back(tag);
						} else {
							//More then one size tag found
							delete tag;
						}
					} else if (!tag->m_name.Cmp(wxT(TAG_SOURCEPORT))) {
						if( entry->tcpport == 0 ) {
							entry->tcpport = tag->GetInt();
							entry->taglist.push_back(tag);
						} else {
							//More then one port tag found
							delete tag;
						}
					} else {
						//TODO: Filter tags
						entry->taglist.push_back(tag);
					}
				}
				tags--;
			}
			if (!strInfo.IsEmpty()) {
				AddDebugLogLineM(false, logClientKadUDP, strInfo);
			}
		} catch(...) {
			delete entry;
			throw;
		}

		if( entry->source == true ) {
			entry->lifetime = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES;
			if( indexed->AddSources(file, target, entry, load ) ) {
				flag = true;
			} else {
				delete entry;
				entry = NULL;
			}
		} else {
			entry->lifetime = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEK;
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
		CMemFile bio2(17);
		bio2.WriteUInt128(file);
		bio2.WriteUInt8(load);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishRes %s")) % Uint32_16toStringIP_Port(ip, port));

		sendPacket( &bio2, KADEMLIA_PUBLISH_RES, ip, port);
	}
}

//KADEMLIA_PUBLISH_ACK
void CKademliaUDPListener::processPublishResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 16) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);

	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	bool loadResponse = false;
	uint8 load = 0;
	if( bio.GetLength() > bio.GetPosition() ) {
		loadResponse = true;
		load = bio.ReadUInt8();
	}

	CSearchManager::processPublishResult(file, load, loadResponse);
}

//KADEMLIA_SRC_NOTES_REQ
void CKademliaUDPListener::processSearchNotesRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 32) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 target = bio.ReadUInt128();
	CUInt128 source = bio.ReadUInt128();

	CKademlia::getIndexed()->SendValidNoteResult(target, source, ip, port);
}

//KADEMLIA_SRC_NOTES_RES
void CKademliaUDPListener::processSearchNotesResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);

	// What search does this relate to
	CByteIO bio(packetData, lenPacket);
	CUInt128 target;
	bio.readUInt128(&target);

	uint16 count = bio.readUInt16();
	while( count > 0 ) {
		// What is the answer
		CUInt128 answer;
		bio.readUInt128(&answer);

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagList* tags = new TagList;
		try{
			bio.readTagList(tags, true/*bOptACP*/);
		} catch(...){
			deleteTagListEntries(tags);
			delete tags;
			tags = NULL;
			throw;
		}
		CSearchManager::processResult(target, ip, port, answer, tags);
		count--;
	}
}

//KADEMLIA_PUB_NOTES_REQ
void CKademliaUDPListener::processPublishNotesRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	if( CKademlia::getPrefs()->getFirewalled() ) {
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;
	}

	CByteIO bio(packetData, lenPacket);
	CUInt128 target;
	bio.readUInt128(&target);

	CUInt128 distance(CKademlia::getPrefs()->getKadID());
	distance.XOR(target);

	if( thePrefs::FilterLanIPs() && distance.get32BitChunk(0) > SEARCHTOLERANCE) {
		return;
	}

	CUInt128 source;
	bio.readUInt128(&source);

	Kademlia::CEntry* entry = new Kademlia::CEntry();
	try {
		entry->ip = ip;
		entry->udpport = port;
		entry->keyID.setValue(target);
		entry->sourceID.setValue(source);
		bio.readTagList(&entry->taglist);
		entry->source = false;
	} catch(...) {
		delete entry;
		throw;
	}

	if( entry == NULL ) {
		throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry == NULL"));
	} else if( entry->taglist.size() == 0 || entry->taglist.size() > 5) {
		delete entry;
		throw wxString(wxT("CKademliaUDPListener::processPublishNotesRequest: entry->taglist.size() == 0 || entry->taglist.size() > 5"));
	}

	uint8 load = 0;
	if( CKademlia::getIndexed()->AddNotes(target, source, entry, load ) ) {
		CMemFile bio2(17);
		bio2.WriteUInt128(target);
		bio2.WriteUInt8(load);
		AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadPublishNotesRes %s")) % Uint32_16toStringIP_Port(ip, port));
		sendPacket( &bio2, KADEMLIA_PUB_NOTES_RES, ip, port);
	} else {
		delete entry;
	}
}

//KADEMLIA_PUB_NOTES_RES
void CKademliaUDPListener::processPublishNotesResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 16){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);		
	}

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);

	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 file = bio.ReadUInt128();

	bool loadResponse = false;
	uint8 load = 0;
	if( bio.GetLength() > bio.GetPosition() ) {
		loadResponse = true;
		load = bio.ReadUInt8();
	}

	CSearchManager::processPublishResult(file, load, loadResponse);
}

//KADEMLIA_FIREWALLED_REQ
void CKademliaUDPListener::processFirewalledRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 2){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	CMemFile bio((byte*)packetData, lenPacket);
	uint16 tcpport = bio.ReadUInt16();

	CContact contact;
	contact.setIPAddress(ip);
	contact.setTCPPort(tcpport);
	contact.setUDPPort(port);
	theApp.clientlist->RequestTCP(&contact);

	// Send response
	CMemFile bio2(4);
	bio2.WriteUInt32(ip);
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFirewalledRes %s")) % Uint32_16toStringIP_Port(ip, port));

	sendPacket(&bio2, KADEMLIA_FIREWALLED_RES, ip, port);
}

//KADEMLIA_FIREWALLED_RES
void CKademliaUDPListener::processFirewalledResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 4){
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);

	CMemFile bio((byte*)packetData, lenPacket);
	uint32 firewalledIP = bio.ReadUInt32();

	//Update con state only if something changes.
	if( CKademlia::getPrefs()->getIPAddress() != firewalledIP ) {
		CKademlia::getPrefs()->setIPAddress(firewalledIP);
		theApp.ShowConnectionState();
	}
	CKademlia::getPrefs()->incRecheckIP();
}

//KADEMLIA_FIREWALLED_ACK
void CKademliaUDPListener::processFirewalledResponse2 (const byte *WXUNUSED(packetData), uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 0) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	// Set contact to alive.
	CKademlia::getRoutingZone()->setAlive(ip, port);
	CKademlia::getPrefs()->incFirewalled();
}

//KADEMLIA_FINDBUDDY_REQ
void CKademliaUDPListener::processFindBuddyRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	if( CKademlia::getPrefs()->getFirewalled() ) {
		//We are firewalled but somehow we still got this packet.. Don't send a response..
		return;
	}

	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 BuddyID = bio.ReadUInt128();
	CUInt128 userID = bio.ReadUInt128();
	uint16 tcpport = bio.ReadUInt16();

	CContact contact;
	contact.setIPAddress(ip);
	contact.setTCPPort(tcpport);
	contact.setUDPPort(port);
	contact.setClientID(userID);
	theApp.clientlist->IncomingBuddy(&contact, &BuddyID);

	CMemFile bio2(34);
	bio2.WriteUInt128(BuddyID);
	bio2.WriteUInt128(CKademlia::getPrefs()->getClientHash());
	bio2.WriteUInt16(thePrefs::GetPort());
	AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadFindBuddyRes %s")) % Uint32_16toStringIP_Port(ip, port));

	sendPacket(&bio2, KADEMLIA_FINDBUDDY_RES, ip, port);
}

//KADEMLIA_FINDBUDDY_RES
void CKademliaUDPListener::processFindBuddyResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}


	CMemFile bio((byte*)packetData, lenPacket);
	CUInt128 check = bio.ReadUInt128();
	check.XOR(CUInt128(true));
	if( CKademlia::getPrefs()->getKadID().compareTo(check)) {
		return;
	}
	CUInt128 userID = bio.ReadUInt128();
	uint16 tcpport = bio.ReadUInt16();

	CContact contact;
	contact.setIPAddress(ip);
	contact.setTCPPort(tcpport);
	contact.setUDPPort(port);
	contact.setClientID(userID);
	theApp.clientlist->RequestBuddy(&contact);
}

//KADEMLIA_CALLBACK_REQ
void CKademliaUDPListener::processCallbackRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34) {
		throw wxString::Format(wxT("***NOTE: Received wrong size (%u) packet in %s"), lenPacket, __FUNCTION__);
	}

	CUpDownClient* buddy = theApp.clientlist->GetBuddy();
	if( buddy != NULL ) {
		CMemFile bio((byte*)packetData, lenPacket);
		CUInt128 check = bio.ReadUInt128();
		// JOHNTODO: Filter bad buddies
		//CUInt128 bud(buddy->GetBuddyID());
		CUInt128 file = bio.ReadUInt128();
		uint16 tcp = bio.ReadUInt16();
		CMemFile bio2(lenPacket+6);
		bio2.WriteUInt128(check);
		bio2.WriteUInt128(file);
		bio2.WriteUInt32(ip);
		bio2.WriteUInt16(tcp);
		CPacket* packet = new CPacket(&bio2, OP_EMULEPROT, OP_CALLBACK);
		if( buddy->GetSocket() ) {
			AddDebugLogLineM(false, logClientKadUDP, CFormat(wxT("KadCallback %s")) % Uint32_16toStringIP_Port(ip, port));
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
          	buddy->GetSocket()->SendPacket(packet);
		} else {
			wxASSERT(0);
		}
	}
}

void CKademliaUDPListener::sendPacket(const byte *data, uint32 lenData, uint32 destinationHost, uint16 destinationPort)
{
	//This is temp.. The entire Kad code will be rewritten using CMemFile and send a Packet object directly.
	CMemFile mem_data((byte*)data+2,lenData-2);	
	sendPacket(&mem_data,data[1],destinationHost, destinationPort);
}

void CKademliaUDPListener::sendPacket(const byte *data, uint32 lenData, byte opcode, uint32 destinationHost, uint16 destinationPort)
{
	CMemFile mem_data((byte*)data,lenData);
	sendPacket(&mem_data,opcode,destinationHost, destinationPort);
}

void CKademliaUDPListener::sendPacket(CMemFile *data, byte opcode, uint32 destinationHost, uint16 destinationPort)
{
	CPacket* packet = new CPacket(data, OP_KADEMLIAHEADER, opcode);
	if( packet->GetPacketSize() > 200 ) {
		packet->PackPacket();
	}
	theStats::AddUpOverheadKad(packet->GetPacketSize());
	theApp.clientudp->SendPacket(packet,wxUINT32_SWAP_ALWAYS(destinationHost), destinationPort);
}

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include "Types.h"

#include <zlib.h>		// Needed for inflateEnd
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "IPFilter.h"		// Needed for CIPFilter
#include "ServerConnect.h"	// Needed for CServerConnect
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "Server.h"		// Needed for CServer
#include "Preferences.h"	// Needed for CPreferences
#include "MemFile.h"		// Needed for CMemFile
#include "Packet.h"		// Needed for CPacket
#include "OtherStructs.h"	// Needed for Requested_Block_Struct
#include "Friend.h"		// Needed for CFriend
#include "ClientList.h"		// Needed for CClientList
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "OPCodes.h"		// Needed for OP_*
#include "updownclient.h"	// Needed for CUpDownClient
#include "FriendList.h"		// Needed for CFriendList
#include "Statistics.h"		// Needed for theStats
#include "Format.h"		// Needed for CFormat
#include "ClientUDPSocket.h"
#include "Logger.h"
#include "DataToText.h"		// Needed for GetSoftName()
#include "OtherFunctions.h"

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "kademlia/kademlia/Prefs.h"
#include "kademlia/kademlia/Search.h"


//#define __PACKET_DEBUG__


// some client testing variables
static wxString crash_name = wxT("[Invalid User Name]");
static wxString empty_name = wxT("[Empty User Name]");

//	members of CUpDownClient
//	which are used by down and uploading functions


CUpDownClient::CUpDownClient(CClientReqSocket* sender)
{
#ifdef __DEBUG__
	m_socket = NULL;
	SetSocket(sender);
#else
	m_socket = sender;
#endif
	Init();
}

CUpDownClient::CUpDownClient(uint16 in_port, uint32 in_userid, uint32 in_serverip, uint16 in_serverport, CPartFile* in_reqfile, bool ed2kID, bool checkfriend)
{
	m_socket = NULL;
	Init();
	m_nUserPort = in_port;

	if(ed2kID && !IsLowID(in_userid)) {
		SetUserIDHybrid( ENDIAN_NTOHL(in_userid) );
	} else {
		SetUserIDHybrid( in_userid);
	}
	
	//If highID and ED2K source, incoming ID and IP are equal..
	//If highID and Kad source, incoming IP needs ntohl for the IP

	if (!HasLowID()) {
		if (ed2kID) {
			m_nConnectIP = in_userid;
		} else {
			m_nConnectIP = ENDIAN_NTOHL(in_userid);
		}
		// Will be on right endianess now
		m_FullUserIP = Uint32toStringIP(m_nConnectIP);
	}

	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
	SetRequestFile( in_reqfile );
	ReGetClientSoft();

	if (checkfriend) {
		CFriend* m_Friend;
		if ((m_Friend = theApp.friendlist->FindFriend(NULL, m_dwUserIP, m_nUserPort)) != NULL){
			m_Friend->LinkClient(this);
		} else{
			// avoid that an unwanted client instance keeps a friend slot
			m_bFriendSlot = false;
		}	
	}
	
}

void CUpDownClient::Init()
{
	Extended_aMule_SO = 0;
	m_bAddNextConnect = false;
	credits = 0;
	m_byChatstate = MS_NONE;
	m_nKadState = KS_NONE;
	m_cShowDR = 0;
	m_reqfile = NULL;	 // No file required yet
	m_nTransferredUp = 0;
	m_cSendblock = 0;
	m_cAsked = 0;
	msReceivedPrev = 0;
	kBpsDown = 0.0;
	fDownAvgFilter = 1.0;
	bytesReceivedCycle = 0;
	m_nServerPort = 0;
	m_iFileListRequested = 0;
	m_dwLastUpRequest = 0;
	m_bEmuleProtocol = false;
	m_bCompleteSource = false;
	m_bFriendSlot = false;
	m_bCommentDirty = false;
	m_bReaskPending = false;
	m_bUDPPending = false;
	m_nUserPort = 0;
	m_nPartCount = 0;
	m_dwLastAskedTime = 0;
	m_nDownloadState = DS_NONE;
	m_dwUploadTime = 0;
	m_nTransferedDown = 0;
	m_nUploadState = US_NONE;
	m_dwLastBlockReceived = 0;
	m_bUnicodeSupport = false;

	m_fSentOutOfPartReqs = 0;
	m_nCurQueueSessionPayloadUp = 0;
	m_addedPayloadQueueSession = 0;
	m_nUpDatarate = 0;
	m_nSumForAvgUpDataRate = 0;

	m_nRemoteQueueRank = 0;
	m_nOldRemoteQueueRank = 0;
	m_dwLastSourceRequest = 0;
	m_dwLastSourceAnswer = 0;
	m_dwLastAskedForSources = 0;

	m_SecureIdentState = IS_UNAVAILABLE;
	m_dwLastSignatureIP = 0;

	m_byInfopacketsReceived = IP_NONE;

	m_bIsHybrid = false;
	m_bIsML = false;
	m_Friend = NULL;
	m_iRating = 0;
	m_nCurSessionUp = 0;
	m_clientSoft=SO_UNKNOWN;

	m_bRemoteQueueFull = false;
	m_HasValidHash = false;
	SetWaitStartTime();

	m_fHashsetRequesting = 0;
	m_fSharedDirectories = 0;
	m_lastPartAsked = 0xffff;
	m_nUpCompleteSourcesCount= 0;
	m_lastRefreshedDLDisplay = 0;
	m_SoftLen = 0;
	m_bHelloAnswerPending = false;
	m_fSentCancelTransfer = 0;
	m_Aggressiveness = 0;
	m_LastFileRequest = 0;

	m_clientState = CS_NEW;

	ClearHelloProperties();

	m_pReqFileAICHHash = NULL;
	m_fSupportsAICH = 0;
	m_fAICHRequested = 0;

	m_dwUserIP = 0;
	m_nConnectIP = 0;
	m_dwServerIP = 0;

	m_fNeedOurPublicIP = false;
	m_bHashsetRequested = false;

	m_nLastBlockOffset = 0;

	m_bMsgFiltered = false;

	m_uploadingfile = NULL;

	m_OSInfo_sent = false;
	
	/* Kad stuff */
	SetBuddyID(NULL);
	m_nBuddyIP = 0;
	m_nBuddyPort = 0;	
	m_nUserIDHybrid = 0;

	m_nSourceFrom = SF_NONE;

	if (m_socket) {
		amuleIPV4Address address;
		m_socket->GetPeer(address);
		m_FullUserIP = address.IPAddress();
		SetIP(StringIPtoUint32(m_FullUserIP));
	} else {
		SetIP(0);
	}

	/* Statistics */
	m_lastClientSoft = (uint32)(-1);
	m_lastClientVersion = 0;
}


CUpDownClient::~CUpDownClient()
{
	if (m_lastClientSoft == SO_UNKNOWN) {
		theStats::RemoveUnknownClient();
	} else if (m_lastClientSoft != (uint32)(-1)) {
		theStats::RemoveKnownClient(m_lastClientSoft, m_lastClientVersion, m_lastOSInfo);
	}
		
	// Indicate that we are not anymore on stats
	m_lastClientSoft = (uint32)(-1);
	

	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
	}

	//theApp.clientlist->RemoveClient(this, wxT("Destructing client object"));
	
	if (m_Friend) {
		m_Friend->UnLinkClient();
		Notify_ChatRefreshFriend(m_Friend->GetIP(), m_Friend->GetPort(), wxEmptyString);
		m_Friend = NULL;
	}

	// The socket should have been removed in Safe_Delete, but it
	// doesn't hurt to have an extra check.
	if (m_socket) {
		m_socket->SetClient( NULL );
		m_socket->Safe_Delete();
		// We're going down anyway....
		m_socket->Destroy();
		// Paranoia
		SetSocket(NULL);
	}


	ClearUploadBlockRequests();
	ClearDownloadBlockRequests();


	for (POSITION pos =m_WaitingPackets_list.GetHeadPosition();pos != 0; ) {
		delete m_WaitingPackets_list.GetNext(pos);
	}


	if (m_iRating>0 || !m_strComment.IsEmpty()) {
		m_iRating = 0;
		m_strComment.Clear();
		if (m_reqfile) {
			m_reqfile->UpdateFileRatingCommentAvail();
		}
	}

	// Ensure that source-counts gets updated in case
	// of a source not on the download-queue
	SetRequestFile( NULL );

	SetUploadFileID(NULL);

	if (m_pReqFileAICHHash != NULL) {
		delete m_pReqFileAICHHash;
		m_pReqFileAICHHash = NULL;
	}
}

void CUpDownClient::ClearHelloProperties()
{
	m_nUDPPort = 0;
	m_byUDPVer = 0;
	m_byDataCompVer = 0;
	m_byEmuleVersion = 0;
	m_bySourceExchangeVer = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_byCompatibleClient = 0;
	m_nKadPort = 0;
	m_bySupportSecIdent = 0;
	m_bSupportsPreview = 0;
	m_nClientVersion = 0;
	m_fSharedDirectories = 0;
	m_bMultiPacket = 0;
	m_SoftLen = 0;
	m_fOsInfoSupport = 0;
	SecIdentSupRec = 0;
	m_byKadVersion = 0;
}

bool CUpDownClient::ProcessHelloPacket(const char *pachPacket, uint32 nSize)
{
	const CMemFile data((byte*)pachPacket,nSize);
	uint8 hashsize = data.ReadUInt8();
	if ( 16 != hashsize ) {
		/*
		 * Hint: We can not accept other sizes here because:
		 *       - the magic number is spread all over the source
		 *       - the answer packet lacks the size field
		 */
		throw wxString(wxT("Invalid Hello packet: Other userhash sizes than 16 are not implemented"));
	}
	// eMule 0.42: reset all client properties; a client may not send a particular emule tag any longer
	ClearHelloProperties();

	return ProcessHelloTypePacket(data);
}

void CUpDownClient::Safe_Delete()
{
	// Because we are delaying the deletion, we might end up trying to delete
	// it twice, however, this is normal and shouldn't trigger any failures
	if ( m_clientState == CS_DYING ) {
		return;
	}

 	m_clientState = CS_DYING;

	// Close the socket to avoid any more connections and related events
	if ( m_socket ) {
		m_socket->SetClient( NULL );
		m_socket->Safe_Delete();
		// Paranoia
		SetSocket(NULL);
	}

	// Schedule the client for deletion if we still have the clientlist
	if ( theApp.clientlist ) {
		theApp.clientlist->AddToDeleteQueue( this );
	} else {
		delete this;
	}
}


bool CUpDownClient::ProcessHelloAnswer(const char *pachPacket, uint32 nSize)
{
	const CMemFile data((byte*)pachPacket,nSize);
	bool bIsMule = ProcessHelloTypePacket(data);
	m_bHelloAnswerPending = false;
	return bIsMule;
}

bool CUpDownClient::ProcessHelloTypePacket(const CMemFile& data)
{

	m_bIsHybrid = false;
	m_bIsML = false;
	m_fNoViewSharedFiles = 0;
	m_bUnicodeSupport = false;
	uint32 dwEmuleTags = 0;

	CMD4Hash hash = data.ReadHash();
	SetUserHash( hash );
	SetUserIDHybrid( data.ReadUInt32() );
	uint16 nUserPort = data.ReadUInt16(); // hmm clientport is sent twice - why?
	uint32 tagcount = data.ReadUInt32();
	for (uint32 i = 0;i < tagcount; i++){
		CTag temptag(data, true);
		switch(temptag.GetNameID()){
			case CT_NAME:
				m_Username = temptag.GetStr();
				break;
				
			case CT_VERSION:
				m_nClientVersion = temptag.GetInt();
				break;
				
			case ET_MOD_VERSION:
				if (temptag.IsStr()) {
					m_strModVersion = temptag.GetStr();
				} else if (temptag.IsInt()) {
					m_strModVersion = wxString::Format(wxT("ModID=%u"), temptag.GetInt());
				} else {
					m_strModVersion = wxT("ModID=<Unknown>");
				}

				break;
				
			case CT_PORT:
				nUserPort = temptag.GetInt();
				break;
				
			case CT_EMULE_UDPPORTS:
				// 16 KAD Port
				// 16 UDP Port
				m_nKadPort = (temptag.GetInt() >> 16) & 0xFFFF;
				m_nUDPPort = temptag.GetInt() & 0xFFFF;
				dwEmuleTags |= 1;
				#ifdef __PACKET_DEBUG__
				printf("Hello type packet processing with eMule ports UDP=%i KAD=%i\n",m_nUDPPort,m_nKadPort);
				#endif
				break;
				
			case CT_EMULE_BUDDYIP:
				// 32 BUDDY IP
				m_nBuddyIP = temptag.GetInt();
				#ifdef __PACKET_DEBUG__
				printf("Hello type packet processing with eMule BuddyIP=%u (%s)\n",m_nBuddyIP, (const char*)unicode2char(Uint32toStringIP(m_nBuddyIP)));
				#endif
				break;				
				
			case CT_EMULE_BUDDYUDP:
				// 16 --Reserved for future use--
				// 16 BUDDY Port
				m_nBuddyPort = (uint16)temptag.GetInt();
				#ifdef __PACKET_DEBUG__
				printf("Hello type packet processing with eMule BuddyPort=%u\n",m_nBuddyPort);
				#endif
				break;
				
			case CT_EMULE_MISCOPTIONS1: {
				//  3 AICH Version (0 = not supported)
				//  1 Unicode
				//  4 UDP version
				//  4 Data compression version
				//  4 Secure Ident
				//  4 Source Exchange
				//  4 Ext. Requests
				//  4 Comments
				//	 1 PeerChache supported
				//	 1 No 'View Shared Files' supported
				//	 1 MultiPacket
				//  1 Preview
				uint32 flags = temptag.GetInt();
				m_fSupportsAICH			= (flags >> (4*7+1)) & 0x07;
				m_bUnicodeSupport		= (flags >> 4*7) & 0x01;
				m_byUDPVer				= (flags >> 4*6) & 0x0f;
				m_byDataCompVer			= (flags >> 4*5) & 0x0f;
				m_bySupportSecIdent		= (flags >> 4*4) & 0x0f;
				m_bySourceExchangeVer	= (flags >> 4*3) & 0x0f;
				m_byExtendedRequestsVer	= (flags >> 4*2) & 0x0f;
				m_byAcceptCommentVer	= (flags >> 4*1) & 0x0f;
				m_fNoViewSharedFiles	= (flags >> 1*2) & 0x01;
				m_bMultiPacket			= (flags >> 1*1) & 0x01;
				m_fSupportsPreview		= (flags >> 1*0) & 0x01;
				dwEmuleTags |= 2;
				#ifdef __PACKET_DEBUG__
				printf("Hello type packet processing with eMule Misc Options:\n");
				printf("m_byUDPVer = %i\n",m_byUDPVer);
				printf("m_byDataCompVer = %i\n",m_byDataCompVer);
				printf("m_bySupportSecIdent = %i\n",m_bySupportSecIdent);
				printf("m_bySourceExchangeVer = %i\n",m_bySourceExchangeVer);
				printf("m_byExtendedRequestsVer = %i\n",m_byExtendedRequestsVer);
				printf("m_byAcceptCommentVer = %i\n",m_byAcceptCommentVer);
				printf("m_fNoViewSharedFiles = %i\n",m_fNoViewSharedFiles);
				printf("m_bMultiPacket = %i\n",m_bMultiPacket);
				printf("m_fSupportsPreview = %i\n",m_fSharedDirectories);
				printf("That's all.\n");
				#endif
				SecIdentSupRec +=  1;
				break;
			}

			case CT_EMULE_MISCOPTIONS2:
				//	28 Reserved
				//   4 Kad Version
				m_byKadVersion			= (temptag.GetInt() >>  0) & 0x0f;
				dwEmuleTags |= 8;
				#ifdef __PACKET_DEBUG__
				printf("Hello type packet processing with eMule Misc Options 2:\n");
				printf("  KadVersion = %u\n" , m_byKadVersion );
				printf("That's all.\n");
				#endif
				break;
				
			// Special tag fo Compat. Clients Misc options.
			case CT_EMULECOMPAT_OPTIONS:
				//  1 Operative System Info
				m_fOsInfoSupport		= (temptag.GetInt() >> 1*0) & 0x01;
				break;
				
			case CT_EMULE_VERSION:
				//  8 Compatible Client ID
				//  7 Mjr Version (Doesn't really matter..)
				//  7 Min Version (Only need 0-99)
				//  3 Upd Version (Only need 0-5)
				//  7 Bld Version (Only need 0-99)
				m_byCompatibleClient = (temptag.GetInt() >> 24);
				m_nClientVersion = temptag.GetInt() & 0x00ffffff;
				m_byEmuleVersion = 0x99;
				m_fSharedDirectories = 1;
				dwEmuleTags |= 4;
				break;				
		}
	}

	m_nUserPort = nUserPort;
	m_dwServerIP = data.ReadUInt32();
	m_nServerPort = data.ReadUInt16();
	// Hybrid now has an extra uint32.. What is it for?
	// Also, many clients seem to send an extra 6? These are not eDonkeys or Hybrids..
	if ( data.GetLength() - data.GetPosition() == sizeof(uint32) ) {
		uint32 test = data.ReadUInt32();
		/*if (test == 'KDLM') below kdlm is converted to ascii values.
		This fixes a warning with gcc 3.4.
		K=4b D=44 L=4c M=4d
		*/
		if (test == 0x4b444c4d) { //if it's == "KDLM"
			m_bIsML=true;
		} else{
			m_bIsHybrid = true;
			m_fSharedDirectories = 1;
		}
	}

	if (m_socket) {
		amuleIPV4Address address;
		m_socket->GetPeer(address);
		m_FullUserIP = address.IPAddress();
		SetIP(StringIPtoUint32(m_FullUserIP));
	} else {
		throw wxString(wxT("Huh, socket failure. Avoided crash this time."));
	}

	if (thePrefs::AddServersFromClient()) {
		CServer* addsrv = new CServer(m_nServerPort, Uint32toStringIP(m_dwServerIP));
		addsrv->SetListName(addsrv->GetAddress());
		if (!theApp.AddServer(addsrv)) {
				delete addsrv;
		}
	}

	//(a)If this is a highID user, store the ID in the Hybrid format.
	//(b)Some older clients will not send a ID, these client are HighID users that are not connected to a server.
	//(c)Kad users with a *.*.*.0 IPs will look like a lowID user they are actually a highID user.. They can be detected easily
	//because they will send a ID that is the same as their IP..
	if(!HasLowID() || m_nUserIDHybrid == 0 || m_nUserIDHybrid == m_dwUserIP )  {
		SetUserIDHybrid(ENDIAN_NTOHL(m_dwUserIP));
	}
	
	// get client credits
	CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(m_UserHash);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			AddDebugLogLineM( false, logClient, CFormat( wxT("Client: %s (%s) Banreason: Userhash changed (Found in TrackedClientsList)") ) % GetUserName() % GetFullIP() );
			Ban();
		}
	} else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		AddDebugLogLineM( false, logClient, CFormat( wxT("Client: %s (%s) Banreason: Userhash changed") ) % GetUserName() % GetFullIP() );
		Ban();
	}

	if ((m_Friend = theApp.friendlist->FindFriend(m_UserHash, m_dwUserIP, m_nUserPort)) != NULL){
		m_Friend->LinkClient(this);
	} else{
		// avoid that an unwanted client instance keeps a friend slot
		SetFriendSlot(false);
	}


	ReGetClientSoft();

	m_byInfopacketsReceived |= IP_EDONKEYPROTPACK;

	// check if at least CT_EMULEVERSION was received, all other tags are optional
	bool bIsMule = (dwEmuleTags & 0x04) == 0x04;
	if (bIsMule) {
		m_bEmuleProtocol = true;
		m_byInfopacketsReceived |= IP_EMULEPROTPACK;
	}


	#ifdef __COMPILE_KAD__
	if( GetKadPort() ) {
		Kademlia::CKademlia::bootstrap(ENDIAN_NTOHL(GetIP()), GetKadPort());
	}
	#endif

	return bIsMule;
}


bool CUpDownClient::SendHelloPacket() {

	if (m_socket == NULL){
		wxASSERT(0);
		return true;
	}

	// if IP is filtered, dont greet him but disconnect...
	amuleIPV4Address address;
	m_socket->GetPeer(address);
	if ( theApp.ipfilter->IsFiltered(StringIPtoUint32(address.IPAddress()))) {
		if (Disconnected(wxT("IPFilter"))) {
			Safe_Delete();
			return false;
		}
		return true;
	}

	CMemFile data(128);
	data.WriteUInt8(16); // size of userhash
	SendHelloTypePacket(&data);
	
	CPacket* packet = new CPacket(&data);
	packet->SetOpCode(OP_HELLO);
	theStats::AddUpOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true);
	m_bHelloAnswerPending = true;
	AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_HELLO to ") + GetFullIP() );
	return true;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer, bool OSInfo) {

	if (m_socket == NULL){
		wxASSERT(0);
		return;
	}

	CPacket* packet = NULL;
	CMemFile data;

	data.WriteUInt8(CURRENT_VERSION_SHORT);
	
	if (OSInfo) {
		
		// Special MuleInfo packet for clients supporting it.
		// This means aMule >= 2.0.0 and Hydranode
		
		// Violently mark it as special Mule Info packet
		// Sending this makes non-supporting-osinfo clients to refuse to read this
		// packet. Anyway, this packet should NEVER get to non-supporting clients.
		
		data.WriteUInt8(/*EMULE_PROTOCOL*/ 0xFF);		

		data.WriteUInt32(1); // One Tag (OS_INFO)

		CTag tag1(ET_OS_INFO,theApp.GetOSType());
		tag1.WriteTagToFile(&data);
		
		m_OSInfo_sent = true; // So we don't send it again

	} else {

		// Normal MuleInfo packet

		data.WriteUInt8(EMULE_PROTOCOL);

		// Tag number
		data.WriteUInt32(9);

		CTag tag1(ET_COMPRESSION,1);
		tag1.WriteTagToFile(&data);
		CTag tag2(ET_UDPVER,4);
		tag2.WriteTagToFile(&data);
		CTag tag3(ET_UDPPORT, thePrefs::GetEffectiveUDPPort());
		tag3.WriteTagToFile(&data);
		CTag tag4(ET_SOURCEEXCHANGE,3);
		tag4.WriteTagToFile(&data);
		CTag tag5(ET_COMMENTS,1);
		tag5.WriteTagToFile(&data);
		CTag tag6(ET_EXTENDEDREQUEST,2);
		tag6.WriteTagToFile(&data);

		uint32 dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);
		// Kry - Needs the preview code from eMule
		/*
		// set 'Preview supported' only if 'View Shared Files' allowed
		if (thePrefs::CanSeeShares() != vsfaNobody) {
			dwTagValue |= 128;
		}
		*/
		CTag tag7(ET_FEATURES, dwTagValue);
		tag7.WriteTagToFile(&data);

		CTag tag8(ET_COMPATIBLECLIENT,SO_AMULE);
		tag8.WriteTagToFile(&data);
	
		// Support for tag ET_MOD_VERSION
		wxString mod_name(MOD_VERSION_LONG);
		CTag tag9(ET_MOD_VERSION, mod_name);
		tag9.WriteTagToFile(&data);
		// Maella end
	
	}

	packet = new CPacket(&data,OP_EMULEPROT);
	
	if (!bAnswer) {
		packet->SetOpCode(OP_EMULEINFO);
	} else {
		packet->SetOpCode(OP_EMULEINFOANSWER);
	}
	
	if (m_socket) {
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		
		if (!bAnswer) {
			if (!OSInfo) {
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_EMULEINFO to ") + GetFullIP() );
			} else {
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_EMULEINFO/OS_INFO to ") + GetFullIP() );
			}
		} else {
			AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_EMULEINFOANSWER to ") + GetFullIP() );
		}
	}
}

bool CUpDownClient::ProcessMuleInfoPacket(const char* pachPacket, uint32 nSize)
{
	uint8 protocol_version;

	const CMemFile data((byte*)pachPacket,nSize);

	// The version number part of this packet will soon be useless since
	// it is only able to go to v.99. Why the version is a uint8 and why
	// it was not done as a tag like the eDonkey hello packet is not known.
	// Therefore, sooner or later, we are going to have to switch over to 
	// using the eDonkey hello packet to set the version. No sense making
	// a third value sent for versions.
	uint8 mule_version = data.ReadUInt8();
	protocol_version = data.ReadUInt8();
	uint32 tagcount = data.ReadUInt32();
	if (protocol_version == 0xFF) {
		// OS Info supporting clients sending a recycled Mule info packet
		for (uint32 i = 0;i < tagcount; i++){
			CTag temptag(data, true);
			switch(temptag.GetNameID()){
				case ET_OS_INFO:
					// Special tag, only supporting clients (aMule/Hydranode)
					// It was recycled from a mod's tag, so if the other side
					// is not supporting OS Info, we're seriously fucked up :)					
					m_sClientOSInfo = temptag.GetStr();

					// If we didn't send our OSInfo to this client, just send it
					if (!m_OSInfo_sent) {
						SendMuleInfoPacket(false,true);
					}

					UpdateStats();

					break;	

				// Your ad... er... I mean TAG, here

				default:
					break;
			}
		}
	} else {
		// Old eMule sending tags
		m_byCompatibleClient = 0;
		m_byEmuleVersion = mule_version;

		if( m_byEmuleVersion == 0x2B ) {
			m_byEmuleVersion = 0x22;
		}

		if (!(m_bEmuleProtocol = (protocol_version == EMULE_PROTOCOL))) {
			return false;	
		}
		
		for (uint32 i = 0;i < tagcount; i++){
			CTag temptag(data, false);
			switch(temptag.GetNameID()){
				case ET_COMPRESSION:
					// Bits 31- 8: 0 - reserved
					// Bits  7- 0: data compression version
					m_byDataCompVer = temptag.GetInt();
					break;
					
				case ET_UDPPORT:
					// Bits 31-16: 0 - reserved
					// Bits 15- 0: UDP port
					m_nUDPPort = temptag.GetInt();
					break;
					
				case ET_UDPVER:
					// Bits 31- 8: 0 - reserved
					// Bits  7- 0: UDP protocol version
					m_byUDPVer = temptag.GetInt();
					break;
					
				case ET_SOURCEEXCHANGE:
					// Bits 31- 8: 0 - reserved
					// Bits  7- 0: source exchange protocol version
					m_bySourceExchangeVer = temptag.GetInt();
					break;
					
				case ET_COMMENTS:
					// Bits 31- 8: 0 - reserved
					// Bits  7- 0: comments version
					m_byAcceptCommentVer = temptag.GetInt();
					break;
					
				case ET_EXTENDEDREQUEST:
					// Bits 31- 8: 0 - reserved
					// Bits  7- 0: extended requests version
					m_byExtendedRequestsVer = temptag.GetInt();
					break;
					
				case ET_COMPATIBLECLIENT:
					// Bits 31- 8: 0 - reserved
					// Bits  7- 0: compatible client ID
					m_byCompatibleClient = temptag.GetInt();
					break;

				case ET_FEATURES:
					// Bits 31- 8: 0 - reserved
					// Bit	    7: Preview
					// Bit   6- 0: secure identification
					m_bySupportSecIdent = temptag.GetInt() & 3;
					m_bSupportsPreview = (temptag.GetInt() & 128) > 0;
					SecIdentSupRec +=  2;
					break;

				case ET_MOD_VERSION:
					if (temptag.IsStr()) {
						m_strModVersion = temptag.GetStr();
					} else if (temptag.IsInt()) {
						m_strModVersion = wxString::Format(wxT("ModID=%u"), temptag.GetInt());
					} else {
						m_strModVersion = wxT("ModID=<Unknown>");
					}

					break;

				default:
					AddDebugLogLineM( false, logPacketErrors,
						CFormat( wxT("Unknown Mule tag (%s) from client: %s") )
							% temptag.GetFullInfo()
							% GetClientFullInfo()
					);

					break;
			}
		}				

		if( m_byDataCompVer == 0 ){
			m_bySourceExchangeVer = 0;
			m_byExtendedRequestsVer = 0;
			m_byAcceptCommentVer = 0;
			m_nUDPPort = 0;
		}

		//implicitly supported options by older clients
		//in the future do not use version to guess about new features
		if(m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x22) {
			m_byUDPVer = 1;
		}

		if(m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x21) {
			m_bySourceExchangeVer = 1;
		}

		if(m_byEmuleVersion == 0x24) {
			m_byAcceptCommentVer = 1;
		}

		// Shared directories are requested from eMule 0.28+ because eMule 0.27 has a bug in
		// the OP_ASKSHAREDFILESDIR handler, which does not return the shared files for a
		// directory which has a trailing backslash.
		if(m_byEmuleVersion >= 0x28 && !m_bIsML) {// MLdonkey currently does not support shared directories
				m_fSharedDirectories = 1;
		}

		ReGetClientSoft();

		m_byInfopacketsReceived |= IP_EMULEPROTPACK;		
	}

	return (protocol_version == 0xFF); // This was a OS_Info?
}


void CUpDownClient::SendHelloAnswer()
{
	if (m_socket == NULL){
		wxASSERT(0);
		return;
	}

	CMemFile data(128);
	SendHelloTypePacket(&data);
	CPacket* packet = new CPacket(&data);
	packet->SetOpCode(OP_HELLOANSWER);

	theStats::AddUpOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true);

	AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_HELLOANSWER to ") + GetFullIP() );
}


void CUpDownClient::SendHelloTypePacket(CMemFile* data)
{
	data->WriteHash(thePrefs::GetUserHash());
	data->WriteUInt32(theApp.serverconnect->GetClientID());
	data->WriteUInt16(thePrefs::GetPort());

	uint32 tagcount = 6;
	#ifdef __COMPILE_KAD__
	if( theApp.clientlist->GetBuddy() && theApp.IsFirewalled() ) {
		tagcount += 2;
	}
	tagcount ++; // eMule misc flags 2 (kad version)
	#endif
	
	#ifdef __CVS__
	// Kry - This is the tagcount!!! Be sure to update it!!
	// Last update: CT_EMULECOMPAT_OPTIONS included
	data->WriteUInt32(tagcount + 1);
	#else
	data->WriteUInt32(tagcount);  // NO MOD_VERSION
	#endif


	CTag tagname(CT_NAME,thePrefs::GetUserNick());
	tagname.WriteTagToFile(data, utf8strRaw);

	CTag tagversion(CT_VERSION,EDONKEYVERSION);
	tagversion.WriteTagToFile(data);
	// eMule UDP Ports

	uint32 kadUDPPort = 0;
	#ifdef __COMPILE_KAD__
	if(Kademlia::CKademlia::isConnected()) {
		kadUDPPort = (uint32)thePrefs::GetEffectiveUDPPort();
	}
	#endif
	CTag tagUdpPorts(CT_EMULE_UDPPORTS,
				(kadUDPPort									<< 16) |
				((uint32)thePrefs::GetEffectiveUDPPort()	     ) );
	tagUdpPorts.WriteTagToFile(data);

	#ifdef __COMPILE_KAD__
	if( theApp.clientlist->GetBuddy() && theApp.IsFirewalled() ) {
		CTag tagBuddyIP(CT_EMULE_BUDDYIP, theApp.clientlist->GetBuddy()->GetIP() ); 
		tagBuddyIP.WriteTagToFile(data);
	
		CTag tagBuddyPort(CT_EMULE_BUDDYUDP, 
//					( RESERVED												)
					((uint32)theApp.clientlist->GetBuddy()->GetUDPPort()  ) 
					);
		tagBuddyPort.WriteTagToFile(data);
	}	
	#endif
	
	// aMule Version
	CTag tagMuleVersion(CT_EMULE_VERSION,
				(SO_AMULE	<< 24) |
				make_full_ed2k_version(VERSION_MJR, VERSION_MIN, VERSION_UPDATE)
				// | (RESERVED			     )
				);
	tagMuleVersion.WriteTagToFile(data);


	// eMule Misc. Options #1
	const uint32 uUdpVer				= 4;
	const uint32 uDataCompVer			= 1;
	const uint32 uSupportSecIdent		= theApp.clientcredits->CryptoAvailable() ? 3 : 0;
	const uint32 uSourceExchangeVer	= 3; 
	const uint32 uExtendedRequestsVer	= 2;
	const uint32 uAcceptCommentVer	= 1;
	const uint32 uNoViewSharedFiles	= (thePrefs::CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
	const uint32 uMultiPacket			= 1;
	const uint32 uSupportPreview		= 0; // No network preview at all.
	const uint32 uPeerCache			= 0; // No peercache for aMule, baby
	const uint32 uUnicodeSupport		= 
#if wxUSE_UNICODE	
												1; 
#else
												0; 
#endif
	const uint32 nAICHVer				= 1; // AICH is ENABLED right now.

	CTag tagMisOptions(CT_EMULE_MISCOPTIONS1,
				(nAICHVer				<< ((4*7)+1)) |
				(uUnicodeSupport		<< 4*7) |
				(uUdpVer				<< 4*6) |
				(uDataCompVer			<< 4*5) |
				(uSupportSecIdent		<< 4*4) |
				(uSourceExchangeVer		<< 4*3) |
				(uExtendedRequestsVer	<< 4*2) |
				(uAcceptCommentVer		<< 4*1) |
				(uPeerCache				<< 1*3) |
				(uNoViewSharedFiles		<< 1*2) |
				(uMultiPacket			<< 1*1) |
				(uSupportPreview		<< 1*0) );
	tagMisOptions.WriteTagToFile(data);

#ifdef __COMPILE_KAD__
	// eMule Misc. Options #2
	const uint32 uKadVersion			= 1;
	CTag tagMisOptions2(CT_EMULE_MISCOPTIONS2, 
//				(RESERVED				     ) 
				(uKadVersion			<<  0) 
				);
	tagMisOptions2.WriteTagToFile(data);
#endif


	const uint32 nOSInfoSupport			= 1; // We support OS_INFO
	
	CTag tagMisCompatOptions(CT_EMULECOMPAT_OPTIONS,
				(nOSInfoSupport		<< 1*0) );
	
	tagMisCompatOptions.WriteTagToFile(data);

#ifdef __CVS__
	wxString mod_name(MOD_VERSION_LONG);
	CTag tagModName(ET_MOD_VERSION, mod_name);
	tagModName.WriteTagToFile(data);
#endif

	uint32 dwIP = 0;
	uint16 nPort = 0;
	if (theApp.IsConnectedED2K()) {
		dwIP = theApp.serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp.serverconnect->GetCurrentServer()->GetPort();
	}
	data->WriteUInt32(dwIP);
	data->WriteUInt16(nPort);
}


void CUpDownClient::ProcessMuleCommentPacket(const char *pachPacket, uint32 nSize)
{
	if (!m_reqfile) {
		throw CInvalidPacket(wxT("Comment packet for unknown file"));
	}

	if (!m_reqfile->IsPartFile()) {
		throw CInvalidPacket(wxT("Comment packet for completed file"));
	}

	const CMemFile data((byte*)pachPacket, nSize);

	m_iRating = data.ReadUInt8();
	m_reqfile->SetHasRating(true);
		
	AddDebugLogLineM( false, logClient, wxString(wxT("Rating for file '")) << m_clientFilename << wxT("' received: ") << m_iRating);

	// The comment is unicoded, with a uin32 len and safe read 
	// (won't break if string size is < than advertised len)
	m_strComment = data.ReadString(GetUnicodeSupport(), 4 /* bytes (it's a uint32)*/, true);

	AddDebugLogLineM( false, logClient, wxString(wxT("Description for file '")) << m_clientFilename << wxT("' received: ") << m_strComment);

	m_reqfile->SetHasComment(true);
	// Update file rating
	m_reqfile->UpdateFileRatingCommentAvail();

	if (!m_strComment.IsEmpty() || m_iRating > 0) {
		m_reqfile->UpdateFileRatingCommentAvail();
		Notify_DownloadCtrlUpdateItem(m_reqfile);
	}
}

void CUpDownClient::ClearDownloadBlockRequests()
{
	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;){
		auto_ptr<Requested_Block_Struct> cur_block(m_DownloadBlocks_list.GetNext(pos));
		if (m_reqfile){
			m_reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
		}
	}
	m_DownloadBlocks_list.RemoveAll();

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;){
		auto_ptr<Pending_Block_Struct> pending(m_PendingBlocks_list.GetNext(pos));
		if (m_reqfile){
			m_reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
		}

		delete pending->block;
		// Not always allocated
		if (pending->zStream){
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
	}
	m_PendingBlocks_list.RemoveAll();
}

bool CUpDownClient::Disconnected(const wxString& strReason, bool bFromSocket){

	// Kad reviewed
	
	//If this is a KAD client object, just delete it!
	SetKadState(KS_NONE);
	
	if (GetUploadState() == US_UPLOADING) {
		theApp.uploadqueue->RemoveFromUploadQueue(this);
	}

	if (GetDownloadState() == DS_DOWNLOADING) {
		SetDownloadState(DS_ONQUEUE);
	} else{
		// ensure that all possible block requests are removed from the partfile
		ClearDownloadBlockRequests();

		if ( GetDownloadState() == DS_CONNECTED ){
			theApp.clientlist->AddDeadSource(this);
			theApp.downloadqueue->RemoveSource(this);
	    }
	}

	// we had still an AICH request pending, handle it
	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
	}

	// The remote client does not have to answer with OP_HASHSETANSWER *immediatly*
	// after we've sent OP_HASHSETREQUEST. It may occure that a (buggy) remote client
	// is sending use another OP_FILESTATUS which would let us change to DL-state to DS_ONQUEUE.
	if (((GetDownloadState() == DS_REQHASHSET) || m_fHashsetRequesting) && (m_reqfile)) {
		m_reqfile->hashsetneeded = true;
	}

	//check if this client is needed in any way, if not delete it
	bool bDelete = true;
	switch(m_nUploadState){
		case US_ONUPLOADQUEUE:
			bDelete = false;
			break;
	};
	switch(m_nDownloadState){
		case DS_ONQUEUE:
		case DS_TOOMANYCONNS:
		case DS_NONEEDEDPARTS:
		case DS_LOWTOLOWIP:
			bDelete = false;
	};

	switch(m_nUploadState){
		case US_CONNECTING:
		case US_WAITCALLBACK:
		case US_ERROR:
			theApp.clientlist->AddDeadSource(this);
			bDelete = true;
	};
	switch(m_nDownloadState){
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_ERROR:
			theApp.clientlist->AddDeadSource(this);
			bDelete = true;
	};


	if (GetChatState() != MS_NONE){
		bDelete = false;
		m_pendingMessage.Clear();
		Notify_ChatConnResult(false,GUI_ID(GetIP(),GetUserPort()),wxEmptyString);
	}

	if (!bFromSocket && m_socket){
		wxASSERT (theApp.listensocket->IsValidSocket(m_socket));
		m_socket->Safe_Delete();
	}

	SetSocket(NULL);

	if (m_iFileListRequested){
		AddLogLineM( false, CFormat(_("Failed to retrieve shared files from user '%s'")) % GetUserName() );
		m_iFileListRequested = 0;
	}

	Notify_ClientCtrlRefreshClient( this );

	if (bDelete) {
		if (m_Friend) {
			// Remove the friend linkage
			Notify_ChatRefreshFriend(m_Friend->GetIP(), m_Friend->GetPort(), wxEmptyString);
		}
		AddDebugLogLineM( false, logClient, wxString() <<
			wxT("--- Deleted client \"") <<	GetClientFullInfo() <<
			wxT("\"; Reason was ") << strReason );
	} else {
		AddDebugLogLineM( false, logClient, wxString() <<
			wxT("--- Disconnected client \"") << GetClientFullInfo() <<
			wxT("\"; Reason was ") << strReason );
		m_fHashsetRequesting = 0;
		SetSentCancelTransfer(0);
		m_bHelloAnswerPending = false;
		m_fSentOutOfPartReqs = 0;
	}
	
	return bDelete;
}

//Returned bool is not if the TryToConnect is successful or not..
//false means the client was deleted!
//true means the client was not deleted!
bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon)
{
	// Kad reviewed
	if (theApp.listensocket->TooManySockets() && !bIgnoreMaxCon )  {
		if (!(m_socket && m_socket->IsConnected())) {
			if(Disconnected(wxT("Too many connections"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}
	}

	// Ipfilter check
	uint32 uClientIP = GetIP();
	if (uClientIP == 0 && !HasLowID()) {
		uClientIP = ENDIAN_NTOHL(m_nUserIDHybrid);
	}
	
	if (uClientIP) {
		// Although we filter all received IPs (server sources, source exchange) and all incomming connection attempts,
		// we do have to filter outgoing connection attempts here too, because we may have updated the ip filter list
		if (theApp.ipfilter->IsFiltered(uClientIP)) {
			AddDebugLogLineM(true, logIPFilter, CFormat(wxT("Filtered ip %u (%s) on TryToConnect\n")) % uClientIP % Uint32toStringIP(uClientIP));
			if (Disconnected(wxT("IPFilter"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}

		// for safety: check again whether that IP is banned
		if (theApp.clientlist->IsBannedClient(uClientIP)) {
			AddDebugLogLineM(false, logClient, wxT("Refused to connect to banned client ") + Uint32toStringIP(uClientIP));
			if (Disconnected(wxT("Banned IP"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}
	}
	
	if( GetKadState() == KS_QUEUED_FWCHECK ) {
		SetKadState(KS_CONNECTING_FWCHECK);
	}
	
	if ( HasLowID() ) {
		if (!theApp.DoCallback(this)) {
			//We cannot do a callback!
			if (GetDownloadState() == DS_CONNECTING) {
				SetDownloadState(DS_LOWTOLOWIP);
			} else if (GetDownloadState() == DS_REQHASHSET) {
				SetDownloadState(DS_ONQUEUE);
				m_reqfile->hashsetneeded = true;
			}
			if (GetUploadState() == US_CONNECTING) {
				if(Disconnected(wxT("LowID->LowID and US_CONNECTING"))) {
					Safe_Delete();
					return false;
				}
			}
			return true;
		}

		#ifdef __COMPILE_KAD__
		//We already know we are not firewalled here as the above condition already detected LowID->LowID and returned.
		//If ANYTHING changes with the "if(!theApp.DoCallback(this))" above that will let you fall through 
		//with the condition that the source is firewalled and we are firewalled, we must
		//recheck it before the this check..
		if( HasValidBuddyID() && !GetBuddyIP() && !GetBuddyPort() && !theApp.serverconnect->IsLocalServer(GetServerIP(), GetServerPort())) {
			//This is a Kad firewalled source that we want to do a special callback because it has no buddyIP or buddyPort.
			if( Kademlia::CKademlia::isConnected() ) {
				//We are connect to Kad
				if( Kademlia::CKademlia::getPrefs()->getTotalSource() > 0 || Kademlia::CSearchManager::alreadySearchingFor(Kademlia::CUInt128(GetBuddyID()))) {
					//There are too many source lookups already or we are already searching this key.
					SetDownloadState(DS_TOOMANYCONNSKAD);
					return true;
				}
			}
		}
		#endif
	}
	
	if (!m_socket || !m_socket->IsConnected()) {
		if (m_socket) {
			m_socket->Safe_Delete();
		}
		m_socket = new CClientReqSocket(this, thePrefs::GetProxyData());
	} else {
		ConnectionEstablished();
		return true;
	}	
	
	
	if (HasLowID()) {
		if (GetDownloadState() == DS_CONNECTING) {
			SetDownloadState(DS_WAITCALLBACK);
		}
		if (GetUploadState() == US_CONNECTING) {
			if(Disconnected(wxT("LowID and US_CONNECTING"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}

		if (theApp.serverconnect->IsLocalServer(m_dwServerIP,m_nServerPort)) {
			CMemFile data;
			// AFAICS, this id must be reversed to be sent to clients
			// But if I reverse it, we do a serve violation ;)
			data.WriteUInt32(m_nUserIDHybrid);
			CPacket* packet = new CPacket(&data);
			packet->SetOpCode(OP_CALLBACKREQUEST);
			theStats::AddUpOverheadServer(packet->GetPacketSize());
			theApp.serverconnect->SendPacket(packet);
			AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CALLBACKREQUEST") );
			//printf("Sending a callback request, ID: %x/%x IP: %s (%i)\n",m_nUserIDHybrid,
			//	ENDIAN_NTOHL(m_nUserIDHybrid), 
			//	(const char*)unicode2char(Uint32toStringIP(m_nUserIDHybrid)),
			//	GetSourceFrom());
			SetDownloadState(DS_WAITCALLBACK);
		} else {
			if (GetUploadState() == US_NONE && (!GetRemoteQueueRank() || m_bReaskPending)) {
				
				if( !HasValidBuddyID() ) {
					theApp.downloadqueue->RemoveSource(this);
					if (Disconnected(wxT("LowID and US_NONE and QR=0"))) {
						Safe_Delete();
						return false;
					}
					return true;
				}
				
				#ifdef __COMPILE_KAD__
				if( !Kademlia::CKademlia::isConnected() ) {
					//We are not connected to Kad and this is a Kad Firewalled source..
					theApp.downloadqueue->RemoveSource(this);
					if(Disconnected(wxT("Kad Firewalled source but not connected to Kad."))) {
						Safe_Delete();
						return false;
					}
					return true;
				}
				
               	 if( GetDownloadState() == DS_WAITCALLBACK ) {
					if( GetBuddyIP() && GetBuddyPort()) {
						CMemFile bio(34);
						bio.WriteUInt128(Kademlia::CUInt128(GetBuddyID()));
						bio.WriteUInt128(Kademlia::CUInt128(m_reqfile->GetFileHash().GetHash()));
						bio.WriteUInt16(thePrefs::GetPort());
						CPacket* packet = new CPacket(&bio, OP_KADEMLIAHEADER, KADEMLIA_CALLBACK_REQ);
						theApp.clientudp->SendPacket(packet, GetBuddyIP(), GetBuddyPort());
						AddDebugLogLineM(false,logLocalClient, wxString::Format(wxT("KADEMLIA_CALLBACK_REQ (%i)"),packet->GetPacketSize()));						
						theStats::AddUpOverheadKad(packet->GetRealPacketSize());
						SetDownloadState(DS_WAITCALLBACKKAD);
					} else {
						printf("Searching buddy for lowid connection\n");
						//Create search to find buddy.
						Kademlia::CSearch *findSource = new Kademlia::CSearch;
						findSource->setSearchTypes(Kademlia::CSearch::FINDSOURCE);
						findSource->setTargetID(GetBuddyID());
						findSource->addFileID(Kademlia::CUInt128(m_reqfile->GetFileHash().GetHash()));
						if(Kademlia::CSearchManager::startSearch(findSource)) {
							//Started lookup..
							SetDownloadState(DS_WAITCALLBACKKAD);
						} else {
							//This should never happen..
							wxASSERT(0);
						}
					}
				}
				#endif
			} else {
				if (GetDownloadState() == DS_WAITCALLBACK) {
					m_bReaskPending = true;
					SetDownloadState(DS_ONQUEUE);
				}
			}	
		}
	} else {
		amuleIPV4Address tmp;
		tmp.Hostname(GetConnectIP());
		tmp.Service(GetUserPort());
		AddDebugLogLineM(false, logClient, wxT("Trying to connect to ") + Uint32_16toStringIP_Port(GetConnectIP(),GetUserPort()));
		m_socket->Connect(tmp, false);
		// We should send hello packets AFTER connecting!
		// so I moved it to OnConnect
	}
	return true;
}

void CUpDownClient::ConnectionEstablished()
{
	// check if we should use this client to retrieve our public IP
	if (theApp.GetPublicIP() == 0 && theApp.IsConnectedED2K()) {
		SendPublicIPRequest();
	}
	
	switch (GetKadState()) {
		case KS_CONNECTING_FWCHECK:
			SetKadState(KS_CONNECTED_FWCHECK);
			break;
		case KS_CONNECTING_BUDDY:
		case KS_INCOMING_BUDDY:
			SetKadState(KS_CONNECTED_BUDDY);
			break;
		default:
			break;
	}

	// ok we have a connection, lets see if we want anything from this client
	if (GetChatState() == MS_CONNECTING) {
		SetChatState( MS_CHATTING );
	}
	
	if (GetChatState() == MS_CHATTING) {
		bool result = true;
		if (!m_pendingMessage.IsEmpty()) {
			result = SendMessage(m_pendingMessage);
		}
		Notify_ChatConnResult(result,GUI_ID(GetIP(),GetUserPort()),m_pendingMessage);
		m_pendingMessage.Clear();
	}

	switch(GetDownloadState()) {
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_WAITCALLBACKKAD:
			m_bReaskPending = false;
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
	}
	if (m_bReaskPending){
		m_bReaskPending = false;
		if (GetDownloadState() != DS_NONE && GetDownloadState() != DS_DOWNLOADING) {
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
		}
	}
	switch(GetUploadState()){
		case US_CONNECTING:
		case US_WAITCALLBACK:
			if (theApp.uploadqueue->IsDownloading(this)) {
				SetUploadState(US_UPLOADING);
				CPacket* packet = new CPacket(OP_ACCEPTUPLOADREQ,0);
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet,true);
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ACCEPTUPLOADREQ") );
			}
	}
	if (m_iFileListRequested == 1) {
		CPacket* packet = new CPacket(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		if (m_fSharedDirectories) {
			AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDDIRS") );
		} else {
			AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_ASKSHAREDFILES") );
		}
	}
	while (!m_WaitingPackets_list.IsEmpty()) {
		SendPacket(m_WaitingPackets_list.RemoveHead());
	}
}


int CUpDownClient::GetHashType() const
{
	if ( m_UserHash[5] == 13  && m_UserHash[14] == 110 ) {
		return SO_OLDEMULE;
	}

	if ( m_UserHash[5] == 14  && m_UserHash[14] == 111 ) {
		return SO_EMULE;
	}

 	if ( m_UserHash[5] == 'M' && m_UserHash[14] == 'L' ) {
		return SO_MLDONKEY;
	}

	return SO_UNKNOWN;
}


void CUpDownClient::ReGetClientSoft()
{
	if (m_Username.IsEmpty()) {
		m_clientSoft=SO_UNKNOWN;
		m_clientVerString = _("Unknown");
		m_SoftLen = m_clientVerString.Length();
		UpdateStats();
		return;
	}

	int iHashType = GetHashType();
	if (iHashType == SO_EMULE) {
		
		m_clientSoft = m_byCompatibleClient;
		m_clientVerString = GetSoftName(m_clientSoft);
		// Special issues:
		if((GetClientModString().IsEmpty() == false) && (m_clientSoft != SO_EMULE)) {
			m_clientVerString = GetClientModString();
			if (m_clientSoft == SO_AMULE) {
				Extended_aMule_SO &= 2;
			}			
		}
		// Isn't xMule annoying?
		if ((m_clientSoft == SO_LXMULE) && (GetMuleVersion() > 0x26) && (GetMuleVersion() != 0x99)) {
			m_clientVerString += wxString::Format(_(" (Fake eMule version %x)"),GetMuleVersion());
		}
		if ((m_clientSoft == SO_EMULE) && 
			(
				wxString(GetClientModString()).MakeLower().Find(wxT("xmule")) != -1 
				|| GetUserName().Find(wxT("xmule.")) != -1
			)
		) {
			// FAKE eMule -a newer xMule faking is ident.
			m_clientSoft = SO_LXMULE;
			if (GetClientModString().IsEmpty() == false) {
				m_clientVerString = GetClientModString() + _(" (Fake eMule)");
			} else {
				m_clientVerString = _("xMule (Fake eMule)"); // don't use GetSoftName, it's not lmule.
			}
		}		
		// Now, what if we don't know this SO_ID?
		if (m_clientVerString.IsEmpty()) {
			if(m_bIsML) {
				m_clientSoft = SO_MLDONKEY;
				m_clientVerString = GetSoftName(m_clientSoft);
			} else if (m_bIsHybrid) {
				m_clientSoft = SO_EDONKEYHYBRID;
				m_clientVerString = GetSoftName(m_clientSoft);
			} else if (m_byCompatibleClient != 0) {
				m_clientSoft = SO_COMPAT_UNK;
				#ifdef __DEBUG__
				printf("Compatible client found with ET_COMPATIBLECLIENT of 0x%x\n",m_byCompatibleClient);
				#endif
				m_clientVerString = GetSoftName(m_clientSoft) + wxString::Format(wxT("(0x%x)"),m_byCompatibleClient);
			} else {
				// If we step here, it might mean 2 things:
				// a eMule
				// a Compat Client that has sent no MuleInfo packet yet.
				m_clientSoft = SO_EMULE;
				m_clientVerString = wxT("eMule");
			}
		}

		m_SoftLen = m_clientVerString.Length();

		if (m_byEmuleVersion == 0) {
			m_nClientVersion = MAKE_CLIENT_VERSION(0,0,0);
		} else if (m_byEmuleVersion != 0x99) {
			uint32 nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
			m_nClientVersion = MAKE_CLIENT_VERSION(0,nClientMinVersion,0);
			switch (m_clientSoft) {
				case SO_AMULE:
					Extended_aMule_SO = 1; // no CVS flag for 1.x, so no &= right now
					m_clientVerString += wxString::Format(_(" (based on eMule v0.%u)"), nClientMinVersion);
					break;
				case SO_LPHANT:
					m_clientVerString += wxT(" < v0.05 ");
					break;
				default:
					if (GetClientModString().IsEmpty() == false) {
						m_clientVerString +=  wxString::Format(wxT(" v0.%u - "), nClientMinVersion) + GetClientModString();
					} else {
						m_clientVerString +=  wxString::Format(wxT(" v0.%u"), nClientMinVersion);
					}
					break;
			}
		} else {
			uint32 nClientMajVersion = (m_nClientVersion >> 17) & 0x7f;
			uint32 nClientMinVersion = (m_nClientVersion >> 10) & 0x7f;
			uint32 nClientUpVersion  = (m_nClientVersion >>  7) & 0x07;

			m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);

			switch (m_clientSoft) {
				case SO_AMULE:
				case SO_LXMULE:
				case SO_HYDRANODE:
					// Kry - xMule started sending correct version tags on 1.9.1b.
					// It only took them 4 months, and being told by me and the
					// eMule+ developers, so I think they're slowly getting smarter.
					// They are based on our implementation, so we use the same format
					// for the version string.
					m_clientVerString +=  wxString::Format(wxT(" v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
					break;
				case SO_LPHANT:
					m_clientVerString +=  wxString::Format(wxT(" v%u.%.2u%c"), nClientMajVersion-1, nClientMinVersion, 'a' + nClientUpVersion);
					break;
				case SO_EMULEPLUS:
					m_clientVerString +=  wxString::Format(wxT(" v%u"), nClientMajVersion);
					if(nClientMinVersion != 0) {
						m_clientVerString +=  wxString::Format(wxT(".%u"), nClientMinVersion);
					}
					if(nClientUpVersion != 0) {
						m_clientVerString +=  wxString::Format(wxT("%c"), 'a' + nClientUpVersion - 1);
					}
					break;
				default:
					if (GetClientModString().IsEmpty() == false) {
						m_clientVerString +=  wxString::Format(wxT(" v%u.%u%c - "), nClientMajVersion, nClientMinVersion, 'a' + nClientUpVersion) + GetClientModString();
					} else {
						m_clientVerString +=  wxString::Format(wxT(" v%u.%u%c"), nClientMajVersion, nClientMinVersion, 'a' + nClientUpVersion);
					}
					break;
			}
		}
	} else if (m_bIsHybrid) {
		// seen:
		// 105010	50.10
		// 10501	50.1
		// 1051		51.0
		// 501		50.1

		m_clientSoft = SO_EDONKEYHYBRID;
		m_clientVerString = GetSoftName(m_clientSoft);
		m_SoftLen = m_clientVerString.Length();

		uint32 nClientMajVersion;
		uint32 nClientMinVersion;
		uint32 nClientUpVersion;
		if (m_nClientVersion > 100000) {
			uint32 uMaj = m_nClientVersion/100000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*100000) / 100;
			nClientUpVersion = m_nClientVersion % 100;
		}
		else if (m_nClientVersion > 10000) {
			uint32 uMaj = m_nClientVersion/10000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*10000) / 10;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion > 1000) {
			uint32 uMaj = m_nClientVersion/1000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = m_nClientVersion - uMaj*1000;
			nClientUpVersion = 0;
		}
		else if (m_nClientVersion > 100) {
			uint32 uMin = m_nClientVersion/10;
			nClientMajVersion = 0;
			nClientMinVersion = uMin;
			nClientUpVersion = m_nClientVersion - uMin*10;
		}
		else{
			nClientMajVersion = 0;
			nClientMinVersion = m_nClientVersion;
			nClientUpVersion = 0;
		}
		m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);
		if (nClientUpVersion) {
			m_clientVerString += wxString::Format(wxT(" v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
		} else {
			m_clientVerString += wxString::Format(wxT(" v%u.%u"), nClientMajVersion, nClientMinVersion);
		}
	} else if (m_bIsML || (iHashType == SO_MLDONKEY)) {
		m_clientSoft = SO_MLDONKEY;
		m_clientVerString = GetSoftName(m_clientSoft);
		m_SoftLen = m_clientVerString.Length();
		uint32 nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		m_clientVerString += wxString::Format(wxT(" v0.%u"), nClientMinVersion);
	} else if (iHashType == SO_OLDEMULE) {
		m_clientSoft = SO_OLDEMULE;
		m_clientVerString = GetSoftName(m_clientSoft);
		m_SoftLen = m_clientVerString.Length();
		uint32 nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		m_clientVerString += wxString::Format(wxT(" v0.%u"), nClientMinVersion);
	} else {
		m_clientSoft = SO_EDONKEY;
		m_clientVerString = GetSoftName(m_clientSoft);
		m_SoftLen = m_clientVerString.Length();
		m_nClientVersion *= 10;
		m_clientVerString += wxString::Format(wxT(" v%u.%u"), m_nClientVersion / 100000, (m_nClientVersion / 1000) % 100);
	}

	UpdateStats();
}

void CUpDownClient::RequestSharedFileList()
{
	if (m_iFileListRequested == 0) {
		AddDebugLogLineM( false, logClient, wxString( wxT("Requesting shared files from ") ) + GetUserName() );
		m_iFileListRequested = 1;
		TryToConnect(true);
	} else {
		AddDebugLogLineM( false, logClient, CFormat( wxT("Requesting shared files from user %s (%u) is already in progress") ) % GetUserName() % GetUserIDHybrid() );
	}
}

void CUpDownClient::ProcessSharedFileList(const char* pachPacket, uint32 nSize, wxString& pszDirectory) {
	if (m_iFileListRequested > 0) {
		m_iFileListRequested--;
		theApp.searchlist->ProcessSearchanswer(pachPacket,nSize,this,NULL,pszDirectory);
	}
}

void CUpDownClient::ResetFileStatusInfo()
{
	m_nPartCount = 0;

	if ( m_reqfile ) {
		m_reqfile->UpdatePartsFrequency( this, false );
	}
	m_downPartStatus.clear();

	m_clientFilename.Clear();

	m_bCompleteSource = false;
	m_dwLastAskedTime = 0;
	m_iRating = 0;
	m_strComment.Clear();

	if (m_pReqFileAICHHash != NULL) {
		delete m_pReqFileAICHHash;
		m_pReqFileAICHHash = NULL;
	}

}

wxString CUpDownClient::GetUploadFileInfo()
{
	// build info text and display it
	wxString sRet;
	sRet = (CFormat(_("NickName: %s ID: %u")) % GetUserName() % GetUserIDHybrid()) + wxT(" ");
	if (m_reqfile) {
		sRet += _("Requested:") + wxString(m_reqfile->GetFileName()) + wxT("\n");
		sRet += CFormat(_("Filestats for this session: Accepted %d of %d requests, %s transferred\n")) % m_reqfile->statistic.GetAccepts() % m_reqfile->statistic.GetRequests() % CastItoXBytes(m_reqfile->statistic.GetTransfered());
		sRet += CFormat(_("Filestats for all sessions: Accepted %d of %d requests, %s transferred\n")) % m_reqfile->statistic.GetAllTimeAccepts() % m_reqfile->statistic.GetAllTimeRequests() % CastItoXBytes(m_reqfile->statistic.GetAllTimeTransfered());
	} else {
		sRet += _("Requested unknown file");
	}
	return sRet;
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false it is _possible_ that this clientobject was deleted, because the connectiontry fails
bool CUpDownClient::SafeSendPacket(CPacket* packet)
{
	if (IsConnected()) {
		SendPacket(packet);
		return true;
	} else {
		m_WaitingPackets_list.AddTail(packet);
		return TryToConnect(true);
	}
}

void CUpDownClient::SendPublicKeyPacket(){
	// send our public key to the client who requested it
	if (m_socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED){
		wxASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;

	CMemFile data;
	data.WriteUInt8(theApp.clientcredits->GetPubKeyLen());
	data.Write(theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
	CPacket* packet = new CPacket(&data, OP_EMULEPROT);
	packet->SetOpCode(OP_PUBLICKEY);

	theStats::AddUpOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true,true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
	
	AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_PUBLICKEY") );
}


void CUpDownClient::SendSignaturePacket(){
	// signate the public key of this client and send it
	if (m_socket == NULL || credits == NULL || m_SecureIdentState == 0){
		wxASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable()) {
		return;
	}
	if (credits->GetSecIDKeyLen() == 0) {
		return; // We don't have his public key yet, will be back here later
	}
	// do we have a challenge value recieved (actually we should if we are in this function)
	if (credits->m_dwCryptRndChallengeFrom == 0){
		AddDebugLogLineM( false, logClient, wxString(wxT("Want to send signature but challenge value is invalid - User ")) + GetUserName());
		return;
	}
	// v2
	// we will use v1 as default, except if only v2 is supported
	bool bUseV2;
	if ( (m_bySupportSecIdent&1) == 1 )
		bUseV2 = false;
	else
		bUseV2 = true;

	uint8 byChaIPKind = 0;
	uint32 ChallengeIP = 0;
	if (bUseV2){
		if (theApp.serverconnect->GetClientID() == 0 || theApp.serverconnect->IsLowID()){
			// we cannot do not know for sure our public ip, so use the remote clients one
			ChallengeIP = GetIP();
			byChaIPKind = CRYPT_CIP_REMOTECLIENT;
		}
		else{
			ChallengeIP = theApp.serverconnect->GetClientID();
			byChaIPKind  = CRYPT_CIP_LOCALCLIENT;
		}
	}
	//end v2
	byte achBuffer[250];

	uint8 siglen = theApp.clientcredits->CreateSignature(credits, achBuffer,  250, ChallengeIP, byChaIPKind );
	if (siglen == 0){
		wxASSERT ( false );
		return;
	}
	CMemFile data;
	data.WriteUInt8(siglen);
	data.Write(achBuffer, siglen);
	if (bUseV2) {
		data.WriteUInt8(byChaIPKind);
	}
	CPacket* packet = new CPacket(&data, OP_EMULEPROT);
	packet->SetOpCode(OP_SIGNATURE);

	theStats::AddUpOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true,true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
	
	AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_SIGNATURE") );
}


void CUpDownClient::ProcessPublicKeyPacket(const byte* pachPacket, uint32 nSize)
{
	theApp.clientlist->AddTrackClient(this);

	if (m_socket == NULL || credits == NULL || pachPacket[0] != nSize-1
		|| nSize == 0 || nSize > 250){
		wxASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;
	// the function will handle everything (mulitple key etc)
	if (credits->SetSecureIdent(pachPacket+1, pachPacket[0])){
		// if this client wants a signature, now we can send him one
		if (m_SecureIdentState == IS_SIGNATURENEEDED){
			SendSignaturePacket();
		}
		else if(m_SecureIdentState == IS_KEYANDSIGNEEDED){
			// something is wrong
			AddDebugLogLineM( false, logClient, wxT("Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket") );
		}
	} else{
		AddDebugLogLineM( false, logClient, wxT("Failed to use new recieved public key") );
	}
}


void CUpDownClient::ProcessSignaturePacket(const byte* pachPacket, uint32 nSize)
{
	// here we spread the good guys from the bad ones ;)

	if (m_socket == NULL || credits == NULL || nSize == 0 || nSize > 250){
		wxASSERT ( false );
		return;
	}

	uint8 byChaIPKind;
	if (pachPacket[0] == nSize-1)
		byChaIPKind = 0;
	else if (pachPacket[0] == nSize-2 && (m_bySupportSecIdent & 2) > 0) //v2
		byChaIPKind = pachPacket[nSize-1];
	else{
		wxASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable())
		return;

	// we accept only one signature per IP, to avoid floods which need a lot cpu time for cryptfunctions
	if (m_dwLastSignatureIP == GetIP()){
		AddDebugLogLineM( false, logClient, wxT("recieved multiple signatures from one client") );
		return;
	}
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0){
		AddDebugLogLineM( false, logClient, wxT("recieved signature for client without public key") );
		return;
	}
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0){
		AddDebugLogLineM( false, logClient, wxT("recieved signature for client with invalid challenge value - User ") + GetUserName() );
		return;
	}

	if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind ) ){
		// result is saved in function above
		AddDebugLogLineM( false, logClient, CFormat( wxT("'%s' has passed the secure identification, V2 State: %i") ) % GetUserName() % byChaIPKind );
	} else {
		AddDebugLogLineM( false, logClient, CFormat( wxT("'%s' has failed the secure identification, V2 State: %i") ) % GetUserName() % byChaIPKind );
	}

	m_dwLastSignatureIP = GetIP();
}

void CUpDownClient::SendSecIdentStatePacket(){
	// check if we need public key and signature
	uint8 nValue = 0;
	if (credits){
		if (theApp.clientcredits->CryptoAvailable()){
			if (credits->GetSecIDKeyLen() == 0) {
				nValue = IS_KEYANDSIGNEEDED;
			} else if (m_dwLastSignatureIP != GetIP()) {
				nValue = IS_SIGNATURENEEDED;
			}
		}
		if (nValue == 0){
			AddDebugLogLineM( false, logClient, wxT("Not sending SecIdentState Packet, because State is Zero") );
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;

		CMemFile data;
		data.WriteUInt8(nValue);
		data.WriteUInt32(dwRandom);
		CPacket* packet = new CPacket(&data, OP_EMULEPROT);
		packet->SetOpCode(OP_SECIDENTSTATE);

		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		
		AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_SECIDENTSTATE") );
	} else {
		wxASSERT ( false );
	}
}


void CUpDownClient::ProcessSecIdentStatePacket(const byte* pachPacket, uint32 nSize)
{
	if ( nSize != 5 ) {
		return;
	}

	if ( !credits ) {
		wxASSERT( credits );
		return;
	}

	CMemFile data((byte*)pachPacket,nSize);

	switch ( data.ReadUInt8() ) {
		case 0:
			m_SecureIdentState = IS_UNAVAILABLE;
			break;
		case 1:
			m_SecureIdentState = IS_SIGNATURENEEDED;
			break;
		case 2:
			m_SecureIdentState = IS_KEYANDSIGNEEDED;
			break;
		default:
			return;
	}

	credits->m_dwCryptRndChallengeFrom = data.ReadUInt32();
}


void CUpDownClient::InfoPacketsReceived()
{
	// indicates that both Information Packets has been received
	// needed for actions, which process data from both packets
	wxASSERT ( m_byInfopacketsReceived == IP_BOTH );
	m_byInfopacketsReceived = IP_NONE;

	if (m_bySupportSecIdent){
		SendSecIdentStatePacket();
	}
}


bool CUpDownClient::CheckHandshakeFinished(uint32 WXUNUSED(protocol), uint32 WXUNUSED(opcode)) const
{
	if (m_bHelloAnswerPending){
		// this triggers way too often.. need more time to look at this -> only create a warning
		AddDebugLogLineM( false, logClient, wxT("Handshake not finished while processing packet.") );
		return false;
	}

	return true;
}


wxString CUpDownClient::GetClientFullInfo() {

	if (m_clientVerString.IsEmpty()) {
		ReGetClientSoft();
	}

	return CFormat( _("Client %s on IP:Port %s:%d using %s") )
		% ( m_Username.IsEmpty() ? wxString(_("Unknown")) : m_Username )
		% GetFullIP()
		% GetUserPort()
		% m_clientVerString;
}



void CUpDownClient::SendPublicIPRequest(){
	if (IsConnected()){
		CPacket* packet = new CPacket(OP_PUBLICIP_REQ,0,OP_EMULEPROT);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true);
		m_fNeedOurPublicIP = true;
		AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_PUBLICIP_REQ") );
	}
}

void CUpDownClient::ProcessPublicIPAnswer(const byte* pbyData, uint32 uSize){
	if (uSize != 4) {
		throw wxString(wxT("Wrong Packet size on Public IP answer"));
	}
	uint32 dwIP = PeekUInt32(pbyData);
	if (m_fNeedOurPublicIP == true){ // did we?
		m_fNeedOurPublicIP = false;
		if (theApp.GetPublicIP() == 0 && !IsLowID(dwIP) ) {
			theApp.SetPublicIP(dwIP);
		}
	}
}


bool CUpDownClient::IsConnected() const
{
	return m_socket && m_socket->IsConnected();
}

bool CUpDownClient::SendPacket(CPacket* packet, bool delpacket, bool controlpacket)
{
	if ( m_socket ) {
		m_socket->SendPacket(packet, delpacket, controlpacket );
		return true;
	} else {
		printf("CAUGHT DEAD SOCKET IN SENDPACKET()\n");
		return false;
	}
}

float CUpDownClient::SetDownloadLimit(uint32 reducedownload)
{

	// lfroen: in daemon it actually can happen
		wxASSERT( m_socket );

	float kBpsClient = CalculateKBpsDown();
	
	if ( m_socket ) {

		if (reducedownload) {
			// (% to reduce * current speed) / 100 and THEN, / 10 because this
			// gets called 10 times per second.
			uint32 limit = (uint32)(((float)reducedownload*(kBpsClient*1024.0))/1000);

			if(limit<1024 && reducedownload >= 200) {
				// If we're going up and this download is < 1kB, 
				// we want it to go up fast. Can be reduced later,
				// and it'll probably be in a more fair way with 
				// other downloads that are faster.
				limit +=1024;
			} else if(limit == 0) {
				// This download is not transferring yet... make it 
				// 1024 so we don't fill the TCP stack and lose the 
				// connection.
				limit = 1024;
			}
			
			m_socket->SetDownloadLimit(limit);
		} else {
			m_socket->DisableDownloadLimit();
		}		
		
	} else {
		printf("CAUGHT DEAD SOCKET IN SETDOWNLOADLIMIT() WITH SPEED %f\n", kBpsClient);
	}
	
	return kBpsClient;
	
}

void CUpDownClient::SetUserIDHybrid(uint32 nUserID)
{
	theApp.clientlist->UpdateClientID( this, nUserID );

	m_nUserIDHybrid = nUserID;
}


void CUpDownClient::SetIP( uint32 val )
{
	theApp.clientlist->UpdateClientIP( this, val );

	m_dwUserIP = val;

	m_nConnectIP = val;
}


void CUpDownClient::SetUserHash(const CMD4Hash& userhash)
{
	theApp.clientlist->UpdateClientHash( this, userhash );

	m_UserHash = userhash;

	ValidateHash();
}

EUtf8Str CUpDownClient::GetUnicodeSupport() const
{
#if wxUSE_UNICODE
	return m_bUnicodeSupport ? utf8strRaw : utf8strNone;
#else 
	return utf8strNone;
#endif
}


uint8 CUpDownClient::GetSecureIdentState() {
	if (m_SecureIdentState != IS_UNAVAILABLE) {
		if (!SecIdentSupRec) {
			// This can be caused by a 0.30x based client which sends the old
			// style Hello packet, and the mule info packet, but between them they
			// send a secure ident state packet (after a hello but before we have 
			// the SUI capabilities). This is a misbehaving client, and somehow I
			// Feel like ti should be dropped. But then again, it won't harm to use
			// this SUI state if they are reporting no SUI (won't be used) and if 
			// they report using SUI on the mule info packet, it's ok to use it.
			
			AddDebugLogLineM(false, logClient, wxT("A client sent secure ident state before telling us the SUI capabilities"));
			AddDebugLogLineM(false, logClient, wxT("Client info: ") + GetClientFullInfo());
			AddDebugLogLineM(false, logClient, wxT("This client won't be disconnected, but it should be. :P"));
		}
	}
	
	return m_SecureIdentState;
}

bool CUpDownClient::SendMessage(const wxString& message) {
	// Already connecting?
	if (GetChatState() == MS_CONNECTING) {
		// Queue all messages till we're able to send them (or discard them)
		if (!m_pendingMessage.IsEmpty()) {
			m_pendingMessage += wxT("\n");
		} else {
			// There must be a message to send
			wxASSERT(0);
		}
		m_pendingMessage += message;		
		return false;
	}
	if (IsConnected()) {
		CMemFile data;
		data.WriteString(message);
		CPacket* packet = new CPacket(&data);
		packet->SetOpCode(OP_MESSAGE);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet, true, true);
		return true;
	} else {
		m_pendingMessage = message;
		SetChatState(MS_CONNECTING);
		// True to ignore "Too Many Connections"
		TryToConnect(true);
		return false;
	}	
}

/* Kad stuff */

void CUpDownClient::SetBuddyID(const byte* pucBuddyID)
{
	if( pucBuddyID == NULL ){
		md4clr(m_achBuddyID);
		m_bBuddyIDValid = false;
		return;
	}
	m_bBuddyIDValid = true;
	md4cpy(m_achBuddyID, pucBuddyID);
}

// Kad added by me

bool CUpDownClient::SendBuddyPing() {
	AddDebugLogLineM(false, logLocalClient,wxT("Local Client: OP_BuddyPing"));
	SetLastBuddyPingPongTime();	
	CPacket* buddyPing = new CPacket(OP_BUDDYPING, 0, OP_EMULEPROT);
	theStats::AddUpOverheadKad(buddyPing->GetPacketSize());
	return SafeSendPacket(buddyPing);
}


/* Statistics */

void CUpDownClient::UpdateStats()
{
	if (m_lastClientSoft != m_clientSoft || m_lastClientVersion != m_nClientVersion || m_lastOSInfo != m_sClientOSInfo) {
		if (m_lastClientSoft == SO_UNKNOWN) {
			theStats::RemoveUnknownClient();
		} else if (m_lastClientSoft != (uint32)(-1)) {
			theStats::RemoveKnownClient(m_lastClientSoft, m_lastClientVersion, m_lastOSInfo);
		}
		
		m_lastClientSoft = m_clientSoft;
		m_lastClientVersion = m_nClientVersion;
		m_lastOSInfo = m_sClientOSInfo;
		
		if (m_clientSoft == SO_UNKNOWN) {
			theStats::AddUnknownClient();
		} else {
			theStats::AddKnownClient(this, m_clientSoft, m_nClientVersion);
		}
	}
}


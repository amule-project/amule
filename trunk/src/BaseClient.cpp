// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "types.h"

#include <zlib.h>		// Needed for inflateEnd
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/tokenzr.h>
#include <wx/utils.h>

#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "IPFilter.h"		// Needed for CIPFilter
#include "sockets.h"		// Needed for CServerConnect
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "server.h"		// Needed for CServer
#include "Preferences.h"	// Needed for CPreferences
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "packets.h"		// Needed for Packet
#include "otherstructs.h"	// Needed for Requested_Block_Struct
#include "Friend.h"		// Needed for CFriend
#include "ClientList.h"		// Needed for CClientList
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "opcodes.h"		// Needed for OP_*
#include "updownclient.h"	// Needed for CUpDownClient


//#define DEBUG_LOCAL_CLIENT_PROTOCOL
//#define __PACKET_DEBUG__


// some client testing variables
static wxString crash_name = wxT("[Invalid User Name]");
static wxString empty_name = wxT("[Empty User Name]");

//	members of CUpDownClient
//	which are used by down and uploading functions

#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
#undef AddDebugLogLineM
#define AddDebugLogLineM(x,y) printf("%s\n",unicode2char(y));
#endif 

CUpDownClient::CUpDownClient(CClientReqSocket* sender)
{
	m_socket = sender;
	Init();
}

CUpDownClient::CUpDownClient(uint16 in_port, uint32 in_userid,uint32 in_serverip, uint16 in_serverport,CPartFile* in_reqfile)
{
	m_socket = NULL;
	Init();
	SetUserID( in_userid );
	m_nUserPort = in_port;

	if (!HasLowID()) {
		m_FullUserIP = Uint32toStringIP(m_nUserID);
	}
	#ifdef __USE_KAD__
	if (!HasLowID()) {
		if (ed2kID) {
			m_nConnectIP = in_userid;
		} else {
			m_nConnectIP = ntohl(in_userid);
		}
	#else
 	if(!HasLowID()) {
		#warning Kry WHY OH WHY!
		m_nConnectIP = ENDIAN_SWAP_32(in_userid);
	}
	#endif

	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
	SetRequestFile( in_reqfile );
	ReGetClientSoft();
}

void CUpDownClient::Init()
{
	Extended_aMule_SO = 0;
	m_bAddNextConnect = false;
	credits = 0;
	m_byChatstate = 0;
	m_cShowDR = 0;
	m_reqfile = NULL;	 // No file required yet
	m_nMaxSendAllowed = 0;
	m_nTransferedUp = 0;
	m_cSendblock = 0;
	m_cAsked = 0;
	msSentPrev = msReceivedPrev = 0;
	kBpsUp = kBpsDown = 0.0;
	fDownAvgFilter = 1.0;
	bytesReceivedCycle = 0;
	m_nUserID = 0;
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
	m_nUpPartCount = 0;
	m_dwLastAskedTime = 0;
	m_nDownloadState = DS_NONE;
	m_dwUploadTime = 0;
	m_nTransferedDown = 0;
	m_nUploadState = US_NONE;
	m_dwLastBlockReceived = 0;

	m_bUnicodeSupport = false;

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
	m_iRate=0;
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

	m_requpfile = NULL;

	m_bMsgFiltered = false;

	if (m_socket) {
		amuleIPV4Address address;
		m_socket->GetPeer(address);
		m_FullUserIP = address.IPAddress();
		SetIP(StringIPtoUint32(m_FullUserIP));
	} else {
		SetIP(0);
	}
	
}


CUpDownClient::~CUpDownClient()
{
	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
	}

	if (m_Friend) {
		m_Friend->m_LinkedClient = NULL;
		Notify_ChatRefreshFriend(m_Friend);
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
		m_socket = NULL;
	}


	ClearUploadBlockRequests();

	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0; ) {
		delete m_DownloadBlocks_list.GetNext(pos);
	}
	m_DownloadBlocks_list.RemoveAll();

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0; ) {
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
		delete pending->block;
		// Not always allocated
		if (pending->zStream) {
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}
	/* eMule 0.30c manage this also, i give it a try ... (Creteil) */

	for (POSITION pos =m_WaitingPackets_list.GetHeadPosition();pos != 0; ) {
		delete m_WaitingPackets_list.GetNext(pos);
	}


	if (m_iRate>0 || m_strComment.Length()>0) {
		m_iRate=0;
		m_strComment = wxEmptyString;
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
}

bool CUpDownClient::ProcessHelloPacket(const char *pachPacket, uint32 nSize)
{
	const CSafeMemFile data((BYTE*)pachPacket,nSize);
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
	if ( m_clientState == CS_DYING )
		return;

 	m_clientState = CS_DYING;

	// Close the socket to avoid any more connections and related events
	if ( m_socket ) {
		m_socket->SetClient( NULL );
		m_socket->Safe_Delete();
		// We're going down anyway....
		m_socket->Destroy();
		// Paranoia
		m_socket = NULL;
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
	const CSafeMemFile data((BYTE*)pachPacket,nSize);
	bool bIsMule = ProcessHelloTypePacket(data);
	m_bHelloAnswerPending = false;
	return bIsMule;
}

bool CUpDownClient::ProcessHelloTypePacket(const CSafeMemFile& data)
{

	m_bIsHybrid = false;
	m_bIsML = false;
	m_fNoViewSharedFiles = 0;
	m_bUnicodeSupport = false;
	DWORD dwEmuleTags = 0;

	try {
		CMD4Hash hash;
		data.ReadHash16(hash);
		SetUserHash( hash );
		SetUserID( data.ReadUInt32() );
		uint16 nUserPort = data.ReadUInt16(); // hmm clientport is sent twice - why?
		uint32 tagcount = data.ReadUInt32();
		for (uint32 i = 0;i < tagcount; i++){
			CTag temptag(data, true);
			switch(temptag.tag.specialtag){
				case CT_NAME:
					if ( !temptag.tag.stringvalue.IsEmpty() ) {
						m_Username = temptag.tag.stringvalue;
					} else {
						m_Username.Clear();
					}

					break;
				case CT_VERSION:
					m_nClientVersion = temptag.tag.intvalue;
					break;
				case ET_MOD_VERSION:
					if (temptag.tag.type == 2) {
						m_strModVersion = temptag.tag.stringvalue;
					} else if (temptag.tag.type == 3) {
						m_strModVersion.Printf( wxT("ModID=%u"), temptag.tag.intvalue);
					} else {
						m_strModVersion = wxT("ModID=<Unknown>");
					}

					break;
				case CT_PORT:
					nUserPort = temptag.tag.intvalue;

					break;
				case CT_EMULE_UDPPORTS:
					// 16 KAD Port
					// 16 UDP Port
					m_nKadPort = (uint16)(temptag.tag.intvalue >> 16);
					m_nUDPPort = (uint16)temptag.tag.intvalue;
					dwEmuleTags |= 1;
					#ifdef __PACKET_DEBUG__
					printf("Hello type packet processing with eMule ports UDP=%i KAD=%i\n",m_nUDPPort,m_nKadPort);
					#endif
					break;
				case CT_EMULE_MISCOPTIONS1:
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
					m_fSupportsAICH			= (temptag.tag.intvalue >> (4*7+1)) & 0x07;
					m_bUnicodeSupport		= (temptag.tag.intvalue >> 4*7) & 0x01;
					m_byUDPVer				= (temptag.tag.intvalue >> 4*6) & 0x0f;
					m_byDataCompVer			= (temptag.tag.intvalue >> 4*5) & 0x0f;
					m_bySupportSecIdent		= (temptag.tag.intvalue >> 4*4) & 0x0f;
					m_bySourceExchangeVer	= (temptag.tag.intvalue >> 4*3) & 0x0f;
					m_byExtendedRequestsVer	= (temptag.tag.intvalue >> 4*2) & 0x0f;
					m_byAcceptCommentVer	= (temptag.tag.intvalue >> 4*1) & 0x0f;
					m_fNoViewSharedFiles	= (temptag.tag.intvalue >> 1*2) & 0x01;
					m_bMultiPacket			= (temptag.tag.intvalue >> 1*1) & 0x01;
					m_fSupportsPreview		= (temptag.tag.intvalue >> 1*0) & 0x01;
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
				// Special tag fo Compat. Clients Misc options.
				case CT_EMULECOMPAT_OPTIONS:
					//  1 Operative System Info
					m_fOsInfoSupport		= (temptag.tag.intvalue >> 1*0) & 0x01;
					break;
				case CT_EMULE_VERSION:
					//  8 Compatible Client ID
					//  7 Mjr Version (Doesn't really matter..)
					//  7 Min Version (Only need 0-99)
					//  3 Upd Version (Only need 0-5)
					//  7 Bld Version (Only need 0-99)
					m_byCompatibleClient = (temptag.tag.intvalue >> 24);
					m_nClientVersion = temptag.tag.intvalue & 0x00ffffff;
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
		if ( data.Length() - data.GetPosition() == sizeof(uint32) ) {
			uint32 test = data.ReadUInt32();
			/*if (test == 'KDLM') below kdlm is converted to ascii values.
			this fix a warning with gcc 3.4.
			K=4b D=44 L=4c M=4d
			i putted that reversed as u can see. please check if works or put plain (0x4b444c4d)
			*/
			if (test == 0x4d4c444b)	{ //if it's == "KDLM"
				m_bIsML=true;
			} else{
				m_bIsHybrid = true;
				m_fSharedDirectories = 1;
			}
		}

	} catch ( CStrangePacket )
	{
		printf("\nWrong Tags on hello type packet!!\n");
		printf("Sent by %s on ip %s port %i using client %x version %x\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetVersion());
		printf("User Disconnected.\n");
		throw wxString(wxT("Wrong Tags on hello type packet"));
	}
	catch ( CInvalidPacket (e))
	{
		printf("Wrong Tags on hello type packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %x version %x\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetVersion());
		printf("User Disconnected.\n");
		throw wxString(wxT("Wrong Tags on hello type packet"));
	}


	if (m_socket) {
		amuleIPV4Address address;
		m_socket->GetPeer(address);
		m_FullUserIP = address.IPAddress();
		SetIP(StringIPtoUint32(m_FullUserIP));
	} else {
		throw wxString(wxT("Huh, socket failure. Avoided crash this time.\n"));
	}

	if (thePrefs::AddServersFromClient()) {
		CServer* addsrv = new CServer(m_nServerPort, Uint32toStringIP(m_dwServerIP));
		addsrv->SetListName(addsrv->GetAddress());
		if (!theApp.AddServer(addsrv)) {
				delete addsrv;
		}
	}

	if(!HasLowID() || m_nUserID == 0) {
		SetUserID( m_dwUserIP );
	}

	// get client credits
	CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(m_UserHash);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			AddDebugLogLineM(false, wxString::Format(_("Clients: %s (%s), Banreason: Userhash changed (Found in TrackedClientsList)"), GetUserName().c_str(), GetFullIP().c_str()));
			Ban();
		}
	} else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		AddDebugLogLineM(false, wxString::Format(_("Clients: %s (%s), Banreason: Userhash changed"), GetUserName().c_str(), GetFullIP().c_str()));
		Ban();
	}

	if ((m_Friend = theApp.FindFriend(&m_UserHash, m_dwUserIP, m_nUserPort)) != NULL){
		// Link the friend to that client
		if (m_Friend->m_LinkedClient){
			if (m_Friend->m_LinkedClient != this){
				bool bFriendSlot = m_Friend->m_LinkedClient->GetFriendSlot();
				// avoid that an unwanted client instance keeps a friend slot
				m_Friend->m_LinkedClient->SetFriendSlot(false);
				m_Friend->m_LinkedClient->m_Friend = NULL;
				m_Friend->m_LinkedClient = this;
				// move an assigned friend slot between different client instances which are/were also friends
				m_Friend->m_LinkedClient->SetFriendSlot(bFriendSlot);
			}
		} else {
			m_Friend->m_LinkedClient = this;
		}
		m_Friend->m_Userhash = GetUserHash();
		m_Friend->m_dwHasHash = !m_Friend->m_Userhash.IsEmpty();
		m_Friend->m_strName = m_Username;
		m_Friend->m_dwLastUsedIP = m_dwUserIP;
		m_Friend->m_nLastUsedPort = m_nUserPort;
		m_Friend->m_dwLastSeen = time(NULL);
		Notify_ChatRefreshFriend(m_Friend);
	}
	else{
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


	#ifdef __USE_KAD__
	if( GetKadPort() && Kademlia::CKademlia::isRunning() )
	{
		Kademlia::CKademlia::getUDPListener()->bootstrap(ntohl(GetIP()), GetKadPort());
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
	#warning Kry - why is IPAddress returning wrong endianess?
	if ( theApp.ipfilter->IsFiltered(ENDIAN_SWAP_32(StringIPtoUint32(address.IPAddress())))) {
		AddDebugLogLineM(true, wxT("Filtered IP: ") +GetFullIP() + wxT("(") + theApp.ipfilter->GetLastHit() + wxT(")"));
		theApp.stat_filteredclients++;
		if (Disconnected(wxT("IPFilter"))) {
			Safe_Delete();
			return false;
		}
		return true;
	}

	CSafeMemFile data(128);
	data.WriteUInt8(16); // size of userhash
	SendHelloTypePacket(&data);
	
	Packet* packet = new Packet(&data);
	packet->SetOpCode(OP_HELLO);
	theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true);
	m_bHelloAnswerPending = true;
	#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
	AddDebugLogLineM(true, wxT("Local Client: OP_HELLO to ") + GetFullIP() + wxT("\n"));
	#endif
	return true;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer, bool OSInfo) {

	if (m_socket == NULL){
		wxASSERT(0);
		return;
	}

	CSafeMemFile* data = new CSafeMemFile();

	data->WriteUInt8(CURRENT_VERSION_SHORT);
	
	if (OSInfo) {
		
		// Special MuleInfo packet for clients supporting it.
		// This means aMule >= 2.0.0 and Hydranode
		
		// Violently mark it as special Mule Info packet
		// Sending this makes non-supporting-osinfo clients to refuse to read this
		// packet. Anyway, this packet should NEVER get to non-supporting clients.
		
		data->WriteUInt8(/*EMULE_PROTOCOL*/ 0xFF);		

		data->WriteUInt32(1); // One Tag (OS_INFO)

		wxStringTokenizer tkz(wxGetOsDescription(), wxT(" "));

		if (tkz.HasMoreTokens()) {
			CTag tag1(ET_OS_INFO,tkz.GetNextToken());
			tag1.WriteTagToFile(data);
		} else {
			CTag tag1(ET_OS_INFO,wxT("Unknown"));
			tag1.WriteTagToFile(data);
		}	

	} else {

		// Normal MuleInfo packet

		data->WriteUInt8(EMULE_PROTOCOL);

		// Tag number
		data->WriteUInt32(9);

		CTag tag1(ET_COMPRESSION,1);
		tag1.WriteTagToFile(data);
		CTag tag2(ET_UDPVER,4);
		tag2.WriteTagToFile(data);
		CTag tag3(ET_UDPPORT,thePrefs::GetUDPPort());
		tag3.WriteTagToFile(data);
		CTag tag4(ET_SOURCEEXCHANGE,3);
		tag4.WriteTagToFile(data);
		CTag tag5(ET_COMMENTS,1);
		tag5.WriteTagToFile(data);
		CTag tag6(ET_EXTENDEDREQUEST,2);
		tag6.WriteTagToFile(data);

		uint32 dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);
		// Kry - Needs the preview code from eMule
		/*
		// set 'Preview supported' only if 'View Shared Files' allowed
		if (thePrefs::CanSeeShares() != vsfaNobody) {
			dwTagValue |= 128;
		}
		*/
		CTag tag7(ET_FEATURES, dwTagValue);
		tag7.WriteTagToFile(data);

		CTag tag8(ET_COMPATIBLECLIENT,SO_AMULE);
		tag8.WriteTagToFile(data);

		// Support for tag ET_MOD_VERSION
		wxString mod_name(MOD_VERSION_LONG);
		CTag tag9(ET_MOD_VERSION, mod_name);
		tag9.WriteTagToFile(data);
		// Maella end

	}

	Packet* packet = new Packet(data,OP_EMULEPROT);
	delete data;
	
	if (!bAnswer) {
		packet->SetOpCode(OP_EMULEINFO);
	} else {
		packet->SetOpCode(OP_EMULEINFOANSWER);
	}
	
	if (m_socket) {
		theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
		if (!bAnswer) {
			if (!OSInfo) {
				AddDebugLogLineM(true, wxT("Local Client: OP_EMULEINFO to ") + GetFullIP() + wxT("\n"));
			} else {
				AddDebugLogLineM(true, wxT("Local Client: OP_EMULEINFO/OS_INFO to ") + GetFullIP() + wxT("\n"));
			}
		} else {
			AddDebugLogLineM(true, wxT("Local Client: OP_EMULEINFOANSWER to ") + GetFullIP() + wxT("\n"));
		}
		#endif
	}
}

bool CUpDownClient::ProcessMuleInfoPacket(const char* pachPacket, uint32 nSize)
{

	uint8 protocol_version;
	
	try {

		const CSafeMemFile data((BYTE*)pachPacket,nSize);

		//The version number part of this packet will soon be useless since it is only able to go to v.99.
		//Why the version is a uint8 and why it was not done as a tag like the eDonkey hello packet is not known..
		//Therefore, sooner or later, we are going to have to switch over to using the eDonkey hello packet to set the version.
		//No sense making a third value sent for versions..
		uint8 mule_version = data.ReadUInt8();

		protocol_version = data.ReadUInt8();

		uint32 tagcount = data.ReadUInt32();
		
		if (protocol_version == 0xFF) {

			// OS Info supporting clients sending a recycled Mule info packet
			for (uint32 i = 0;i < tagcount; i++){
				CTag temptag(data, true);
				switch(temptag.tag.specialtag){
					case ET_OS_INFO:
						// Special tag, only supporting clients (aMule/Hydranode)
	
						// It was recycled from a mod's tag, so if the other side
						// is not supporting OS Info, we're seriously fucked up :)
							
						wxASSERT(temptag.tag.type == 2); // tag must be a string
					
				
						m_sClientOSInfo = temptag.tag.stringvalue;
	
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
				switch(temptag.tag.specialtag){
					case ET_COMPRESSION:
						// Bits 31- 8: 0 - reserved
						// Bits  7- 0: data compression version
						m_byDataCompVer = temptag.tag.intvalue;
						break;
					case ET_UDPPORT:
						// Bits 31-16: 0 - reserved
						// Bits 15- 0: UDP port
						m_nUDPPort = temptag.tag.intvalue;
						break;
					case ET_UDPVER:
						// Bits 31- 8: 0 - reserved
						// Bits  7- 0: UDP protocol version
						m_byUDPVer = temptag.tag.intvalue;
						break;
					case ET_SOURCEEXCHANGE:
						// Bits 31- 8: 0 - reserved
						// Bits  7- 0: source exchange protocol version
						m_bySourceExchangeVer = temptag.tag.intvalue;
						break;
					case ET_COMMENTS:
						// Bits 31- 8: 0 - reserved
						// Bits  7- 0: comments version
						m_byAcceptCommentVer = temptag.tag.intvalue;
						break;
					case ET_EXTENDEDREQUEST:
							// Bits 31- 8: 0 - reserved
						// Bits  7- 0: extended requests version
						m_byExtendedRequestsVer = temptag.tag.intvalue;
						break;
					case ET_COMPATIBLECLIENT:
						// Bits 31- 8: 0 - reserved
						// Bits  7- 0: compatible client ID
						m_byCompatibleClient = temptag.tag.intvalue;
						break;
					case ET_FEATURES:
						// Bits 31- 8: 0 - reserved
						// Bit	    7: Preview
						// Bit   6- 0: secure identification
						m_bySupportSecIdent = temptag.tag.intvalue & 3;
						m_bSupportsPreview = (temptag.tag.intvalue & 128) > 0;
						SecIdentSupRec +=  2;
						break;
					case ET_MOD_VERSION:
						if (temptag.tag.type == 2) {
							m_strModVersion = temptag.tag.stringvalue;
						} else if (temptag.tag.type == 3) {
							m_strModVersion.Printf(wxT("ModID=%u"), temptag.tag.intvalue);
						} else {
							m_strModVersion = wxT("ModID=<Unknown>");
						}
	
						break;

					default:
						//printf("Mule Unk Tag 0x%02x=%x\n", temptag.tag.specialtag, (UINT)temptag.tag.intvalue);
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
	}
	catch ( CStrangePacket )
	{
		printf("\nWrong Tags on Mule Info packet!!\n");
		printf("Sent by %s on ip %s port %i using client %x version %x\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		printf("Packet Dump:\n");
		DumpMem(pachPacket,nSize);
		throw wxString(wxT("Wrong Tags on Mule Info packet"));
	}
	catch ( CInvalidPacket (e))
	{
		printf("Wrong Tags on Mule Info packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %x version %x\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		printf("Packet Dump:\n");
		DumpMem(pachPacket,nSize);
		throw wxString(wxT("Wrong Tags on Mule Info packet"));
	}

	return (protocol_version == 0xFF); // This was a OS_Info?
	
}

void CUpDownClient::SendHelloAnswer()
{

	if (m_socket == NULL){
		wxASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	SendHelloTypePacket(&data);
	Packet* packet = new Packet(&data);
	packet->SetOpCode(OP_HELLOANSWER);

	theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true);

	#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
	AddDebugLogLineM(true, wxT("Local Client: OP_HELLOANSWER to ") + GetFullIP());
	#endif
}


void CUpDownClient::SendHelloTypePacket(CSafeMemFile* data)
{
	data->WriteHash16(thePrefs::GetUserHash());
	data->WriteUInt32(theApp.serverconnect->GetClientID());
	data->WriteUInt16(thePrefs::GetPort());

	#ifdef __CVS__
	// Kry - This is the tagcount!!! Be sure to update it!!
	// Last update: CT_EMULECOMPAT_OPTIONS included
	data->WriteUInt32(7);
	#else
	data->WriteUInt32(6);  // NO MOD_VERSION
	#endif


	CTag tagname(CT_NAME,thePrefs::GetUserNick());
	tagname.WriteTagToFile(data, utf8strRaw);

	CTag tagversion(CT_VERSION,EDONKEYVERSION);
	tagversion.WriteTagToFile(data);
	// eMule UDP Ports

	uint32 kadUDPPort = 0;
	#ifdef __USE_KAD__
	if(Kademlia::CKademlia::isConnected())
	{
		kadUDPPort = thePrefs::GetUDPPort();
	}
	#endif
	CTag tagUdpPorts(CT_EMULE_UDPPORTS,
				(kadUDPPort									<< 16) |
				((uint32)thePrefs::GetUDPPort()         ) );
	tagUdpPorts.WriteTagToFile(data);

	// aMule Version
	CTag tagMuleVersion(CT_EMULE_VERSION,
				(SO_AMULE	<< 24) |
				(VERSION_MJR			<< 17) |
				(VERSION_MIN			<< 10) |
				(VERSION_UPDATE			<<  7)//|
				//(RESERVED			     )
				);
	tagMuleVersion.WriteTagToFile(data);


	// eMule Misc. Options #1
	const UINT uUdpVer				= 4;
	const UINT uDataCompVer			= 1;
	const UINT uSupportSecIdent		= theApp.clientcredits->CryptoAvailable() ? 3 : 0;
	const UINT uSourceExchangeVer	= 3; 
	const UINT uExtendedRequestsVer	= 2;
	const UINT uAcceptCommentVer	= 1;
	const UINT uNoViewSharedFiles	= (thePrefs::CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
	const UINT uMultiPacket			= 1;
	const UINT uSupportPreview		= 0; // No network preview at all.
	const UINT uPeerCache			= 0; // No peercache for aMule, baby
	const UINT uUnicodeSupport		= 1; // No unicode support yet.
	const UINT nAICHVer				= 1; // AICH is ENABLED right now.

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


	const UINT nOSInfoSupport			= 1; // We support OS_INFO
	
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
	if (theApp.serverconnect->IsConnected()) {
		dwIP = theApp.serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp.serverconnect->GetCurrentServer()->GetPort();
	}
	data->WriteUInt32(dwIP);
	data->WriteUInt16(nPort);
}


void CUpDownClient::ProcessMuleCommentPacket(const char *pachPacket, uint32 nSize)
{
	char* desc = NULL;

	try
	{
		if (!m_reqfile) {
			throw CInvalidPacket("Comment packet for unknown file");
		}

		if (!m_reqfile->IsPartFile()) {
			throw CInvalidPacket("Comment packet for completed file");
		}

		const CSafeMemFile data((BYTE*)pachPacket, nSize);

		m_iRate = data.ReadUInt8();
		m_reqfile->SetHasRating(true);
		AddDebugLogLineM(false, wxT("Rating for file '") + m_clientFilename + wxString::Format(wxT("' received: %i"), m_iRate));

		uint32 length = data.ReadUInt32();

		// Avoid triggering exception, even if part of the comment is missing
		if ( length > data.GetLength() - data.GetPosition() ) {
			length = data.GetLength() - data.GetPosition();
		}

		if ( length > 50 ) {
			length = 50;
		}

		if ( length > 0 ) {
			#warning Lacks Comment Filtering

			desc = new char[length + 1];
			desc[length] = 0;

			data.Read(desc, length);

			m_strComment = char2unicode(desc);

			AddDebugLogLineM(false, wxT("Description for file '") + m_clientFilename + wxT("' received: ") + m_strComment);

			m_reqfile->SetHasComment(true);
		}

	}
	catch ( CStrangePacket )
	{
		delete[] desc;

		printf("\nInvalid MuleComment packet!\n");
		printf("Sent by %s on ip %s port %i using client %i version %i\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		throw wxString(wxT("Wrong MuleComment packet"));
	}
	catch ( CInvalidPacket e )
	{
		delete[] desc;

		printf("\nInvalid MuleComment packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %i version %i\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		throw wxString(wxT("Wrong MuleComment packet"));
	}
	catch (...)
	{
		delete[] desc;

		printf("\nInvalid MuleComment packet - Uncatched exception\n\n");
		printf("Sent by %s on ip %s port %i using client %i version %i\n",unicode2char(GetUserName()),unicode2char(GetFullIP()),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		throw wxString(wxT("Wrong MuleComment packet"));
	}

	if (!m_strComment.IsEmpty() || m_iRate > 0) {
		m_reqfile->UpdateFileRatingCommentAvail();
		Notify_DownloadCtrlUpdateItem(m_reqfile);
	}

	delete[] desc;
}

void CUpDownClient::ClearDownloadBlockRequests()
{
	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;){
		Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetNext(pos);
		if (m_reqfile){
			m_reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
		}
		delete cur_block;
	}
	m_DownloadBlocks_list.RemoveAll();

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;){
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
		if (m_reqfile){
			m_reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
		}

		delete pending->block;
		// Not always allocated
		if (pending->zStream){
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}
	m_PendingBlocks_list.RemoveAll();
}

bool CUpDownClient::Disconnected(const wxString& strReason, bool bFromSocket){
	//If this is a KAD client object, just delete it!
	//wxASSERT(theApp.clientlist->IsValidClient(this));

	#ifdef __USE_KAD__
	SetKadState(KS_NONE);
	#endif
	
	//printf("Client disconnected! (%s)\n",unicode2char(strReason));
	
	if (GetUploadState() == US_UPLOADING) {
		theApp.uploadqueue->RemoveFromUploadQueue(this);
	}

	if (GetDownloadState() == DS_DOWNLOADING) {
		SetDownloadState(DS_ONQUEUE);
	}
	else{
		// ensure that all possible block requests are removed from the partfile
		ClearDownloadBlockRequests();

		if(GetDownloadState() == DS_CONNECTED){
		    // client didn't responsed to our request for some reasons (remotely banned?)
		    // or it just doesn't has this file, so try to swap first
		    if (!SwapToAnotherFile(true, true, true, NULL)){
			    theApp.downloadqueue->RemoveSource(this);
			    //DEBUG_ONLY(AddDebugLogLine(false, "Removed %s from downloadqueue - didn't responsed to filerequests",GetUserName()));
		    }
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

	//wxASSERT(theApp.clientlist->IsValidClient(this));

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
			bDelete = true;
	};
	switch(m_nDownloadState){
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_ERROR:
			bDelete = true;
	};


	if (GetChatState() != MS_NONE){
		bDelete = false;
		Notify_ChatConnResult(this,false);
	}

	if (!bFromSocket && m_socket){
		wxASSERT (theApp.listensocket->IsValidSocket(m_socket));
		m_socket->Safe_Delete();
	}

	m_socket = NULL;

    if (m_iFileListRequested){
		AddLogLineM(true, wxString(_("Failed to retrieve shared files from ")) +GetUserName());
		m_iFileListRequested = 0;
	}
	if (m_Friend) {
		Notify_ChatRefreshFriend(m_Friend);
	}

	Notify_ClientCtrlRefreshClient( this );

	if (bDelete){
		AddDebugLogLineM(false, wxString::Format(wxT("--- Deleted client \"") + 
			GetClientFullInfo() + wxT("\"; Reason was ") + strReason + wxT("\n")));
		return true;
	}
	else{
		AddDebugLogLineM(false, wxString::Format(wxT("--- Disconnected client \"") + 
			GetClientFullInfo() + wxT("\"; Reason was ") + strReason + wxT("\n")));
		m_fHashsetRequesting = 0;
		SetSentCancelTransfer(0);
		m_bHelloAnswerPending = false;
		return false;
	}
}

//Returned bool is not if the TryToConnect is successful or not..
//false means the client was deleted!
//true means the client was not deleted!
bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon)
{
	if (theApp.listensocket->TooManySockets() && !bIgnoreMaxCon )  {
		if (!m_socket) {
			if(Disconnected(wxT("Too many connections"))) {
				Safe_Delete();
				return false;
			}
			return true;
		} else if (!m_socket->IsConnected()) {
			if(Disconnected(wxT("Too many connections"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}
	}

	#ifdef __USE_KAD__
	if( GetKadState() == KS_QUEUED_FWCHECK ) {
		SetKadState(KS_CONNECTING_FWCHECK);
	}
	#endif

	#ifdef __USE_KAD__
	if (HasLowID() && !theApp.DoCallback(this)) {
	#else
	if (HasLowID() && (theApp.serverconnect->GetClientID() < 16777216)) {
	#endif
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

	if (!m_socket) {
#ifdef TESTING_PROXY
//		m_socket = new CClientReqSocket(this, thePrefs::GetProxyData());
		m_socket = new CClientReqSocket(this);
#else
		m_socket = new CClientReqSocket(this);
#endif
		if (!m_socket->Create()) {
			m_socket->Safe_Delete();
			return true;
		}
	} else if (!m_socket->IsConnected()) {
		m_socket->Safe_Delete();
#ifdef TESTING_PROXY
//		m_socket = new CClientReqSocket(this, thePrefs::GetProxyData());
		m_socket = new CClientReqSocket(this);
#else
		m_socket = new CClientReqSocket(this);
#endif
		if (!m_socket->Create()) {
			m_socket->Safe_Delete();
			return true;
		}
	} else {
		ConnectionEstablished();
		return true;
	}
	if (HasLowID()) {
		if (GetDownloadState() == DS_CONNECTING) {
			SetDownloadState(DS_WAITCALLBACK);
		}
		if (GetUploadState() == US_CONNECTING) {
			if(Disconnected(wxT("LowID and US_CONNECTING")))
			{
				Safe_Delete();
				return false;
			}
			return true;
		}

		if (theApp.serverconnect->IsLocalServer(m_dwServerIP,m_nServerPort)) {
			CSafeMemFile data;
			data.WriteUInt32(m_nUserID);
			Packet* packet = new Packet(&data);
			packet->SetOpCode(OP_CALLBACKREQUEST);

			theApp.uploadqueue->AddUpDataOverheadServer(packet->GetPacketSize());
			theApp.serverconnect->SendPacket(packet);
			#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
			AddDebugLogLineM(true, wxT("Local Client: OP_CALLBACKREQUEST\n"));
			#endif
		} else {
			if (GetUploadState() == US_NONE && (!GetRemoteQueueRank() || m_bReaskPending)) {
				theApp.downloadqueue->RemoveSource(this);
				if(Disconnected(wxT("LowID and US_NONE and QR=0")))
				{
					Safe_Delete();
					return false;
				}
				return true;
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
		//printf("Connecting to source %x\n",this);
		m_socket->Connect(tmp, false);
		// We should send hello packets AFTER connecting!
		// so I moved it to OnConnect
	}
	return true;
}

void CUpDownClient::ConnectionEstablished()
{
	// check if we should use this client to retrieve our public IP
	if (theApp.GetPublicIP() == 0 && theApp.serverconnect->IsConnected() /* && m_fPeerCache */)
		SendPublicIPRequest();

// 0.42e
	#ifdef __USE_KAD__
	switch(GetKadState())
	{
		case KS_CONNECTING_FWCHECK:
            SetKadState(KS_CONNECTED_FWCHECK);
			break;
		case KS_QUEUED_BUDDY:
			SetKadState(KS_CONNECTED_BUDDY);
			break;
	}
	#endif

	// ok we have a connection, lets see if we want anything from this client
	if (GetChatState() == MS_CONNECTING || GetChatState() == MS_CHATTING) {
		Notify_ChatConnResult(this,true);
	}

	switch(GetDownloadState()) {
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
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
				Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
				theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet,true);
				#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
				AddDebugLogLineM(true, wxT("Local Client: OP_ACCEPTUPLOADREQ\n"));
				#endif
			}
	}
	if (m_iFileListRequested == 1) {
		Packet* packet = new Packet(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
		if (m_fSharedDirectories) {
			AddDebugLogLineM(true, wxT("Local Client: OP_ASKSHAREDDIRS\n"));
		} else {
			AddDebugLogLineM(true, wxT("Local Client: OP_ASKSHAREDFILES\n"));
		}
		#endif
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
		return;
	}

	int iHashType = GetHashType();
	if (iHashType == SO_EMULE) {
		switch(m_byCompatibleClient){
			case SO_CDONKEY:
				m_clientSoft = SO_CDONKEY;
				m_clientVerString = wxT("cDonkey");
				break;
			case SO_LXMULE:
				m_clientSoft = SO_LXMULE;
				if(GetClientModString().IsEmpty() == false) {
					m_clientVerString = GetClientModString();
				} else {
					m_clientVerString = wxT("xMule");
				}
				if ((GetMuleVersion() > 0x26) && (GetMuleVersion() != 0x99)) {
					m_clientVerString += wxString::Format(_(" (Fake eMule version %x)"),GetMuleVersion());
				}
				break;
			case SO_AMULE:
				m_clientSoft = SO_AMULE;
				if(GetClientModString().IsEmpty() == false) {
					Extended_aMule_SO &= 2;
					m_clientVerString = GetClientModString();
				} else {
					m_clientVerString = wxT("aMule");
				}
				break;
			case SO_SHAREAZA:
				m_clientSoft = SO_SHAREAZA;
				m_clientVerString = wxT("Shareaza");
				break;
			case SO_MLDONKEY:
				m_clientSoft = SO_MLDONKEY;
				m_clientVerString = _("Old MlDonkey");
				break;
			case SO_NEW_MLDONKEY:
			case SO_NEW2_MLDONKEY:
				m_clientSoft = SO_NEW_MLDONKEY;
				m_clientVerString = _("New MlDonkey");
				break;
			case SO_LPHANT:
				m_clientSoft = SO_LPHANT;
				m_clientVerString = wxT("lphant");
				break;
			case SO_EMULEPLUS:
				m_clientSoft = SO_EMULEPLUS;
				m_clientVerString = wxT("eMule+");
				break;
			case SO_HYDRANODE:
				m_clientSoft = SO_HYDRANODE;
				m_clientVerString = wxT("HydraNode");
				break;
			default:
				if (m_bIsML){
					m_clientSoft = SO_MLDONKEY;
					m_clientVerString = wxT("MLdonkey");
				}
				else if (m_bIsHybrid){
					m_clientSoft = SO_EDONKEYHYBRID;
					m_clientVerString = wxT("eDonkeyHybrid");
				}
				else if (m_byCompatibleClient != 0){
					m_clientSoft = SO_COMPAT_UNK;
					#ifdef __DEBUG__
					printf("Compatible client found with ET_COMPATIBLECLIENT of 0x%x\n",m_byCompatibleClient);
					#endif
					m_clientVerString = wxString::Format(_("eMule Compat(0x%x)"),m_byCompatibleClient);
				}
				else if (wxString(GetClientModString()).MakeLower().Find(wxT("xmule"))!=-1 || GetUserName().Find(wxT("xmule."))!=-1) {
					// FAKE eMule -a newer xMule faking is ident.
					m_clientSoft = SO_LXMULE;
					if (GetClientModString().IsEmpty() == false) {
						m_clientVerString = GetClientModString() + _(" (Fake eMule)");
					} else {
						m_clientVerString = _("xMule (Fake eMule)");
					}
				} else {
					// If we step here, it might mean 2 things:
					// a eMule
					// a Compat Client that has sent no MuleInfo packet yet.
					m_clientSoft = SO_EMULE;
					m_clientVerString = wxT("eMule");
				}
		}

		m_SoftLen = m_clientVerString.Length();

		if (m_byEmuleVersion == 0){
			m_nClientVersion = MAKE_CLIENT_VERSION(0,0,0);
		} else if (m_byEmuleVersion != 0x99) {
			UINT nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
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
			UINT nClientMajVersion = (m_nClientVersion >> 17) & 0x7f;
			UINT nClientMinVersion = (m_nClientVersion >> 10) & 0x7f;
			UINT nClientUpVersion  = (m_nClientVersion >>  7) & 0x07;

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
				default:
					if (GetClientModString().IsEmpty() == false) {
						m_clientVerString +=  wxString::Format(wxT(" v%u.%u%c - "), nClientMajVersion, nClientMinVersion, 'a' + nClientUpVersion) + GetClientModString();
					} else {
						m_clientVerString +=  wxString::Format(wxT(" v%u.%u%c"), nClientMajVersion, nClientMinVersion, 'a' + nClientUpVersion);
					}
					break;
			}
		}
		return;
	}

	if (m_bIsHybrid){
		m_clientSoft = SO_EDONKEYHYBRID;
		// seen:
		// 105010	50.10
		// 10501	50.1
		// 1051		51.0
		// 501		50.1

		UINT nClientMajVersion;
		UINT nClientMinVersion;
		UINT nClientUpVersion;
		if (m_nClientVersion > 100000){
			UINT uMaj = m_nClientVersion/100000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*100000) / 100;
			nClientUpVersion = m_nClientVersion % 100;
		}
		else if (m_nClientVersion > 10000){
			UINT uMaj = m_nClientVersion/10000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*10000) / 10;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion > 1000){
			UINT uMaj = m_nClientVersion/1000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = m_nClientVersion - uMaj*1000;
			nClientUpVersion = 0;
		}
		else if (m_nClientVersion > 100){
			UINT uMin = m_nClientVersion/10;
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

		m_clientVerString = wxT("eDonkeyHybrid");

		m_SoftLen = m_clientVerString.Length();

		if (nClientUpVersion) {
			m_clientVerString += wxString::Format(wxT(" v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
		} else {
			m_clientVerString += wxString::Format(wxT(" v%u.%u"), nClientMajVersion, nClientMinVersion);
		}

		return;
	}

	if (m_bIsML || (iHashType == SO_MLDONKEY)){
		m_clientSoft = SO_MLDONKEY;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		m_clientVerString = wxT("MLdonkey");
		m_SoftLen = m_clientVerString.Length();
		m_clientVerString += wxString::Format(wxT(" v0.%u"), nClientMinVersion);
		return;
	}


	if (iHashType == SO_OLDEMULE){
		m_clientSoft = SO_OLDEMULE;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		m_clientVerString = _("Old eMule");
		m_SoftLen = m_clientVerString.Length();
		m_clientVerString += wxString::Format(wxT(" v0.%u"), nClientMinVersion);
		return;
	}

	m_clientSoft = SO_EDONKEY;
	UINT nClientMinVersion = m_nClientVersion;
	m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
	m_clientVerString = wxT("eDonkey");
	m_SoftLen = m_clientVerString.Length();
	m_clientVerString += wxString::Format(wxT(" v0.%u"), nClientMinVersion);

}

void CUpDownClient::RequestSharedFileList()
{
	if (m_iFileListRequested == 0) {
		AddDebugLogLineM(true,wxString(wxT("Requesting shared files from ")) + GetUserName());
		m_iFileListRequested = 1;
		TryToConnect(true);
	} else {
		AddDebugLogLineM(true,wxString(wxT("Requesting shared files from user ")) + GetUserName() + wxString::Format(wxT(" (%u) is already in progress"),GetUserID()));
	}
}

void CUpDownClient::ProcessSharedFileList(const char* pachPacket, uint32 nSize, LPCTSTR pszDirectory) {
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
	m_iRate=0;
	m_strComment = wxEmptyString;

	if (m_pReqFileAICHHash != NULL) {
		delete m_pReqFileAICHHash;
		m_pReqFileAICHHash = NULL;
	}

}

wxString CUpDownClient::GetUploadFileInfo()
{
	if(this == NULL) return wxEmptyString;
	wxString sRet;

	// build info text and display it
	sRet = _("NickName: ");
	sRet += GetUserName() + wxString::Format(wxT(" ID: %u "),GetUserID());
	if (m_reqfile) {
		sRet += _("Requested:") + wxString(m_reqfile->GetFileName()) + wxT("\n");
		wxString stat;
		stat.Printf(_("Filestats for this session: Accepted %d of %d requests, %s transferred\n")+wxString(_("Filestats for all sessions: Accepted %d of %d requests")),
		m_reqfile->statistic.GetAccepts(), m_reqfile->statistic.GetRequests(), unicode2char(CastItoXBytes(m_reqfile->statistic.GetTransfered())),
		m_reqfile->statistic.GetAllTimeAccepts(),
		m_reqfile->statistic.GetAllTimeRequests(), unicode2char(CastItoXBytes(m_reqfile->statistic.GetAllTimeTransfered())));
		sRet += stat;
	} else {
		sRet += _("Requested unknown file");
	}
	return sRet;
	return wxEmptyString;
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false it is _possible_ that this clientobject was deleted, because the connectiontry fails
bool CUpDownClient::SafeSendPacket(Packet* packet)
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

	CSafeMemFile data;
	data.WriteUInt8(theApp.clientcredits->GetPubKeyLen());
	data.Write(theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
	Packet* packet = new Packet(&data, OP_EMULEPROT);
	packet->SetOpCode(OP_PUBLICKEY);

	theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true,true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
	#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
	AddDebugLogLineM(true, wxT("Local Client: OP_PUBLICKEY\n"));
	#endif
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
		AddDebugLogLineM(false, wxString(wxT("Want to send signature but challenge value is invalid - User ")) + GetUserName());
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
	uchar achBuffer[250];

	uint8 siglen = theApp.clientcredits->CreateSignature(credits, achBuffer,  250, ChallengeIP, byChaIPKind );
	if (siglen == 0){
		wxASSERT ( false );
		return;
	}
	CSafeMemFile data;
	data.WriteUInt8(siglen);
	data.Write(achBuffer, siglen);
	if (bUseV2) {
		data.WriteUInt8(byChaIPKind);
	}
	Packet* packet = new Packet(&data, OP_EMULEPROT);
	packet->SetOpCode(OP_SIGNATURE);

	theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true,true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
	#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
	AddDebugLogLineM(true, wxT("Local Client: OP_SIGNATURE\n"));
	#endif
}


void CUpDownClient::ProcessPublicKeyPacket(const uchar* pachPacket, uint32 nSize)
{
	theApp.clientlist->AddTrackClient(this);

	///* delete this line later*/ DEBUG_ONLY(AddDebugLogLine(false, "recieving public key from '%s'", GetUserName()));
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
			AddDebugLogLineM(false, wxT("Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket"));
		}
	}
	else{
		AddDebugLogLineM(false, wxT("Failed to use new recieved public key"));
	}
}

void CUpDownClient::ProcessSignaturePacket(const uchar* pachPacket, uint32 nSize){
	///* delete this line later*/ DEBUG_ONLY(AddDebugLogLine(false, "receiving signature from '%s'", GetUserName()));
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
		AddDebugLogLineM(false, wxT("recieved multiple signatures from one client"));
		return;
	}
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0){
		AddDebugLogLineM(false, wxT("recieved signature for client without public key"));
		return;
	}
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0){
		AddDebugLogLineM(false, wxT("recieved signature for client with invalid challenge value - User ") + GetUserName());
		return;
	}

	if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind ) ){
		// result is saved in function abouve
		//AddDebugLogLine(false, "'%s' has passed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);
	}
	else {
		AddDebugLogLineM(false,  GetUserName() + wxString::Format(wxT(" has failed the secure identification, V2 State: %i"), byChaIPKind));
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
			AddDebugLogLineM(false, wxT("Not sending SecIdentState Packet, because State is Zero"));
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;

		CSafeMemFile data;
		data.WriteUInt8(nValue);
		data.WriteUInt32(dwRandom);
		Packet* packet = new Packet(&data, OP_EMULEPROT);
		packet->SetOpCode(OP_SECIDENTSTATE);

		theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
		AddDebugLogLineM(true, wxT("Local Client: OP_SECIDENTSTATE\n"));
		#endif
	} else {
		wxASSERT ( false );
	}
}


void CUpDownClient::ProcessSecIdentStatePacket(const uchar* pachPacket, uint32 nSize)
{
	if ( nSize != 5 ) {
		return;
	}

	if ( !credits ) {
		wxASSERT( credits );
		return;
	}

	CSafeMemFile data((BYTE*)pachPacket,nSize);

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


bool CUpDownClient::CheckHandshakeFinished(UINT WXUNUSED(protocol), UINT WXUNUSED(opcode)) const
{
	if (m_bHelloAnswerPending){
		// this triggers way too often.. need more time to look at this -> only create a warning
		if (thePrefs::GetVerbose()) {
			AddLogLineM(false, _("Handshake not finished while processing packet."));
		}
		return false;
	}

	return true;
}


wxString CUpDownClient::GetClientFullInfo() {

	if (m_clientVerString.IsEmpty()) {
		ReGetClientSoft();
	}

	wxString FullVerName;
	FullVerName = _("Client ");
	if (m_Username.IsEmpty()) {
		FullVerName += _("(Unknown)");
	} else {
		FullVerName += m_Username;
	}
	FullVerName += _(" on IP ") + GetFullIP() + wxString::Format(_(" port %u using "),GetUserPort()) + m_clientVerString;
	return (FullVerName);
}



void CUpDownClient::SendPublicIPRequest(){
	if (IsConnected()){
		Packet* packet = new Packet(OP_PUBLICIP_REQ,0,OP_EMULEPROT);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true);
		m_fNeedOurPublicIP = true;
		#ifdef DEBUG_LOCAL_CLIENT_PROTOCOL
		AddDebugLogLineM(true, wxT("Local Client: OP_PUBLICIP_REQ\n"));
		#endif
	}
}

void CUpDownClient::ProcessPublicIPAnswer(const BYTE* pbyData, UINT uSize){
	if (uSize != 4)
		throw wxString(wxT("Wrong Packet size on Public IP answer\n"));
	uint32 dwIP = PeekUInt32(pbyData);
	if (m_fNeedOurPublicIP == true){ // did we?
		m_fNeedOurPublicIP = false;
		if (theApp.GetPublicIP() == 0 && !IsLowIDED2K(dwIP) )
			theApp.SetPublicIP(dwIP);
	}
}


bool CUpDownClient::IsConnected() const
{
	return m_socket && m_socket->IsConnected();
}

bool CUpDownClient::SendPacket(Packet* packet, bool delpacket, bool controlpacket)
{
	if ( m_socket ) {
		return m_socket->SendPacket(packet, delpacket, controlpacket );
	} else {
//#ifndef AMULE_DAEMON
		printf("CAUGHT DEAD SOCKET IN SENDPACKET()\n");
//#endif
		return false;
	}
}

bool CUpDownClient::SetDownloadLimit(uint32 limit)
{
	if ( m_socket ) {
		m_socket->SetDownloadLimit( limit );
		return true;
	} else {
#ifndef AMULE_DAEMON
		printf("CAUGHT DEAD SOCKET IN SETDOWNLOADLIMIT()\n");
#endif
		return false;
	}
}

bool CUpDownClient::DisableDownloadLimit()
{
	if ( m_socket ) {
		m_socket->DisableDownloadLimit();
		return true;
	} else {
#ifndef AMULE_DAEMON
		printf("CAUGHT DEAD SOCKET IN DISABLEDOWNLOADLIMIT()\n");
#endif
		return false;
	}
}


void CUpDownClient::SetUserID(uint32 nUserID)
{
	theApp.clientlist->UpdateClientID( this, nUserID );

	m_nUserID = nUserID;
}


void CUpDownClient::SetIP( uint32 val )
{
	theApp.clientlist->UpdateClientIP( this, val );

	m_dwUserIP = val;

	#warning Kry - WHY OH WHY!!!!!!!
	m_nConnectIP = ENDIAN_SWAP_32(val);
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
	if (m_bUnicodeSupport)
		return utf8strRaw;
#endif
	return utf8strNone;
}

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/wx.h>
#include <wx/mstream.h>
#include <wx/tokenzr.h>

#include "updownclient.h"	// Needed for CUpDownClient
#include "SharedFileList.h"	// Needed for CSharedFileList

#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/ed2k/ClientSoftware.h>
#include <protocol/kad/Client2Client/UDP.h>
#include <protocol/kad2/Constants.h>
#include <protocol/kad2/Client2Client/TCP.h>
#include <protocol/kad2/Client2Client/UDP.h>

#include <common/ClientVersion.h>

#include <tags/ClientTags.h>

#include <zlib.h>		// Needed for inflateEnd

#include <common/Format.h>	// Needed for CFormat

#include "SearchList.h"		// Needed for CSearchList
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "IPFilter.h"		// Needed for CIPFilter
#include "ServerConnect.h"	// Needed for CServerConnect
#include "ClientCredits.h"	// Needed for CClientCredits
#include "ClientCreditsList.h"	// Needed for CClientCreditsList
#include "Server.h"		// Needed for CServer
#include "Preferences.h"	// Needed for CPreferences
#include "MemFile.h"		// Needed for CMemFile
#include "Packet.h"		// Needed for CPacket
#include "Friend.h"		// Needed for CFriend
#include "ClientList.h"		// Needed for CClientList
#ifndef AMULE_DAEMON
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CaptchaDialog.h"	// Needed for CCaptchaDialog
#include "CaptchaGenerator.h"
#include "ChatWnd.h"		// Needed for CChatWnd
#endif
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "FriendList.h"		// Needed for CFriendList
#include "Statistics.h"		// Needed for theStats
#include "ClientUDPSocket.h"
#include "Logger.h"
#include "DataToText.h"		// Needed for GetSoftName()
#include "GuiEvents.h"		// Needed for Notify_
#include "ServerList.h"		// For CServerList

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "kademlia/kademlia/Search.h"
#include "kademlia/kademlia/UDPFirewallTester.h"
#include "kademlia/routing/RoutingZone.h"


//#define __PACKET_DEBUG__


// some client testing variables
static wxString crash_name = wxT("[Invalid User Name]");
static wxString empty_name = wxT("[Empty User Name]");

//	members of CUpDownClient
//	which are used by down and uploading functions


CUpDownClient::CUpDownClient(CClientTCPSocket* sender)
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
		SetUserIDHybrid( wxUINT32_SWAP_ALWAYS(in_userid) );
	} else {
		SetUserIDHybrid( in_userid);
	}
	
	//If highID and ED2K source, incoming ID and IP are equal..
	//If highID and Kad source, incoming IP needs swap for the IP

	if (!HasLowID()) {
		if (ed2kID) {
			m_nConnectIP = in_userid;
		} else {
			m_nConnectIP = wxUINT32_SWAP_ALWAYS(in_userid);
		}
		// Will be on right endianess now
		m_FullUserIP = m_nConnectIP;
	}

	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
	SetRequestFile( in_reqfile );
	ReGetClientSoft();

	if (checkfriend) {
		if ((m_Friend = theApp->friendlist->FindFriend(CMD4Hash(), m_dwUserIP, m_nUserPort)) != NULL){
			m_Friend->LinkClient(CCLIENTREF(this, wxT("CUpDownClient::CUpDownClient m_Friend->LinkClient")));
		} else{
			// avoid that an unwanted client instance keeps a friend slot
			m_bFriendSlot = false;
		}	
	}
	
}

void CUpDownClient::Init()
{
	m_linked = 0;
#ifdef DEBUG_ZOMBIE_CLIENTS
	m_linkedDebug = false;
#endif
	m_bAddNextConnect = false;
	credits = NULL;
	m_byChatstate = MS_NONE;
	m_nKadState = KS_NONE;
	m_nChatCaptchaState = CA_NONE;
	m_cShowDR = 0;
	m_reqfile = NULL;	 // No file required yet
	m_nTransferredUp = 0;
	m_cSendblock = 0;
	m_cAsked = 0;
	msReceivedPrev = 0;
	kBpsDown = 0.0;
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
	m_nTransferredDown = 0;
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
	m_waitingPosition = 0;
	m_score = 0;
	m_lastRefreshedDLDisplay = 0;
	m_bHelloAnswerPending = false;
	m_fSentCancelTransfer = 0;
	m_Aggressiveness = 0;
	m_LastFileRequest = 0;

	m_clientState = CS_NEW;

	ClearHelloProperties();

	m_pReqFileAICHHash = NULL;
	m_fSupportsAICH = 0;
	m_fAICHRequested = 0;
	m_fSupportsLargeFiles = 0;
	m_fExtMultiPacket = 0;
	m_fIsSpammer = 0;

	m_dwUserIP = 0;
	m_nConnectIP = 0;
	m_dwServerIP = 0;

	m_fNeedOurPublicIP = false;
	m_bHashsetRequested = false;

	m_lastDownloadingPart = 0;

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
		SetIP(StringIPtoUint32(address.IPAddress()));
	} else {
		SetIP(0);
	}

	/* Statistics */
	m_lastClientSoft = (uint32)(-1);
	m_lastClientVersion = 0;
	
	/* Creation time (for buddies timeout) */
	m_nCreationTime = ::GetTickCount();
	
	m_MaxBlockRequests = STANDARD_BLOCKS_REQUEST; // Safe starting amount

	m_last_block_start = 0;
	m_lastaverage = 0;

	SetLastBuddyPingPongTime();	
	m_fRequestsCryptLayer = 0;
	m_fSupportsCryptLayer = 0;
	m_fRequiresCryptLayer = 0;
	m_fSupportsSourceEx2 = 0;
	m_fSupportsCaptcha = 0;
	m_fDirectUDPCallback = 0;
	m_dwDirectCallbackTimeout = 0;

	m_hasbeenobfuscatinglately = false;

	m_cCaptchasSent = 0;
	m_cMessagesReceived = 0;
	m_cMessagesSent = 0;

}


CUpDownClient::~CUpDownClient()
{
	#ifdef __DEBUG__
	if (!connection_reason.IsEmpty()) {
		AddDebugLogLineN(logClient, wxT("Client to check for ") + connection_reason + wxT(" was deleted without connection."));
	}
	#endif	
	
	
	if (m_lastClientSoft == SO_UNKNOWN) {
		theStats::RemoveUnknownClient();
	} else if (m_lastClientSoft != (uint32)(-1)) {
		theStats::RemoveKnownClient(m_lastClientSoft, m_lastClientVersion, m_lastOSInfo);
	}
		
	// Indicate that we are not anymore on stats
	m_lastClientSoft = (uint32)(-1);
	
	// The socket should have been removed in Safe_Delete, but it
	// doesn't hurt to have an extra check.
	if (m_socket) {
		m_socket->Safe_Delete();
		// Paranoia
		SetSocket(NULL);
	}

	ClearUploadBlockRequests();
	ClearDownloadBlockRequests();

	DeleteContents(m_WaitingPackets_list);
	
	// Allow detection of deleted clients that didn't go through Safe_Delete
 	m_clientState = CS_DYING;
}


void CUpDownClient::ClearHelloProperties()
{
	m_nUDPPort = 0;
	m_byUDPVer = 0;
	m_byDataCompVer = 0;
	m_byEmuleVersion = 0;
	m_bySourceExchange1Ver = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_byCompatibleClient = 0;
	m_nKadPort = 0;
	m_bySupportSecIdent = 0;
	m_bSupportsPreview = 0;
	m_nClientVersion = 0;
	m_fSharedDirectories = 0;
	m_bMultiPacket = 0;
	m_fOsInfoSupport = 0;
	m_fValueBasedTypeTags = 0;
	SecIdentSupRec = 0;
	m_byKadVersion = 0;
	m_fRequestsCryptLayer = 0;
	m_fSupportsCryptLayer = 0;
	m_fRequiresCryptLayer = 0;
	m_fSupportsSourceEx2 = 0;
	m_fSupportsCaptcha = 0;
	m_fDirectUDPCallback = 0;
}

bool CUpDownClient::ProcessHelloPacket(const byte* pachPacket, uint32 nSize)
{
	const CMemFile data(pachPacket,nSize);
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

	// If called from background, post an event to process it in main thread
	if (!wxThread::IsMain()) {
		CoreNotify_Client_Delete(CCLIENTREF(this, wxT("CUpDownClient::Safe_Delete CoreNotify_Client_Delete")));
		return;
	}

 	m_clientState = CS_DYING;

	// Make sure client doesn't get deleted until this method is finished
	CClientRef ref(CCLIENTREF(this, wxT("CUpDownClient::Safe_Delete reflocker")));

	// Close the socket to avoid any more connections and related events
	if ( m_socket ) {
		m_socket->Safe_Delete();
		// Paranoia
		SetSocket(NULL);
	}

	// Remove the client from the clientlist if we still have it
	if ( theApp->clientlist ) {
		theApp->clientlist->RemoveClient( this );
	}

	// Doing what RemoveClient used to do. Just to be sure...
	if (theApp->uploadqueue) {
		theApp->uploadqueue->RemoveFromUploadQueue(this);
		theApp->uploadqueue->RemoveFromWaitingQueue(this);
	}
	if (theApp->downloadqueue) {
		theApp->downloadqueue->RemoveSource(this);
	}

	// For security, remove it from the lists unconditionally.
	Notify_SharedCtrlRemoveClient(ECID(), (CKnownFile*)NULL);
	Notify_SourceCtrlRemoveSource(ECID(), (CPartFile*)NULL);

	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
	}

	if (m_Friend) {
		m_Friend->UnLinkClient();	// this notifies
		m_Friend = NULL;
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

	delete m_pReqFileAICHHash;
	m_pReqFileAICHHash = NULL;

#ifdef DEBUG_ZOMBIE_CLIENTS
	if (m_linked > 1) {
		AddLogLineC(CFormat(wxT("Client %d still linked in %d places: %s")) % ECID() % (m_linked - 1) % GetLinkedFrom());
		m_linkedDebug = true;
	}
#endif
}


bool CUpDownClient::ProcessHelloAnswer(const byte* pachPacket, uint32 nSize)
{
	const CMemFile data(pachPacket,nSize);
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
					m_strModVersion = CFormat(wxT("ModID=%u")) % temptag.GetInt();
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
				SetKadPort((temptag.GetInt() >> 16) & 0xFFFF);
				m_nUDPPort = temptag.GetInt() & 0xFFFF;
				dwEmuleTags |= 1;
				#ifdef __PACKET_DEBUG__
				AddLogLineNS(CFormat(wxT("Hello type packet processing with eMule ports UDP=%i KAD=%i")) % m_nUDPPort % m_nKadPort);
				#endif
				break;
				
			case CT_EMULE_BUDDYIP:
				// 32 BUDDY IP
				m_nBuddyIP = temptag.GetInt();
				#ifdef __PACKET_DEBUG__
				AddLogLineNS(CFormat(wxT("Hello type packet processing with eMule BuddyIP=%u (%s)")) % m_nBuddyIP % Uint32toStringIP(m_nBuddyIP));
				#endif
				break;				
				
			case CT_EMULE_BUDDYUDP:
				// 16 --Reserved for future use--
				// 16 BUDDY Port
				m_nBuddyPort = (uint16)temptag.GetInt();
				#ifdef __PACKET_DEBUG__
				AddLogLineNS(CFormat(wxT("Hello type packet processing with eMule BuddyPort=%u")) % m_nBuddyPort);
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
				//	 1 PeerCache supported
				//	 1 No 'View Shared Files' supported
				//	 1 MultiPacket
				//  1 Preview
				uint32 flags = temptag.GetInt();
				m_fSupportsAICH			= (flags >> (4*7+1)) & 0x07;
				m_bUnicodeSupport		= (flags >> 4*7) & 0x01;
				m_byUDPVer				= (flags >> 4*6) & 0x0f;
				m_byDataCompVer			= (flags >> 4*5) & 0x0f;
				m_bySupportSecIdent		= (flags >> 4*4) & 0x0f;
				m_bySourceExchange1Ver	= (flags >> 4*3) & 0x0f;
				m_byExtendedRequestsVer	= (flags >> 4*2) & 0x0f;
				m_byAcceptCommentVer	= (flags >> 4*1) & 0x0f;
				m_fNoViewSharedFiles	= (flags >> 1*2) & 0x01;
				m_bMultiPacket			= (flags >> 1*1) & 0x01;
				m_fSupportsPreview		= (flags >> 1*0) & 0x01;
				dwEmuleTags |= 2;
				#ifdef __PACKET_DEBUG__
				AddLogLineNS(wxT("Hello type packet processing with eMule Misc Options:"));
				AddLogLineNS(CFormat(wxT("m_byUDPVer = %i")) % m_byUDPVer);
				AddLogLineNS(CFormat(wxT("m_byDataCompVer = %i")) % m_byDataCompVer);
				AddLogLineNS(CFormat(wxT("m_bySupportSecIdent = %i")) % m_bySupportSecIdent);
				AddLogLineNS(CFormat(wxT("m_bySourceExchangeVer = %i")) % m_bySourceExchange1Ver);
				AddLogLineNS(CFormat(wxT("m_byExtendedRequestsVer = %i")) % m_byExtendedRequestsVer);
				AddLogLineNS(CFormat(wxT("m_byAcceptCommentVer = %i")) % m_byAcceptCommentVer);
				AddLogLineNS(CFormat(wxT("m_fNoViewSharedFiles = %i")) % m_fNoViewSharedFiles);
				AddLogLineNS(CFormat(wxT("m_bMultiPacket = %i")) % m_bMultiPacket);
				AddLogLineNS(CFormat(wxT("m_fSupportsPreview = %i")) % m_fSharedDirectories);
				AddLogLineNS(wxT("That's all."));
				#endif
				SecIdentSupRec +=  1;
				break;
			}

			case CT_EMULE_MISCOPTIONS2:
				//  19 Reserved
				//   1 Direct UDP Callback supported and available
				//   1 Supports ChatCaptchas
				//   1 Supports SourceExachnge2 Packets, ignores SX1 Packet Version
				//   1 Requires CryptLayer
				//   1 Requests CryptLayer
				//   1 Supports CryptLayer
				//   1 Reserved (ModBit)
				//   1 Ext Multipacket (Hash+Size instead of Hash)
				//   1 Large Files (includes support for 64bit tags)
				//   4 Kad Version - will go up to version 15 only (may need to add another field at some point in the future)
				m_fDirectUDPCallback	= (temptag.GetInt() >> 12) & 0x01;
				m_fSupportsCaptcha	    = (temptag.GetInt() >> 11) & 0x01;
				m_fSupportsSourceEx2	= (temptag.GetInt() >> 10) & 0x01;
				m_fRequiresCryptLayer	= (temptag.GetInt() >>  9) & 0x01;
				m_fRequestsCryptLayer	= (temptag.GetInt() >>  8) & 0x01;
				m_fSupportsCryptLayer	= (temptag.GetInt() >>  7) & 0x01;
				// reserved 1
				m_fExtMultiPacket	= (temptag.GetInt() >>  5) & 0x01;
				m_fSupportsLargeFiles   = (temptag.GetInt() >>  4) & 0x01;
				m_byKadVersion		= (temptag.GetInt() >>  0) & 0x0f;
				dwEmuleTags |= 8;

				m_fRequestsCryptLayer &= m_fSupportsCryptLayer;
				m_fRequiresCryptLayer &= m_fRequestsCryptLayer;
			
				#ifdef __PACKET_DEBUG__
				AddLogLineNS(wxT("Hello type packet processing with eMule Misc Options 2:"));
				AddLogLineNS(CFormat(wxT("	m_fDirectUDPCallback	= %i")) % m_fDirectUDPCallback);
				AddLogLineNS(CFormat(wxT("	m_fSupportsCaptcha		= %i")) % m_fSupportsCaptcha);
				AddLogLineNS(CFormat(wxT("	m_fSupportsSourceEx2	= %i")) % m_fSupportsSourceEx2);
				AddLogLineNS(CFormat(wxT("	m_fRequiresCryptLayer	= %i")) % m_fRequiresCryptLayer);
				AddLogLineNS(CFormat(wxT("	m_fRequestsCryptLayer	= %i")) % m_fRequestsCryptLayer);
				AddLogLineNS(CFormat(wxT("	m_fSupportsCryptLayer	= %i")) % m_fSupportsCryptLayer);
				AddLogLineNS(CFormat(wxT("	m_fExtMultiPacket	= %i")) % m_fExtMultiPacket);
				AddLogLineNS(CFormat(wxT("	m_fSupportsLargeFiles	= %i")) % m_fSupportsLargeFiles);
				AddLogLineNS(CFormat(wxT("	KadVersion		= %u")) % m_byKadVersion);
				AddLogLineNS(wxT("That's all."));
				#endif			
				break;

			// Special tag for Compat. Clients Misc options.
			case CT_EMULECOMPAT_OPTIONS:
				//  1 Operative System Info
				//	1 Value-based-type int tags (experimental!)
				m_fValueBasedTypeTags	= (temptag.GetInt() >> 1*1) & 0x01;
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
		SetIP(StringIPtoUint32(address.IPAddress()));
	} else {
		throw wxString(wxT("Huh, socket failure. Avoided crash this time."));
	}

	if (thePrefs::AddServersFromClient()) {
		CServer* addsrv = new CServer(m_nServerPort, Uint32toStringIP(m_dwServerIP));
		addsrv->SetListName(addsrv->GetAddress());
		if (!theApp->AddServer(addsrv)) {
				delete addsrv;
		}
	}

	//(a)If this is a highID user, store the ID in the Hybrid format.
	//(b)Some older clients will not send a ID, these client are HighID users that are not connected to a server.
	//(c)Kad users with a *.*.*.0 IPs will look like a lowID user they are actually a highID user.. They can be detected easily
	//because they will send a ID that is the same as their IP..
	if(!HasLowID() || m_nUserIDHybrid == 0 || m_nUserIDHybrid == m_dwUserIP )  {
		SetUserIDHybrid(wxUINT32_SWAP_ALWAYS(m_dwUserIP));
	}
	
	// get client credits
	CClientCredits* pFoundCredits = theApp->clientcredits->GetCredit(m_UserHash);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp->clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			AddDebugLogLineN( logClient, CFormat( wxT("Client: %s (%s) Banreason: Userhash changed (Found in TrackedClientsList)") ) % GetUserName() % GetFullIP() );
			Ban();
		}
	} else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		AddDebugLogLineN( logClient, CFormat( wxT("Client: %s (%s) Banreason: Userhash changed") ) % GetUserName() % GetFullIP() );
		Ban();
	}

	if ((m_Friend = theApp->friendlist->FindFriend(m_UserHash, m_dwUserIP, m_nUserPort)) != NULL){
		m_Friend->LinkClient(CCLIENTREF(this, wxT("CUpDownClient::ProcessHelloTypePacket m_Friend->LinkClient")));
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

	if (GetKadPort() && GetKadVersion() > 1) {
		Kademlia::CKademlia::Bootstrap(wxUINT32_SWAP_ALWAYS(GetIP()), GetKadPort());
	}

	return bIsMule;
}


bool CUpDownClient::SendHelloPacket()
{
	if (m_socket == NULL) {
		wxFAIL;
		return true;
	}

	// if IP is filtered, don't greet him but disconnect...
	amuleIPV4Address address;
	m_socket->GetPeer(address);
	if ( theApp->ipfilter->IsFiltered(StringIPtoUint32(address.IPAddress()))) {
		if (Disconnected(wxT("IPFilter"))) {
			Safe_Delete();
			return false;
		}
		return true;
	}

	CMemFile data(128);
	data.WriteUInt8(16); // size of userhash
	SendHelloTypePacket(&data);
	
	CPacket* packet = new CPacket(data, OP_EDONKEYPROT, OP_HELLO);
	theStats::AddUpOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true);
	m_bHelloAnswerPending = true;
	AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_HELLO to ") + GetFullIP() );
	return true;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer, bool OSInfo) {

	if (m_socket == NULL){
		wxFAIL;
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

		CTagString tag1(ET_OS_INFO,theApp->GetOSType());
		tag1.WriteTagToFile(&data);
		
		m_OSInfo_sent = true; // So we don't send it again

	} else {

		// Normal MuleInfo packet

		// Kry - There's no point on upgrading to VBT tags here
		// as no client supporting it uses mule info packet.
		
		data.WriteUInt8(EMULE_PROTOCOL);

		// Tag number
		data.WriteUInt32(9);

		CTagInt32 tag1(ET_COMPRESSION,1);
		tag1.WriteTagToFile(&data);
		CTagInt32 tag2(ET_UDPVER,4);
		tag2.WriteTagToFile(&data);
		CTagInt32 tag3(ET_UDPPORT, thePrefs::GetEffectiveUDPPort());
		tag3.WriteTagToFile(&data);
		CTagInt32 tag4(ET_SOURCEEXCHANGE,3);
		tag4.WriteTagToFile(&data);
		CTagInt32 tag5(ET_COMMENTS,1);
		tag5.WriteTagToFile(&data);
		CTagInt32 tag6(ET_EXTENDEDREQUEST,2);
		tag6.WriteTagToFile(&data);

		uint32 dwTagValue = (theApp->CryptoAvailable() ? 3 : 0);
		// Kry - Needs the preview code from eMule
		/*
		// set 'Preview supported' only if 'View Shared Files' allowed
		if (thePrefs::CanSeeShares() != vsfaNobody) {
			dwTagValue |= 128;
		}
		*/
		CTagInt32 tag7(ET_FEATURES, dwTagValue);
		tag7.WriteTagToFile(&data);

		CTagInt32 tag8(ET_COMPATIBLECLIENT,SO_AMULE);
		tag8.WriteTagToFile(&data);
	
		// Support for tag ET_MOD_VERSION
		wxString mod_name(MOD_VERSION_LONG);
		CTagString tag9(ET_MOD_VERSION, mod_name);
		tag9.WriteTagToFile(&data);
		// Maella end
	
	}

	packet = new CPacket(data, OP_EMULEPROT, (bAnswer ? OP_EMULEINFOANSWER : OP_EMULEINFO));
	
	if (m_socket) {
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		
		if (!bAnswer) {
			if (!OSInfo) {
				AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_EMULEINFO to ") + GetFullIP() );
			} else {
				AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_EMULEINFO/OS_INFO to ") + GetFullIP() );
			}
		} else {
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_EMULEINFOANSWER to ") + GetFullIP() );
		}
	}
}

bool CUpDownClient::ProcessMuleInfoPacket(const byte* pachPacket, uint32 nSize)
{
	uint8 protocol_version;

	const CMemFile data(pachPacket,nSize);

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
					m_bySourceExchange1Ver = temptag.GetInt();
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
						m_strModVersion = CFormat(wxT("ModID=%u")) % temptag.GetInt();
					} else {
						m_strModVersion = wxT("ModID=<Unknown>");
					}

					break;

				default:
					AddDebugLogLineN( logPacketErrors,
						CFormat( wxT("Unknown Mule tag (%s) from client: %s") )
							% temptag.GetFullInfo()
							% GetClientFullInfo()
					);

					break;
			}
		}				

		if( m_byDataCompVer == 0 ){
			m_bySourceExchange1Ver = 0;
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
			m_bySourceExchange1Ver = 1;
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
		wxFAIL;
		return;
	}

	CMemFile data(128);
	SendHelloTypePacket(&data);
	CPacket* packet = new CPacket(data, OP_EDONKEYPROT, OP_HELLOANSWER);
	theStats::AddUpOverheadOther(packet->GetPacketSize());
	AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_HELLOANSWER to ") + GetFullIP() );
	SendPacket(packet,true);
}


void CUpDownClient::SendHelloTypePacket(CMemFile* data)
{
	data->WriteHash(thePrefs::GetUserHash());
	data->WriteUInt32(theApp->GetID());
	data->WriteUInt16(thePrefs::GetPort());

	uint32 tagcount = 6;

	if( theApp->clientlist->GetBuddy() && theApp->IsFirewalled() ) {
		tagcount += 2;
	}
	tagcount ++; // eMule misc flags 2 (kad version)
	
	#ifdef __SVN__
	// Kry - This is the tagcount!!! Be sure to update it!!
	// Last update: CT_EMULECOMPAT_OPTIONS included
	data->WriteUInt32(tagcount + 1);
	#else
	data->WriteUInt32(tagcount);  // NO MOD_VERSION
	#endif


	CTagString tagname(CT_NAME,thePrefs::GetUserNick());
	tagname.WriteTagToFile(data, utf8strRaw);

	CTagVarInt tagversion(CT_VERSION, EDONKEYVERSION, GetVBTTags() ? 0 : 32);
	tagversion.WriteTagToFile(data);
	// eMule UDP Ports

	uint32 kadUDPPort = 0;

	if(Kademlia::CKademlia::IsConnected()) {
		if (Kademlia::CKademlia::GetPrefs()->GetExternalKadPort() != 0 && Kademlia::CKademlia::GetPrefs()->GetUseExternKadPort() && Kademlia::CUDPFirewallTester::IsVerified()) {
			kadUDPPort = Kademlia::CKademlia::GetPrefs()->GetExternalKadPort();
		} else {
			kadUDPPort = Kademlia::CKademlia::GetPrefs()->GetInternKadPort();
		}
	}

	CTagVarInt tagUdpPorts(CT_EMULE_UDPPORTS, (kadUDPPort << 16) | ((uint32)thePrefs::GetEffectiveUDPPort()), GetVBTTags() ? 0 : 32);
	tagUdpPorts.WriteTagToFile(data);

	if( theApp->clientlist->GetBuddy() && theApp->IsFirewalled() ) {
		CTagVarInt tagBuddyIP(CT_EMULE_BUDDYIP, theApp->clientlist->GetBuddy()->GetIP(), GetVBTTags() ? 0 : 32);
		tagBuddyIP.WriteTagToFile(data);
	
		CTagVarInt tagBuddyPort(CT_EMULE_BUDDYUDP, 
//					( RESERVED												)
					((uint32)theApp->clientlist->GetBuddy()->GetUDPPort()  )
					, GetVBTTags() ? 0 : 32);
		tagBuddyPort.WriteTagToFile(data);
	}	
	
	// aMule Version
	CTagVarInt tagMuleVersion(CT_EMULE_VERSION,
				(SO_AMULE	<< 24) |
				make_full_ed2k_version(VERSION_MJR, VERSION_MIN, VERSION_UPDATE)
				// | (RESERVED			     )
				, GetVBTTags() ? 0 : 32);
	tagMuleVersion.WriteTagToFile(data);


	// eMule Misc. Options #1
	const uint32 uUdpVer			= 4;
	const uint32 uDataCompVer		= 1;
	const uint32 uSupportSecIdent		= theApp->CryptoAvailable() ? 3 : 0;
	const uint32 uSourceExchangeVer		= 3; 
	const uint32 uExtendedRequestsVer	= 2;
	const uint32 uAcceptCommentVer		= 1;
	const uint32 uNoViewSharedFiles		= (thePrefs::CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
	const uint32 uMultiPacket		= 1;
	const uint32 uSupportPreview		= 0; // No network preview at all.
	const uint32 uPeerCache			= 0; // No peercache for aMule, baby
	const uint32 uUnicodeSupport		= 1; 
	const uint32 nAICHVer			= 1; // AICH is ENABLED right now.

	CTagVarInt tagMisOptions(CT_EMULE_MISCOPTIONS1,
				(nAICHVer			<< ((4*7)+1)) |
				(uUnicodeSupport		<< 4*7) |
				(uUdpVer			<< 4*6) |
				(uDataCompVer			<< 4*5) |
				(uSupportSecIdent		<< 4*4) |
				(uSourceExchangeVer		<< 4*3) |
				(uExtendedRequestsVer		<< 4*2) |
				(uAcceptCommentVer		<< 4*1) |
				(uPeerCache			<< 1*3) |
				(uNoViewSharedFiles		<< 1*2) |
				(uMultiPacket			<< 1*1) |
				(uSupportPreview		<< 1*0) 
				, GetVBTTags() ? 0 : 32);
	tagMisOptions.WriteTagToFile(data);

	// eMule Misc. Options #2
	const uint32 uKadVersion		= KADEMLIA_VERSION;
	const uint32 uSupportLargeFiles		= 1;
	const uint32 uExtMultiPacket		= 1;
	const uint32 uReserved			= 0; // mod bit
	const uint32 uSupportsCryptLayer	= thePrefs::IsClientCryptLayerSupported() ? 1 : 0;
	const uint32 uRequestsCryptLayer	= thePrefs::IsClientCryptLayerRequested() ? 1 : 0;
	const uint32 uRequiresCryptLayer	= thePrefs::IsClientCryptLayerRequired() ? 1 : 0;	
	const uint32 uSupportsSourceEx2		= 1;
#ifdef AMULE_DAEMON
// captcha for daemon/remotegui not supported for now
	const uint32 uSupportsCaptcha		= 0;
#else
	const uint32 uSupportsCaptcha		= 1;
#endif
	// direct callback is only possible if connected to kad, tcp firewalled and verified UDP open (for example on a full cone NAT)
	const uint32 uDirectUDPCallback		= (Kademlia::CKademlia::IsRunning() && Kademlia::CKademlia::IsFirewalled()
						   && !Kademlia::CUDPFirewallTester::IsFirewalledUDP(true) && Kademlia::CUDPFirewallTester::IsVerified()) ? 1 : 0;

	CTagVarInt tagMisOptions2(CT_EMULE_MISCOPTIONS2, 
//				(RESERVED				     )
				(uDirectUDPCallback	<< 12) |
 				(uSupportsCaptcha	<< 11) |	
				(uSupportsSourceEx2	<< 10) |
				(uRequiresCryptLayer	<<  9) |
				(uRequestsCryptLayer	<<  8) |
				(uSupportsCryptLayer	<<  7) |
				(uReserved		<<  6) |
				(uExtMultiPacket	<<  5) |
				(uSupportLargeFiles	<<  4) |
				(uKadVersion		<<  0) 
				, GetVBTTags() ? 0 : 32	);
	tagMisOptions2.WriteTagToFile(data);

	const uint32 nOSInfoSupport			= 1; // We support OS_INFO
	const uint32 nValueBasedTypeTags	= 0; // Experimental, disabled
	
	CTagVarInt tagMisCompatOptions(CT_EMULECOMPAT_OPTIONS,
				(nValueBasedTypeTags	<< 1*1) |
				(nOSInfoSupport			<< 1*0) 
				, GetVBTTags() ? 0 : 32);
	
	tagMisCompatOptions.WriteTagToFile(data);

#ifdef __SVN__
	wxString mod_name(MOD_VERSION_LONG);
	CTagString tagModName(ET_MOD_VERSION, mod_name);
	tagModName.WriteTagToFile(data);
#endif

	uint32 dwIP = 0;
	uint16 nPort = 0;
	if (theApp->IsConnectedED2K()) {
		dwIP = theApp->serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp->serverconnect->GetCurrentServer()->GetPort();
	}
	data->WriteUInt32(dwIP);
	data->WriteUInt16(nPort);
}


void CUpDownClient::ProcessMuleCommentPacket(const byte* pachPacket, uint32 nSize)
{
	if (!m_reqfile) {
		throw CInvalidPacket(wxT("Comment packet for unknown file"));
	}

	if (!m_reqfile->IsPartFile()) {
		throw CInvalidPacket(wxT("Comment packet for completed file"));
	}

	const CMemFile data(pachPacket, nSize);

	uint8 rating = data.ReadUInt8();
	if (rating > 5) {
		AddDebugLogLineN( logClient, wxString(wxT("Invalid Rating for file '")) << m_clientFilename << wxT("' received: ") << rating);
		m_iRating = 0;
	} else {
		m_iRating = rating;
		AddDebugLogLineN( logClient, wxString(wxT("Rating for file '")) << m_clientFilename << wxT("' received: ") << m_iRating);
	}

	// The comment is unicoded, with a uin32 len and safe read 
	// (won't break if string size is < than advertised len)
	// Truncated to MAXFILECOMMENTLEN size
	m_strComment = data.ReadString((GetUnicodeSupport() != utf8strNone), 4 /* bytes (it's a uint32)*/, true).Left(MAXFILECOMMENTLEN);
	
	AddDebugLogLineN( logClient, wxString(wxT("Description for file '")) << m_clientFilename << wxT("' received: ") << m_strComment);

	// Update file rating
	m_reqfile->UpdateFileRatingCommentAvail();
}


void CUpDownClient::ClearDownloadBlockRequests()
{
	{
		std::list<Requested_Block_Struct*>::iterator it = m_DownloadBlocks_list.begin();
		for (; it != m_DownloadBlocks_list.end(); ++it) {
			Requested_Block_Struct* cur_block = *it;
			
			if (m_reqfile){
				m_reqfile->RemoveBlockFromList(cur_block->StartOffset, cur_block->EndOffset);
			}
			
			delete cur_block;
		}
		
		m_DownloadBlocks_list.clear();
	}

	{
		std::list<Pending_Block_Struct*>::iterator it = m_PendingBlocks_list.begin();
		for (; it != m_PendingBlocks_list.end(); ++it) {
			Pending_Block_Struct* pending = *it;
			
			if (m_reqfile) {
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
		
		m_PendingBlocks_list.clear();
	}
}


bool CUpDownClient::Disconnected(const wxString& strReason, bool bFromSocket)
{
	//wxASSERT(theApp->clientlist->IsValidClient(this));

	if (HasBeenDeleted()) {
		AddDebugLogLineN(logClient, wxT("Disconnected() called for already deleted client on ip ") + Uint32toStringIP(GetConnectIP()));
		return false;
	}
	
	// was this a direct callback?
	if (m_dwDirectCallbackTimeout != 0) {
		theApp->clientlist->RemoveDirectCallback(this);
		m_dwDirectCallbackTimeout = 0;
		theApp->clientlist->AddDeadSource(this);
		AddDebugLogLineN(logClient, wxT("Direct callback failed to client on ip ") + Uint32toStringIP(GetConnectIP()));
	}

	if (GetKadState() == KS_QUEUED_FWCHECK_UDP || GetKadState() == KS_CONNECTING_FWCHECK_UDP) {
		Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, true, wxUINT32_SWAP_ALWAYS(GetConnectIP()), 0); // inform the tester that this test was cancelled
	} else if (GetKadState() == KS_FWCHECK_UDP) {
		Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, false, wxUINT32_SWAP_ALWAYS(GetConnectIP()), 0); // inform the tester that this test has failed
	} else if (GetKadState() == KS_CONNECTED_BUDDY) {
		AddDebugLogLineN(logClient, wxT("Buddy client disconnected - ") + strReason);
	}

	//If this is a KAD client object, just delete it!
	SetKadState(KS_NONE);
	
	if (GetUploadState() == US_UPLOADING) {
		// sets US_NONE
		theApp->uploadqueue->RemoveFromUploadQueue(this);
	}

	if (GetDownloadState() == DS_DOWNLOADING) {
		SetDownloadState(DS_ONQUEUE);
	} else {
		// ensure that all possible block requests are removed from the partfile
		ClearDownloadBlockRequests();

		if (GetDownloadState() == DS_CONNECTED) {
			// successfully connected, but probably didn't respond to our filerequest
			theApp->clientlist->AddDeadSource(this);
			theApp->downloadqueue->RemoveSource(this);
	    }
	}

	// we had still an AICH request pending, handle it
	if (IsAICHReqPending()) {
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
	}

	// The remote client does not have to answer with OP_HASHSETANSWER *immediatly*
	// after we've sent OP_HASHSETREQUEST. It may occure that a (buggy) remote client
	// is sending use another OP_FILESTATUS which would let us change to DL-state to DS_ONQUEUE.
	if (((GetDownloadState() == DS_REQHASHSET) || m_fHashsetRequesting) && (m_reqfile)) {
		m_reqfile->SetHashSetNeeded(true);
	}

	SourceItemType source_type = UNAVAILABLE_SOURCE;
	SourceItemType peer_type = UNAVAILABLE_SOURCE;
	
	//check if this client is needed in any way, if not delete it
	bool bDelete = true;
	switch (m_nUploadState) {
		case US_ONUPLOADQUEUE:
			bDelete = false;
			peer_type = AVAILABLE_SOURCE;
			break;
	};
	
	switch (m_nDownloadState) {
		case DS_ONQUEUE:
			source_type = A4AF_SOURCE; // Will be checked.		
		case DS_TOOMANYCONNS:
		case DS_NONEEDEDPARTS:
		case DS_LOWTOLOWIP:
			bDelete = false;
			break;
	};

	switch (m_nUploadState) {
		case US_CONNECTING:
		case US_WAITCALLBACK:
		case US_ERROR:
			theApp->clientlist->AddDeadSource(this);
			bDelete = true;
	};

	switch (m_nDownloadState) {
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_ERROR:
		case DS_BANNED:
			theApp->clientlist->AddDeadSource(this);
			bDelete = true;
	};


	// We keep chat partners in any case
	if (GetChatState() != MS_NONE) {
		bDelete = false;
		m_pendingMessage.Clear();
		Notify_ChatConnResult(false,GUI_ID(GetIP(),GetUserPort()),wxEmptyString);
	}

	// Delete socket
	if (!bFromSocket && m_socket) {
		wxASSERT (theApp->listensocket->IsValidSocket(m_socket));
		m_socket->Safe_Delete();
	}

	SetSocket(NULL);

	if (m_iFileListRequested) {
		AddLogLineN(CFormat(_("Failed to retrieve shared files from user '%s'")) % GetUserName() );
		m_iFileListRequested = 0;
	}


	if (bDelete) {
		if (m_Friend) {
			// Remove the friend linkage
			m_Friend->UnLinkClient();	// this notifies
		}
	} else {
		Notify_SharedCtrlRefreshClient(ECID(), peer_type);
		Notify_SourceCtrlUpdateSource(ECID(), source_type);
		
		m_fHashsetRequesting = 0;
		SetSentCancelTransfer(0);
		m_bHelloAnswerPending = false;
		m_fSentOutOfPartReqs = 0;
	}
	AddDebugLogLineN(logClient, CFormat(wxT("--- %s client D:%d U:%d \"%s\"; Reason was %s"))
		% (bDelete ? wxT("Deleted") : wxT("Disconnected")) 
		% m_nDownloadState % m_nUploadState % GetClientFullInfo() % strReason );
	
	return bDelete;
}

//Returned bool is not if the TryToConnect is successful or not..
//false means the client was deleted!
//true means the client was not deleted!
bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon)
{
	// Kad reviewed
	if (theApp->listensocket->TooManySockets() && !bIgnoreMaxCon )  {
		if (!(m_socket && m_socket->IsConnected())) {
			if(Disconnected(wxT("Too many connections"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}
	}

	// Do not try to connect to source which are incompatible with our encryption setting (one requires it, and the other one doesn't supports it)
	if ( (RequiresCryptLayer() && !thePrefs::IsClientCryptLayerSupported()) || (thePrefs::IsClientCryptLayerRequired() && !SupportsCryptLayer()) ){
		if(Disconnected(wxT("CryptLayer-Settings (Obfuscation) incompatible"))){
			Safe_Delete();
			return false;
		} else {
			return true;
		}
	}	
	
	// Ipfilter check
	uint32 uClientIP = GetIP();
	if (uClientIP == 0 && !HasLowID()) {
		uClientIP = wxUINT32_SWAP_ALWAYS(m_nUserIDHybrid);
	}
	
	if (uClientIP) {
		// Although we filter all received IPs (server sources, source exchange) and all incomming connection attempts,
		// we do have to filter outgoing connection attempts here too, because we may have updated the ip filter list
		if (theApp->ipfilter->IsFiltered(uClientIP)) {
			AddDebugLogLineN(logIPFilter, CFormat(wxT("Filtered ip %u (%s) on TryToConnect\n")) % uClientIP % Uint32toStringIP(uClientIP));
			if (Disconnected(wxT("IPFilter"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}

		// for safety: check again whether that IP is banned
		if (theApp->clientlist->IsBannedClient(uClientIP)) {
			AddDebugLogLineN(logClient, wxT("Refused to connect to banned client ") + Uint32toStringIP(uClientIP));
			if (Disconnected(wxT("Banned IP"))) {
				Safe_Delete();
				return false;
			}
			return true;
		}
	}

	if (GetKadState() == KS_QUEUED_FWCHECK) {
		SetKadState(KS_CONNECTING_FWCHECK);
	} else if (GetKadState() == KS_QUEUED_FWCHECK_UDP) {
		SetKadState(KS_CONNECTING_FWCHECK_UDP);
	}

	if (HasLowID()) {
		if (!theApp->CanDoCallback(GetServerIP(), GetServerPort())) {
			//We cannot do a callback!
			if (GetDownloadState() == DS_CONNECTING) {
				SetDownloadState(DS_LOWTOLOWIP);
			} else if (GetDownloadState() == DS_REQHASHSET) {
				SetDownloadState(DS_ONQUEUE);
				m_reqfile->SetHashSetNeeded(true);
			}
			if (GetUploadState() == US_CONNECTING) {
				if(Disconnected(wxT("LowID->LowID and US_CONNECTING"))) {
					Safe_Delete();
					return false;
				}
			}
			return true;
		}

		//We already know we are not firewalled here as the above condition already detected LowID->LowID and returned.
		//If ANYTHING changes with the "if(!theApp->CanDoCallback(this))" above that will let you fall through 
		//with the condition that the source is firewalled and we are firewalled, we must
		//recheck it before the this check..
		if (HasValidBuddyID() && !GetBuddyIP() && !GetBuddyPort() && !theApp->serverconnect->IsLocalServer(GetServerIP(), GetServerPort())
		    && !(SupportsDirectUDPCallback() && thePrefs::GetEffectiveUDPPort() != 0)) {
			//This is a Kad firewalled source that we want to do a special callback because it has no buddyIP or buddyPort.
			if( Kademlia::CKademlia::IsConnected() ) {
				//We are connect to Kad
				if( Kademlia::CKademlia::GetPrefs()->GetTotalSource() > 0 || Kademlia::CSearchManager::AlreadySearchingFor(Kademlia::CUInt128(GetBuddyID()))) {
					//There are too many source lookups already or we are already searching this key.
					SetDownloadState(DS_TOOMANYCONNSKAD);
					return true;
				}
			}
		}
	}
	
	if (!m_socket || !m_socket->IsConnected()) {
		if (m_socket) {
			m_socket->Safe_Delete();
		}
		m_socket = new CClientTCPSocket(this, thePrefs::GetProxyData());
	} else {
		ConnectionEstablished();
		return true;
	}	
	
	
	if (HasLowID() && SupportsDirectUDPCallback() && thePrefs::GetEffectiveUDPPort() != 0 && GetConnectIP() != 0) { // LOWID with DirectCallback
		if (m_dwDirectCallbackTimeout != 0) {
			AddDebugLogLineN(logClient, wxT("ERROR: Trying Direct UDP Callback while already trying to connect to client on ip ") + Uint32toStringIP(GetConnectIP()));
			return true;	// We're already trying a direct connection to this client
		}
		// a direct callback is possible - since no other parties are involved and only one additional packet overhead 
		// is used we basically handle it like a normal connection try, no restrictions apply
		// we already check above with !theApp->CanDoCallback(this) if any callback is possible at all
		m_dwDirectCallbackTimeout = ::GetTickCount() + SEC2MS(45);
		theApp->clientlist->AddDirectCallbackClient(this);
		AddDebugLogLineN(logClient, CFormat(wxT("Direct Callback on port %u to client on ip %s")) % GetKadPort() % Uint32toStringIP(GetConnectIP()));

		CMemFile data;
		data.WriteUInt16(thePrefs::GetPort()); // needs to know our port
		data.WriteHash(thePrefs::GetUserHash()); // and userhash
		// our connection settings
		data.WriteUInt8(Kademlia::CPrefs::GetMyConnectOptions(true, false));
		AddDebugLogLineN(logClientUDP, wxT("Sending OP_DIRECTCALLBACKREQ to ") + Uint32_16toStringIP_Port(GetConnectIP(), GetKadPort()));
		CPacket* packet = new CPacket(data, OP_EMULEPROT, OP_DIRECTCALLBACKREQ);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		theApp->clientudp->SendPacket(packet, GetConnectIP(), GetKadPort(), ShouldReceiveCryptUDPPackets(), GetUserHash().GetHash(), false, 0);
	} else if (HasLowID()) { // LOWID
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

		if (theApp->serverconnect->IsLocalServer(m_dwServerIP,m_nServerPort)) {
			CMemFile data;
			// AFAICS, this id must be reversed to be sent to clients
			// But if I reverse it, we do a serve violation ;)
			data.WriteUInt32(m_nUserIDHybrid);
			CPacket* packet = new CPacket(data, OP_EDONKEYPROT, OP_CALLBACKREQUEST);
			theStats::AddUpOverheadServer(packet->GetPacketSize());
			AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_CALLBACKREQUEST to ") + GetFullIP());
			theApp->serverconnect->SendPacket(packet);
			SetDownloadState(DS_WAITCALLBACK);
		} else {
			if (GetUploadState() == US_NONE && (!GetRemoteQueueRank() || m_bReaskPending)) {
				
				if( !HasValidBuddyID() ) {
					theApp->downloadqueue->RemoveSource(this);
					if (Disconnected(wxT("LowID and US_NONE and QR=0"))) {
						Safe_Delete();
						return false;
					}
					return true;
				}
				
				if( !Kademlia::CKademlia::IsConnected() ) {
					//We are not connected to Kad and this is a Kad Firewalled source..
					theApp->downloadqueue->RemoveSource(this);
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
						CPacket* packet = new CPacket(bio, OP_KADEMLIAHEADER, KADEMLIA_CALLBACK_REQ);
						// eMule FIXME: We don't know which kadversion the buddy has, so we need to send unencrypted
						theApp->clientudp->SendPacket(packet, GetBuddyIP(), GetBuddyPort(), false, NULL, true, 0);
						AddDebugLogLineN(logClientKadUDP, CFormat(wxT("KadCallbackReq (size=%i) to %s")) % packet->GetPacketSize() % Uint32_16toStringIP_Port(GetBuddyIP(), GetBuddyPort()));
						theStats::AddUpOverheadKad(packet->GetRealPacketSize());
						SetDownloadState(DS_WAITCALLBACKKAD);
					} else {
						AddLogLineN(_("Searching buddy for lowid connection"));
						//Create search to find buddy.
						Kademlia::CSearch *findSource = new Kademlia::CSearch;
						findSource->SetSearchTypes(Kademlia::CSearch::FINDSOURCE);
						findSource->SetTargetID(Kademlia::CUInt128(GetBuddyID()));
						findSource->AddFileID(Kademlia::CUInt128(m_reqfile->GetFileHash().GetHash()));
						if(Kademlia::CSearchManager::StartSearch(findSource)) {
							//Started lookup..
							SetDownloadState(DS_WAITCALLBACKKAD);
						} else {
							//This should never happen..
							wxFAIL;
						}
					}
				}
			} else {
				if (GetDownloadState() == DS_WAITCALLBACK) {
					m_bReaskPending = true;
					SetDownloadState(DS_ONQUEUE);
				}
			}	
		}
	} else { // HIGHID
		if (!Connect()) {
			return false;
		}
	}
	return true;
}

bool CUpDownClient::Connect()
{
	m_hasbeenobfuscatinglately = false;
	
	if (!m_socket->IsOk()) {
		// Enable or disable crypting based on our and the remote clients preference
		if (HasValidHash() && SupportsCryptLayer() && thePrefs::IsClientCryptLayerSupported() && (RequestsCryptLayer() || thePrefs::IsClientCryptLayerRequested())){
			m_socket->SetConnectionEncryption(true, GetUserHash().GetHash(), false);
		} else {
			m_socket->SetConnectionEncryption(false, NULL, false);
		}
		amuleIPV4Address tmp;
		tmp.Hostname(GetConnectIP());
		tmp.Service(GetUserPort());
		AddDebugLogLineN(logClient, wxT("Trying to connect to ") + Uint32_16toStringIP_Port(GetConnectIP(),GetUserPort()));
		m_socket->Connect(tmp, false);
		// We should send hello packets AFTER connecting!
		// so I moved it to OnConnect	
		return true;
	} else {
		return false;
	}
}

void CUpDownClient::ConnectionEstablished()
{
	/* Kry - First thing, check if this client was just used to retrieve 
	   info. That's some debug thing for myself... check connection_reason
	   definition */
	
	m_hasbeenobfuscatinglately = (m_socket && m_socket->IsConnected() && m_socket->IsObfusicating());
	
	#ifdef __DEBUG__
	if (!connection_reason.IsEmpty()) {
		AddLogLineN(CFormat(wxT("Got client info checking for %s: %s\nDisconnecting and deleting.")) % connection_reason % GetClientFullInfo());
		connection_reason.Clear(); // So we don't re-print on destructor.
		Safe_Delete();
		return;
	}
	#endif
	
	// Check if we should use this client to retrieve our public IP
	// Ignore local ip on GetPublicIP (could be wrong)
	if (theApp->GetPublicIP(true) == 0 && theApp->IsConnectedED2K()) {
		SendPublicIPRequest();
	}
	
	// was this a direct callback?
	if (m_dwDirectCallbackTimeout != 0){
		theApp->clientlist->RemoveDirectCallback(this);
		m_dwDirectCallbackTimeout = 0;
		AddDebugLogLineN(logClient, wxT("Direct Callback succeeded, connection established to ") + Uint32toStringIP(GetConnectIP()));
	}

	switch (GetKadState()) {
		case KS_CONNECTING_FWCHECK:
			SetKadState(KS_CONNECTED_FWCHECK);
			break;
		case KS_CONNECTING_BUDDY:
		case KS_INCOMING_BUDDY:
			SetKadState(KS_CONNECTED_BUDDY);
			break;
		case KS_CONNECTING_FWCHECK_UDP:
			SetKadState(KS_FWCHECK_UDP);
			SendFirewallCheckUDPRequest();
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
			result = SendChatMessage(m_pendingMessage);
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
			if (theApp->uploadqueue->IsDownloading(this)) {
				SetUploadState(US_UPLOADING);
				CPacket* packet = new CPacket(OP_ACCEPTUPLOADREQ, 0, OP_EDONKEYPROT);
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet,true);
				AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ACCEPTUPLOADREQ to ") + GetFullIP() );
			}
	}
	if (m_iFileListRequested == 1) {
		CPacket* packet = new CPacket(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES, 0, OP_EDONKEYPROT);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		SendPacket(packet,true,true);
		if (m_fSharedDirectories) {
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDDIRS to ") + GetFullIP() );
		} else {
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDFILES to ") + GetFullIP() );
		}
	}
	
	while (!m_WaitingPackets_list.empty()) {
		CPacket* packet = m_WaitingPackets_list.front();
		m_WaitingPackets_list.pop_front();
		
		SendPacket(packet);
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


void CUpDownClient::SetSocket(CClientTCPSocket* socket)
{
#ifdef __DEBUG__
	if (m_socket == NULL && socket != NULL) {
		theStats::SocketAssignedToClient();
	} else if (m_socket != NULL && socket == NULL) {
		theStats::SocketUnassignedFromClient();
	}
#endif
	m_socket = socket;
}


void CUpDownClient::ReGetClientSoft()
{
	if (m_Username.IsEmpty()) {
		m_clientSoft=SO_UNKNOWN;
		m_clientVerString = m_clientSoftString = m_clientVersionString = m_fullClientVerString = _("Unknown");
		UpdateStats();
		return;
	}

	int iHashType = GetHashType();
	wxString clientModString;
	if (iHashType == SO_EMULE) {
		
		m_clientSoft = m_byCompatibleClient;
		m_clientSoftString = GetSoftName(m_clientSoft);
		// Special issues:
		if(!GetClientModString().IsEmpty() && (m_clientSoft != SO_EMULE)) {
			m_clientSoftString = GetClientModString();
		}
		// Isn't xMule annoying?
		if ((m_clientSoft == SO_LXMULE) && (GetMuleVersion() > 0x26) && (GetMuleVersion() != 0x99)) {
			m_clientSoftString += CFormat(_(" (Fake eMule version %#x)")) % GetMuleVersion();
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
				m_clientSoftString = GetClientModString() + _(" (Fake eMule)");
			} else {
				m_clientSoftString = _("xMule (Fake eMule)"); // don't use GetSoftName, it's not lmule.
			}
		}		
		// Now, what if we don't know this SO_ID?
		if (m_clientSoftString.IsEmpty()) {
			if(m_bIsML) {
				m_clientSoft = SO_MLDONKEY;
				m_clientSoftString = GetSoftName(m_clientSoft);
			} else if (m_bIsHybrid) {
				m_clientSoft = SO_EDONKEYHYBRID;
				m_clientSoftString = GetSoftName(m_clientSoft);
			} else if (m_byCompatibleClient != 0) {
				m_clientSoft = SO_COMPAT_UNK;
				#ifdef __DEBUG__
				if (
					// Exceptions:
					(m_byCompatibleClient != 0xf0)	// Chinese leech mod
					&& (1==1) 						// Your ad here
					) {
					AddLogLineNS(CFormat(wxT("Compatible client found with ET_COMPATIBLECLIENT of %x")) % m_byCompatibleClient);
				}
				#endif
				m_clientSoftString = CFormat(wxT("%s(%#x)")) % GetSoftName(m_clientSoft) % m_byCompatibleClient;
			} else {
				// If we step here, it might mean 2 things:
				// a eMule
				// a Compat Client that has sent no MuleInfo packet yet.
				m_clientSoft = SO_EMULE;
				m_clientSoftString = wxT("eMule");
			}
		}

		if (m_byEmuleVersion == 0) {
			m_nClientVersion = MAKE_CLIENT_VERSION(0,0,0);
		} else if (m_byEmuleVersion != 0x99) {
			uint32 nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
			m_nClientVersion = MAKE_CLIENT_VERSION(0,nClientMinVersion,0);
			switch (m_clientSoft) {
				case SO_AMULE:
					m_clientVerString = CFormat(_("1.x (based on eMule v0.%u)")) % nClientMinVersion;
					break;
				case SO_LPHANT:
					m_clientVerString = wxT("< v0.05");
					break;
				default:
					clientModString = GetClientModString();
					m_clientVerString = CFormat(wxT("v0.%u")) % nClientMinVersion;
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
				case SO_MLDONKEY:
				case SO_NEW_MLDONKEY:
				case SO_NEW2_MLDONKEY:					
					// Kry - xMule started sending correct version tags on 1.9.1b.
					// It only took them 4 months, and being told by me and the
					// eMule+ developers, so I think they're slowly getting smarter.
					// They are based on our implementation, so we use the same format
					// for the version string.
					m_clientVerString = CFormat(wxT("v%u.%u.%u")) % nClientMajVersion % nClientMinVersion % nClientUpVersion;
					break;
				case SO_LPHANT:
					m_clientVerString = CFormat(wxT(" v%u.%.2u%c")) % (nClientMajVersion-1) % nClientMinVersion % ('a' + nClientUpVersion);
					break;
				case SO_EMULEPLUS:
					m_clientVerString = CFormat(wxT("v%u")) % nClientMajVersion;
					if(nClientMinVersion != 0) {
						m_clientVerString += CFormat(wxT(".%u")) % nClientMinVersion;
					}
					if(nClientUpVersion != 0) {
						m_clientVerString += CFormat(wxT("%c")) % ('a' + nClientUpVersion - 1);
					}
					break;
				default:
					clientModString = GetClientModString();
					m_clientVerString = CFormat(wxT("v%u.%u%c")) % nClientMajVersion % nClientMinVersion % ('a' + nClientUpVersion);
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
		m_clientSoftString = GetSoftName(m_clientSoft);

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
			m_clientVerString = CFormat(wxT("v%u.%u.%u")) % nClientMajVersion % nClientMinVersion % nClientUpVersion;
		} else {
			m_clientVerString = CFormat(wxT("v%u.%u")) % nClientMajVersion % nClientMinVersion;
		}
	} else if (m_bIsML || (iHashType == SO_MLDONKEY)) {
		m_clientSoft = SO_MLDONKEY;
		m_clientSoftString = GetSoftName(m_clientSoft);
		uint32 nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		m_clientVerString = CFormat(wxT("v0.%u")) % nClientMinVersion;
	} else if (iHashType == SO_OLDEMULE) {
		m_clientSoft = SO_OLDEMULE;
		m_clientSoftString = GetSoftName(m_clientSoft);
		uint32 nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		m_clientVerString = CFormat(wxT("v0.%u")) % nClientMinVersion;
	} else {
		m_clientSoft = SO_EDONKEY;
		m_clientSoftString = GetSoftName(m_clientSoft);
		m_nClientVersion *= 10;
		m_clientVerString = CFormat(wxT("v%u.%u")) % (m_nClientVersion / 100000) % ((m_nClientVersion / 1000) % 100);
	}

	m_clientVersionString = m_clientVerString;
	if (!clientModString.IsEmpty()) {
		m_clientVerString += wxT(" - ") + clientModString;
	}
	m_fullClientVerString = m_clientSoftString + wxT(" ") + m_clientVerString;

	UpdateStats();
}

void CUpDownClient::RequestSharedFileList()
{
	if (m_iFileListRequested == 0) {
		AddDebugLogLineN( logClient, wxString( wxT("Requesting shared files from ") ) + GetUserName() );
		m_iFileListRequested = 1;
		TryToConnect(true);
	} else {
		AddDebugLogLineN( logClient, CFormat( wxT("Requesting shared files from user %s (%u) is already in progress") ) % GetUserName() % GetUserIDHybrid() );
	}
}


void CUpDownClient::ProcessSharedFileList(const byte* pachPacket, uint32 nSize, wxString& pszDirectory)
{
	if (m_iFileListRequested > 0) {
		m_iFileListRequested--;
		theApp->searchlist->ProcessSharedFileList(pachPacket, nSize, this, NULL, pszDirectory);
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
		sRet += CFormat(_("Requested: %s\n")) % m_reqfile->GetFileName();
		sRet += CFormat(
				wxPLURAL("Filestats for this session: Accepted %d of %d request, %s transferred\n", "Filestats for this session: Accepted %d of %d requests, %s transferred\n", m_reqfile->statistic.GetRequests())
				) % m_reqfile->statistic.GetAccepts() % m_reqfile->statistic.GetRequests() % CastItoXBytes(m_reqfile->statistic.GetTransferred());
		sRet += CFormat(
				wxPLURAL("Filestats for all sessions: Accepted %d of %d request, %s transferred\n", "Filestats for all sessions: Accepted %d of %d requests, %s transferred\n", m_reqfile->statistic.GetAllTimeRequests())
				) % m_reqfile->statistic.GetAllTimeAccepts() % m_reqfile->statistic.GetAllTimeRequests() % CastItoXBytes(m_reqfile->statistic.GetAllTimeTransferred());
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
		SendPacket(packet, true);
		return true;
	} else {
		m_WaitingPackets_list.push_back(packet);
		return TryToConnect(true);
	}
}

void CUpDownClient::SendPublicKeyPacket(){
	// send our public key to the client who requested it
	if (m_socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED){
		wxFAIL;
		return;
	}
	if (!theApp->CryptoAvailable())
		return;

	CMemFile data;
	data.WriteUInt8(theApp->clientcredits->GetPubKeyLen());
	data.Write(theApp->clientcredits->GetPublicKey(), theApp->clientcredits->GetPubKeyLen());
	CPacket* packet = new CPacket(data, OP_EMULEPROT, OP_PUBLICKEY);

	theStats::AddUpOverheadOther(packet->GetPacketSize());
	AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_PUBLICKEY to ") + GetFullIP() );
	SendPacket(packet,true,true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
}


void CUpDownClient::SendSignaturePacket(){
	// signate the public key of this client and send it
	if (m_socket == NULL || credits == NULL || m_SecureIdentState == 0){
		wxFAIL;
		return;
	}

	if (!theApp->CryptoAvailable()) {
		return;
	}
	if (credits->GetSecIDKeyLen() == 0) {
		return; // We don't have his public key yet, will be back here later
	}
	// do we have a challenge value received (actually we should if we are in this function)
	if (credits->m_dwCryptRndChallengeFrom == 0){
		AddDebugLogLineN( logClient, wxString(wxT("Want to send signature but challenge value is invalid - User ")) + GetUserName());
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
		if (::IsLowID(theApp->GetED2KID())) {
			// we cannot do not know for sure our public ip, so use the remote clients one
			ChallengeIP = GetIP();
			byChaIPKind = CRYPT_CIP_REMOTECLIENT;
		} else {
			ChallengeIP = theApp->GetED2KID();
			byChaIPKind  = CRYPT_CIP_LOCALCLIENT;
		}
	}
	//end v2
	byte achBuffer[250];

	uint8 siglen = theApp->clientcredits->CreateSignature(credits, achBuffer,  250, ChallengeIP, byChaIPKind );
	if (siglen == 0){
		wxFAIL;
		return;
	}
	CMemFile data;
	data.WriteUInt8(siglen);
	data.Write(achBuffer, siglen);
	if (bUseV2) {
		data.WriteUInt8(byChaIPKind);
	}
	
	CPacket* packet = new CPacket(data, OP_EMULEPROT, OP_SIGNATURE);

	theStats::AddUpOverheadOther(packet->GetPacketSize());
	AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_SIGNATURE to ") + GetFullIP() );
	SendPacket(packet,true,true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
}


void CUpDownClient::ProcessPublicKeyPacket(const byte* pachPacket, uint32 nSize)
{
	theApp->clientlist->AddTrackClient(this);

	if (m_socket == NULL || credits == NULL || pachPacket[0] != nSize-1
		|| nSize == 0 || nSize > 250){
		wxFAIL;
		return;
	}
	if (!theApp->CryptoAvailable())
		return;
	// the function will handle everything (mulitple key etc)
	if (credits->SetSecureIdent(pachPacket+1, pachPacket[0])){
		// if this client wants a signature, now we can send him one
		if (m_SecureIdentState == IS_SIGNATURENEEDED){
			SendSignaturePacket();
		}
		else if (m_SecureIdentState == IS_KEYANDSIGNEEDED) {
			// something is wrong
			AddDebugLogLineN( logClient, wxT("Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket") );
		}
	} else {
		AddDebugLogLineN( logClient, wxT("Failed to use new received public key") );
	}
}


void CUpDownClient::ProcessSignaturePacket(const byte* pachPacket, uint32 nSize)
{
	// here we spread the good guys from the bad ones ;)

	if (m_socket == NULL || credits == NULL || nSize == 0 || nSize > 250){
		wxFAIL;
		return;
	}

	uint8 byChaIPKind;
	if (pachPacket[0] == nSize-1)
		byChaIPKind = 0;
	else if (pachPacket[0] == nSize-2 && (m_bySupportSecIdent & 2) > 0) //v2
		byChaIPKind = pachPacket[nSize-1];
	else{
		wxFAIL;
		return;
	}

	if (!theApp->CryptoAvailable())
		return;

	// we accept only one signature per IP, to avoid floods which need a lot cpu time for cryptfunctions
	if (m_dwLastSignatureIP == GetIP()){
		AddDebugLogLineN( logClient, wxT("received multiple signatures from one client") );
		return;
	}
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0){
		AddDebugLogLineN( logClient, wxT("received signature for client without public key") );
		return;
	}
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0){
		AddDebugLogLineN( logClient, wxT("received signature for client with invalid challenge value - User ") + GetUserName() );
		return;
	}

	if (theApp->clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind ) ) {
		// result is saved in function above
		AddDebugLogLineN( logClient, CFormat( wxT("'%s' has passed the secure identification, V2 State: %i") ) % GetUserName() % byChaIPKind );
	} else {
		AddDebugLogLineN( logClient, CFormat( wxT("'%s' has failed the secure identification, V2 State: %i") ) % GetUserName() % byChaIPKind );
	}

	m_dwLastSignatureIP = GetIP();
}

void CUpDownClient::SendSecIdentStatePacket(){
	// check if we need public key and signature
	uint8 nValue = 0;
	if (credits){
		if (theApp->CryptoAvailable()){
			if (credits->GetSecIDKeyLen() == 0) {
				nValue = IS_KEYANDSIGNEEDED;
			} else if (m_dwLastSignatureIP != GetIP()) {
				nValue = IS_SIGNATURENEEDED;
			}
		}
		if (nValue == 0){
			AddDebugLogLineN( logClient, wxT("Not sending SecIdentState Packet, because State is Zero") );
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;

		CMemFile data;
		data.WriteUInt8(nValue);
		data.WriteUInt32(dwRandom);
		CPacket* packet = new CPacket(data, OP_EMULEPROT, OP_SECIDENTSTATE);

		theStats::AddUpOverheadOther(packet->GetPacketSize());
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_SECIDENTSTATE to ") + GetFullIP() );
		SendPacket(packet,true,true);
	} else {
		wxFAIL;
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

	CMemFile data(pachPacket,nSize);

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


bool CUpDownClient::CheckHandshakeFinished() const
{
	if (m_bHelloAnswerPending) {
		// this triggers way too often.. need more time to look at this -> only create a warning
		// The reason for this is that 2 clients are connecting to each other at the same time..
		AddDebugLogLineN( logClient, wxT("Handshake not finished while processing packet.") );
		return false;
	}

	return true;
}


wxString CUpDownClient::GetClientFullInfo()
{
	if (m_clientVerString.IsEmpty()) {
		ReGetClientSoft();
	}

	return CFormat( wxT("Client %s on IP:Port %s:%d using %s %s %s") )
		% ( m_Username.IsEmpty() ? wxString(_("Unknown")) : m_Username )
		% GetFullIP()
		% GetUserPort()
		% m_clientSoftString 
		% m_clientVerString
		% m_strModVersion;
}


wxString CUpDownClient::GetClientShortInfo()
{
	if (m_clientVerString.IsEmpty()) {
		ReGetClientSoft();
	}

	return CFormat( wxT("'%s' (%s %s %s)") )
		% ( m_Username.IsEmpty() ? wxString(_("Unknown")) : m_Username )
		% m_clientSoftString 
		% m_clientVerString
		% m_strModVersion;
}


void CUpDownClient::SendPublicIPRequest()
{
	if (IsConnected()){
		CPacket* packet = new CPacket(OP_PUBLICIP_REQ,0,OP_EMULEPROT);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_PUBLICIP_REQ to ") + GetFullIP());
		SendPacket(packet,true);
		m_fNeedOurPublicIP = true;
	}
}

void CUpDownClient::ProcessPublicIPAnswer(const byte* pbyData, uint32 uSize)
{
	if (uSize != 4) {
		throw wxString(wxT("Wrong Packet size on Public IP answer"));
	}
	uint32 dwIP = PeekUInt32(pbyData);
	if (m_fNeedOurPublicIP == true){ // did we?
		m_fNeedOurPublicIP = false;
		// Ignore local ip on GetPublicIP (could be wrong)
		if (theApp->GetPublicIP(true) == 0 && !IsLowID(dwIP) ) {
			theApp->SetPublicIP(dwIP);
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
		AddLogLineN(wxT("CAUGHT DEAD SOCKET IN SENDPACKET()"));
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
			// (% to reduce * current speed) / 100 and THEN, / (1000 / CORE_TIMER_PERIOD)
			// which is how often it is called per second.
			uint32 limit = (uint32)(reducedownload * kBpsClient * 1024.0 / 100000.0 * CORE_TIMER_PERIOD);

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
		AddLogLineNS(CFormat(wxT("CAUGHT DEAD SOCKET IN SETDOWNLOADLIMIT() WITH SPEED %f")) % kBpsClient);
	}
	
	return kBpsClient;
}

void CUpDownClient::SetUserIDHybrid(uint32 nUserID)
{
	theApp->clientlist->UpdateClientID( this, nUserID );

	m_nUserIDHybrid = nUserID;
}


void CUpDownClient::SetIP( uint32 val )
{
	theApp->clientlist->UpdateClientIP( this, val );

	m_dwUserIP = val;

	m_nConnectIP = val;
	
	m_FullUserIP = val;
}


void CUpDownClient::SetUserHash(const CMD4Hash& userhash)
{
	theApp->clientlist->UpdateClientHash( this, userhash );

	m_UserHash = userhash;

	ValidateHash();
}

EUtf8Str CUpDownClient::GetUnicodeSupport() const
{
	return m_bUnicodeSupport ? utf8strRaw : utf8strNone;
}

void CUpDownClient::SetSpammer(bool bVal)
{ 
	if (bVal) {
		Ban();
	} else if (IsBanned() && m_fIsSpammer) {
		UnBan();
	}
	m_fIsSpammer = bVal;
}

uint8 CUpDownClient::GetSecureIdentState()
{
	if (m_SecureIdentState != IS_UNAVAILABLE) {
		if (!SecIdentSupRec) {
			// This can be caused by a 0.30x based client which sends the old
			// style Hello packet, and the mule info packet, but between them they
			// send a secure ident state packet (after a hello but before we have 
			// the SUI capabilities). This is a misbehaving client, and somehow I
			// feel like it should be dropped. But then again, it won't harm to use
			// this SUI state if they are reporting no SUI (won't be used) and if 
			// they report using SUI on the mule info packet, it's ok to use it.

			AddDebugLogLineN(logClient, wxT("A client sent secure ident state before telling us the SUI capabilities"));
			AddDebugLogLineN(logClient, wxT("Client info: ") + GetClientFullInfo());
			AddDebugLogLineN(logClient, wxT("This client won't be disconnected, but it should be. :P"));
		}
	}

	return m_SecureIdentState;
}


bool CUpDownClient::SendChatMessage(const wxString& message)
{
	if (GetChatCaptchaState() == CA_CAPTCHARECV) {
		m_nChatCaptchaState = CA_SOLUTIONSENT;
	} else if (GetChatCaptchaState() == CA_SOLUTIONSENT) {
		wxFAIL; // we responsed to a captcha but didn't heard from the client afterwards - hopefully its just lag and this message will get through
	} else {
		m_nChatCaptchaState = CA_ACCEPTING;
	}

	SetSpammer(false);
	IncMessagesSent();
	// Already connecting?
	if (GetChatState() == MS_CONNECTING) {
		// Queue all messages till we're able to send them (or discard them)
		if (!m_pendingMessage.IsEmpty()) {
			m_pendingMessage += wxT("\n");
		} else {
			// There must be a message to send
			// - except if we got disconnected. No need to assert therefore.
		}
		m_pendingMessage += message;		
		return false;
	}
	if (IsConnected()) {
		// If we are already connected when we send the first message,
		// we have to update the chat status.
		SetChatState(MS_CHATTING);
		CMemFile data;
		data.WriteString(message, GetUnicodeSupport());
		CPacket* packet = new CPacket(data, OP_EDONKEYPROT, OP_MESSAGE);
		theStats::AddUpOverheadOther(packet->GetPacketSize());
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_MESSAGE to ") + GetFullIP());
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
	SetLastBuddyPingPongTime();	
	CPacket* buddyPing = new CPacket(OP_BUDDYPING, 0, OP_EMULEPROT);
	theStats::AddUpOverheadKad(buddyPing->GetPacketSize());
	AddDebugLogLineN(logLocalClient,wxT("Local Client: OP_BUDDYPING to ") + GetFullIP());
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
			theStats::AddKnownClient(this);
		}
	}
}

bool CUpDownClient::IsIdentified() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED);
}

bool CUpDownClient::IsBadGuy() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY);
}

bool CUpDownClient::SUIFailed() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDFAILED);
}

bool CUpDownClient::SUINeeded() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_IDNEEDED);
}

bool CUpDownClient::SUINotSupported() const 
{
	return (credits && credits->GetCurrentIdentState(GetIP()) == IS_NOTAVAILABLE);
}

uint64 CUpDownClient::GetDownloadedTotal() const 
{
	return credits ? credits->GetDownloadedTotal() : 0;
}
	
uint64 CUpDownClient::GetUploadedTotal() const 
{
	return credits ? credits->GetUploadedTotal() : 0;
}
	
double CUpDownClient::GetScoreRatio() const 
{
	return credits ? credits->GetScoreRatio(GetIP(), theApp->CryptoAvailable()) : 0;
}

const wxString CUpDownClient::GetServerName() const
{
	wxString ret;
	wxString srvaddr = Uint32toStringIP(GetServerIP());
	CServer* cserver = theApp->serverlist->GetServerByAddress(
		srvaddr, GetServerPort()); 
	if (cserver) {
		ret = cserver->GetListName();
	} else {
		ret = _("Unknown");
	}
	
	return ret;
}

bool CUpDownClient::ShouldReceiveCryptUDPPackets() const 
{
	return (thePrefs::IsClientCryptLayerSupported() && SupportsCryptLayer() && theApp->GetPublicIP() != 0
		&& HasValidHash() && (thePrefs::IsClientCryptLayerRequested() || RequestsCryptLayer()) );
}

#ifdef AMULE_DAEMON

void CUpDownClient::ProcessCaptchaRequest(CMemFile* WXUNUSED(data)) {}
void CUpDownClient::ProcessCaptchaReqRes(uint8 WXUNUSED(nStatus)) {}
void CUpDownClient::ProcessChatMessage(const wxString WXUNUSED(message)) {}

#else

void CUpDownClient::ProcessCaptchaRequest(CMemFile* data)
{
	uint64 id = GUI_ID(GetIP(),GetUserPort());
	// received a captcha request, check if we actually accept it (only after sending a message ourself to this client)
	if (GetChatCaptchaState() == CA_ACCEPTING && GetChatState() != MS_NONE
		&& theApp->amuledlg->m_chatwnd->IsIdValid(id)) {
		// read tags (for future use)
		uint8 nTagCount = data->ReadUInt8();
		if (nTagCount) {
			AddDebugLogLineN(logClient, CFormat(wxT("Received captcha request from client (%s) with (%u) tags")) % GetFullIP() % nTagCount);
			// and ignore them for now
			for (uint32 i = 0; i < nTagCount; i++) {
				CTag tag(*data, true);
			}
		}

		// sanitize checks - we want a small captcha not a wallpaper
		uint32 nSize = (uint32)(data->GetLength() - data->GetPosition());

		if ( nSize > 128 && nSize < 4096 ) {
			uint64 pos = data->GetPosition();
			wxMemoryInputStream memstr(data->GetRawBuffer() + pos, nSize);
			wxImage imgCaptcha(memstr, wxBITMAP_TYPE_BMP);

			if (imgCaptcha.IsOk() && imgCaptcha.GetHeight() > 10 && imgCaptcha.GetHeight() < 50
				&& imgCaptcha.GetWidth() > 10 && imgCaptcha.GetWidth() < 150 ) {
				m_nChatCaptchaState = CA_CAPTCHARECV;
				CCaptchaDialog * dialog = new CCaptchaDialog(theApp->amuledlg, imgCaptcha, id);
				dialog->Show();

			} else {
				AddDebugLogLineN(logClient, CFormat(wxT("Received captcha request from client, processing image failed or invalid pixel size (%s)")) % GetFullIP());
			}
		} else {
			AddDebugLogLineN(logClient, CFormat(wxT("Received captcha request from client, size sanitize check failed (%u) (%s)")) % nSize % GetFullIP());
		}
	} else {
		AddDebugLogLineN(logClient, CFormat(wxT("Received captcha request from client, but don't accepting it at this time (%s)")) % GetFullIP());
	}
}

void CUpDownClient::ProcessCaptchaReqRes(uint8 nStatus)
{
	uint64 id = GUI_ID(GetIP(),GetUserPort());
	if (GetChatCaptchaState() == CA_SOLUTIONSENT && GetChatState() != MS_NONE
		&& theApp->amuledlg->m_chatwnd->IsIdValid(id)) {
		wxASSERT( nStatus < 3 );
		m_nChatCaptchaState = CA_NONE;
		theApp->amuledlg->m_chatwnd->ShowCaptchaResult(id, nStatus == 0);
	} else {
		m_nChatCaptchaState = CA_NONE;
		AddDebugLogLineN(logClient, CFormat(wxT("Received captcha result from client, but not accepting it at this time (%s)")) % GetFullIP());
	}
}

void CUpDownClient::ProcessChatMessage(wxString message)
{
	if (IsMessageFiltered(message)) {
		AddLogLineC(CFormat(_("Message filtered from '%s' (IP:%s)")) % GetUserName() % GetFullIP());
		return;
	}

	// advanced spamfilter check
	if (thePrefs::IsChatCaptchaEnabled() && !IsFriend()) {
		// captcha checks outrank any further checks - if the captcha has been solved, we assume it's not spam
		// first check if we need to send a captcha request to this client
		if (GetMessagesSent() == 0 && GetMessagesReceived() == 0 && GetChatCaptchaState() != CA_CAPTCHASOLVED) {
			// we have never sent a message to this client, and no message from him has ever passed our filters
			if (GetChatCaptchaState() != CA_CHALLENGESENT) {
				// we also aren't currently expecting a captcha response
				if (m_fSupportsCaptcha) {
					// and he supports captcha, so send him one and store the message (without showing for now)
					if (m_cCaptchasSent < 3) {	// no more than 3 tries
						m_strCaptchaPendingMsg = message;
						wxMemoryOutputStream memstr;
						memstr.PutC(0); // no tags, for future use
						CCaptchaGenerator captcha(4);
						if (captcha.WriteCaptchaImage(memstr)){
							m_strCaptchaChallenge = captcha.GetCaptchaText();
							m_nChatCaptchaState = CA_CHALLENGESENT;
							m_cCaptchasSent++;
							CMemFile fileAnswer((byte*) memstr.GetOutputStreamBuffer()->GetBufferStart(), memstr.GetLength());
							CPacket* packet = new CPacket(fileAnswer, OP_EMULEPROT, OP_CHATCAPTCHAREQ);
							theStats::AddUpOverheadOther(packet->GetPacketSize());
							AddLogLineN(CFormat(wxT("sent Captcha %s (%d)")) % m_strCaptchaChallenge % packet->GetPacketSize());
							SafeSendPacket(packet);
						} else {
							wxFAIL;
						}
					}
				} else {
					// client doesn't support captchas, but we require them, tell him that it's not going to work out
					// with an answer message (will not be shown and doesn't count as sent message)
					if (m_cCaptchasSent < 1) {	// don't send this notifier more than once
						m_cCaptchasSent++;
						// always sent in english
						SendChatMessage(wxT("In order to avoid spam messages, this user requires you to solve a captcha before you can send a message to him. However your client does not supports captchas, so you will not be able to chat with this user."));
						AddDebugLogLineN(logClient, CFormat(wxT("Received message from client not supporting captchas, filtered and sent notifier (%s)")) % GetClientFullInfo());
					} else {
						AddDebugLogLineN(logClient, CFormat(wxT("Received message from client not supporting captchas, filtered, didn't send notifier (%s)")) % GetClientFullInfo());
					}
				}
				return;
			} else { // (GetChatCaptchaState() == CA_CHALLENGESENT)
				// this message must be the answer to the captcha request we sent him, let's verify
				wxASSERT( !m_strCaptchaChallenge.IsEmpty() );
				if (m_strCaptchaChallenge.CmpNoCase(message.Trim().Right(std::min(message.Length(), m_strCaptchaChallenge.Length()))) == 0) {
					// allright
					AddDebugLogLineN(logClient, CFormat(wxT("Captcha solved, showing withheld message (%s)")) % GetClientFullInfo());
					m_nChatCaptchaState = CA_CAPTCHASOLVED; // this state isn't persitent, but the messagecounter will be used to determine later if the captcha has been solved
					// replace captchaanswer with withheld message and show it
					message = m_strCaptchaPendingMsg;
					m_cCaptchasSent = 0;
					m_strCaptchaChallenge.Clear();
					CPacket* packet = new CPacket(OP_CHATCAPTCHARES, 1, OP_EMULEPROT, false);
					byte statusResponse = 0; // status response
					packet->CopyToDataBuffer(0, &statusResponse, 1);
					theStats::AddUpOverheadOther(packet->GetPacketSize());
					SafeSendPacket(packet);
				} else { // wrong, cleanup and ignore
					AddDebugLogLineN(logClient, CFormat(wxT("Captcha answer failed (%s)")) % GetClientFullInfo());
					m_nChatCaptchaState = CA_NONE;
					m_strCaptchaChallenge.Clear();
					m_strCaptchaPendingMsg.Clear();
					CPacket* packet = new CPacket(OP_CHATCAPTCHARES, 1, OP_EMULEPROT, false);
					byte statusResponse = (m_cCaptchasSent < 3) ? 1 : 2; // status response
					packet->CopyToDataBuffer(0, &statusResponse, 1);
					theStats::AddUpOverheadOther(packet->GetPacketSize());
					SafeSendPacket(packet);
					return; // nothing more todo
				}
			}	
		}
	}

	if (thePrefs::IsAdvancedSpamfilterEnabled() && !IsFriend()) { // friends are never spammer... (but what if two spammers are friends :P )
		bool bIsSpam = false;
		if (m_fIsSpammer) {
			bIsSpam = true;
		} else {
			// first fixed criteria: If a client sends me an URL in his first message before I respond to him
			// there is a 99,9% chance that it is some poor guy advising his leech mod, or selling you .. well you know :P
			if (GetMessagesSent() == 0) {
				static wxArrayString urlindicators(wxStringTokenize(wxT("http:|www.|.de |.net |.com |.org |.to |.tk |.cc |.fr |ftp:|ed2k:|https:|ftp.|.info|.biz|.uk|.eu|.es|.tv|.cn|.tw|.ws|.nu|.jp"), wxT("|")));
				for (size_t pos = urlindicators.GetCount(); pos--;) {
					if (message.Find(urlindicators[pos]) != wxNOT_FOUND) {
						bIsSpam = true;
						break;
					}
				}
				// second fixed criteria: he sent me 4 or more messages and I didn't answer him once
				if (GetMessagesReceived() > 3) {
					bIsSpam = true;
				}
			}
		}
		if (bIsSpam) {
			AddDebugLogLineN(logClient, CFormat(wxT("'%s' has been marked as spammer")) % GetUserName());
			SetSpammer(true);
			theApp->amuledlg->m_chatwnd->EndSession(GUI_ID(GetIP(),GetUserPort()));
			return;
		}
	}


	wxString logMsg = CFormat(_("New message from '%s' (IP:%s)")) % GetUserName() % GetFullIP();
	if(thePrefs::ShowMessagesInLog()) {
		logMsg += wxT(": ") + message;
	}
	AddLogLineC(logMsg);
	IncMessagesReceived();

	Notify_ChatProcessMsg(GUI_ID(GetIP(), GetUserPort()), GetUserName() + wxT("|") + message);
}

bool CUpDownClient::IsMessageFiltered(const wxString& message)
{
	bool filtered = false;
	// If we're chatting to the guy, we don't want to filter!
	if (GetChatState() != MS_CHATTING) {
		if (thePrefs::MsgOnlyFriends() && !IsFriend()) {
			filtered = true;
		} else if (thePrefs::MsgOnlySecure() && GetUserName().IsEmpty() ) {
			filtered = true;
		} else if (thePrefs::MustFilterMessages()) {
			filtered = thePrefs::IsMessageFiltered(message);
		}
	}
	if (filtered) {
		SetSpammer(true);
	}
	return filtered;
}

#endif

void CUpDownClient::SendSharedDirectories()
{
		// This list will contain all (unique) folders.
		PathList foldersToSend;
	   
		// The shared folders
		const unsigned folderCount = theApp->glob_prefs->shareddir_list.size();
		for (unsigned i = 0; i < folderCount; ++i) {
			foldersToSend.push_back(theApp->glob_prefs->shareddir_list[i]);
		}
		
		// ... the categories folders ... (category 0 -> incoming)
		for (unsigned i = 0; i < theApp->glob_prefs->GetCatCount(); ++i) {
			foldersToSend.push_back(theApp->glob_prefs->GetCategory(i)->path);
		}

		// Strip duplicates
		foldersToSend.sort();
		foldersToSend.unique();
		
		// Build packet
		CMemFile tempfile(80);
		tempfile.WriteUInt32(foldersToSend.size());

		PathList::iterator it = foldersToSend.begin();
		for (; it != foldersToSend.end(); ++it) {
			// Note: the public shared name contains the 'raw' path, so we can recognize it again.
			//       the 'raw' path is determined using CPath::GetRaw()
			tempfile.WriteString( theApp->sharedfiles->GetPublicSharedDirName(*it), GetUnicodeSupport() );
		}

		// ... and the Magic thing from the eDonkey Hybrids...
		tempfile.WriteString(OP_INCOMPLETE_SHARED_FILES, GetUnicodeSupport());

		// Send the packet.
		CPacket* replypacket = new CPacket(tempfile, OP_EDONKEYPROT, OP_ASKSHAREDDIRSANS);
		theStats::AddUpOverheadOther(replypacket->GetPacketSize());
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDDIRSANS to ") + GetFullIP() );
		SendPacket(replypacket, true, true);
}

void CUpDownClient::SendSharedFilesOfDirectory(const wxString& strReqDir)
{
	CKnownFilePtrList list;
	
	if (strReqDir == OP_INCOMPLETE_SHARED_FILES) {
		// get all shared files from download queue
		int iQueuedFiles = theApp->downloadqueue->GetFileCount();
		for (int i = 0; i < iQueuedFiles; i++) {
			CPartFile* pFile = theApp->downloadqueue->GetFileByIndex(i);
			if (pFile == NULL || pFile->GetStatus(true) != PS_READY) {
				continue;
			}
			list.push_back(pFile);
		}
	} else {
		// get all shared files for the requested directory
		const CPath *sharedDir = theApp->sharedfiles->GetDirForPublicSharedDirName(strReqDir);
		if (sharedDir) {
			theApp->sharedfiles->GetSharedFilesByDirectory(sharedDir->GetRaw(), list);
		} else {
			AddLogLineC(CFormat(_("User %s (%u) requested sharedfiles-list for not existing directory '%s' -> Ignored")) % GetUserName() % GetUserIDHybrid() % strReqDir);
		}
	}

	CMemFile tempfile(80);
	tempfile.WriteString(strReqDir, GetUnicodeSupport());
	tempfile.WriteUInt32(list.size());
	
	while (!list.empty()) {
		if (!list.front()->IsLargeFile() || SupportsLargeFiles()) {
			list.front()->CreateOfferedFilePacket(&tempfile, NULL, this);
		}
		list.pop_front();
	}
	
	CPacket* replypacket = new CPacket(tempfile, OP_EDONKEYPROT, OP_ASKSHAREDFILESDIRANS);
	theStats::AddUpOverheadOther(replypacket->GetPacketSize());
	AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_ASKSHAREDFILESDIRANS to ") + GetFullIP() );
	SendPacket(replypacket, true, true);
}

void CUpDownClient::SendFirewallCheckUDPRequest()
{
	wxASSERT(GetKadState() == KS_FWCHECK_UDP);

	if (!Kademlia::CKademlia::IsRunning()) {
		SetKadState(KS_NONE);
		return;
	} else if (GetUploadState() != US_NONE || GetDownloadState() != DS_NONE || GetChatState() != MS_NONE || GetKadVersion() <= 5 || GetKadPort() == 0) {
		Kademlia::CUDPFirewallTester::SetUDPFWCheckResult(false, true, wxUINT32_SWAP_ALWAYS(GetIP()), 0); // inform the tester that this test was cancelled
		SetKadState(KS_NONE);
		return;
	}

	CMemFile data;
	data.WriteUInt16(Kademlia::CKademlia::GetPrefs()->GetInternKadPort());
	data.WriteUInt16(Kademlia::CKademlia::GetPrefs()->GetExternalKadPort());
	data.WriteUInt32(Kademlia::CKademlia::GetPrefs()->GetUDPVerifyKey(GetConnectIP()));
	CPacket* packet = new CPacket(data, OP_EMULEPROT, OP_FWCHECKUDPREQ);
	theStats::AddUpOverheadKad(packet->GetPacketSize());
	SafeSendPacket(packet);
}

void CUpDownClient::ProcessFirewallCheckUDPRequest(CMemFile* data)
{
	if (!Kademlia::CKademlia::IsRunning() || Kademlia::CKademlia::GetUDPListener() == NULL) {
		//DebugLogWarning(_T("Ignored Kad Firewallrequest UDP because Kad is not running (%s)"), GetClientFullInfo());
		return;
	}

	// first search if we know this IP already, if so the result might be biased and we need tell the requester 
	bool errorAlreadyKnown = false;
	if (GetUploadState() != US_NONE || GetDownloadState() != DS_NONE || GetChatState() != MS_NONE) {
		errorAlreadyKnown = true;
	} else if (Kademlia::CKademlia::GetRoutingZone()->GetContact(wxUINT32_SWAP_ALWAYS(GetConnectIP()), 0, false) != NULL) {
		errorAlreadyKnown = true;
	}

	uint16_t remoteInternPort = data->ReadUInt16();
	uint16_t remoteExternPort = data->ReadUInt16();
	uint32_t senderKey = data->ReadUInt32();
	if (remoteInternPort == 0) {
		//DebugLogError(_T("UDP Firewallcheck requested with Intern Port == 0 (%s)"), GetClientFullInfo());
		return;
	}
// 	if (senderKey == 0)
// 		DebugLogWarning(_T("UDP Firewallcheck requested with SenderKey == 0 (%s)"), GetClientFullInfo());
	
	CMemFile testPacket1;
	testPacket1.WriteUInt8(errorAlreadyKnown ? 1 : 0);
	testPacket1.WriteUInt16(remoteInternPort);
	DebugSend(Kad2FirewallUDP, wxUINT32_SWAP_ALWAYS(GetConnectIP()), remoteInternPort);
	Kademlia::CKademlia::GetUDPListener()->SendPacket(testPacket1, KADEMLIA2_FIREWALLUDP, wxUINT32_SWAP_ALWAYS(GetConnectIP()), remoteInternPort, Kademlia::CKadUDPKey(senderKey, theApp->GetPublicIP(false)), NULL);

	// if the client has a router with PAT (and therefore a different extern port than intern), test this port too
	if (remoteExternPort != 0 && remoteExternPort != remoteInternPort) {
		CMemFile testPacket2;
		testPacket2.WriteUInt8(errorAlreadyKnown ? 1 : 0);
		testPacket2.WriteUInt16(remoteExternPort);
		DebugSend(Kad2FirewalledUDP, wxUINT32_SWAP_ALWAYS(GetConnectIP()), remoteExternPort);
		Kademlia::CKademlia::GetUDPListener()->SendPacket(testPacket2, KADEMLIA2_FIREWALLUDP, wxUINT32_SWAP_ALWAYS(GetConnectIP()), remoteExternPort, Kademlia::CKadUDPKey(senderKey, theApp->GetPublicIP(false)), NULL);
	}
	//DebugLog(_T("Answered UDP Firewallcheck request (%s)"), GetClientFullInfo());
}

void CUpDownClient::SetConnectOptions(uint8_t options, bool encryption, bool callback)
{
	SetCryptLayerSupport((options & 0x01) != 0 && encryption);
	SetCryptLayerRequest((options & 0x02) != 0 && encryption);
	SetCryptLayerRequires((options & 0x04) != 0 && encryption);
	SetDirectUDPCallbackSupport((options & 0x08) != 0 && callback);
}


#ifdef DEBUG_ZOMBIE_CLIENTS
void CUpDownClient::Unlink(const wxString& from)
{
	std::multiset<wxString>::iterator it = m_linkedFrom.find(from);
	if (it != m_linkedFrom.end()) {
		m_linkedFrom.erase(it);
	}
	m_linked--;
	if (!m_linked) {
		if (m_linkedDebug) {
			AddLogLineN(CFormat(wxT("Last reference to client %d %p unlinked, delete it.")) % ECID() % this);
		}
		delete this;
	}
}

#else

void CUpDownClient::Unlink()
{
	m_linked--;
	if (!m_linked) {
		delete this;
	}
}
#endif


// File_checked_for_headers

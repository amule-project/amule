// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "types.h"

#ifdef __WXMSW__
	#include <winsock.h>
#else
#ifdef __OPENBSD__
       #include <sys/types.h>
#endif /* __OPENBSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <zlib.h>		// Needed for inflateEnd
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/object.h>		// Needed by wx/sckaddr.h
#include <wx/sckaddr.h>		// Needed for wxIPV4address
#include "otherfunctions.h"	// Needed for nstrdup

#include "SearchList.h"		// Needed for CSearchList
#include "ChatSelector.h"	// Needed for CChatSelector
#include "ChatWnd.h"		// Needed for CChatWnd
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "IPFilter.h"		// Needed for CIPFilter
#include "sockets.h"		// Needed for CServerConnect
#include "ClientCredits.h"	// Needed for CClientCreditsList
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "server.h"		// Needed for CServer
#include "Preferences.h"	// Needed for CPreferences
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "packets.h"		// Needed for Packet
#include "otherstructs.h"	// Needed for Requested_Block_Struct
#include "FriendList.h"		// Needed for CFriendList
#include "Friend.h"		// Needed for CFriend
#include "ClientList.h"		// Needed for CClientList
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "opcodes.h"		// Needed for SOURCESSLOTS
#include "updownclient.h"	// Needed for CUpDownClient
#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address

//#define NET_TEST 

// some client testing variables
static wxString crash_name   = "[Invalid User Name]"; 
static wxString empty_name = "[Empty User Name]";

/*
// prevent fscking dns queries
class amuleIPV4Address : public wxIPV4address {
public:
	amuleIPV4Address() : wxIPV4address() {};
#ifndef __WXMSW__
	virtual bool Hostname(unsigned long addr) {
		return GAddress_INET_SetHostAddress(m_address,addr)==GSOCK_NOERROR;
	};
	virtual bool Hostname(char* addr) {
		struct in_addr inaddr;
		inet_aton(addr,&inaddr);
		return GAddress_INET_SetHostAddress(m_address,inaddr.s_addr)==GSOCK_NOERROR;
	}
#endif
};
*/

//	members of CUpDownClient
//	which are used by down and uploading functions 

CUpDownClient::CUpDownClient(CClientReqSocket* sender)
{
	socket = sender;
	//printf("Socket %x set on client %x\n",socket, this);
	reqfile = 0;
	Init();
}

CUpDownClient::CUpDownClient(uint16 in_port, uint32 in_userid,uint32 in_serverip, uint16 in_serverport,CPartFile* in_reqfile)
{
	socket = NULL;
	//printf("Socket %x set on client %x\n",socket, this);
	Init();
	m_nUserID = in_userid;
	m_nUserPort = in_port;
	sourcesslot=m_nUserID%SOURCESSLOTS;
	if (!HasLowID()) {
		sprintf(m_szFullUserIP,"%i.%i.%i.%i",(uint8)m_nUserID,(uint8)(m_nUserID>>8),(uint8)(m_nUserID>>16),(uint8)(m_nUserID>>24));
	}
	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
	reqfile = in_reqfile;
	ReGetClientSoft();
}

void CUpDownClient::Init()
{
	memset(m_szFullUserIP,0,21);
	credits = 0;
	//memset(reqfileid, 0, sizeof reqfileid);
	memset(requpfileid, 0, sizeof requpfileid);
	// m_nAvDownDatarate = 0;  // unused
	m_byChatstate = 0;
	m_cShowDR = 0;
	m_nUDPPort = 0;
	m_cFailed = 0;
	m_dwBanTime = 0;
	m_nMaxSendAllowed = 0;
	m_nTransferedUp = 0;
	m_cSendblock = 0;
	m_cAsked = 0;
	m_cDownAsked = 0;
#ifdef DOWNLOADRATE_FILTERED
	msSentPrev = msReceivedPrev = 0;
	kBpsUp = kBpsDown = 0.0;
	fDownAvgFilter = 1.0;
	bytesReceivedCycle = 0;
#else
	m_nDownDatarate = 0;
	m_nDownDataRateMS = 0;
	m_nSumForAvgDownDataRate = 0;
#endif
	m_pszUsername = 0;
	m_dwUserIP = 0;
	m_nUserID = 0;
	m_nServerPort = 0;
	m_bBanned = false;
	//m_bFileListRequested = false;
	m_iFileListRequested = 0;
	m_dwLastUpRequest = 0;
	m_bEmuleProtocol = false;
	usedcompressiondown = false;
	m_bUsedComprUp = false;
	m_bCompleteSource = false;
	m_bFriendSlot = false;
	m_bCommentDirty = false;
	m_bReaskPending = false;
	m_bUDPPending = false;
	m_nUserPort = 0;
	m_nPartCount = 0;
	m_nUpPartCount = 0;
	m_abyPartStatus = 0;
	m_abyUpPartStatus = 0;
	m_dwLastAskedTime = 0;
	m_nDownloadState = DS_NONE;
	m_pszClientFilename = 0;
	m_dwUploadTime = 0;
	m_nTransferedDown = 0;
	m_byUploadState = US_NONE;
	m_dwLastBlockReceived = 0;
	m_byEmuleVersion = 0;
	m_byDataCompVer = 0;
	m_byUDPVer = 0;
	m_bySourceExchangeVer = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_nRemoteQueueRank = 0;
	m_dwLastSourceRequest = 0;
	m_dwLastSourceAnswer = 0;
	m_dwLastAskedForSources = 0;
	
	m_SecureIdentState = IS_UNAVAILABLE;
	m_dwLastSignatureIP = 0;
	m_bySupportSecIdent = 0;
	m_byInfopacketsReceived = IP_NONE;	
	
	m_byCompatibleClient = 0;
	m_bIsHybrid = false;
	m_bIsNewMLD = false;
	m_bIsML = false;
	m_Friend = NULL;
	m_iRate=0;
	m_strComment="";
	m_nCurSessionUp = 0;
	m_clientSoft=SO_UNKNOWN;
	m_nClientVersion=0;
	m_bRemoteQueueFull = false;
	memset( m_achUserHash, 0, 16);
	SetWaitStartTime();
	if (socket) {
		struct sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		wxIPV4address address;
		socket->GetPeer(address);
		//uint32 nSockAddrLen = sizeof(sockAddr);
		//socket->GetPeerName((SOCKADDR*)&sockAddr,(int*)&nSockAddrLen);
		sockAddr.sin_addr.s_addr = inet_addr(address.IPAddress().c_str());
		m_dwUserIP = sockAddr.sin_addr.s_addr;
		strcpy(m_szFullUserIP,inet_ntoa(sockAddr.sin_addr));
	}
	sourcesslot=0;
	m_fHashsetRequesting = 0;
	m_fSharedDirectories = 0;
	m_dwEnteredConnectedState = 0;
	// At the beginning, client is't a thief :)
	leechertype = 0;
	thief = false;
	m_lastPartAsked = 0xffff;
	m_nUpCompleteSourcesCount= 0;
	m_lastRefreshedDLDisplay = 0;
	m_bAddNextConnect = false;  // VQB Fix for LowID slots only on connection
	m_SoftLen = 0;
	m_bHelloAnswerPending = false;
}


CUpDownClient::~CUpDownClient()
{
	/* Razor 1a - Modif by MikaelB */
	if(reqfile != NULL) {
		reqfile->RemoveDownloadingSource(this);
	}
	/* End modif */
	//printf("1...");
	theApp.clientlist->RemoveClient(this);
	if (m_Friend) {
		m_Friend->m_LinkedClient = NULL;
		theApp.friendlist->RefreshFriend(m_Friend);
		m_Friend = NULL;
	}
	//printf("2...");
	if (socket) {
		socket->client = 0; // Kry - Doesn't Safe_Delete do this already?
		socket->Safe_Delete();
	}
	//printf("3...");
	if (m_pszUsername) {
		delete[] m_pszUsername;
		m_pszUsername = NULL;
	}
	
	if (m_pszClientFilename) {
		delete[] m_pszClientFilename;
		m_pszClientFilename = NULL;
	}
	
	//printf("4...");
	if (m_abyPartStatus) {
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	//printf("5...");
	if (m_abyUpPartStatus) {
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;
	}
	//printf("6...");
	ClearUploadBlockRequests();
	//printf("7...");
	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;m_DownloadBlocks_list.GetNext(pos)) {
		delete m_DownloadBlocks_list.GetAt(pos);
	}
	//printf("8...");
	m_DownloadBlocks_list.RemoveAll();
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;m_RequestedFiles_list.GetNext(pos)) {
		delete m_RequestedFiles_list.GetAt(pos);
	}
	//printf("9...");
	m_RequestedFiles_list.RemoveAll();
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;m_PendingBlocks_list.GetNext(pos)) {
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetAt(pos);
		delete pending->block;
		// Not always allocated
		if (pending->zStream) {
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}
	/* eMule 0.30c manage this also, i give it a try ... (Creteil) */
	//printf("10...");
	for (POSITION pos =m_WaitingPackets_list.GetHeadPosition();pos != 0;m_WaitingPackets_list.GetNext(pos)) {
		delete m_WaitingPackets_list.GetAt(pos);
	}

	//printf("11...");
	if (m_iRate>0 || m_strComment.GetLength()>0) {
		m_iRate=0; m_strComment="";
		if (reqfile) {
			reqfile->UpdateFileRatingCommentAvail();
		}
	}
	//printf("12...");
	m_PendingBlocks_list.RemoveAll();
#ifndef DOWNLOADRATE_FILTERED
	m_AvarageDDR_list.RemoveAll();
#endif
	//DEBUG_ONLY (theApp.listensocket->Debug_ClientDeleted(this));
	SetUploadFileID((uchar*)NULL);
	//printf("END\n");
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
}

bool CUpDownClient::ProcessHelloPacket(char* pachPacket, uint32 nSize)
{
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	uint8 hashsize;
	if ( (1!=data.Read(hashsize)) ) {
		throw wxString(wxT("Invalid Hello packet: Short packet when reading hash size"));
	}
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
	return ProcessHelloTypePacket(&data);
}

void CUpDownClient::ProcessHelloAnswer(char* pachPacket, uint32 nSize)
{
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	m_bHelloAnswerPending = false;
	ProcessHelloTypePacket(&data);
}

bool CUpDownClient::ProcessHelloTypePacket(CSafeMemFile* data)
{
		
	m_bIsHybrid = false;
	m_bIsML = false;
	DWORD dwEmuleTags = 0;
	
	try {	
	
		data->ReadRaw((unsigned char*)m_achUserHash,16);
		data->Read(m_nUserID);
		uint16 nUserPort = 0;
		data->Read(nUserPort); // hmm clientport is sent twice - why?
		uint32 tagcount;
		data->Read(tagcount);
		for (uint32 i = 0;i < tagcount; i++){
			CTag temptag(data);
			switch(temptag.tag.specialtag){
				case CT_NAME:
					if (m_pszUsername){
						delete[] m_pszUsername;
						m_pszUsername = NULL; // needed, in case 'nstrdup' fires an exception!!
					}
					if( temptag.tag.stringvalue )
						m_pszUsername = nstrdup(temptag.tag.stringvalue);
					break;
				case CT_VERSION:
					m_nClientVersion = temptag.tag.intvalue;
					break;
				case ET_MOD_VERSION:
					if (temptag.tag.type == 2) {
						m_strModVersion = temptag.tag.stringvalue;
					} else if (temptag.tag.type == 3) {
						m_strModVersion.Format(_T("ModID=%u"), temptag.tag.intvalue);						
					} else {
						m_strModVersion = _T("ModID=<Unknwon>");
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
					//  4 --Reserved for future use--
					//  4 UDP version
					//  4 Data compression version
					//  4 Secure Ident
					//  4 Source Exchange
					//  4 Ext. Requests
					//  4 Comments
					//	1 --Reserved for future use--
					//	1 No 'View Shared Files' supported
					//	1 MultiPacket
					//  1 Preview
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
		data->Read(m_dwServerIP);
		data->Read(m_nServerPort);
		// Hybrid now has an extra uint32.. What is it for?
		// Also, many clients seem to send an extra 6? These are not eDonkeys or Hybrids..
		if ( data->GetLength() - data->GetPosition() == 4 ){
			// Kry - Changes on eMule code for compat.
			char test[4];
			// lemonfan - this is not an "normal" string, so wxString cant be used
			data->ReadRaw(&test, 4);
			if ((test[0]=='M') && (test[1]=='L') && (test[2]=='D') && (test[3]=='K')) {
				m_bIsML=true;
			} else{
				m_bIsHybrid = true;
				m_fSharedDirectories = 1;
			}
		}
		
	} catch ( CStrangePacket )
	{
		printf("\nWrong Tags on hello type packet!!\n");
		printf("Sent by %s on ip %s port %i using client %x version %x\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetVersion());
		printf("User Disconnected.\n");
		throw wxString("Wrong Tags on hello type packet");
	}
	catch ( CInvalidPacket (e))
	{
		printf("Wrong Tags on hello type packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %x version %x\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetVersion());
		printf("User Disconnected.\n");		
		throw wxString("Wrong Tags on hello type packet");
	}
	/* Kry - Added the CT_EMULE_VERSION tag - probably no more need for this
	
	if( m_nClientVersion > 10000 && m_nClientVersion < 100000 )
		m_nClientVersion = m_nClientVersion - (m_nClientVersion/10000)*10000;
	if( m_nClientVersion > 1000 )
		m_nClientVersion = m_nClientVersion - (m_nClientVersion/1000)*1000;
	if( m_nClientVersion < 100 )
		m_nClientVersion *= 10;
	
	*/
	
	// tecxx 1609 2002 - add client's servet to serverlist (Moved to uploadqueue.cpp)

	if (socket) {
		struct sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		wxIPV4address address;
		socket->GetPeer(address);
		sockAddr.sin_addr.s_addr=inet_addr(address.IPAddress().c_str());
		m_dwUserIP = sockAddr.sin_addr.s_addr;
		strcpy(m_szFullUserIP,inet_ntoa(sockAddr.sin_addr));
	} else {
		printf("Huh, socket failure. Avoided crash this time.\n");
	}
	
	if (theApp.glob_prefs->AddServersFromClient()) {
		in_addr addhost;
		addhost.s_addr = m_dwServerIP;
		CServer* addsrv = new CServer(m_nServerPort, inet_ntoa(addhost));
		addsrv->SetListName(addsrv->GetAddress());
			if (!theApp.amuledlg->serverwnd->serverlistctrl->AddServer(addsrv, true)) {
				delete addsrv;
		}
	}

	if(!HasLowID() || m_nUserID == 0)
		m_nUserID = m_dwUserIP;

	// get client credits
	uchar key[16];
	md4cpy(key,m_achUserHash);
	CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(key);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			theApp.amuledlg->AddDebugLogLine(false, "Clients: %s (%s), Banreason: Userhash changed (Found in TrackedClientsList)", GetUserName(), GetFullIP()); 
			Ban();
		}	
	}
	else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		theApp.amuledlg->AddDebugLogLine(false, "Clients: %s (%s), Banreason: Userhash changed", GetUserName(),GetFullIP()); 
		Ban();
	}

	if ((m_Friend = theApp.friendlist->SearchFriend(key, m_dwUserIP, m_nUserPort)) != NULL){
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
		}
		else
			m_Friend->m_LinkedClient = this;
		md4cpy(m_Friend->m_abyUserhash,GetUserHash());
		m_Friend->m_dwHasHash = md4cmp(m_Friend->m_abyUserhash, CFriend::sm_abyNullHash) ? 1 : 0;
		m_Friend->m_strName = m_pszUsername;
		m_Friend->m_dwLastUsedIP = m_dwUserIP;
		m_Friend->m_nLastUsedPort = m_nUserPort;
		m_Friend->m_dwLastSeen = time(NULL);
		theApp.friendlist->RefreshFriend(m_Friend);
	}
	else{
		// avoid that an unwanted client instance keeps a friend slot
		SetFriendSlot(false);
	}

	
	// We want to educate Users of major comercial GPL breaking mods by telling them about the effects
	// check for known advertising in usernames
	// the primary aim is not to technical block those but to make users use a GPL-conform version
	CString strBuffer = m_pszUsername;
	strBuffer.MakeUpper();
	strBuffer.Remove(' ');
	if (strBuffer.Find("EMULE-CLIENT") != -1 || strBuffer.Find("POWERMULE") != -1){
		m_bGPLEvildoer = true;  
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


void CUpDownClient::SendHelloPacket() {
	if (socket) {
		struct sockaddr_in sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		//uint32 nSockAddrLen = sizeof(sockAddr);
		//socket->GetPeerName((SOCKADDR*)&sockAddr,(int*)&nSockAddrLen);
		wxIPV4address address;
		socket->GetPeer(address);
		sockAddr.sin_addr.s_addr = inet_addr(address.IPAddress().c_str());
		if ( theApp.ipfilter->IsFiltered(sockAddr.sin_addr.s_addr)) {
			theApp.amuledlg->AddDebugLogLine(true,CString(_("Filtered IP: %s (%s)")).GetData(),GetFullIP(),theApp.ipfilter->GetLastHit().GetData());
			Disconnected();
			theApp.stat_filteredclients++;
			return;
		}
	}
	CMemFile* data = new CMemFile();
	data->Write((uint8)16);
	SendHelloTypePacket(data);
	Packet* packet = new Packet(data);
	delete data;
	packet->opcode = OP_HELLO;
	if (socket) {
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true);
	}
	m_bHelloAnswerPending = true;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer) {
	CMemFile* data = new CMemFile();
	data->Write((uint8)CURRENT_VERSION_SHORT);
	data->Write((uint8)EMULE_PROTOCOL);

	// Support for ET_MOD_VERSION [BlackRat]
	// lemonfan - I count 9 tags...
	// Kry - Yes, my fault
	data->Write((uint32)9); /* 7 -> 8 */ 

	CTag tag1(ET_COMPRESSION,1);
	tag1.WriteTagToFile(data);
	CTag tag2(ET_UDPVER,3);
	tag2.WriteTagToFile(data);
	CTag tag3(ET_UDPPORT,theApp.glob_prefs->GetUDPPort());
	tag3.WriteTagToFile(data);
	CTag tag4(ET_SOURCEEXCHANGE,2);
	tag4.WriteTagToFile(data);
	CTag tag5(ET_COMMENTS,1);
	tag5.WriteTagToFile(data);
	CTag tag6(ET_EXTENDEDREQUEST,2);
	tag6.WriteTagToFile(data);
	
	uint32 dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);

	// Kry - Needs the preferences && the preview code form eMule
	/*
	if (theApp.glob_prefs->IsPreviewEnabled())
		dwTagValue |= 128;
	*/
	CTag tag7(ET_FEATURES, dwTagValue);
	tag7.WriteTagToFile(data);
	
	CTag tag8(ET_COMPATIBLECLIENT,SO_AMULE);
	tag8.WriteTagToFile(data);

	// Support for tag ET_MOD_VERSION
	wxString mod_name(MOD_VERSION);
	CTag tag9(ET_MOD_VERSION, mod_name);
	tag9.WriteTagToFile(data);
	// Maella end

	Packet* packet = new Packet(data,OP_EMULEPROT);
	delete data;
	if (!bAnswer)
		packet->opcode = OP_EMULEINFO;
	else
		packet->opcode = OP_EMULEINFOANSWER;
	if (socket) {
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true,true);
	}
}

void CUpDownClient::ProcessMuleInfoPacket(char* pachPacket, uint32 nSize)
{

	try {
		
		//DumpMem(pachPacket,nSize);
		CSafeMemFile data((BYTE*)pachPacket,nSize);
		m_byCompatibleClient = 0;
		//The version number part of this packet will soon be useless since it is only able to go to v.99.
		//Why the version is a uint8 and why it was not done as a tag like the eDonkey hello packet is not known..
		//Therefore, sooner or later, we are going to have to switch over to using the eDonkey hello packet to set the version.
		//No sense making a third value sent for versions..
		data.Read(m_byEmuleVersion);
		if( m_byEmuleVersion == 0x2B ) {
			m_byEmuleVersion = 0x22;
		}	
		uint8 protversion;
		data.Read(protversion);

		//implicitly supported options by older clients
		if (protversion == EMULE_PROTOCOL) {
			//in the future do not use version to guess about new features

			if(m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x22)
				m_byUDPVer = 1;

			if(m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x21)
				m_bySourceExchangeVer = 1;		

			if(m_byEmuleVersion == 0x24)
				m_byAcceptCommentVer = 1;

			// Shared directories are requested from eMule 0.28+ because eMule 0.27 has a bug in 
			// the OP_ASKSHAREDFILESDIR handler, which does not return the shared files for a 
			// directory which has a trailing backslash.
			if(m_byEmuleVersion >= 0x28 && !m_bIsML) // MLdonkey currently does not support shared directories
				m_fSharedDirectories = 1;

		} else {
			return;
		}	
		
		m_bEmuleProtocol = true;

		uint32 tagcount;
		data.Read(tagcount);
		
		for (uint32 i = 0;i < tagcount; i++){
			CTag temptag(&data);
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
					break;
				case ET_MOD_VERSION:
					if (temptag.tag.type == 2) {
						m_strModVersion = temptag.tag.stringvalue;
					}
					else if (temptag.tag.type == 3) {
						m_strModVersion.Format(_T("ModID=%u"), temptag.tag.intvalue);
					}
					else {
						m_strModVersion = _T("ModID=<Unknwon>");
					}
					break;
				default:
					//printf("Mule Unk Tag 0x%02x=%x\n", temptag.tag.specialtag, (UINT)temptag.tag.intvalue);
					break;
			}
		}
	}
	catch ( CStrangePacket )
	{
		printf("\nWrong Tags on Mule Info packet!!\n");
		printf("Sent by %s on ip %s port %i using client %x version %x\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		printf("Packet Dump:\n");
		DumpMem(pachPacket,nSize);
		throw wxString("Wrong Tags on Mule Info packet");
	}
	catch ( CInvalidPacket (e))
	{
		printf("Wrong Tags on Mule Info packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %x version %x\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");		
		printf("Packet Dump:\n");		
		DumpMem(pachPacket,nSize);
		throw wxString("Wrong Tags on Mule Info packet");
	}
	
	if( m_byDataCompVer == 0 ){
		m_bySourceExchangeVer = 0;
		m_byExtendedRequestsVer = 0;
		m_byAcceptCommentVer = 0;
		m_nUDPPort = 0;
	}

	ReGetClientSoft();

	m_byInfopacketsReceived |= IP_EMULEPROTPACK;
	if (m_byInfopacketsReceived == IP_BOTH)
		InfoPacketsReceived();
}

void CUpDownClient::SendHelloAnswer()
{
	CMemFile* data = new CMemFile();
	SendHelloTypePacket(data);
	Packet* packet = new Packet(data);
	delete data;
	packet->opcode = OP_HELLOANSWER;
	if (socket) {
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true);
	}
}

void CUpDownClient::SendHelloTypePacket(CMemFile* data)
{
	data->WriteRaw(theApp.glob_prefs->GetUserHash(),16);
	data->Write(theApp.serverconnect->GetClientID());
	data->Write(theApp.glob_prefs->GetPort());
	
	// Kry - This is the tagcount!!! Be sure to update it!!
	data->Write((uint32)5);
	
	CTag tagname(CT_NAME,theApp.glob_prefs->GetUserNick());
	tagname.WriteTagToFile(data);
	
	CTag tagversion(CT_VERSION,EDONKEYVERSION);
	tagversion.WriteTagToFile(data);
	// eMule UDP Ports
	
	uint32 kadUDPPort = 0;
	#ifdef __USE_KAD__
	if(Kademlia::CKademlia::isConnected())
	{
		kadUDPPort = theApp.glob_prefs->GetUDPPort();
	}
	#endif
	CTag tagUdpPorts(CT_EMULE_UDPPORTS, 
				(kadUDPPort									<< 16) |
				((uint32)theApp.glob_prefs->GetUDPPort()         ) ); 
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
	const UINT uSourceExchangeVer	= 2; //3; Kry - Our source exchange it type 2, TODO
	const UINT uExtendedRequestsVer	= 2;
	const UINT uAcceptCommentVer	= 1;
	const UINT uNoViewSharedFiles	= (theApp.glob_prefs->CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
	const UINT uMultiPacket			= 1;
	const UINT uSupportPreview		= 0; //(thePrefs.CanSeeShares() != vsfaNobody) ? 1 : 0; // set 'Preview supported' only if 'View Shared Files' allowed
	CTag tagMisOptions(CT_EMULE_MISCOPTIONS1, 
//				(						<< 4*7) |
				(uUdpVer				<< 4*6) |
				(uDataCompVer			<< 4*5) |
				(uSupportSecIdent		<< 4*4) |
				(uSourceExchangeVer		<< 4*3) |
				(uExtendedRequestsVer	<< 4*2) |
				(uAcceptCommentVer		<< 4*1) |
//				(						<< 1*3) |
				(uNoViewSharedFiles		<< 1*2) |
				(uMultiPacket			<< 1*1) |
				(uSupportPreview		<< 1*0) );
	tagMisOptions.WriteTagToFile(data);


	
	uint32 dwIP = 0;
	uint16 nPort = 0;
	if (theApp.serverconnect->IsConnected()) {
		dwIP = theApp.serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp.serverconnect->GetCurrentServer()->GetPort();
	}
	data->Write(dwIP);
	data->Write(nPort);
}


void CUpDownClient::ProcessMuleCommentPacket(char* pachPacket, uint32 nSize)
{
	try
	{
		if (!reqfile) {
			throw CInvalidPacket("comment packet for unknown file");
		}

		CSafeMemFile data((BYTE*)pachPacket,nSize);
		uint32 length;
		if ( sizeof(m_iRate) != data.Read(m_iRate) )
			throw CInvalidPacket("short packet reading rating");
		if ( sizeof(length) != data.Read(length) )
			throw CInvalidPacket("short packet reading comment length");
		
		reqfile->SetHasRating(true);
		theApp.amuledlg->AddDebugLogLine(false,_("Rating for file '%s' received: %i"),m_pszClientFilename,m_iRate);
		if (length>50) length=50;
		if (length>0){
			char* desc=new char[length+1];
			memset(desc,0,length+1);
			if ( (unsigned int)length != data.ReadRaw(desc,length) ) {
				throw CInvalidPacket("short packet reading comment string");
			}
			theApp.amuledlg->AddDebugLogLine(false,_("Description for file '%s' received: %s"), m_pszClientFilename, desc);
			m_strComment.Format("%s",desc);				
			theApp.amuledlg->AddDebugLogLine(false,_("Description for file '%s' received: %s"), m_pszClientFilename, m_strComment.GetData());
			reqfile->SetHasComment(true);
			delete[] desc;
		}

	}
	catch ( CStrangePacket )
	{
		printf("\nInvalid MuleComment packet!\n");
		printf("Sent by %s on ip %s port %i using client %i version %i\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		return;
		throw wxString("Wrong MuleComment packet");
	}
	catch ( CInvalidPacket (e))
	{
		printf("\nInvalid MuleComment packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %i version %i\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");		
		throw wxString("Wrong MuleComment packet");
		return;
	}		

	if (reqfile->HasRating() || reqfile->HasComment()) theApp.amuledlg->transferwnd->downloadlistctrl->UpdateItem(reqfile);
}

void CUpDownClient::Disconnected()
{
	#ifdef NET_TEST
	printf("Disconnecting...\n");
	#endif
	
	
	if (GetUploadState() == US_UPLOADING) {
		#ifdef NET_TEST
		printf("UPLOADING Disconnect - Out of queue\n");
		#endif
		theApp.uploadqueue->RemoveFromUploadQueue(this);
	}

	if (m_BlockSend_queue.GetCount() > 0) {
		// Although this should not happen, it happens sometimes. The problem we may run into here is as follows:
		//
		// 1.) If we do not clear the block send requests for that client, we will send those blocks next time the client
		// gets an upload slot. But because we are starting to send any available block send requests right _before_ the
		// remote client had a chance to prepare to deal with them, the first sent blocks will get dropped by the client.
		// Worst thing here is, because the blocks are zipped and can therefore only be uncompressed when the first block
		// was received, all of those sent blocks will create a lot of uncompress errors at the remote client.
		//
		// 2.) The remote client may have already received those blocks from some other client when it gets the next
		// upload slot.
		theApp.amuledlg->AddDebugLogLine(false, "Disconnected client %u. Block send queue=%u.", GetUserID(), m_BlockSend_queue.GetCount());
		ClearUploadBlockRequests();
	}

	if (GetDownloadState() == DS_DOWNLOADING){
		#ifdef NET_TEST
		printf("DOWNLOADING Disconnect - To QUEUE\n");
		#endif
		SetDownloadState(DS_ONQUEUE);
	} else if(GetDownloadState() == DS_CONNECTED) {
		// client didn't responsed to our request for some reasons (remotely banned?)
		// or it just doesn't has this file, so try to swap first
		// if (!SwapToAnotherFile()) {
		if (!SwapToAnotherFile(true, true, true, NULL)) {
			theApp.downloadqueue->RemoveSource(this);
		}
	}


	// The remote client does not have to answer with OP_HASHSETANSWER *immediatly* 
	// after we've sent OP_HASHSETREQUEST. It may occure that a (buggy) remote client 
	// is sending use another OP_FILESTATUS which would let us change to DL-state to DS_ONQUEUE.
	if (((GetDownloadState() == DS_REQHASHSET) || m_fHashsetRequesting) && (reqfile)) {
		#ifdef NET_TEST
		printf("Requesting HASHSET Disconnect - Set Needed\n");
		#endif
        	reqfile->hashsetneeded= true;
	}

	//check if this client is needed in any way, if not delete it
	bool bDelete = true;
	switch(m_byUploadState) {
		case US_ONUPLOADQUEUE:
			#ifdef NET_TEST
			printf("ULState - On upload queue - don't delete\n");
			#endif
			bDelete = false;
	}
	switch(m_nDownloadState) {
		case DS_ONQUEUE:
		case DS_TOOMANYCONNS:
		case DS_NONEEDEDPARTS:
		case DS_LOWTOLOWIP:
			#ifdef NET_TEST
			printf("DLState - On queue or TooManyConns or NoNeededParts or LOWTOLOWIP - don't delete\n");
			#endif
			bDelete = false;
	}
	switch(m_byUploadState) {
		case US_CONNECTING:
		case US_WAITCALLBACK:
		case US_ERROR:
			#ifdef NET_TEST		
			printf("ULState - Connecting or WaitCallback or Error - Delete\n");
			#endif
			bDelete = true;
	}
	switch(m_nDownloadState) {
		case DS_CONNECTING: {
			#ifdef NET_TEST
			printf("DLState - Connecting\n");
			#endif
			m_cFailed++;
			if (m_cFailed <= 2) {
				#ifdef NET_TEST
				printf("\tLess than 2 retries - retrying...\n");
				#endif
				TryToConnect();
				return;
			}
		}
		case DS_WAITCALLBACK:
		case DS_ERROR:
			#ifdef NET_TEST
			printf("DLState - Connecting or WaitCallback or Error - delete\n");
			#endif
			bDelete = true;
	}

	if (GetChatState() != MS_NONE) {
		bDelete = false;
		#ifdef NET_TEST
		printf("Disconnecting from ChatState %i\n",GetChatState());
		#endif
		theApp.amuledlg->chatwnd->chatselector->ConnectingResult(this,false);
	}
	if (socket) {
		#ifdef NET_TEST
		printf("Socket Exists - deleting\n");
		#endif
		socket->Safe_Delete();
	}
	socket = NULL;
	//printf("Socket %x set on client %x\n",socket, this);
	if (m_iFileListRequested) {
		theApp.amuledlg->AddDebugLogLine(false,_("Unable to retrieve shared files from '%s'"),GetUserName());
		m_iFileListRequested = 0;
	}
	if (m_Friend) {
		theApp.friendlist->RefreshFriend(m_Friend);
	}
	if (bDelete) {
		delete this;
	} else {
		m_fHashsetRequesting = 0;
		m_dwEnteredConnectedState = 0;
		m_bHelloAnswerPending = false;
	}
}

bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon)
{
	#ifdef NET_TEST
	printf("Trying to connect\n");
	#endif
	if (theApp.listensocket->TooManySockets() && !bIgnoreMaxCon )  {
		if (!socket) { 
			#ifdef NET_TEST
			printf("Too many sockets\n");
			#endif
			Disconnected();
			return false;
		} else if (!socket->IsConnected()) {
			#ifdef NET_TEST
			printf("Too many sockets\n");
			#endif
			Disconnected();
			return false;			
		}
	}
	if ((theApp.serverconnect->GetClientID() < 16777216) && HasLowID()) {
		#ifdef NET_TEST
		printf("Has LOWID\n");
		#endif
		if (GetDownloadState() == DS_CONNECTING) {
			SetDownloadState(DS_LOWTOLOWIP);
		} else if (GetDownloadState() == DS_REQHASHSET) {
			SetDownloadState(DS_ONQUEUE);
			if (reqfile) {
				reqfile->hashsetneeded = true;
			}
		}
		if (GetUploadState() == US_CONNECTING) {
			Disconnected();
			return false;
		}
	}

	if (!socket) {
		#ifdef NET_TEST
		printf("No socket\n");
		#endif
		socket = new CClientReqSocket(theApp.glob_prefs,this);
	//	printf("Socket %x set on client %x\n",socket, this);
		#ifdef NET_TEST
		printf("Creating Socket... ");
		#endif
		if (!socket->Create()) {
			#ifdef NET_TEST
			printf("Creating Socket Failed!\n");
			#endif
			socket->Safe_Delete();
			return true;
		} else {
			#ifdef NET_TEST
			printf("Creating Socket Done\n");
			#endif
		}
	} else if (!socket->IsConnected()) {
		#ifdef NET_TEST
		printf("Socket Not Connected\n");
		#endif
		socket->Safe_Delete();
		socket = new CClientReqSocket(theApp.glob_prefs,this);
	//	printf("Socket %x set on client %x\n",socket, this);
		#ifdef NET_TEST
		printf("Creating Socket... ");
		#endif
		if (!socket->Create()) {
			#ifdef NET_TEST
			printf("Creating Socket Failed!\n");
			#endif
			socket->Safe_Delete();
			return true;
		} else {
			#ifdef NET_TEST
			printf("Creating Socket Done\n");
			#endif
		}
	} else {
		#ifdef NET_TEST
		printf("Connection OK\n");
		#endif
		ConnectionEstablished();
		return true;
	}
	if (HasLowID()) {

		#ifdef NET_TEST
		printf("LOWID Client\n");
		#endif

		if (GetDownloadState() == DS_CONNECTING) {

			#ifdef NET_TEST
			printf("Download Connecting - WAITCALLBACK\n");
			#endif

			SetDownloadState(DS_WAITCALLBACK);
		}
		if (GetUploadState() == US_CONNECTING) {
			#ifdef NET_TEST
			printf("Upload Connecting - Disconnected\n");
			#endif
			Disconnected();
			return false;
		}

		if (theApp.serverconnect->IsLocalServer(m_dwServerIP,m_nServerPort)) {
			#ifdef NET_TEST
			printf("Client on local server\n");
			#endif
			CMemFile data;
			data.Write(m_nUserID);
			Packet* packet = new Packet(&data);
			packet->opcode = OP_CALLBACKREQUEST;

//			Packet* packet = new Packet(OP_CALLBACKREQUEST,4);
//			memcpy(packet->pBuffer,&m_nUserID,4);

			theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
			theApp.serverconnect->SendPacket(packet);
		} else {
			#ifdef NET_TEST
			printf("Client on non-local server\n");
			#endif
			if (GetUploadState() == US_NONE && (!GetRemoteQueueRank() || m_bReaskPending)) {
				#ifdef NET_TEST
				printf("Not uploading && (No remote queue rank || ReaskPending)\n");
				#endif
				theApp.downloadqueue->RemoveSource(this);
				Disconnected();
				return false;
			} else {
				#ifdef NET_TEST
				printf("NOT (Not uploading && (No remote queue rank || ReaskPending))\n");
				#endif
				if (GetDownloadState() == DS_WAITCALLBACK) {
					#ifdef NET_TEST
					printf("On WAITCALLBACK\n");
					#endif
					m_bReaskPending = true;
					SetDownloadState(DS_ONQUEUE);
				}
			}
		}
	} else {
		#ifdef NET_TEST
		printf("HIGHID Client\n");
		#endif
		amuleIPV4Address tmp;
		tmp.Hostname(GetFullIP());
		tmp.Service(GetUserPort());
		#ifdef NET_TEST
		printf("Connecting socket\n");
		#endif
		socket->Connect(tmp,FALSE);
		// socket->Connect(GetFullIP(),GetUserPort());
		SendHelloPacket();
	}
	return true;
}

void CUpDownClient::ConnectionEstablished()
{
	m_cFailed = 0;
	// ok we have a connection, lets see if we want anything from this client
	#ifdef NET_TEST
	printf("Connection established\n");
	#endif
	if (GetChatState() == MS_CONNECTING) {
		#ifdef NET_TEST
		printf("Chat connecting OK\n");
		#endif
		theApp.amuledlg->chatwnd->chatselector->ConnectingResult(this,true);
	}

	switch(GetDownloadState()) {
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
			#ifdef NET_TEST
			printf("Download Source Connected\n");
			#endif
			m_bReaskPending = false;
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
	}
	if (m_bReaskPending){
		#ifdef NET_TEST
		printf("Reasking OK\n");
		#endif
		m_bReaskPending = false;
		if (GetDownloadState() != DS_NONE && GetDownloadState() != DS_DOWNLOADING) {
			#ifdef NET_TEST
			printf("Client not downloading neither doing nothing - Connected\n");
			#endif
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
		}
	}
	switch(GetUploadState()){
		case US_CONNECTING:
		case US_WAITCALLBACK:
			#ifdef NET_TEST
			printf("Upload connecting or WaitCallback\n");
			#endif
			if (theApp.uploadqueue->IsDownloading(this)) {
				SetUploadState(US_UPLOADING);
				Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
				theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
				socket->SendPacket(packet,true);
			}
	}
	if (m_iFileListRequested == 1) {
		#ifdef NET_TEST
		printf("Sending FileList\n");
		#endif
		Packet* packet = new Packet(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true,true);
	}
	while (!m_WaitingPackets_list.IsEmpty()) {
		socket->SendPacket(m_WaitingPackets_list.RemoveHead());
	}
}

int CUpDownClient::GetHashType() const
{
	if (m_achUserHash[5] == 13 && m_achUserHash[14] == 110)
		return SO_OLDEMULE;
	else if (m_achUserHash[5] == 14 && m_achUserHash[14] == 111)
		return SO_EMULE;
 	else if (m_achUserHash[5] == 'M' && m_achUserHash[14] == 'L')
		return SO_MLDONKEY;
	else
		return SO_UNKNOWN;
}

void CUpDownClient::ReGetClientSoft()
{

	if (!m_pszUsername) {
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
				m_clientVerString = _("cDonkey");
				break;
			case SO_LXMULE:
				m_clientSoft = SO_LXMULE;
				if(GetClientModString().IsEmpty() == false) {
					m_clientVerString = wxString::Format(("xMule %s"), GetClientModString().c_str());
				} else {
					m_clientVerString = _("xMule");
				}
				if (GetMuleVersion() > 0x26) {
					m_clientVerString += wxString::Format(" (Fake eMule version %x)",GetMuleVersion());
				}
				break;
			case SO_AMULE:
				m_clientSoft = SO_AMULE;
				if(GetClientModString().IsEmpty() == false) {
					m_clientVerString = wxString::Format("aMule %s", GetClientModString().c_str());
				} else {
					m_clientVerString = _("aMule");
				}
				break;
			case SO_SHAREAZA:
				m_clientSoft = SO_SHAREAZA;
				m_clientVerString = _("Shareaza");
				break;
			case SO_MLDONKEY:
				m_clientVerString = _("Old MlDonkey");
				break;
			case SO_NEW_MLDONKEY:
				m_clientVerString = _("New MlDonkey");
				break;		
			default:
				if (m_bIsML){
					m_clientSoft = SO_MLDONKEY;
					m_clientVerString = _("MLdonkey");
				}
				else if (m_bIsHybrid){
					m_clientSoft = SO_EDONKEYHYBRID;
					m_clientVerString = _("eDonkeyHybrid");
				}
				else if (m_byCompatibleClient != 0){
					m_clientSoft = SO_COMPAT_UNK;
					m_clientVerString = _("eMule Compat");
				}
				else {
					// If we step here, it might mean 2 things:
					// a eMule
					// a Compat Client that has sent no MuleInfo packet yet.
					m_clientSoft = SO_EMULE;
					m_clientVerString = _("eMule");
				}
		}	
		
		m_SoftLen = m_clientVerString.Length();
		
		if (m_byEmuleVersion == 0){
			m_nClientVersion = 0;
		} else if (m_byEmuleVersion != 0x99) {		
			UINT nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
			m_nClientVersion = nClientMinVersion*100*10;
			if (m_clientSoft == SO_AMULE) {
				m_clientVerString += wxString::Format(" 1.x (based on eMule v0.%u)", nClientMinVersion);
			} else {
				m_clientVerString +=  wxString::Format(" v0.%u", nClientMinVersion);
			}
		} else {					
			UINT nClientMajVersion = (m_nClientVersion >> 17) & 0x7f;
			UINT nClientMinVersion = (m_nClientVersion >> 10) & 0x7f;
			UINT nClientUpVersion  = (m_nClientVersion >>  7) & 0x07;
			
			m_nClientVersion = nClientMajVersion*100*10*100 + nClientMinVersion*100*10 + nClientUpVersion*100;
			if (m_clientSoft == SO_AMULE) {
				if (nClientMajVersion >= 0x0f) {
					m_clientVerString +=  wxString::Format(" v%u.%u.%u CVS", nClientMajVersion & 0x0f, nClientMinVersion, nClientUpVersion);						;
				} else {
					m_clientVerString +=  wxString::Format(" v%u.%u.%u", nClientMajVersion, nClientMinVersion, nClientUpVersion);						
				}
			} else {
				m_clientVerString +=  wxString::Format(" v%u.%u%c", nClientMajVersion, nClientMinVersion, 'a' + nClientUpVersion);
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
		m_nClientVersion = nClientMajVersion*100*10*100 + nClientMinVersion*100*10 + nClientUpVersion*100;

		m_clientVerString = "eDonkeyHybrid";

		m_SoftLen = m_clientVerString.Length();		
		
		if (nClientUpVersion) {
			m_clientVerString += wxString::Format(" v%u.%u.%u", nClientMajVersion, nClientMinVersion, nClientUpVersion);
		} else {
			m_clientVerString += wxString::Format(" v%u.%u", nClientMajVersion, nClientMinVersion);
		}
	
		return;
	}

	if (m_bIsML || (iHashType == SO_MLDONKEY)){
		m_clientSoft = SO_MLDONKEY;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = nClientMinVersion*100*10;
		m_clientVerString = "MLdonkey";
		m_SoftLen = m_clientVerString.Length();
		m_clientVerString += wxString::Format(" v0.%u", nClientMinVersion);
		return;
	}


	if (iHashType == SO_OLDEMULE){
		m_clientSoft = SO_OLDEMULE;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = nClientMinVersion*100*10;
		m_clientVerString = "Old eMule";
		m_SoftLen = m_clientVerString.Length();
		m_clientVerString += wxString::Format(" v0.%u", nClientMinVersion);		
		return;
	}

	m_clientSoft = SO_EDONKEY;
	UINT nClientMinVersion = m_nClientVersion;
	m_nClientVersion = nClientMinVersion*100*10;
	m_clientVerString = "eDonkey";
	m_SoftLen = m_clientVerString.Length();
	m_clientVerString += wxString::Format(" v0.%u", nClientMinVersion);
	
}


void CUpDownClient::SetUserName(char* pszNewName)
{
	if (m_pszUsername) {
		delete[] m_pszUsername;
	}
	if( pszNewName ) {
		m_pszUsername = nstrdup(pszNewName);
	} else {
		m_pszUsername = NULL;
	}
}

void CUpDownClient::RequestSharedFileList()
{
	if (m_iFileListRequested == 0) {
		theApp.amuledlg->AddDebugLogLine(true,_("Requesting shared files from '%s'"),GetUserName());
		m_iFileListRequested = 1;
		TryToConnect(true);
	} else {
		theApp.amuledlg->AddDebugLogLine(true,_("Requesting shared files from user %s (%u) is already in progress"),GetUserName(),GetUserID());
	}
}

void CUpDownClient::ProcessSharedFileList(char* pachPacket, uint32 nSize, LPCTSTR pszDirectory){
    if (m_iFileListRequested > 0){
        m_iFileListRequested--;
		theApp.searchlist->ProcessSearchanswer(pachPacket,nSize,this,NULL,pszDirectory);
	}
}

void CUpDownClient::ResetFileStatusInfo()
{
	if (m_abyPartStatus) {
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	m_nPartCount = 0;

	#warning ADDME - Import needed // m_strClientFilename = "";

	m_bCompleteSource = false;
	m_dwLastAskedTime = 0;
	m_iRate=0;
	m_strComment="";
}

wxString CUpDownClient::GetUploadFileInfo()
{
	if(this == NULL) return "";
	wxString sRet;
 
	// build info text and display it
	sRet.Printf(_("NickName: %s\n"), GetUserName(), GetUserID());
	if (reqfile) {
		sRet += _("Requested:") + wxString(reqfile->GetFileName()) + "\n";
		wxString stat;
		stat.Printf(_("Filestats for this session: Accepted %d of %d requests, %s transferred\n")+CString(_("Filestats for all sessions: Accepted %d of %d requests")),
		reqfile->statistic.GetAccepts(), reqfile->statistic.GetRequests(), CastItoXBytes(reqfile->statistic.GetTransfered()).GetData(),
		reqfile->statistic.GetAllTimeAccepts(),
		reqfile->statistic.GetAllTimeRequests(), CastItoXBytes(reqfile->statistic.GetAllTimeTransfered()).GetData() );
		sRet += stat;
	} else {
		sRet += _("Requested unknown file");
	}
	return sRet;
	return "";
}

void CUpDownClient::Destroy()
{
	if (socket) {
		//delete socket;
		socket->Safe_Delete();
		socket = NULL;
	}
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false it is _possible_ that this clientobject was deleted, because the connectiontry fails 
bool CUpDownClient::SafeSendPacket(Packet* packet)
{
	if (socket && socket->IsConnected()) {
		socket->SendPacket(packet);
		return true;
	} else {
		m_WaitingPackets_list.AddTail(packet);
		return TryToConnect(true);
	}
}

void CUpDownClient::SendPublicKeyPacket(){
	///* delete this line later*/ DEBUG_ONLY(AddDebugLogLine(false, "sending public key to '%s'", GetUserName()));
	// send our public key to the client who requested it
	if (socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED){
		wxASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;

	CMemFile data;
	data.Write(theApp.clientcredits->GetPubKeyLen());
	data.WriteRaw(theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
	Packet* packet = new Packet(&data, OP_EMULEPROT); 
	packet->opcode = OP_PUBLICKEY;
//	Packet* packet = new Packet(OP_PUBLICKEY,theApp.clientcredits->GetPubKeyLen() + 1,OP_EMULEPROT);
//	memcpy(packet->pBuffer+1,theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
//	packet->pBuffer[0] = theApp.clientcredits->GetPubKeyLen();

	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true,true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
}

void CUpDownClient::SendSignaturePacket(){
	// signate the public key of this client and send it
	if (socket == NULL || credits == NULL || m_SecureIdentState == 0){
		wxASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable())
		return;
	if (credits->GetSecIDKeyLen() == 0)
		return; // We don't have his public key yet, will be back here later
		///* delete this line later*/ DEBUG_ONLY(AddDebugLogLine(false, "sending signature key to '%s'", GetUserName()));
	// do we have a challenge value recieved (actually we should if we are in this function)
	if (credits->m_dwCryptRndChallengeFrom == 0){
		theApp.amuledlg->AddDebugLogLine(false, "Want to send signature but challenge value is invalid ('%s')", GetUserName());
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

	uint8 siglen = theApp.clientcredits->CreateSignature(credits, achBuffer,  250, ENDIAN_SWAP_32(ChallengeIP), byChaIPKind );
	if (siglen == 0){
		wxASSERT ( false );
		return;
	}
	CMemFile data;
	data.Write(siglen);
	data.WriteRaw(achBuffer, siglen);
	if (bUseV2) {
		data.Write(byChaIPKind);
	}	
	Packet* packet = new Packet(&data, OP_EMULEPROT);
	packet->opcode = OP_SIGNATURE;

//	Packet* packet = new Packet(OP_SIGNATURE,siglen + 1+ ( (bUseV2)? 1:0 ),OP_EMULEPROT);
//	memcpy(packet->pBuffer+1,achBuffer, siglen);
//	packet->pBuffer[0] = siglen;
//	if (bUseV2)
//		packet->pBuffer[1+siglen] = byChaIPKind;

	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true,true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
}

void CUpDownClient::ProcessPublicKeyPacket(uchar* pachPacket, uint32 nSize){
	theApp.clientlist->AddTrackClient(this);

	///* delete this line later*/ DEBUG_ONLY(AddDebugLogLine(false, "recieving public key from '%s'", GetUserName()));
	if (socket == NULL || credits == NULL || pachPacket[0] != nSize-1
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
			theApp.amuledlg->AddDebugLogLine(false, "Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket");
		}
	}
	else{
		theApp.amuledlg->AddDebugLogLine(false, "Failed to use new recieved public key");
	}
}

void CUpDownClient::ProcessSignaturePacket(uchar* pachPacket, uint32 nSize){
	///* delete this line later*/ DEBUG_ONLY(AddDebugLogLine(false, "receiving signature from '%s'", GetUserName()));
	// here we spread the good guys from the bad ones ;)

	if (socket == NULL || credits == NULL || nSize == 0 || nSize > 250){
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
		theApp.amuledlg->AddDebugLogLine(false, "recieved multiple signatures from one client");
		return;
	}
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0){
		theApp.amuledlg->AddDebugLogLine(false, "recieved signature for client without public key");
		return;
	}
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0){
		theApp.amuledlg->AddDebugLogLine(false, "recieved signature for client with invalid challenge value ('%s')", GetUserName());
		return;
	}

	if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], ENDIAN_SWAP_32(GetIP()), byChaIPKind ) ){
		// result is saved in function abouve
		//AddDebugLogLine(false, "'%s' has passed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);
	}
	else {
		theApp.amuledlg->AddDebugLogLine(false, "'%s' has failed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);
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
			theApp.amuledlg->AddDebugLogLine(false, "Not sending SecIdentState Packet, because State is Zero");
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;
		// Kry - Too much output, it already works.
		//theApp.amuledlg->AddDebugLogLine(false, "sending SecIdentState Packet, state: %i (to '%s')", nValue, GetUserName() );

		CMemFile data;
		data.Write(nValue);
		data.Write(dwRandom);
		Packet* packet = new Packet(&data, OP_EMULEPROT);
		packet->opcode = OP_SECIDENTSTATE;		
//		Packet* packet = new Packet(OP_SECIDENTSTATE,5,OP_EMULEPROT);
//		packet->pBuffer[0] = nValue;
//		memcpy(packet->pBuffer+1,&dwRandom, sizeof(dwRandom));

		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true,true);
	}
	else {
		wxASSERT ( false );
	}
}

void CUpDownClient::ProcessSecIdentStatePacket(uchar* pachPacket, uint32 nSize){
	if (nSize != 5)
		return;
	if (!credits){
		wxASSERT ( false );
		return;
	}
	switch(pachPacket[0]){
			case 0:
				m_SecureIdentState = IS_UNAVAILABLE;
				break;
			case 1:
				m_SecureIdentState = IS_SIGNATURENEEDED;
				break;
			case 2:
				m_SecureIdentState = IS_KEYANDSIGNEEDED;
				break;
		}
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	// Kry:  + 1 on the original one.
	try {
		byte discard;
		data.Read(discard);		
		uint32 dwRandom;
		data.Read(dwRandom);
		credits->m_dwCryptRndChallengeFrom = dwRandom;
	} 
	catch ( CStrangePacket )
	{
		printf("\nWrong Tags on SecIdentState packet!!\n");
		printf("Sent by %s on ip %s port %i using client %i version %i\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");
		throw wxString("Wrong Tags on SecIdentState packet");
	}
	catch ( CInvalidPacket (e))
	{
		printf("Wrong Tags on SecIdentState packet - %s\n\n",e.what());
		printf("Sent by %s on ip %s port %i using client %i version %i\n",GetUserName(),GetFullIP(),GetUserPort(),GetClientSoft(),GetMuleVersion());
		printf("User Disconnected.\n");		
		throw wxString("Wrong Tags on SecIdentState packet");
	}
			
	
	//DEBUG_ONLY(AddDebugLogLine(false, "recieved SecIdentState Packet, state: %i", pachPacket[0]));
}


void CUpDownClient::InfoPacketsReceived(){
	// indicates that both Information Packets has been received
	// needed for actions, which process data from both packets
	wxASSERT ( m_byInfopacketsReceived == IP_BOTH );
	m_byInfopacketsReceived = IP_NONE;
	
	if (m_bySupportSecIdent){
		SendSecIdentStatePacket();
	}
}

bool CUpDownClient::CheckHandshakeFinished(UINT protocol, UINT opcode) const
{
	if (m_bHelloAnswerPending){
		//throw CString(_T("Handshake not finished")); // -> disconnect client
		// this triggers way too often.. need more time to look at this -> only create a warning
		if (theApp.glob_prefs->GetVerbose()) {
			theApp.amuledlg->AddLogLine(false, _("Handshake not finished while processing packet."));
		}
		return false;
	}

	return true;
}

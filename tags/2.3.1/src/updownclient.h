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

#ifndef UPDOWNCLIENT_H
#define UPDOWNCLIENT_H

#include "Constants.h"		// Needed for ESourceFrom
#include "GetTickCount.h"	// Needed for GetTickCount
#include "MD4Hash.h"
#include <common/StringFunctions.h>
#include "NetworkFunctions.h"
#include "OtherStructs.h"
#include "ClientCredits.h"	// Needed for EIdentState
#include <ec/cpp/ECID.h>	// Needed for CECID
#include "BitVector.h"		// Needed for BitVector
#include "ClientRef.h"		// Needed for debug defines

#include <map>


class CPartFile;
class CClientTCPSocket;
class CPacket;
class CFriend;
class CKnownFile;
class CMemFile;
class CAICHHash;


enum EChatCaptchaState {
	CA_NONE				= 0,
	CA_CHALLENGESENT,
	CA_CAPTCHASOLVED,
	CA_ACCEPTING,
	CA_CAPTCHARECV,
	CA_SOLUTIONSENT
};

enum ESecureIdentState {
	IS_UNAVAILABLE		= 0,
	IS_ALLREQUESTSSEND	= 0,
	IS_SIGNATURENEEDED	= 1,
	IS_KEYANDSIGNEEDED	= 2
};

enum EInfoPacketState {
	IP_NONE			= 0,
	IP_EDONKEYPROTPACK	= 1,
	IP_EMULEPROTPACK	= 2,
	IP_BOTH			= 3
};

enum EKadState {
	KS_NONE,
	KS_QUEUED_FWCHECK,
	KS_CONNECTING_FWCHECK,
	KS_CONNECTED_FWCHECK,
	KS_QUEUED_BUDDY,
	KS_INCOMING_BUDDY,
	KS_CONNECTING_BUDDY,
	KS_CONNECTED_BUDDY,
	KS_QUEUED_FWCHECK_UDP,
	KS_FWCHECK_UDP,
	KS_CONNECTING_FWCHECK_UDP
};

//! Used to keep track of the state of the client
enum ClientState
{
	//! New is for clients that have just been created.
	CS_NEW = 0,
	//! Listed is for clients that are on the clientlist
	CS_LISTED,
	//! Dying signifies clients that have been queued for deletion
	CS_DYING
};

// This is fixed on ed2k v1, but can be any number on ED2Kv2
#define STANDARD_BLOCKS_REQUEST 3

class CUpDownClient : public CECID
{
	friend class CClientList;
	friend class CClientRef;
private:
	/**
	 * Please note that only the ClientList is allowed to delete the clients.
	 * To schedule a client for deletion, call the CClientList::AddToDeleteQueue
	 * funtion, which will safely remove dead clients once every second.
	 */
	~CUpDownClient();

	/**
	 * Reference count which is increased whenever client is linked to a clientref.
	 * Clients are to be stored only by ClientRefs, CUpDownClient * are for temporary 
	 * use only.
	 * Linking is done only by CClientRef which is friend, so methods are private.
	 */
	uint16 m_linked;
#ifdef DEBUG_ZOMBIE_CLIENTS
	bool	m_linkedDebug;
	std::multiset<wxString> m_linkedFrom;
	void	Link(const wxString& from)		{ m_linked++; m_linkedFrom.insert(from); }
	void	Unlink(const wxString& from);
	wxString GetLinkedFrom() {
		wxString ret;
		for (std::multiset<wxString>::iterator it = m_linkedFrom.begin(); it != m_linkedFrom.end(); it++) {
			ret += *it + wxT(", ");
		}
		return ret;
	}
#else
	void	Link()		{ m_linked++; }
	void	Unlink();
#endif

public:
	//base
	CUpDownClient(CClientTCPSocket* sender = 0);
	CUpDownClient(uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport,CPartFile* in_reqfile, bool ed2kID, bool checkfriend);

	/**
	 * This function is to be called when the client object is to be deleted.
	 * It'll close the socket of the client and remove it from various lists
	 * that can own it.
	 *
	 * The client will really be deleted only after thelast reference to it
	 * is unlinked;
	 */	
	void		Safe_Delete();

	/**
	 * Specifies if the client has been queued for deletion.
	 *
	 * @return True if Safe_Delete has been called, false otherwise.
	 */
	bool		HasBeenDeleted()		{ return m_clientState == CS_DYING; }

	ClientState	GetClientState()		{ return m_clientState; }

	bool		Disconnected(const wxString& strReason, bool bFromSocket = false);
	bool		TryToConnect(bool bIgnoreMaxCon = false);
	bool		Connect();
	void		ConnectionEstablished();
	const wxString&	GetUserName() const		{ return m_Username; }
	//Only use this when you know the real IP or when your clearing it.
	void		SetIP( uint32 val );
	uint32		GetIP() const 			{ return m_dwUserIP; }
	bool		HasLowID() const 		{ return IsLowID(m_nUserIDHybrid); }
	wxString	GetFullIP() const		{ return Uint32toStringIP(m_FullUserIP); }
	uint32		GetConnectIP() const		{ return m_nConnectIP; }
	uint32		GetUserIDHybrid() const		{ return m_nUserIDHybrid; }
	void		SetUserIDHybrid(uint32 val);
	uint16_t	GetUserPort() const		{ return m_nUserPort; }
	void		SetUserPort(uint16_t port)	{ m_nUserPort = port; }
	uint64		GetTransferredDown() const	{ return m_nTransferredDown; }
	uint32		GetServerIP() const		{ return m_dwServerIP; }
	void		SetServerIP(uint32 nIP)		{ m_dwServerIP = nIP; }
	uint16		GetServerPort()	const		{ return m_nServerPort; }
	void		SetServerPort(uint16 nPort)	{ m_nServerPort = nPort; }	
	const CMD4Hash&	GetUserHash() const		{ return m_UserHash; }
	void		SetUserHash(const CMD4Hash& userhash);
	void		ValidateHash()			{ m_HasValidHash = !m_UserHash.IsEmpty(); }
	bool		HasValidHash() const		{ return m_HasValidHash; }
	uint32		GetVersion() const		{ return m_nClientVersion;}
	uint8		GetMuleVersion() const		{ return m_byEmuleVersion;}
	bool		ExtProtocolAvailable() const	{ return m_bEmuleProtocol;}
	bool		IsEmuleClient()	const		{ return (m_byEmuleVersion > 0);}
	bool		IsBanned() const;
	const wxString&	GetClientFilename() const	{ return m_clientFilename; }
	uint16		GetUDPPort() const		{ return m_nUDPPort; }
	void		SetUDPPort(uint16 nPort)	{ m_nUDPPort = nPort; }
	uint8		GetUDPVersion() const		{ return m_byUDPVer; }
	uint8		GetExtendedRequestsVersion() const { return m_byExtendedRequestsVer; }
	bool		IsFriend() const 		{ return m_Friend != NULL; }
	bool		IsML() const			{ return m_bIsML; }
	bool		IsHybrid() const		{ return m_bIsHybrid; }
	uint32		GetCompatibleClient() const	{ return m_byCompatibleClient; }

	void		ClearDownloadBlockRequests();
	void		RequestSharedFileList();
	void		ProcessSharedFileList(const byte* pachPacket, uint32 nSize, wxString& pszDirectory);
	void		SendSharedDirectories();
	void		SendSharedFilesOfDirectory(const wxString& strReqDir);

	wxString	GetUploadFileInfo();

	void		SetUserName(const wxString& NewName) { m_Username = NewName; }

	uint8		GetClientSoft() const		{ return m_clientSoft; }
	void		ReGetClientSoft();
	bool		ProcessHelloAnswer(const byte* pachPacket, uint32 nSize);
	bool		ProcessHelloPacket(const byte* pachPacket, uint32 nSize);
	void		SendHelloAnswer();
	bool		SendHelloPacket();
	void		SendMuleInfoPacket(bool bAnswer, bool OSInfo = false);
	bool		ProcessMuleInfoPacket(const byte* pachPacket, uint32 nSize);
	void		ProcessMuleCommentPacket(const byte* pachPacket, uint32 nSize);
	bool		Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash = false) const;
	void		SetLastSrcReqTime()		{ m_dwLastSourceRequest = ::GetTickCount(); }
	void		SetLastSrcAnswerTime()		{ m_dwLastSourceAnswer = ::GetTickCount(); }
	void		SetLastAskedForSources()	{ m_dwLastAskedForSources = ::GetTickCount(); }
	uint32		GetLastSrcReqTime() const 	{ return m_dwLastSourceRequest; }
	uint32		GetLastSrcAnswerTime() const	{ return m_dwLastSourceAnswer; }
	uint32		GetLastAskedForSources() const	{ return m_dwLastAskedForSources; }
	bool		GetFriendSlot() const 		{ return m_bFriendSlot; }
	void		SetFriendSlot(bool bNV)		{ m_bFriendSlot = bNV; }
	void		SetCommentDirty(bool bDirty = true)	{ m_bCommentDirty = bDirty; }
	uint8		GetSourceExchange1Version() const	{ return m_bySourceExchange1Ver; }
	bool		SupportsSourceExchange2() const		{ return m_fSupportsSourceEx2; }
	
	bool		SafeSendPacket(CPacket* packet);

	void		ProcessRequestPartsPacket(const byte* pachPacket, uint32 nSize, bool largeblocks);
	void		ProcessRequestPartsPacketv2(const CMemFile& data);	
	
	void		SendPublicKeyPacket();
	void		SendSignaturePacket();
	void		ProcessPublicKeyPacket(const byte* pachPacket, uint32 nSize);
	void		ProcessSignaturePacket(const byte* pachPacket, uint32 nSize);
	uint8		GetSecureIdentState(); 

	void		SendSecIdentStatePacket();
	void		ProcessSecIdentStatePacket(const byte* pachPacket, uint32 nSize);

	uint8		GetInfoPacketsReceived() const	{ return m_byInfopacketsReceived; }
	void		InfoPacketsReceived();

	//upload
	uint8		GetUploadState() const		{ return m_nUploadState; }
	void		SetUploadState(uint8 news);
	uint64		GetTransferredUp() const	{ return m_nTransferredUp; }
	uint64		GetSessionUp() const		{ return m_nTransferredUp - m_nCurSessionUp; }
	void		ResetSessionUp();
	uint32		GetUploadDatarate() const	{ return m_nUpDatarate; }

	//uint32		GetWaitTime() const 		{ return m_dwUploadTime - GetWaitStartTime(); }
	uint32		GetUpStartTimeDelay() const	{ return ::GetTickCount() - m_dwUploadTime; }
	uint32		GetWaitStartTime() const;

	bool		IsDownloading()	const 		{ return (m_nUploadState == US_UPLOADING); }

	uint32		GetScore() const	{ return m_score; }
	uint32		CalculateScore()	{ m_score = CalculateScoreInternal(); return m_score; }
	void		ClearScore()		{ m_score = 0; }
	uint16		GetUploadQueueWaitingPosition() const	{ return m_waitingPosition; }
	void		SetUploadQueueWaitingPosition(uint16 pos)	{ m_waitingPosition = pos; }
	uint8		GetObfuscationStatus() const;
	uint16		GetNextRequestedPart() const;

	void		AddReqBlock(Requested_Block_Struct* reqblock);
	void		CreateNextBlockPackage();
	void		SetUpStartTime() 		{ m_dwUploadTime = ::GetTickCount(); }
	void		SetWaitStartTime();
	void		ClearWaitStartTime();
	void		SendHashsetPacket(const CMD4Hash& forfileid);
	bool		SupportMultiPacket() const	{ return m_bMultiPacket; }
	bool		SupportExtMultiPacket() const	{ return m_fExtMultiPacket; }

	void		SetUploadFileID(CKnownFile *newreqfile);

	/**
	 *Gets the file actually on upload
	 *
	 */
	const CKnownFile* GetUploadFile() const		{ return m_uploadingfile; }

	void		SendOutOfPartReqsAndAddToWaitingQueue();
	void		ProcessExtendedInfo(const CMemFile *data, CKnownFile *tempreqfile);
	void		ProcessFileInfo(const CMemFile* data, const CPartFile* file);
	void		ProcessFileStatus(bool bUdpPacket, const CMemFile* data, const CPartFile* file);

	const CMD4Hash&	GetUploadFileID() const		{ return m_requpfileid; }
	void		SetUploadFileID(const CMD4Hash& new_id);
	void		ClearUploadFileID()		{ m_requpfileid.Clear(); m_uploadingfile = NULL;}
	uint32		SendBlockData();
	void		ClearUploadBlockRequests();
	void		SendRankingInfo();
	void		SendCommentInfo(CKnownFile *file);
	bool 		IsDifferentPartBlock() const;
	void		UnBan();
	void		Ban();
	bool		m_bAddNextConnect;      // VQB Fix for LowID slots only on connection
	uint32		GetAskedCount() const 		{ return m_cAsked; }
	void		AddAskedCount()			{ m_cAsked++; }
	void		ClearAskedCount()		{ m_cAsked = 1; }	// 1, because it's cleared *after* the first request...
	void		FlushSendBlocks();	// call this when you stop upload, 
						// or the socket might be not able to send
	void		SetLastUpRequest()		{ m_dwLastUpRequest = ::GetTickCount(); }
	uint32		GetLastUpRequest() const 	{ return m_dwLastUpRequest; }
	size_t		GetUpPartCount() const 		{ return m_upPartStatus.size(); }


	//download
	void 		SetRequestFile(CPartFile* reqfile); 
	CPartFile*	GetRequestFile() const		{ return m_reqfile; }

	uint8		GetDownloadState() const	{ return m_nDownloadState; }
	void		SetDownloadState(uint8 byNewState);
	uint32		GetLastAskedTime() const	{ return m_dwLastAskedTime; }
	void		ResetLastAskedTime()		{ m_dwLastAskedTime = 0; }

	bool		IsPartAvailable(uint16 iPart) const
					{ return ( iPart < m_downPartStatus.size() ) ? m_downPartStatus.get(iPart) : 0; }
	bool		IsUpPartAvailable(uint16 iPart) const 
					{ return ( iPart < m_upPartStatus.size() ) ? m_upPartStatus.get(iPart) : 0;}

	const BitVector& GetPartStatus() const		{ return m_downPartStatus; }
	const BitVector& GetUpPartStatus() const	{ return m_upPartStatus; }
	float		GetKBpsDown() const				{ return kBpsDown; }
	float		CalculateKBpsDown();
	uint16		GetRemoteQueueRank() const	{ return m_nRemoteQueueRank; }
	uint16		GetOldRemoteQueueRank() const	{ return m_nOldRemoteQueueRank; }
	void		SetRemoteQueueFull(bool flag)	{ m_bRemoteQueueFull = flag; }
	bool		IsRemoteQueueFull() const 	{ return m_bRemoteQueueFull; }
	void		SetRemoteQueueRank(uint16 nr);
	bool		AskForDownload();
	void		SendStartupLoadReq();
	void		SendFileRequest();
	void		ProcessHashSet(const byte* packet, uint32 size);
	bool		AddRequestForAnotherFile(CPartFile* file);
	bool		DeleteFileRequest(CPartFile* file);
	void		DeleteAllFileRequests();
	void		SendBlockRequests();
	void		ProcessBlockPacket(const byte* packet, uint32 size, bool packed, bool largeblocks);
	uint16		GetAvailablePartCount() const;

	bool		SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL);
	void		UDPReaskACK(uint16 nNewQR);
	void		UDPReaskFNF();
	void		UDPReaskForDownload();
	bool		IsSourceRequestAllowed();
	uint16		GetUpCompleteSourcesCount() const	{ return m_nUpCompleteSourcesCount; }
	void		SetUpCompleteSourcesCount(uint16 n)	{ m_nUpCompleteSourcesCount = n; }

	//chat
	uint8		GetChatState()			{ return m_byChatstate; }
	void		SetChatState(uint8 nNewS)	{ m_byChatstate = nNewS; }
	EChatCaptchaState GetChatCaptchaState() const	{ return (EChatCaptchaState)m_nChatCaptchaState; }
	void		ProcessCaptchaRequest(CMemFile* data);
	void		ProcessCaptchaReqRes(uint8 nStatus);
	void		ProcessChatMessage(wxString message);
	// message filtering
	uint8		GetMessagesReceived() const	{ return m_cMessagesReceived; }
	void		IncMessagesReceived()		{ m_cMessagesReceived < 255 ? ++m_cMessagesReceived : 255; }
	uint8		GetMessagesSent() const		{ return m_cMessagesSent; }
	void		IncMessagesSent()		{ m_cMessagesSent < 255 ? ++m_cMessagesSent : 255; }
	bool		IsSpammer() const		{ return m_fIsSpammer; }
	void		SetSpammer(bool bVal);
	bool		IsMessageFiltered(const wxString& message);

	//File Comment
	const wxString&	GetFileComment() const 		{ return m_strComment; }
	uint8		GetFileRating() const		{ return m_iRating; }

	const wxString&	GetSoftStr() const 		{ return m_clientSoftString; }
	const wxString&	GetSoftVerStr() const		{ return m_clientVerString; }
	const wxString GetServerName() const;
	
	uint16		GetKadPort() const		{ return m_nKadPort; }
	void		SetKadPort(uint16 nPort)	{ m_nKadPort = nPort; }

	// Kry - AICH import
	void		SetReqFileAICHHash(CAICHHash* val);
	CAICHHash*	GetReqFileAICHHash() const	{return m_pReqFileAICHHash;}
	bool		IsSupportingAICH() const	{return m_fSupportsAICH & 0x01;}
	void		SendAICHRequest(CPartFile* pForFile, uint16 nPart);
	bool		IsAICHReqPending() const	{return m_fAICHRequested; }
	void		ProcessAICHAnswer(const byte* packet, uint32 size);
	void		ProcessAICHRequest(const byte* packet, uint32 size);
	void		ProcessAICHFileHash(CMemFile* data, const CPartFile* file);	

	EUtf8Str	GetUnicodeSupport() const;

	// Barry - Process zip file as it arrives, don't need to wait until end of block
	int 		unzip(Pending_Block_Struct *block, byte *zipped, uint32 lenZipped, byte **unzipped, uint32 *lenUnzipped, int iRecursion = 0);
	void 		UpdateDisplayedInfo(bool force = false);
	int 		GetFileListRequested() const 	{ return m_iFileListRequested; }
	void 		SetFileListRequested(int iFileListRequested) { m_iFileListRequested = iFileListRequested; }

	void		ResetFileStatusInfo();

	bool		CheckHandshakeFinished() const;

	bool		GetSentCancelTransfer() const	{ return m_fSentCancelTransfer; }
	void		SetSentCancelTransfer(bool bVal)	{ m_fSentCancelTransfer = bVal; }

	wxString	GetClientFullInfo();
	wxString	GetClientShortInfo();

	const wxString& GetClientOSInfo() const		{ return m_sClientOSInfo; }

	void		ProcessPublicIPAnswer(const byte* pbyData, uint32 uSize);
	void		SendPublicIPRequest();

	/**
	 * Sets the current socket of the client.
	 * 
	 * @param socket The pointer to the new socket, can be NULL.
	 *
	 * Please note that this function DOES NOT delete the old socket.
	 */
	void 		SetSocket(CClientTCPSocket* socket);

	/**
	 * Function for accessing the socket owned by a client.
	 * 
	 * @return The pointer (can be NULL) to the socket used by this client.
	 *
	 * Please note that the socket object is quite volatile and can be removed
	 * from one function call to the next, therefore, you should normally use 
	 * the safer functions below, which all check if the socket is valid before
	 * deferring it.
	 */
	CClientTCPSocket* GetSocket() const		{ return m_socket; }

	/**
	 * Safe function for checking if the socket is connected.
	 *
	 * @return True if the socket exists and is connected, false otherwise.
	 */
	bool		IsConnected() const;

	/**
	 * Safe function for sending packets.
	 *
	 * @return True if the socket exists and the packet was sent, false otherwise.
	 */
	bool		SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true);

	/**
	 * Safe function for setting the download limit of the socket.
	 *
	 * @return Current download speed of the client.
	 */
	float		SetDownloadLimit(uint32 reducedownload);

	/**
	 * Sends a message to a client
	 *
	 * @return True if sent, false if connecting
	 */
	bool		SendChatMessage(const wxString& message);

	bool		HasBlocks() const		{ return !m_BlockRequests_queue.empty(); }

	/* Source comes from? */
	ESourceFrom		GetSourceFrom() const	{ return m_nSourceFrom; }
	void			SetSourceFrom(ESourceFrom val)	{ m_nSourceFrom = val; }

	/* Kad buddy support */
	// ID
	const byte*	GetBuddyID() const		{ return m_achBuddyID; }
	void		SetBuddyID(const byte* m_achTempBuddyID);
	bool		HasValidBuddyID() const		{ return m_bBuddyIDValid; }
	/* IP */
	void		SetBuddyIP( uint32 val )	{ m_nBuddyIP = val; }
	uint32		GetBuddyIP() const		{ return m_nBuddyIP; }
	/* Port */
	void		SetBuddyPort( uint16 val )	{ m_nBuddyPort = val; }
	uint16		GetBuddyPort() const		{ return m_nBuddyPort; }

	//KadIPCheck
	bool		SendBuddyPingPong()		{ return m_dwLastBuddyPingPongTime < ::GetTickCount(); }
	bool		AllowIncomeingBuddyPingPong()	{ return m_dwLastBuddyPingPongTime < (::GetTickCount()-(3*60*1000)); }
	void		SetLastBuddyPingPongTime()	{ m_dwLastBuddyPingPongTime = (::GetTickCount()+(10*60*1000)); }	
	EKadState	GetKadState() const		{ return m_nKadState; }
	void		SetKadState(EKadState nNewS)	{ m_nKadState = nNewS; }
	uint8		GetKadVersion()			{ return m_byKadVersion; }
	void		ProcessFirewallCheckUDPRequest(CMemFile *data);
	// Kad added by me
	bool		SendBuddyPing();
	
	/* Returns the client hash type (SO_EMULE, mldonkey, etc) */
	int		GetHashType() const;

	/**
	 * Checks that a client isn't aggressively re-asking for files.
	 * 
	 * Call this when a file is requested. If the time since the last request is
	 * less than MIN_REQUESTTIME, 3 is added to the m_Aggressiveness variable.
	 * If the time since the last request is >= MIN_REQUESTTIME, the variable is
	 * decremented by 1. The client is banned if the variable reaches 10 or above.
	 *
	 * To check if a client is aggressive use the IsClientAggressive() function.
	 * 
	 * Currently this function is called when the following packets are received:
	 *  - OP_STARTUPLOADREQ
	 *  - OP_REASKFILEPING
	 */
	void		CheckForAggressive();

	const wxString&	GetClientModString() const	{ return m_strModVersion; }

	const wxString&	GetClientVerString() const	{ return m_fullClientVerString; }

	const wxString&	GetVersionString() const	{ return m_clientVersionString; }

	void		UpdateStats();

	/* Returns a pointer to the credits, only for hash purposes */
	void*		GetCreditsHash() const { return (void*)credits; }
	
	uint16		GetLastDownloadingPart() const { return m_lastDownloadingPart; }
	
	bool		GetOSInfoSupport() const { return m_fOsInfoSupport; }
	
	bool		GetVBTTags() const { return m_fValueBasedTypeTags; }
	
	uint16		GetLastPartAsked() const { return m_lastPartAsked; }
	
	void		SetLastPartAsked(uint16 nPart) { m_lastPartAsked = nPart; }
	
	CFriend*	GetFriend() const { return m_Friend; }
	
	void		SetFriend(CFriend* newfriend) { m_Friend = newfriend; }
	
	bool		IsIdentified() const;
	
	bool		IsBadGuy() const;
	
	bool		SUIFailed() const;
	
	bool		SUINeeded() const;
	
	bool		SUINotSupported() const;

	uint64		GetDownloadedTotal() const;
	
	uint64		GetUploadedTotal() const;
	
	double 		GetScoreRatio() const;
	
	uint32		GetCreationTime() const { return m_nCreationTime; }
	
	bool		SupportsLargeFiles() const { return m_fSupportsLargeFiles; }

	EIdentState	GetCurrentIdentState() const { return credits ? credits->GetCurrentIdentState(GetIP()) : IS_NOTAVAILABLE; }
	
#ifdef __DEBUG__
	/* Kry - Debug. See connection_reason definition comment below */
	void		SetConnectionReason(const wxString& reason) { connection_reason = reason; }
#endif

	// Encryption / Obfuscation / ConnectOptions
	bool		SupportsCryptLayer() const			{ return m_fSupportsCryptLayer; }
	bool		RequestsCryptLayer() const			{ return SupportsCryptLayer() && m_fRequestsCryptLayer; }
	bool		RequiresCryptLayer() const			{ return RequestsCryptLayer() && m_fRequiresCryptLayer; }
	bool		SupportsDirectUDPCallback() const		{ return m_fDirectUDPCallback != 0 && HasValidHash() && GetKadPort() != 0; }
	uint32_t	GetDirectCallbackTimeout() const		{ return m_dwDirectCallbackTimeout; }
	bool		HasObfuscatedConnectionBeenEstablished() const	{ return m_hasbeenobfuscatinglately; }

	void		SetCryptLayerSupport(bool bVal)			{ m_fSupportsCryptLayer = bVal ? 1 : 0; }
	void		SetCryptLayerRequest(bool bVal)			{ m_fRequestsCryptLayer = bVal ? 1 : 0; }
	void		SetCryptLayerRequires(bool bVal)		{ m_fRequiresCryptLayer = bVal ? 1 : 0; }
	void		SetDirectUDPCallbackSupport(bool bVal)		{ m_fDirectUDPCallback = bVal ? 1 : 0; }
	void		SetConnectOptions(uint8_t options, bool encryption = true, bool callback = true); // shortcut, sets crypt, callback, etc from the tagvalue we receive
	bool		ShouldReceiveCryptUDPPackets() const;

	bool		HasDisabledSharedFiles() const { return m_fNoViewSharedFiles; }
	
private:
	
	CClientCredits	*credits;
	CFriend 	*m_Friend;

	uint64		m_nTransferredUp;
	sint64		m_nCurQueueSessionPayloadUp;
	sint64		m_addedPayloadQueueSession;

	struct TransferredData {
		uint32	datalen;
		uint32	timestamp;
	};

	//////////////////////////////////////////////////////////
	// Upload data rate computation
	//
	uint32		m_nUpDatarate;
	uint32		m_nSumForAvgUpDataRate;
	std::list<TransferredData> m_AvarageUDR_list;


	/**
	 * This struct is used to keep track of CPartFiles which this source shares.
	 */
	struct A4AFStamp {
		//! Signifies if this sources has needed parts for this file. 
		bool NeededParts;
		//! This is set when we wish to avoid swapping to this file for a while.
		uint32 timestamp;
	};

	//! I typedef in the name of readability!
	typedef std::map<CPartFile*, A4AFStamp> A4AFList;
	//! This list contains all PartFiles which this client can be used as a source for.
	A4AFList m_A4AF_list;

	/**
	 * Helper function used by SwapToAnotherFile().
	 *
	 * @param it The iterator of the PartFile to be examined.
	 * @param ignorenoneeded Do not check for the status NoNeededParts when checking the file.
	 * @param ignoresuspended Do not check the timestamp when checking the file.
	 * @return True if the file is a viable target, false otherwise.
	 *
	 * This function is used to perform checks to see if we should consider 
	 * this file a viable target for A4AF swapping. Unless ignoresuspended is
	 * true, it will examine the timestamp of the file and reset it if needed.
	 */
	bool		IsValidSwapTarget( A4AFList::iterator it, bool ignorenoneeded = false, bool ignoresuspended = false );

	CPartFile*	m_reqfile;

	// base
	void		Init();
	bool		ProcessHelloTypePacket(const CMemFile& data);
	void		SendHelloTypePacket(CMemFile* data);
	void		SendFirewallCheckUDPRequest();
	void		ClearHelloProperties(); // eMule 0.42

	uint32		m_dwUserIP;
	uint32		m_nConnectIP;		// holds the supposed IP or (after we had a connection) the real IP
	uint32		m_dwServerIP;
	uint32		m_nUserIDHybrid;
	uint16_t	m_nUserPort;
	int16		m_nServerPort;
	uint32		m_nClientVersion;
	uint32		m_cSendblock;
	uint8		m_byEmuleVersion;
	uint8		m_byDataCompVer;
	bool		m_bEmuleProtocol;
	wxString	m_Username;
	uint32		m_FullUserIP;
	CMD4Hash	m_UserHash;
	bool		m_HasValidHash;
	uint16		m_nUDPPort;
	uint8		m_byUDPVer;
	uint8		m_bySourceExchange1Ver;
	uint8		m_byAcceptCommentVer;
	uint8		m_byExtendedRequestsVer;
	uint8		m_clientSoft;
	uint32		m_dwLastSourceRequest;
	uint32		m_dwLastSourceAnswer;
	uint32		m_dwLastAskedForSources;
	int		m_iFileListRequested;
	bool		m_bFriendSlot;
	bool		m_bCommentDirty;
	bool		m_bIsHybrid;
	bool		m_bIsML;
 	bool		m_bSupportsPreview;
	bool		m_bUnicodeSupport;	
	uint16		m_nKadPort;
	bool		m_bMultiPacket;
	ClientState	m_clientState;
	CClientTCPSocket*	m_socket;		
	bool		m_fNeedOurPublicIP; // we requested our IP from this client

	// Kry - Secure User Ident import
	ESecureIdentState	m_SecureIdentState; 
	uint8		m_byInfopacketsReceived;		// have we received the edonkeyprot and emuleprot packet already (see InfoPacketsReceived() )
	uint32		m_dwLastSignatureIP;
	uint8		m_bySupportSecIdent;

	uint32		m_byCompatibleClient;
	std::list<CPacket*>	m_WaitingPackets_list;
	uint32		m_lastRefreshedDLDisplay;

	//upload
	void CreateStandardPackets(const unsigned char* data,uint32 togo, Requested_Block_Struct* currentblock);
	void CreatePackedPackets(const unsigned char* data,uint32 togo, Requested_Block_Struct* currentblock);
	uint32 CalculateScoreInternal();

	uint8		m_nUploadState;
	uint32		m_dwUploadTime;
	uint32		m_cAsked;
	uint32		m_dwLastUpRequest;
	uint32		m_nCurSessionUp;
	uint16		m_nUpPartCount;
	CMD4Hash	m_requpfileid;
	uint16		m_nUpCompleteSourcesCount;
	uint32		m_score;
	uint16		m_waitingPosition;

	//! This vector contains the avilability of parts for the file that the user
	//! is requesting. When changing it, be sure to call CKnownFile::UpdatePartsFrequency
	//! so that the files know the actual availability of parts.
	BitVector	m_upPartStatus;
	uint16		m_lastPartAsked;
	wxString	m_strModVersion;

	std::list<Requested_Block_Struct*>	m_BlockRequests_queue;
	std::list<Requested_Block_Struct*>	m_DoneBlocks_list;

	//download
	bool		m_bRemoteQueueFull;
	uint8		m_nDownloadState;
	uint16		m_nPartCount;
	uint32		m_dwLastAskedTime;
	wxString	m_clientFilename;
	uint64		m_nTransferredDown;
	uint16		m_lastDownloadingPart;   // last Part that was downloading
	uint16		m_cShowDR;
	uint32		m_dwLastBlockReceived;
	uint16		m_nRemoteQueueRank;
	uint16		m_nOldRemoteQueueRank;
	bool		m_bCompleteSource;
	bool		m_bReaskPending;
	bool		m_bUDPPending;
	bool		m_bHashsetRequested;

	std::list<Pending_Block_Struct*>	m_PendingBlocks_list;
	std::list<Requested_Block_Struct*>	m_DownloadBlocks_list;

	// download speed calculation
	float		kBpsDown;
	uint32		msReceivedPrev;
	uint32		bytesReceivedCycle;
	// chat
	wxString	m_strComment;
	uint8 		m_byChatstate;
	uint8		m_nChatCaptchaState;
	uint8		m_cCaptchasSent;
	int8		m_iRating;
	uint8		m_cMessagesReceived;		// count of chatmessages he sent to me
	uint8		m_cMessagesSent;			// count of chatmessages I sent to him
	wxString	m_strCaptchaChallenge;
	wxString	m_strCaptchaPendingMsg;

	unsigned int
		m_fHashsetRequesting : 1, // we have sent a hashset request to this client
		m_fNoViewSharedFiles : 1, // client has disabled the 'View Shared Files' feature, 
					  // if this flag is not set, we just know that we don't know 
					  // for sure if it is enabled
		m_fSupportsPreview   : 1,
		m_fIsSpammer	     : 1,
		m_fSentCancelTransfer: 1, // we have sent an OP_CANCELTRANSFER in the current connection
		m_fSharedDirectories : 1, // client supports OP_ASKSHAREDIRS opcodes
		m_fSupportsAICH      : 3,
		m_fAICHRequested     : 1,
		m_fSupportsLargeFiles: 1,
		m_fSentOutOfPartReqs : 1,
		m_fExtMultiPacket    : 1,
		m_fRequestsCryptLayer: 1,
		m_fSupportsCryptLayer: 1,
		m_fRequiresCryptLayer: 1,
		m_fSupportsSourceEx2 : 1,
		m_fSupportsCaptcha   : 1,
		m_fDirectUDPCallback : 1;

	unsigned int
		m_fOsInfoSupport : 1,
		m_fValueBasedTypeTags : 1;

	/* Razor 1a - Modif by MikaelB */

	bool		m_bHelloAnswerPending;

	//! This vector contains the avilability of parts for the file we requested 
	//! from this user. When changing it, be sure to call CPartFile::UpdatePartsFrequency
	//! so that the files know the actual availability of parts.
	BitVector	m_downPartStatus;

	CAICHHash* 	m_pReqFileAICHHash; 

	ESourceFrom	m_nSourceFrom;

	/* Kad Stuff */
	byte		m_achBuddyID[16];
	bool		m_bBuddyIDValid;
	uint32		m_nBuddyIP;
	uint16		m_nBuddyPort;

	EKadState	m_nKadState;	

	uint8		m_byKadVersion;
	uint32		m_dwLastBuddyPingPongTime;
	uint32_t	m_dwDirectCallbackTimeout;

	//! This keeps track of aggressive requests for files. 
	uint16		m_Aggressiveness;
	//! This tracks the time of the last time since a file was requested
	uint32		m_LastFileRequest;

	bool 		m_OSInfo_sent;
	
	wxString	m_clientSoftString;	/* software name */
	wxString	m_clientVerString;	/* version + optional mod name */
	wxString	m_clientVersionString;	/* version string */
	wxString	m_fullClientVerString;	/* full info string */
	wxString	m_sClientOSInfo;
	wxString	m_pendingMessage;

	int		SecIdentSupRec;

	CKnownFile*	m_uploadingfile;

	uint8		m_MaxBlockRequests;

	// needed for stats
	uint32		m_lastClientSoft;
	uint32		m_lastClientVersion;
	wxString	m_lastOSInfo;
	
	/* For buddies timeout */
	uint32		m_nCreationTime;
	
	/* Calculation of last average speed */
	uint32		m_lastaverage;
	uint32		m_last_block_start;
	
	/* Save the encryption status for display when disconnected */
	bool		m_hasbeenobfuscatinglately;
	
	/* Kry - Debug thing. Clients created just to check their data
	   have this string set to the reason we want to check them. 
	   Obviously, once checked, we disconnect them. Take that, sucker.
	   This debug code is just for me I'm afraid. */
#ifdef __DEBUG__
	wxString	connection_reason;
#endif
};


#define	MAKE_CLIENT_VERSION(mjr, min, upd) \
	((uint32)(mjr)*100U*10U*100U + (uint32)(min)*100U*10U + (uint32)(upd)*100U)


#endif // UPDOWNCLIENT_H
// File_checked_for_headers

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

#ifndef UPDOWNCLIENT_H
#define UPDOWNCLIENT_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString
#include <wx/dcmemory.h>	// Needed for wxMemoryDC
#include <wx/gdicmn.h>		// Needed for wxRect
#include "types.h"		// Needed for int8, int16, uint8, uint16, uint32 and uint64
#include "CString.h"		// Needed for CString
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "GetTickCount.h"	// Needed for GetTickCount

class CPartFile;
class CClientReqSocket;
class CClientCredits;
class Packet;
class CFriend;
class Requested_Block_Struct;
class CKnownFile;
class Pending_Block_Struct;
class CSafeMemFile;
class CMemFile;
class CBarShader;
class Requested_File_Struct;
class TransferredData;

// uploadstate
#define	US_UPLOADING			0
#define	US_ONUPLOADQUEUE		1
#define	US_WAITCALLBACK			2
#define	US_CONNECTING			3
#define	US_PENDING			4
#define	US_LOWTOLOWIP			5
#define US_BANNED			6
#define US_ERROR			7
#define US_NONE				8

// downloadstate
#define	DS_DOWNLOADING			0
#define	DS_ONQUEUE			1
#define	DS_CONNECTED			2
#define	DS_CONNECTING			3
#define	DS_WAITCALLBACK			4
#define	DS_REQHASHSET			5
#define	DS_NONEEDEDPARTS		6
#define	DS_TOOMANYCONNS			7
#define	DS_LOWTOLOWIP			8
#define DS_BANNED			9
#define DS_ERROR			10
#define	DS_NONE				11

// m_byChatstate
#define	MS_NONE				0
#define	MS_CHATTING			1
#define	MS_CONNECTING			2
#define	MS_UNABLETOCONNECT		3

#if 0
// clientsoft
#define SO_EMULE			0
#define SO_CDONKEY			1
#define SO_LXMULE			2
#define SO_AMULE			3
#define	SO_SHAREAZA			4
#define SO_EDONKEYHYBRID		50
#define	SO_EDONKEY			51
#define SO_MLDONKEY			52
#define SO_NEW_MLDONKEY			152
#define SO_OLDEMULE			53
#define SO_UNKNOWN			54
#endif

enum EClientSoftware{
	SO_EMULE			= 0,
	SO_CDONKEY			= 1,
	SO_LXMULE			= 2,
	SO_AMULE			= 3,
	SO_SHAREAZA			= 4,
	SO_EDONKEYHYBRID	= 50,
	SO_EDONKEY			= 51,
	SO_MLDONKEY			= 52,
	SO_OLDEMULE			= 53,
	SO_UNKNOWN			= 54,
	SO_NEW_MLDONKEY		= 152
};

enum ESecureIdentState{
	IS_UNAVAILABLE		= 0,
	IS_ALLREQUESTSSEND  = 0,
	IS_SIGNATURENEEDED	= 1,
	IS_KEYANDSIGNEEDED	= 2,
};
enum EInfoPacketState{
	IP_NONE				= 0,
	IP_EDONKEYPROTPACK  = 1,
	IP_EMULEPROTPACK	= 2,
	IP_BOTH				= 3,
};

#define DOWNLOADRATE_FILTERED  // see note in CUpDownClient::CalculateDownloadRate if you undefine this


struct PartFileStamp {
	CPartFile*			file;
	DWORD				timestamp;
};

class CUpDownClient {
	friend class CUploadQueue;
public:
	//base
	CUpDownClient(CClientReqSocket* sender = 0);
	CUpDownClient(uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport,CPartFile* in_reqfile);
	~CUpDownClient();
	void			Destroy();
	void			Disconnected();
	bool			TryToConnect(bool bIgnoreMaxCon = false);
	void			ConnectionEstablished();
	uint32			GetUserID()					{return m_nUserID;}
	void			SetUserID(uint32 nUserID)	{m_nUserID = nUserID;}	
	char*			GetUserName()				{return m_pszUsername;}
	uint32			GetIP()						{return m_dwUserIP;}
	bool			HasLowID()					{return (m_nUserID < 16777216);}
	char*			GetFullIP()					{return m_szFullUserIP;}
	uint32			GetUserPort()				{return m_nUserPort;}
	uint32			GetTransferedUp()			{return m_nTransferedUp;}
	uint32			GetTransferedDown()			{return m_nTransferedDown;}
	uint32			GetServerIP()				{return m_dwServerIP;}
	void			SetServerIP(uint32 nIP)		{m_dwServerIP = nIP;}
	uint16			GetServerPort()				{return m_nServerPort;}
	void			SetServerPort(uint16 nPort)	{m_nServerPort = nPort;}	
	unsigned char*		GetUserHash()				{return (unsigned char*)m_achUserHash;}
	void			SetUserHash(unsigned char* achUserHash)		{if(achUserHash) memcpy(m_achUserHash,achUserHash,16); else memset(m_achUserHash,0,16); }
	bool			HasValidHash()				{return ((int*)m_achUserHash)[0] != 0 || ((int*)m_achUserHash)[1] != 0 ||
												        ((int*)m_achUserHash)[2] != 0 || ((int*)m_achUserHash)[3] != 0; }
	uint32			GetVersion()				{return m_nClientVersion;}
	uint8			GetMuleVersion()			{return m_byEmuleVersion;}
	bool			ExtProtocolAvailable()		{return m_bEmuleProtocol;}
	bool			IsEmuleClient()				{return m_byEmuleVersion;}
	CClientCredits* Credits()					{return credits;}
	bool			IsBanned()					{return (m_bBanned && m_nDownloadState != DS_DOWNLOADING);}
	char*			GetClientFilename()			{return m_pszClientFilename;}
	bool			SupportsUDP()				{return m_byUDPVer != 0 && m_nUDPPort != 0;}
	uint16			GetUDPPort()				{return m_nUDPPort;}
	void			SetUDPPort(uint16 nPort)	{ m_nUDPPort = nPort; }
	uint8			GetUDPVersion()				{return m_byUDPVer;}
	uint8			GetExtendedRequestsVersion(){return m_byExtendedRequestsVer;}
	bool			IsFriend()					{return m_Friend != NULL;}
	float			GetCompression()	{return (float)compressiongain/notcompressed*100.0f;} // Add rod show compression
	void			ResetCompressionGain() {compressiongain = 0; notcompressed=1;} // Add show compression

	void			RequestSharedFileList();
	void			ProcessSharedFileList(char* pachPacket, uint32 nSize, LPCTSTR pszDirectory = NULL);
	
        wxString                 GetUploadFileInfo();

	void			SetUserName(char* pszNewName);
	uint8			GetClientSoft()				{return m_clientSoft;}
	void			ReGetClientSoft();
	void			ProcessHelloAnswer(char* pachPacket, uint32 nSize);
	void			ProcessHelloPacket(char* pachPacket, uint32 nSize);
	void			SendHelloAnswer();
	void			SendHelloPacket();
	void			SendMuleInfoPacket(bool bAnswer);
	void			ProcessMuleInfoPacket(char* pachPacket, uint32 nSize);
	void			ProcessMuleCommentPacket(char* pachPacket, uint32 nSize);
	bool			Compare(CUpDownClient* tocomp, bool bIgnoreUserhash = false);	
	void			SetLastSrcReqTime()			{m_dwLastSourceRequest = ::GetTickCount();}
	void			SetLastSrcAnswerTime()		{m_dwLastSourceAnswer = ::GetTickCount();}
	void			SetLastAskedForSources()	{m_dwLastAskedForSources = ::GetTickCount();}
	uint32		GetLastSrcReqTime()			{return m_dwLastSourceRequest;}
	uint32		GetLastSrcAnswerTime()		{return m_dwLastSourceAnswer;}
	uint32		GetLastAskedForSources()	{return m_dwLastAskedForSources;}	
	DWORD			GetEnteredConnectedState()	{return m_dwEnteredConnectedState;}
	bool			GetFriendSlot()				{return m_bFriendSlot;}
	void			SetFriendSlot(bool bNV)		{m_bFriendSlot = bNV;}
	void			SetCommentDirty(bool bDirty = true) {m_bCommentDirty = bDirty;}
	uint8			GetSourceExchangeVersion()	{return m_bySourceExchangeVer;}
	bool			SafeSendPacket(Packet* packet);
	
	void			SendPublicKeyPacket();
	void			SendSignaturePacket();
	void			ProcessPublicKeyPacket(uchar* pachPacket, uint32 nSize);
	void			ProcessSignaturePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetSecureIdentState()		{return m_SecureIdentState;}
	void			SendSecIdentStatePacket();
	void			ProcessSecIdentStatePacket(uchar* pachPacket, uint32 nSize);

	void			InfoPacketsReceived();

	CClientReqSocket*	socket;
	CClientCredits*		credits;
	CFriend*			m_Friend;

	//upload
	uint32			compressiongain; // Add show compression
	uint32  		notcompressed; // Add show compression
	uint8			GetUploadState()			{return m_byUploadState;}
	void			SetUploadState(uint8 news)	{m_byUploadState = news;}
	uint32			GetWaitStartTime()			{return m_dwWaitTime;}
	uint32			GetWaitTime()				{return m_dwUploadTime-m_dwWaitTime;}
	bool			IsDownloading()				{return (m_byUploadState == US_UPLOADING);}
	bool			HasBlocks()					{return !(m_BlockSend_queue.IsEmpty() && m_BlockRequests_queue.IsEmpty());}
	float			GetKBpsUp()					{return kBpsUp;}
	uint32			GetScore(bool sysvalue, bool isdownloading = false, bool onlybasevalue = false);
	void			AddReqBlock(Requested_Block_Struct* reqblock);
	bool			CreateNextBlockPackage();
	void			SetUpStartTime(uint32 dwTime = 0);
	uint32			GetUpStartTimeDelay()		{return ::GetTickCount() - m_dwUploadTime;}
	void			SetWaitStartTime(uint32 dwTime = 0);
	void			SendHashsetPacket(char* forfileid);
	void			SetUploadFileID(unsigned char* tempreqfileid);
	unsigned char*			GetUploadFileID()       {return requpfileid;}
	CPartFile*		GetDownloadFile()	{return reqfile;}
	uint32			SendBlockData(float kBpsToSend);
	void			ClearUploadBlockRequests();
	void			SendRankingInfo();
	void			SendCommentInfo(CKnownFile *file);
	uint32			GetLastAskedDelay();
	void			AddRequestCount(unsigned char* fileid);
	bool 			IsDifferentPartBlock();
	void			UnBan();
	void			Ban();
	bool			m_bAddNextConnect;  // VQB Fix for LowID slots only on connection	
	uint32			GetBanTime()				{return m_dwBanTime;}
	uint32			GetAskedCount()				{return m_cAsked;}
	void			AddAskedCount()				{m_cAsked++;}
	void			SetAskedCount( uint32 m_cInAsked)				{m_cAsked = m_cInAsked;}
	void			FlushSendBlocks();			// call this when you stop upload, or the socket might be not able to send
	void			SetLastUpRequest()			{m_dwLastUpRequest = ::GetTickCount();}
	uint32			GetLastUpRequest()			{return m_dwLastUpRequest;}
	void			UDPFileReasked();
	uint32			GetSessionUp()			{return m_nTransferedUp - m_nCurSessionUp;}
	void			ResetSessionUp()		{m_nCurSessionUp = m_nTransferedUp;} 
	void			ProcessUpFileStatus(char* packet,uint32 size);
	uint16			GetUpPartCount()			{return m_nUpPartCount;}
	void			DrawUpStatusBar(wxMemoryDC* dc, wxRect rect, bool onlygreyrect, bool  bFlat);

	//download
	uint32			GetAskedCountDown()				{return m_cDownAsked;} //<<--
	void			AddAskedCountDown()				{m_cDownAsked++;}
	void			SetAskedCountDown( uint32 m_cInDownAsked)				{m_cDownAsked = m_cInDownAsked;}
	uint8			GetDownloadState()			{return m_nDownloadState;}
	void			SetDownloadState(uint8 byNewState);
	uint32			GetLastAskedTime()			{return m_dwLastAskedTime;}
	inline bool		IsPartAvailable(uint16 iPart)	{return	( (iPart >= m_nPartCount) || (!m_abyPartStatus) )? 0:m_abyPartStatus[iPart];}
	bool			IsUpPartAvailable(uint16 iPart) {return ( (iPart >= m_nUpPartCount) || (!m_abyUpPartStatus) )? 0:m_abyUpPartStatus[iPart];}
	uint8*			GetPartStatus()				{return m_abyPartStatus;}
#ifdef DOWNLOADRATE_FILTERED
	float			GetKBpsDown()				{return kBpsDown;}	// Emilio
	float			CalculateKBpsDown();
#else
	float			GetKBpsDown()				{return m_nDownDatarate/1024.0;}
	uint32			CalculateDownloadRate();  // replaced by CalculateKBpsDown
#endif
	uint16			GetRemoteQueueRank()		{return m_nRemoteQueueRank;}
	void			SetRemoteQueueFull( bool flag )	{m_bRemoteQueueFull = flag;}
	bool			IsRemoteQueueFull()			{return m_bRemoteQueueFull;}
	void			SetRemoteQueueRank(uint16 nr);
	void			DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool onlygreyrect, bool  bFlat);
	void			AskForDownload();
	void			SendStartupLoadReq();
	void			SendFileRequest();
	void			ProcessFileInfo(char* packet,uint32 size);
	void			ProcessFileStatus(char* packet,uint32 size);
	void			ProcessHashSet(char* packet,uint32 size);
	bool			AddRequestForAnotherFile(CPartFile* file);
	void			SendBlockRequests();
	void			ProcessBlockPacket(char* packet, uint32 size, bool packed = false);
	uint16			GetAvailablePartCount();
	bool			SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL);
	void			DontSwapTo(CPartFile* file);
	bool			IsSwapSuspended(CPartFile* file);
	bool			DoSwap(CPartFile* SwapTo, bool anotherfile=false);
	void			UDPReaskACK(uint16 nNewQR);
	void			UDPReaskFNF();
	void			UDPReaskForDownload();
	bool			IsSourceRequestAllowed();
	// -khaos--+++> Download Sessions Stuff Imported from eMule 0.30c (Creteil) BEGIN ...
	void			SetDownStartTime()			{m_dwDownStartTime = ::GetTickCount();}
	uint32			GetDownTimeDifference()			{uint32 myTime = m_dwDownStartTime; m_dwDownStartTime = 0; return ::GetTickCount() - myTime;}
	bool			GetTransferredDownMini()		{return m_bTransferredDownMini;}
	void			SetTransferredDownMini()		{m_bTransferredDownMini=true;}
	void			InitTransferredDownMini()		{m_bTransferredDownMini=false;}
	//
	//	A4AF Stats Stuff:
	//		In CPartFile::Process, I am going to keep a tally of how many clients
	//		in that PF's source list are A4AF for other files.  This tally is worthless
	//		to the PartFile that it belongs to, but when we add all of these counts up for
	//		each PartFile, we will get an accurate count of how many A4AF requests there are
	//		total.  This is for the Found Sources section of the tree.  This is a better, faster
	//		option than looping through the lists for unavailable sources.
	//
	uint16			GetA4AFCount()				{return m_OtherRequests_list.GetCount();}
	// -khaos--+++> Download Sessions Stuff Imported from eMule 0.30c (Creteil) END ...

	uint16			GetUpCompleteSourcesCount()	{return m_nUpCompleteSourcesCount;}
	void			SetUpCompleteSourcesCount(uint16 n)	{m_nUpCompleteSourcesCount= n;}
	
	int				sourcesslot;

	//chat
	uint8			GetChatState()				{return m_byChatstate;}
	void			SetChatState(uint8 nNewS)		{m_byChatstate = nNewS;}

	//File Comment 
	CString			GetFileComment()			{return m_strComment;} 
	void			SetFileComment(char *desc)		{m_strComment.Format("%s",desc);}
	uint8			GetFileRate()				{return m_iRate;}
	void			SetFileRate(int8 iNewRate)		{m_iRate=iNewRate;}

	// Barry - Process zip file as it arrives, don't need to wait until end of block
	int unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, int iRecursion = 0);
	// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN
	void 			ShowDownloadingParts(CString *partsYN);
	void 			UpdateDisplayedInfo(bool force=false);
	int 			GetFileListRequested() { return m_iFileListRequested; }
	void 			SetFileListRequested(int iFileListRequested) { m_iFileListRequested = iFileListRequested; }
	
	void			ResetFileStatusInfo();
	
	CPartFile*		reqfile;
	
private:

	// base
	void	Init();
	void	ProcessHelloTypePacket(CSafeMemFile* data);
	void	SendHelloTypePacket(CMemFile* data);
	bool	m_bIsBotuser;
//	bool	isfriend;
	uint32	m_dwUserIP;
	uint32	m_dwServerIP;
	uint32	m_nUserID;
	int16	m_nUserPort;
	int16	m_nServerPort;
	uint32	m_nClientVersion;
	uint32	m_cSendblock;
	uint8	m_byEmuleVersion;
	uint8	m_byDataCompVer;
	bool	m_bEmuleProtocol;
	char*	m_pszUsername;
	char	m_szFullUserIP[21];
	char	m_achUserHash[16];
	uint16	m_nUDPPort;
	uint8	m_byUDPVer;
	uint8	m_bySourceExchangeVer;
	uint8	m_byAcceptCommentVer;
	uint8	m_byExtendedRequestsVer;
	uint8	m_cFailed;
	uint8	m_clientSoft;
	uint32	m_dwLastSourceRequest;
	uint32	m_dwLastSourceAnswer;
	uint32	m_dwLastAskedForSources;
	int	m_iFileListRequested;
	bool	m_bFriendSlot;
	bool	m_bCommentDirty;
	bool	m_bIsHybrid;
	bool	m_bIsNewMLD;
	bool	m_bIsML;
	bool	m_bGPLEvildoer;
 	bool	m_bSupportsPreview;
 	bool	m_bPreviewReqPending;
 	bool	m_bPreviewAnsPending;	
 	
	// Kry - Secure Hash import
	ESecureIdentState	m_SecureIdentState; 
	uint8	m_byInfopacketsReceived;			// have we received the edonkeyprot and emuleprot packet already (see InfoPacketsReceived() )
	uint32	m_dwLastSignatureIP;
	uint8	m_bySupportSecIdent;
	
	uint32	m_byCompatibleClient;
	CTypedPtrList<CPtrList, Packet*>				m_WaitingPackets_list;
	CList<PartFileStamp, PartFileStamp>				m_DontSwap_list;
	DWORD	m_lastRefreshedDLDisplay;

	//upload
	void CreateStandartPackets(unsigned char* data,uint32 togo, Requested_Block_Struct* currentblock);
	void CreatePackedPackets(unsigned char* data,uint32 togo, Requested_Block_Struct* currentblock);
	float		kBpsUp;
	uint32		msSentPrev;
	bool		m_bBanned;
	uint32		m_nTransferedUp;
	uint8		m_byUploadState;
	uint32		m_dwWaitTime;
	uint32		m_dwUploadTime;
	uint32		m_nMaxSendAllowed;
	uint32		m_cAsked;
	uint32		m_dwLastUpRequest;
	uint32		m_dwBanTime;
	bool		m_bUsedComprUp;	//only used for interface output
	uint32		m_nCurSessionUp;
	uint16		m_nUpPartCount;
	static		CBarShader s_UpStatusBar;
	unsigned char		requpfileid[16];
	uint16		m_nUpCompleteSourcesCount;
	public:
	uint8*		m_abyUpPartStatus;
	uint16		m_lastPartAsked;
	DWORD		m_dwEnteredConnectedState;

	private:
	CTypedPtrList<CPtrList, Packet*>				 m_BlockSend_queue;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_BlockRequests_queue;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DoneBlocks_list;
	CTypedPtrList<CPtrList, Requested_File_Struct*>	 m_RequestedFiles_list;
	//download
	bool		m_bRemoteQueueFull;
	bool		usedcompressiondown; //only used for interface output
	uint8		m_nDownloadState;
	uint16		m_nPartCount;
	uint32		m_cDownAsked;
	uint8*		m_abyPartStatus;
	uint32		m_dwLastAskedTime;
	char*		m_pszClientFilename;
	uint32		m_nTransferedDown;
	// -khaos--+++> Download Session Stats Imported from eMule 0.30c (Creteil) BEGIN ...
	bool		m_bTransferredDownMini;
	uint32		m_dwDownStartTime;
	// -khaos--+++> Download Session Stats Imported from eMule 0.30c (Creteil) END ...
	uint32      m_nLastBlockOffset;   // Patch for show parts that you download [Cax2]
	uint16		m_cShowDR;
	uint32		m_dwLastBlockReceived;
	uint16		m_nRemoteQueueRank;
	bool		m_bCompleteSource;
	bool		m_bReaskPending;
	bool		m_bUDPPending;
	bool		m_bHashsetRequested;

	CTypedPtrList<CPtrList,Pending_Block_Struct*>	m_PendingBlocks_list;
	CTypedPtrList<CPtrList,Requested_Block_Struct*>	m_DownloadBlocks_list;
#ifdef DOWNLOADRATE_FILTERED
// Emilio: simplified download rate calculation
	float		kBpsDown;
	float		fDownAvgFilter;
	uint32		msReceivedPrev;
	uint32		bytesReceivedCycle;
#else
	uint32		m_nDownDatarate;
	uint32		m_nDownDataRateMS;
	uint32		m_nAvDownDatarate;  // unused
	uint32		m_nSumForAvgDownDataRate;
	CList<TransferredData>					m_AvarageDDR_list;
	sint32	sumavgDDR;	// unused
	sint32	sumavgUDR;	// unused
#endif
	static CBarShader s_StatusBar;
	// chat
	uint8 m_byChatstate;
	CString m_strComment;
	int8 m_iRate;
	unsigned int m_fHashsetRequesting : 1, // we have sent a hashset request to this client
	m_fSharedDirectories : 1; // client supports OP_ASKSHAREDIRS opcodes

	/* Razor 1a - Modif by MikaelB */

public:

	/* m_OtherRequests_list --> public instead of private */
	CTypedPtrList<CPtrList, CPartFile*>	m_OtherRequests_list;
	/* Same for m_OtherNoNeeded_list --> public instead of private (Creteil) */
	CTypedPtrList<CPtrList, CPartFile*>	m_OtherNoNeeded_list;

	/* IsValidSource function
	 * @return true id it's valid source
	*/
	bool IsValidSource() const
	{
		return m_ValidSource;
	};

	/* SetValidSource function
	 * @param boolean - set valid source
	*/
	void SetValidSource(bool in)
	{
		m_ValidSource = in;
	};

	/* SwapToThisFile function
	 * @param CPartFile* - the file
	*/
	void SwapToThisFile(CPartFile* file);

private:

	/* valid source attribute */
	bool m_ValidSource;

	/* End modif */

	// Support for tag ET_MOD_VERSION [BlackRat]
public:

	const wxString	GetClientModString() const { return m_clientModString; }
	const wxString	GetClientVerString() const { return m_clientVerString; }

private:

	wxString		m_clientModString; 
	wxString		m_clientVerString;
	
	// Hash anti-thief from HoaX_69 [BlackRat]
public:

	bool			thief;  // is a thief ?
	uint64	getUID() 
	{
		uint64 ip = ((uint64) m_dwUserIP) << 32;
		uint64 port = ((uint64) m_nUserPort) << 16;
		uint64 uid = ip + port;
		return uid;
	}
	//return ((uint64)m_dwUserIP<<32)+((uint64)m_nUserPort<<16); }
	int	leechertype; // what kind of leecher is it ?
  
};

#endif // UPDOWNCLIENT_H

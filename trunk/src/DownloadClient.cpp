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



#include <zlib.h>
#include <cmath>			// Needed for std:exp

#include "ClientCredits.h"	// Needed for CClientCredits
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "otherfunctions.h"	// Needed for md4cmp
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "sockets.h"		// Needed for CServerConnect
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "otherstructs.h"	// Needed for Requested_Block_Struct
#include "Preferences.h"	// Needed for CPreferences
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "ListenSocket.h"	// Needed for CListenSocket
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "BarShader.h"		// Needed for CBarShader
#include "updownclient.h"	// Needed for CUpDownClient
#include "otherfunctions.h" // md4hash


// members of CUpDownClient
// which are mainly used for downloading functions

void CUpDownClient::DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool onlygreyrect, bool  bFlat)
{
	static CBarShader s_StatusBar(16);

	// 0.42e
	DWORD crBoth;
	DWORD crNeither;
	DWORD crClientOnly;
	DWORD crPending;
	DWORD crNextPending;

	if(bFlat) {
		crBoth = RGB(0, 150, 0);
		crNeither = RGB(224, 224, 224);
		crClientOnly = RGB(0, 0, 0);
		crPending = RGB(255,208,0);
		crNextPending = RGB(255,255,100);
	} else {
		crBoth = RGB(0, 192, 0);
		crNeither = RGB(240, 240, 240);
		crClientOnly = RGB(104, 104, 104);
		crPending = RGB(255, 208, 0);
		crNextPending = RGB(255,255,100);
	} 

	s_StatusBar.SetFileSize(m_reqfile->GetFileSize()); 
	s_StatusBar.SetHeight(rect.height - rect.y);
	s_StatusBar.SetWidth(rect.width - rect.x);
	s_StatusBar.Fill(crNeither);
	s_StatusBar.Set3dDepth( theApp.glob_prefs->Get3DDepth() );

	// Barry - was only showing one part from client, even when reserved bits from 2 parts
	wxString gettingParts = ShowDownloadingParts();

	if (!onlygreyrect && m_reqfile && m_abyPartStatus) {
		for (uint32 i = 0;i != m_nPartCount;i++) {
			if (m_abyPartStatus[i]) {
				uint32 uEnd;
				if (PARTSIZE*(i+1) > m_reqfile->GetFileSize()) {
					uEnd = m_reqfile->GetFileSize();
				} else {
					uEnd = PARTSIZE*(i+1);
				}
				if (m_reqfile->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1)) {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crBoth);
				} else if (m_nDownloadState == DS_DOWNLOADING && m_nLastBlockOffset < uEnd && m_nLastBlockOffset >= PARTSIZE*i) {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crPending);
				} else if (gettingParts.GetChar((uint16)i) == 'Y') {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crNextPending);
				} else {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crClientOnly);
				}
			}
		}
	}
	s_StatusBar.Draw(dc, rect.x, rect.y, bFlat);
}

bool CUpDownClient::Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash){
	if(!bIgnoreUserhash && HasValidHash() && tocomp->HasValidHash())
	    return (this->GetUserHash() == tocomp->GetUserHash());
	if (HasLowID())
		return ((this->GetUserID() == tocomp->GetUserID()) && (GetServerIP() == tocomp->GetServerIP()) && (this->GetUserPort() == tocomp->GetUserPort()));
	else
		return ((this->GetUserID() == tocomp->GetUserID() && this->GetUserPort() == tocomp->GetUserPort()) || (this->GetIP() && (this->GetIP() == tocomp->GetIP() && this->GetUserPort() == tocomp->GetUserPort())) );
}


bool CUpDownClient::AskForDownload()
{
	// 0.42e
	if (theApp.listensocket->TooManySockets()) {
		if (!m_socket) {
			if (GetDownloadState() != DS_TOOMANYCONNS) {
				SetDownloadState(DS_TOOMANYCONNS);
			}
			return true;
		} else if (!m_socket->IsConnected()) {
			if (GetDownloadState() != DS_TOOMANYCONNS) {
				SetDownloadState(DS_TOOMANYCONNS);
			}
			return true;
		}
	}
	m_bUDPPending = false;
	m_dwLastAskedTime = ::GetTickCount();
	SetDownloadState(DS_CONNECTING);
	return TryToConnect();
}


void CUpDownClient::SendStartupLoadReq()
{
	// 0.42e
	if (m_socket==NULL || m_reqfile==NULL) {
		return;
	}
	SetDownloadState(DS_ONQUEUE);
	CSafeMemFile dataStartupLoadReq(16);
	dataStartupLoadReq.WriteHash16(m_reqfile->GetFileHash());
	Packet* packet = new Packet(&dataStartupLoadReq);
	packet->SetOpCode(OP_STARTUPLOADREQ);
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
	SendPacket(packet, true, true);
}


bool CUpDownClient::IsSourceRequestAllowed()
{
	// 0.42e
	DWORD dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
	uint32 nTimePassedClient = dwTickCount - GetLastSrcAnswerTime();
	uint32 nTimePassedFile   = dwTickCount - m_reqfile->GetLastAnsweredTime();
	bool bNeverAskedBefore = (GetLastAskedForSources() == 0);

	UINT uSources = m_reqfile->GetSourceCount();
	return (
		// if client has the correct extended protocol
		ExtProtocolAvailable() && m_bySourceExchangeVer >= 1 &&
		// AND if we need more sources
		theApp.glob_prefs->GetMaxSourcePerFileSoft() > uSources &&
		// AND if...
		(
		//source is not complete and file is very rare
			( !m_bCompleteSource
			&& (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS)
			&& (uSources <= RARE_FILE/5)
			) ||
			//source is not complete and file is rare
			( !m_bCompleteSource
			&& (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS)
			&& (uSources <= RARE_FILE || uSources - m_reqfile->GetValidSourcesCount() <= RARE_FILE / 2)
			&& (nTimePassedFile > SOURCECLIENTREASKF)
			) ||
			// OR if file is not rare
			( (bNeverAskedBefore || nTimePassedClient > (unsigned)(SOURCECLIENTREASKS * MINCOMMONPENALTY)) 
			&& (nTimePassedFile > (unsigned)(SOURCECLIENTREASKF * MINCOMMONPENALTY))
			)
		)
	);
}


void CUpDownClient::SendFileRequest()
{
	// 0.42e
	wxASSERT(m_reqfile != NULL);
	
	if(!m_reqfile) {
		return;
	}
	
	AddAskedCountDown();

	CSafeMemFile dataFileReq(16+16);
	dataFileReq.WriteHash16(m_reqfile->GetFileHash());

	if( SupportMultiPacket() ) {
		dataFileReq.Write((uint8)OP_REQUESTFILENAME);
		//Extended information
		if( GetExtendedRequestsVersion() > 0 ) {
			m_reqfile->WritePartStatus(&dataFileReq);
		}
		if( GetExtendedRequestsVersion() > 1 ) {
			m_reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		if (m_reqfile->GetPartCount() > 1) {
			dataFileReq.Write((uint8)OP_SETREQFILEID);
		}
		if( IsEmuleClient() ) {
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}
		if(IsSourceRequestAllowed())	{
			dataFileReq.Write((uint8)OP_REQUESTSOURCES);
			m_reqfile->SetLastAnsweredTimeTimeout();
			SetLastAskedForSources();
			#ifdef __USE_DEBUG__
			if (thePrefs.GetDebugSourceExchange()) 
				AddDebugLogLineF( false, "Send:Source Request User(%s) File(%s)", unicode2char(GetUserName()), m_reqfile->GetFileName() );
			#endif
		}
		#ifdef __USE_DEBUG__
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__MultiPacket", this, (char*)m_reqfile->GetFileHash());
		#endif
		Packet* packet = new Packet(&dataFileReq, OP_EMULEPROT);
		packet->SetOpCode(OP_MULTIPACKET);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
		SendPacket(packet, true);
	} else {
		//This is extended information
		if( GetExtendedRequestsVersion() > 0 ){
			m_reqfile->WritePartStatus(&dataFileReq);
		}
		if( GetExtendedRequestsVersion() > 1 ){
			m_reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		#ifdef __USE_DEBUG__
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__FileRequest", this, (char*)m_reqfile->GetFileHash());
		#endif
		Packet* packet = new Packet(&dataFileReq);
		packet->SetOpCode(OP_REQUESTFILENAME);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
		SendPacket(packet, true);
	
		// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
		// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
		// know that the file is shared, we know also that the file is complete and don't need to request the file status.
		if (m_reqfile->GetPartCount() > 1) {
			#ifdef __USE_DEBUG__
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				DebugSend("OP__SetReqFileID", this, (char*)m_reqfile->GetFileHash());
			#endif
			CSafeMemFile dataSetReqFileID(16);
			dataSetReqFileID.WriteRaw(m_reqfile->GetFileHash(),16);
			packet = new Packet(&dataSetReqFileID);
			packet->SetOpCode(OP_SETREQFILEID);
			theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
			SendPacket(packet, true);
		}
	
		if( IsEmuleClient() ) {
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}	
		if(IsSourceRequestAllowed()) {
			#ifdef __USE_DEBUG__
		    if (thePrefs.GetDebugClientTCPLevel() > 0){
			    DebugSend("OP__RequestSources", this, (char*)m_reqfile->GetFileHash());
			    if (GetLastAskedForSources() == 0)
				    Debug("  first source request\n");
			    else
				    Debug("  last source request was before %s\n", CastSecondsToHM((GetTickCount() - GetLastAskedForSources())/1000));
		    }
		    #endif
			m_reqfile->SetLastAnsweredTimeTimeout();
			Packet* packet = new Packet(OP_REQUESTSOURCES,16,OP_EMULEPROT);
			packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
			theApp.uploadqueue->AddUpDataOverheadSourceExchange(packet->GetPacketSize());
			SendPacket(packet,true,true);
			SetLastAskedForSources();
		    #ifdef __USE_DEBUG__
			if (thePrefs.GetDebugSourceExchange())
				AddDebugLogLineF( false, "Send:Source Request User(%s) File(%s)", unicode2char(GetUserName()), m_reqfile->GetFileName() );
			#endif
		}
	}
}

void CUpDownClient::ProcessFileInfo(const CSafeMemFile* data, const CPartFile* file)
{
	// 0.42e
	if (file==NULL) {
		throw wxString(_("ERROR: Wrong file ID (ProcessFileInfo; file==NULL)"));
	}
	if (m_reqfile==NULL) {
		throw wxString(_("ERROR: Wrong file ID (ProcessFileInfo; m_reqfile==NULL)"));
	}
	if (file != m_reqfile) {
		throw wxString(_("ERROR: Wrong file ID (ProcessFileInfo; m_reqfile!=file)"));
	}	

	data->Read(ClientFilename);
			
	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (m_reqfile->GetPartCount() == 1)
	{
		if (m_abyPartStatus)
		{
			delete[] m_abyPartStatus;
			m_abyPartStatus = NULL;
		}
		m_nPartCount = m_reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		memset(m_abyPartStatus,1,m_nPartCount);
		m_bCompleteSource = true;

		#if __USE_DEBUG__
		if (thePrefs.GetDebugClientTCPLevel() > 0)
		{
		    int iNeeded = 0;
		    for (int i = 0; i < m_nPartCount; i++)
			    if (!m_reqfile->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1))
				    iNeeded++;
			char* psz = new char[m_nPartCount + 1];
			for (int i = 0; i < m_nPartCount; i++)
				psz[i] = m_abyPartStatus[i] ? '#' : '.';
			psz[i] = '\0';
			Debug("  Parts=%u  %s  Needed=%u\n", m_nPartCount, psz, iNeeded);
			delete[] psz;
		}
		#endif
		UpdateDisplayedInfo();
		m_reqfile->UpdateAvailablePartsCount();
		// even if the file is <= PARTSIZE, we _may_ need the hashset for that file (if the file size == PARTSIZE)
		if (m_reqfile->hashsetneeded)
		{
			if (m_socket)
			{
				Packet* packet = new Packet(OP_HASHSETREQUEST,16);
				packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
				theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet,true,true);
				SetDownloadState(DS_REQHASHSET);
				m_fHashsetRequesting = 1;
				m_reqfile->hashsetneeded = false;
			}
			else {
				wxASSERT(0);
			}
		}
		else {
			SendStartupLoadReq();
		}
		m_reqfile->UpdatePartsInfo();
	}
}

void CUpDownClient::ProcessFileStatus(bool bUdpPacket, const CSafeMemFile* data, const CPartFile* file)
{
	// 0.42e
	
	if ( !m_reqfile || file != m_reqfile ){
		if (m_reqfile==NULL) {
			throw wxString(_("ERROR: Wrong file ID (ProcessFileStatus; m_reqfile==NULL)"));
		}
		throw wxString(_("ERROR: Wrong file ID (ProcessFileStatus; m_reqfile!=file)"));
	}
	
	uint16 nED2KPartCount;
	data->Read(nED2KPartCount);
	if (m_abyPartStatus)
	{
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	bool bPartsNeeded = false;
	int iNeeded = 0;
	if (!nED2KPartCount)
	{
		m_nPartCount = m_reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		memset(m_abyPartStatus,1,m_nPartCount);
		bPartsNeeded = true;
		m_bCompleteSource = true;

		#if __USE_DEBUG__
		if (bUdpPacket ? (thePrefs.GetDebugClientUDPLevel() > 0) : (thePrefs.GetDebugClientTCPLevel() > 0)) 		{
			for (int i = 0; i < m_nPartCount; i++) {
				if (!m_reqfile->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)) {
					iNeeded++;
				}
			}
		}
		#endif
	}
	else
	{
		if (m_reqfile->GetED2KPartCount() != nED2KPartCount)
		{
			wxString strError;
			strError.Printf(_("ProcessFileStatus - wrong part number recv=%u  expected=%u  "), nED2KPartCount, m_reqfile->GetED2KPartCount());
			strError +=  EncodeBase16(m_reqfile->GetFileHash(), 16);
			m_nPartCount = 0;
			throw strError;
		}
		m_nPartCount = m_reqfile->GetPartCount();

		m_bCompleteSource = false;
		m_abyPartStatus = new uint8[m_nPartCount];
		uint16 done = 0;
		while (done != m_nPartCount)
		{
			uint8 toread;
			data->Read(toread);
			for (sint32 i = 0;i != 8;i++)
			{
				m_abyPartStatus[done] = ((toread>>i)&1)? 1:0; 	
				if (m_abyPartStatus[done])
				{
					if (!m_reqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)){
						bPartsNeeded = true;
						iNeeded++;
					}
				}
				done++;
				if (done == m_nPartCount) {
					break;
				}
			}
		}
	}
	
	#ifdef __USE_DEBUG__	
	if (bUdpPacket ? (thePrefs.GetDebugClientUDPLevel() > 0) : (thePrefs.GetDebugClientTCPLevel() > 0))
	{
		char* psz = new char[m_nPartCount + 1];
		for (int i = 0; i < m_nPartCount; i++)
			psz[i] = m_abyPartStatus[i] ? '#' : '.';
		psz[i] = '\0';
		Debug("  Parts=%u  %s  Needed=%u\n", m_nPartCount, psz, iNeeded);
		delete[] psz;
	}
	#endif

	UpdateDisplayedInfo();
	m_reqfile->UpdateAvailablePartsCount();

	// NOTE: This function is invoked from TCP and UDP socket!
	if (!bUdpPacket) {
		if (!bPartsNeeded) {
			SetDownloadState(DS_NONEEDEDPARTS);
		} else if (m_reqfile->hashsetneeded) {
			//If we are using the eMule filerequest packets, this is taken care of in the Multipacket!
			if (m_socket) {
				#ifdef __USE_DEBUG__
				if (thePrefs.GetDebugClientTCPLevel() > 0) {
					DebugSend("OP__HashSetRequest", this, (char*)m_reqfile->GetFileHash());
				}
				#endif
				Packet* packet = new Packet(OP_HASHSETREQUEST,16);
				packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
				theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet, true, true);
				SetDownloadState(DS_REQHASHSET);
				m_fHashsetRequesting = 1;
				m_reqfile->hashsetneeded = false;
			} else {
				wxASSERT(0);
			}
		}
		else {
			SendStartupLoadReq();
		}
	}
	else {
		if (!bPartsNeeded) {
			SetDownloadState(DS_NONEEDEDPARTS);
		} else {
			SetDownloadState(DS_ONQUEUE);
		}
	}
	m_reqfile->UpdatePartsInfo();
}

bool CUpDownClient::AddRequestForAnotherFile(CPartFile* file)
{
	if ( m_A4AF_list.find( file ) == m_A4AF_list.end() ) {
		// When we access a non-existing entry entry, it will be zeroed by default, 
		// so we have to set NeededParts. All in one go.
		m_A4AF_list[file].NeededParts = true;
		file->A4AFsrclist.insert( this );
		return true;
	} else {
		return false;
	}
}

bool CUpDownClient::DeleteFileRequest(CPartFile* file)
{
	return m_A4AF_list.erase( file );
}

void CUpDownClient::DeleteAllFileRequests()
{
	m_A4AF_list.clear();
}


/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
void CUpDownClient::SetDownloadState(uint8 byNewState)
{
	if (m_nDownloadState != byNewState) {
		if (m_reqfile) {
			if (byNewState == DS_DOWNLOADING) {
				m_reqfile->AddDownloadingSource(this);
			} else if (m_nDownloadState == DS_DOWNLOADING) {
				m_reqfile->RemoveDownloadingSource(this);
			}
		}
		if (byNewState == DS_CONNECTED && m_nDownloadState != DS_CONNECTED) {
			m_dwEnteredConnectedState = GetTickCount();
		}
		if (m_nDownloadState == DS_DOWNLOADING) {
			#if 0
			// -khaos--+++> Extended Statistics (Successful/Failed Download Sessions)
			if (m_bTransferredDownMini && byNewState != DS_ERROR) {
				// Increment our counters for successful sessions (Cumulative AND Session)
				theApp.glob_prefs->Add2DownSuccessfulSessions();
			} else {
				// Increment our counters failed sessions (Cumulative AND Session)
				theApp.glob_prefs->Add2DownFailedSessions();
			}
			theApp.glob_prefs->Add2DownSAvgTime(GetDownTimeDifference()/1000);
			// <-----khaos-
			#endif

			m_nDownloadState = byNewState;
			for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0; ) {
				Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetNext(pos);
				m_reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
				delete cur_block;
			}
			m_DownloadBlocks_list.RemoveAll();

			for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0; ) {
				Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
				if (m_reqfile) {
					m_reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
				}
				delete pending->block;
				// Not always allocated
				if (pending->zStream) {
					inflateEnd(pending->zStream);
					delete pending->zStream;
				}
				delete pending;
			}
			m_PendingBlocks_list.RemoveAll();
			kBpsDown = 0.0;
			bytesReceivedCycle = 0;
			msReceivedPrev = 0;
			if (byNewState == DS_NONE) {
				if (m_abyPartStatus) {
					delete[] m_abyPartStatus;
					m_abyPartStatus = NULL;
				}
				m_nPartCount = 0;
			}
			if (m_socket && byNewState != DS_ERROR) {
				m_socket->DisableDownloadLimit();
			}
		}
		m_nDownloadState = byNewState;
		if(GetDownloadState() == DS_DOWNLOADING) {
			if (IsEmuleClient()) {
				SetRemoteQueueFull(false);
			}
			SetRemoteQueueRank(0); // eMule 0.30c set like this ...
			SetAskedCountDown(0);
		}
		UpdateDisplayedInfo(true);
	}
}
/* eMule 0.30c implementation, i give it a try (Creteil) END ... */

void CUpDownClient::ProcessHashSet(const char *packet, uint32 size)
{
	if ((!m_reqfile) || md4cmp(packet,m_reqfile->GetFileHash())) {
		throw wxString(wxT("Wrong fileid sent (ProcessHashSet)"));
	}
	if (!m_fHashsetRequesting) {
		throw wxString(wxT("Received unsolicited hashset, ignoring it."));
	}
	CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
	if (m_reqfile->LoadHashsetFromFile(data,true)) {
		m_fHashsetRequesting = 0;
	} else {
		m_reqfile->hashsetneeded = true;
		delete data;	//mf
		throw wxString(wxT("Corrupted or invalid hashset received"));
	}
	delete data;
	SendStartupLoadReq();
}

void CUpDownClient::SendBlockRequests()
{
	m_dwLastBlockReceived = ::GetTickCount();
	if (!m_reqfile) {
		return;
	}
	if (m_DownloadBlocks_list.IsEmpty()) {
		// Barry - instead of getting 3, just get how many is needed
		uint16 count = 3 - m_PendingBlocks_list.GetCount();
		Requested_Block_Struct** toadd = new Requested_Block_Struct*[count];
		if (m_reqfile->GetNextRequestedBlock(this,toadd,&count)) {
			for (int i = 0; i != count; i++) {
				m_DownloadBlocks_list.AddTail(toadd[i]);
			}
		}
		delete[] toadd;
	}

	// Barry - Why are unfinished blocks requested again, not just new ones?

	while (m_PendingBlocks_list.GetCount() < 3 && !m_DownloadBlocks_list.IsEmpty()) {
		Pending_Block_Struct* pblock = new Pending_Block_Struct;
		pblock->block = m_DownloadBlocks_list.RemoveHead();
		pblock->zStream = NULL;
		pblock->totalUnzipped = 0;
		pblock->fZStreamError = 0;
		pblock->fRecovered = 0;
		m_PendingBlocks_list.AddTail(pblock);
	}
	if (m_PendingBlocks_list.IsEmpty()) {
		if (!GetSentCancelTransfer()){
			Packet* packet = new Packet(OP_CANCELTRANSFER,0);
			theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
			SendPacket(packet,true,true);
			SetSentCancelTransfer(1);
		}
		SetDownloadState(DS_NONEEDEDPARTS);
		return;
	}
	#define iPacketSize 16+(3*4)+(3*4) // 40
	Packet* packet = new Packet(OP_REQUESTPARTS,iPacketSize);
	char *tempbuf = new char[iPacketSize];
	CSafeMemFile data((BYTE *)tempbuf, iPacketSize);
	data.WriteHash16(m_reqfile->GetFileHash());
	POSITION pos = m_PendingBlocks_list.GetHeadPosition();

	for (uint32 i = 0; i != 3; i++) {
		if (pos) {
			Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
			wxASSERT( pending->block->StartOffset <= pending->block->EndOffset );
			//ASSERT( pending->zStream == NULL );
			//ASSERT( pending->totalUnzipped == 0 );
			pending->fZStreamError = 0;
			pending->fRecovered = 0;
			data.Write((uint32)pending->block->StartOffset);
		} else {
			data.Write((uint32)0);
		}
	}
	pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++) {
		if (pos) {
			Requested_Block_Struct* block = m_PendingBlocks_list.GetNext(pos)->block;
			uint32 endpos = block->EndOffset+1;
			data.Write(endpos);			
		} else {
			data.Write((uint32)0);
		}
	}

	packet->CopyToDataBuffer(0, tempbuf, iPacketSize);
	delete [] tempbuf;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
	SendPacket(packet, true, true);
}

/*
Barry - Originally this only wrote to disk when a full 180k block
had been received from a client, and only asked for data in
180k blocks.

This meant that on average 90k was lost for every connection
to a client data source. That is a lot of wasted data.

To reduce the lost data, packets are now written to a buffer
and flushed to disk regularly regardless of size downloaded.

This includes compressed packets.

Data is also requested only where gaps are, not in 180k blocks.
The requests will still not exceed 180k, but may be smaller to
fill a gap.
*/

void CUpDownClient::ProcessBlockPacket(const char *packet, uint32 size, bool packed)
{
	try {
		// Ignore if no data required
		if (!(GetDownloadState() == DS_DOWNLOADING || GetDownloadState() == DS_NONEEDEDPARTS)) {
			return;
		}

		const int HEADER_SIZE = 24;

		// Update stats
		m_dwLastBlockReceived = ::GetTickCount();


		// Read data from packet
		const CSafeMemFile *data = new CSafeMemFile((BYTE*)packet, size);
		uchar fileID[16];
		data->ReadRaw(fileID,16);

		// Check that this data is for the correct file
		if ((!m_reqfile) || md4cmp(packet, m_reqfile->GetFileHash())) {
			delete data;
			throw wxString(wxT("Wrong fileid sent (ProcessBlockPacket)"));
		}

		// Find the start & end positions, and size of this chunk of data
		uint32 nStartPos;
		uint32 nEndPos;
		uint32 nBlockSize = 0;
		data->Read(nStartPos);
		if (packed) {
			data->Read(nBlockSize);
			nEndPos = nStartPos + (size - HEADER_SIZE);
			usedcompressiondown = true;
		} else {
			data->Read(nEndPos);
		}
		delete data;

		// Check that packet size matches the declared data size + header size (24)
		if ( nEndPos == nStartPos || size != ((nEndPos - nStartPos) + HEADER_SIZE)) {
			throw wxString(wxT("Corrupted or invalid DataBlock received (ProcessBlockPacket)"));
		}
		// Move end back one, should be inclusive
		theApp.UpdateReceivedBytes(size - HEADER_SIZE);
		bytesReceivedCycle += size - HEADER_SIZE;

		credits->AddDownloaded(size - HEADER_SIZE, GetIP());
		nEndPos--;

		// Loop through to find the reserved block that this is within
		Pending_Block_Struct *cur_block;
		for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL; m_PendingBlocks_list.GetNext(pos)) {
			cur_block = m_PendingBlocks_list.GetAt(pos);
			if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos)) {
				// Found reserved block
				
				if (cur_block->fZStreamError){
					AddDebugLogLineF(false, _("Ignoring %u bytes of block %u-%u because of errornous zstream state for file \"%s\""), size - HEADER_SIZE, nStartPos, nEndPos, m_reqfile->GetFileName().GetData());
					m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
					return;
				}
	    
				// Remember this start pos, used to draw part downloading in list
				m_nLastBlockOffset = nStartPos;

				// Occasionally packets are duplicated, no point writing it twice
				// This will be 0 in these cases, or the length written otherwise
				uint32 lenWritten = 0;

				// Handle differently depending on whether packed or not
				if (!packed) {
					// Write to disk (will be buffered in part file class)
					lenWritten = m_reqfile->WriteToBuffer(size - HEADER_SIZE, 
					(BYTE *) (packet + HEADER_SIZE), nStartPos, nEndPos,
					cur_block->block );
				} else {
					// Packed
					wxASSERT( (int)size > 0 );
					// Create space to store unzipped data, the size is
					// only an initial guess, will be resized in unzip()
					// if not big enough
					uint32 lenUnzipped = (size * 2);
					// Don't get too big
					if (lenUnzipped > (BLOCKSIZE + 300)) {
						lenUnzipped = (BLOCKSIZE + 300);
					}
					BYTE *unzipped = new BYTE[lenUnzipped];

					// Try to unzip the packet
					int result = unzip(cur_block, (BYTE *)(packet + HEADER_SIZE), (size - HEADER_SIZE), &unzipped, &lenUnzipped);
					if (result == Z_OK && ((int)lenUnzipped >= 0)) {
						
						// Write any unzipped data to disk
						if (lenUnzipped > 0) {
							wxASSERT( (int)lenUnzipped > 0 );
							// Use the current start and end
							// positions for the uncompressed data
							nStartPos = cur_block->block->StartOffset + cur_block->totalUnzipped - lenUnzipped;
							nEndPos = cur_block->block->StartOffset + cur_block->totalUnzipped - 1;

							if (nStartPos > cur_block->block->EndOffset || nEndPos > cur_block->block->EndOffset) {
								AddDebugLogLineF(false, _("Corrupted compressed packet for %s received (error %i)"),m_reqfile->GetFileName().GetData(),666);
								m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
							} else {
								// Write uncompressed data to file
								lenWritten = m_reqfile->WriteToBuffer(size - HEADER_SIZE,
								unzipped, nStartPos, nEndPos,
								cur_block->block );
							}
						}
					} else {
						wxString strZipError;
						if (cur_block->zStream && cur_block->zStream->msg) {
							strZipError.Printf(_T(" - %s"), cur_block->zStream->msg);
						} 
						AddDebugLogLineM(false, wxString(_("Corrupted compressed packet for")) + m_reqfile->GetFileName() + wxString::Format(_("received (error %i) ") , result) + strZipError );
						m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);

						// If we had an zstream error, there is no chance that we could recover from it nor that we
						// could use the current zstream (which is in error state) any longer.
						if (cur_block->zStream){
							inflateEnd(cur_block->zStream);
							delete cur_block->zStream;
							cur_block->zStream = NULL;
						}

						// Although we can't further use the current zstream, there is no need to disconnect the sending 
						// client because the next zstream (a series of 10K-blocks which build a 180K-block) could be
						// valid again. Just ignore all further blocks for the current zstream.
						cur_block->fZStreamError = 1;
						cur_block->totalUnzipped = 0; // bluecow's fix
					}
					delete [] unzipped;
				}
				// These checks only need to be done if any data was written
				if (lenWritten > 0) {
					m_nTransferedDown += lenWritten;

					// If finished reserved block
					if (nEndPos == cur_block->block->EndOffset) {
						m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
						delete cur_block->block;
						// Not always allocated
						if (cur_block->zStream) {
					  		inflateEnd(cur_block->zStream);
					 		delete cur_block->zStream;
						}
						delete cur_block;
						m_PendingBlocks_list.RemoveAt(pos);

						// Request next block
						SendBlockRequests();
					}
				}
				// Stop looping and exit method
				return;
			}
		}
 	} catch (...) {
		AddDebugLogLineM(false,wxString::Format(wxT("Unknown exception in %s: file \""), __FUNCTION__) + (m_reqfile ? m_reqfile->GetFileName() : wxT("?")) + wxT("%s\""));
	}
}

int CUpDownClient::unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, int iRecursion)
{
	int err = Z_DATA_ERROR;
	try {
		// Save some typing
		z_stream *zS = block->zStream;

		// Is this the first time this block has been unzipped
		if (zS == NULL) {
			// Create stream
			block->zStream = new z_stream;
			zS = block->zStream;

			// Initialise stream values
			zS->zalloc = (alloc_func)0;
			zS->zfree = (free_func)0;
			zS->opaque = (voidpf)0;

			// Set output data streams, do this here to avoid overwriting on recursive calls
			zS->next_out = (*unzipped);
			zS->avail_out = (*lenUnzipped);

			// Initialise the z_stream
			err = inflateInit(zS);
			if (err != Z_OK) {
				return err;
			}
		}

		// Use whatever input is provided
		zS->next_in  = zipped;
		zS->avail_in = lenZipped;

		// Only set the output if not being called recursively
		if (iRecursion == 0) {
			zS->next_out = (*unzipped);
			zS->avail_out = (*lenUnzipped);
		}

		// Try to unzip the data
		err = inflate(zS, Z_SYNC_FLUSH);

		// Is zip finished reading all currently available input and writing
		// all generated output
		if (err == Z_STREAM_END) {
			// Finish up
			err = inflateEnd(zS);
			if (err != Z_OK) {
				return err;
			}

			// Got a good result, set the size to the amount unzipped in this call
			//  (including all recursive calls)
			(*lenUnzipped) = (zS->total_out - block->totalUnzipped);
			block->totalUnzipped = zS->total_out;
		} else if ((err == Z_OK) && (zS->avail_out == 0) && (zS->avail_in != 0)) {
			// Output array was not big enough,
			// call recursively until there is enough space

			// What size should we try next
			uint32 newLength = (*lenUnzipped) *= 2;
			if (newLength == 0) {
				newLength = lenZipped * 2;
			}
			// Copy any data that was successfully unzipped to new array
			BYTE *temp = new BYTE[newLength];
			assert( zS->total_out - block->totalUnzipped <= newLength );
			memcpy(temp, (*unzipped), (zS->total_out - block->totalUnzipped));
			delete [] (*unzipped);
			(*unzipped) = temp;
			(*lenUnzipped) = newLength;

			// Position stream output to correct place in new array
			zS->next_out = (*unzipped) + (zS->total_out - block->totalUnzipped);
			zS->avail_out = (*lenUnzipped) - (zS->total_out - block->totalUnzipped);

			// Try again
			err = unzip(block, zS->next_in, zS->avail_in, unzipped, lenUnzipped, iRecursion + 1);
		} else if ((err == Z_OK) && (zS->avail_in == 0)) {
			// All available input has been processed, everything ok.
			// Set the size to the amount unzipped in this call
			// (including all recursive calls)
			(*lenUnzipped) = (zS->total_out - block->totalUnzipped);
			block->totalUnzipped = zS->total_out;
		} else {
			// Should not get here unless input data is corrupt
			wxString strZipError;
			if (zS->msg)
				strZipError.Printf(_T(" %d '%s'"), err, zS->msg);
			else if (err != Z_OK)
				strZipError.Printf(_T(" %d"), err);
			AddDebugLogLineM(false, wxString::Format(wxT("Unexpected zip error ")) +  strZipError + wxT("in file \"") + (m_reqfile ? m_reqfile->GetFileName() : wxT("?")) + wxT("\""));
		}

		if (err != Z_OK) {
			(*lenUnzipped) = 0;
		}
	} catch (...) {
		AddDebugLogLineM(false, wxString::Format(wxT("Unknown exception in %s: file \""), __FUNCTION__) + (m_reqfile ? m_reqfile->GetFileName() : wxT("?")) + wxT("\""));
		err = Z_DATA_ERROR;
	}
	return err;
}


// Emilio: rewrite of eMule code to eliminate use of lists for averaging and fix
// errors in calculation (32-bit rollover and time measurement)  This function 
// uses a first-order filter with variable time constant (initially very short 
// to quickly reach the right value without spiking, then gradually approaching 
// the value of 50 seconds which is equivalent to the original averaging period 
// used in eMule).  The download rate is measured using actual timestamps.  The 
// filter-based averaging however uses a simplified algorithm that assumes a 
// fixed loop time - this does not introduce any measurement error, it simply 
// makes the degree of smoothing slightly imprecise (the true TC of the filter 
// varies inversely with the true loop time), which is of no importance here.

float CUpDownClient::CalculateKBpsDown() {
												// -- all timing values are in seconds --
	const	float tcLoop   =  0.1;				// _assumed_ Process() loop time = 0.1 sec
	const	float tcInit   =  0.4;				// initial filter time constant
	const	float tcFinal  = 50.0;				// final filter time constant
	const	float tcReduce =  5.0;				// transition from tcInit to tcFinal
	
	const	float fInit  = tcLoop/tcInit;		// initial averaging factor
	const	float fFinal = tcLoop/tcFinal;		// final averaging factor
	const	float fReduce = std::exp(std::log(fFinal/fInit) / (tcReduce/tcLoop)) * 0.99999;
	
	uint32	msCur = ::GetTickCount();

	if (msReceivedPrev == 0) {  // initialize the averaging filter
		fDownAvgFilter = fInit;
		// "kBpsDown =  bytesReceivedCycle/1024.0 / tcLoop"  would be technically correct,
		// but the first loop often receives a large chunk of data and then produces a spike
		kBpsDown = /* 0.0 * (1.0-fInit) + */ bytesReceivedCycle/1024.0 / tcLoop * fInit;
		bytesReceivedCycle = 0;
	} else if (msCur != msReceivedPrev) {	// (safeguard against divide-by-zero)
		if (fDownAvgFilter > fFinal) {		// reduce time constant during ramp-up phase
			fDownAvgFilter *= fReduce;		// this approximates averaging a lengthening list
		}
		kBpsDown = kBpsDown * (1.0 - fDownAvgFilter) 
					+ (bytesReceivedCycle/1.024)/((float)(msCur-msReceivedPrev)) * fDownAvgFilter;
		bytesReceivedCycle = 0;
	}
	msReceivedPrev = msCur;	

	m_cShowDR++;
	if (m_cShowDR == 30){
		m_cShowDR = 0;
		UpdateDisplayedInfo();
	}
	if ((::GetTickCount() - m_dwLastBlockReceived) > DOWNLOADTIMEOUT){
		if (!GetSentCancelTransfer()){
			Packet* packet = new Packet(OP_CANCELTRANSFER,0);
			theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->GetPacketSize());
			SendPacket(packet,true,true);
			SetSentCancelTransfer(1);
		}
		SetDownloadState(DS_ONQUEUE);		
	}
		
	return kBpsDown;
}

uint16 CUpDownClient::GetAvailablePartCount() const
{
	uint16 result = 0;
	for (int i = 0;i != m_nPartCount;i++){
		if (IsPartAvailable(i))
			result++;
	}
	return result;
}

void CUpDownClient::SetRemoteQueueRank(uint16 nr)
{
	m_nOldRemoteQueueRank = m_nRemoteQueueRank;
	m_nRemoteQueueRank = nr;
	UpdateDisplayedInfo();
}

void CUpDownClient::UDPReaskACK(uint16 nNewQR)
{
	// 0.42e
	m_bUDPPending = false;
	SetRemoteQueueRank(nNewQR);
	m_dwLastAskedTime = ::GetTickCount();
}

void CUpDownClient::UDPReaskFNF()
{
	if (GetDownloadState()!=DS_DOWNLOADING){ // avoid premature deletion of 'this' client
		// 0.42e
		m_bUDPPending = false;
		theApp.downloadqueue->RemoveSource(this);
		if (!m_socket) {
			if (Disconnected(wxT("UDPReaskFNF m_socket=NULL"))) {
				Safe_Delete();
			}
		}
	} else {
		AddDebugLogLineM(false,wxString::Format(wxT("UDP ANSWER FNF : %s - did not remove client because of current download state"),unicode2char(GetUserName())));
	}
}

void CUpDownClient::UDPReaskForDownload()
{
	if(!m_reqfile || m_bUDPPending) {
		return;
	}

	//the line "m_bUDPPending = true;" use to be here

	
	if(m_nUDPPort != 0 && theApp.glob_prefs->GetUDPPort() != 0 &&
	   !HasLowID() && !IsConnected())
	{ 
		// deadlake PROXYSUPPORT
		//don't use udp to ask for sources
		if(IsSourceRequestAllowed()) {
			return;
		}
		m_bUDPPending = true;
		CSafeMemFile data(128);
		data.WriteRaw(m_reqfile->GetFileHash(),16);
		if (GetUDPVersion() > 3)
		{
			if (m_reqfile->IsPartFile()) {
				((CPartFile*)m_reqfile)->WritePartStatus(&data);
			}
			else {
				data.Write((uint16) 0);
			}
		}
		if (GetUDPVersion() > 2) {
			data.Write(m_reqfile->m_nCompleteSourcesCount);
		}
		/*
		if (thePrefs.GetDebugClientUDPLevel() > 0)
			DebugSend("OP__ReaskFilePing", this, (char*)m_reqfile->GetFileHash());
		*/
		Packet* response = new Packet(&data, OP_EMULEPROT);
		response->SetOpCode(OP_REASKFILEPING);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(response->GetPacketSize());
		theApp.clientudp->SendPacket(response,GetIP(),GetUDPPort());
	
	}
}

// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN

wxString CUpDownClient::ShowDownloadingParts()
{
	// Initialise to all N's
	wxString Parts(wxT('N'), m_nPartCount);
	
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0; ) {
		Parts.SetChar((m_PendingBlocks_list.GetNext(pos)->block->StartOffset / PARTSIZE), 'Y');
	}
	
	return Parts;
}

void CUpDownClient::UpdateDisplayedInfo(bool force)
{
	DWORD curTick = ::GetTickCount();
	if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
		Notify_DownloadCtrlUpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
}


// IgnoreNoNeeded = will switch to files of which this source has no needed parts (if no better fiels found)
// ignoreSuspensions = ignore timelimit for A4Af jumping
// bRemoveCompletely = do not readd the file which the source is swapped from to the A4AF lists (needed if deleting or stopping a file)
// toFile = Try to swap to this partfile only

bool CUpDownClient::SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile)
{
	// Fail if m_reqfile is invalid
	if ( m_reqfile == NULL ) {
		return false;
	}
	
	// It would be stupid to swap away a downloading source
	if (GetDownloadState() == DS_DOWNLOADING) {
		return false;
	}

	// The iterator of the final target
	A4AFList::iterator target = m_A4AF_list.end();
	
	// Do we want to swap to a specific file?
	if ( toFile != NULL ) {
		A4AFList::iterator it = m_A4AF_list.find( toFile );
		if ( it != m_A4AF_list.end() ) {

			// We force ignoring of noneeded flag and timestamps
			if ( IsValidSwapTarget( it, true, true ) ) {
				// Set the target
				target = it;
			}
		}
	} else { 
		// We want highest priority possible, but need to start with 
		// a value less than any other priority
		char priority = -1;
		
		A4AFList::iterator it = m_A4AF_list.begin();
		for ( ; it != m_A4AF_list.end(); ++it ) {
			if ( IsValidSwapTarget( it, bIgnoreNoNeeded, ignoreSuspensions ) ) {
				char cur_priority = it->first->GetDownPriority();

				// We would prefer to get files with needed parts, thus rate them higher.
				// However, this really only matters if bIgnoreNoNeeded is true.
				if ( it->second.NeededParts )
						cur_priority += 10;
				
				// Change target if the current file has a higher rate than the previous
				if ( cur_priority > priority ) {
					priority = cur_priority;
			
					// Set the new target
					target = it;

					// Break on the first High-priority file with needed parts
					if ( priority == PR_HIGH + 10 ) {
						break;
					}
				}
			}
		}
	}

	// Try to swap if we found a valid target
	if ( target != m_A4AF_list.end() ) {
		
		// Sainity check, if reqfile doesn't own the source, then something
		// is wrong and the swap cannot proceed.
		if ( m_reqfile->m_SrcList.erase( this ) ) {
			CPartFile* SwapTo = target->first;
			
			// remove this client from the A4AF list of our new m_reqfile
			if ( SwapTo->A4AFsrclist.erase( this ) ) {
				Notify_DownloadCtrlRemoveSource(this, SwapTo);
			}

			m_reqfile->RemoveDownloadingSource( this );

			// Do we want to remove it completly? Say if the old file is getting deleted
			if ( !bRemoveCompletely ) {
				m_reqfile->A4AFsrclist.insert( this );
				
				// Set the status of the old file
				m_A4AF_list[m_reqfile].NeededParts = (GetDownloadState() != DS_NONEEDEDPARTS);
				
				// Avoid swapping to this file for a while
				m_A4AF_list[m_reqfile].timestamp = ::GetTickCount(); 
							
				Notify_DownloadCtrlAddSource(m_reqfile, this, true);
			} else {
				Notify_DownloadCtrlRemoveSource( this, m_reqfile );
			}
		
			SetDownloadState(DS_NONE);
			ResetFileStatusInfo();

			m_nRemoteQueueRank = 0;
			m_nOldRemoteQueueRank = 0;

			m_reqfile->UpdatePartsInfo();
			m_reqfile->UpdateAvailablePartsCount();
	
			SetRequestFile( SwapTo );

			SwapTo->m_SrcList.insert( this );
		
			Notify_DownloadCtrlAddSource(SwapTo, this, false);

			// Remove the new reqfile from the list of other files
			m_A4AF_list.erase( target );
			
			return true;
		}
	}

	return false;
}


bool CUpDownClient::IsValidSwapTarget( A4AFList::iterator it, bool ignorenoneeded, bool ignoresuspended )
{
	wxASSERT( it != m_A4AF_list.end() && it->first );
		
	// Check if this file has been suspended
	if ( !ignoresuspended ) {
		if ( ::GetTickCount() - it->second.timestamp >= PURGESOURCESWAPSTOP ) {
			// The wait-time has been exceeded and the file is now a valid target
			it->second.timestamp = 0;
		} else {
			// The file was still suspended and we are not ignoring suspensions
			return false;
		}
	}
	
	// Check if the client has needed parts
	if ( !ignorenoneeded ) {
		if ( !it->second.NeededParts ) {
			return false;
		}
	}
	
	// Final checks to see if the client is a valid target
	CPartFile* cur_file = it->first;
	if ( ( cur_file != m_reqfile && !cur_file->IsStopped() ) && 
	     ( cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY ) &&
		 ( cur_file->IsPartFile() ) )
	{
		return true;
	} else {
		return false;
	}
}


void CUpDownClient::SetRequestFile(CPartFile* reqfile)
{
	if ( m_reqfile != reqfile ) {
		// Paranoia. Ensure that no calls to IsPartAvailble will seg-fault while we delete stuff
		m_nPartCount = 0;
		if ( m_abyPartStatus ) {
			delete[] m_abyPartStatus;
			m_abyPartStatus = NULL;
		}
				
		m_reqfile = reqfile;
		
		if ( reqfile ) {
			m_nPartCount = reqfile->GetPartCount();
		}
	}
}

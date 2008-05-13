//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "otherfunctions.h"	// Needed for md4cmp
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "sockets.h"		// Needed for CServerConnect
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ClientCredits.h"	// Needed for CClientCredits
#include "otherstructs.h"	// Needed for Requested_Block_Struct
#include "Preferences.h"	// Needed for CPreferences
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "ListenSocket.h"	// Needed for CListenSocket
#include "amule.h"		// Needed for theApp
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "PartFile.h"		// Needed for CPartFile
#include "BarShader.h"		// Needed for CBarShader
#include "updownclient.h"	// Needed for CUpDownClient
#include "otherfunctions.h" // md4hash

#define AddDebugLogLine	   theApp.amuledlg->AddDebugLogLine

// members of CUpDownClient
// which are mainly used for downloading functions
CBarShader CUpDownClient::s_StatusBar(16);

void CUpDownClient::DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool onlygreyrect, bool  bFlat)
{
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

	//s_StatusBar.SetFileSize(reqfile->GetFileSize()); // Look below to see where it moved (Creteil)
	s_StatusBar.SetHeight(rect.height - rect.y);
	s_StatusBar.SetWidth(rect.width - rect.x);
	s_StatusBar.Fill(crNeither);

	// Barry - was only showing one part from client, even when reserved bits from 2 parts
	CString gettingParts;
	ShowDownloadingParts(&gettingParts);

	if (!onlygreyrect && reqfile && m_abyPartStatus) {
		s_StatusBar.SetFileSize(reqfile->GetFileSize()); // Moved here bu x86_64 men (Creteil)
		for (uint32 i = 0;i != m_nPartCount;i++) {
			if (m_abyPartStatus[i]) {
				uint32 uEnd;
				if (PARTSIZE*(i+1) > reqfile->GetFileSize()) {
					uEnd = reqfile->GetFileSize();
				} else {
					uEnd = PARTSIZE*(i+1);
				}
				if (reqfile->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1)) {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crBoth);
				} else if (m_nDownloadState == DS_DOWNLOADING && m_nLastBlockOffset < uEnd && m_nLastBlockOffset >= PARTSIZE*i) {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crPending);
				} else if (gettingParts.GetChar(i) == 'Y') {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crNextPending);
				} else {
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crClientOnly);
				}
			}
		}
	}
	s_StatusBar.Draw(dc, rect.x, rect.y, bFlat);
}

bool CUpDownClient::Compare(CUpDownClient* tocomp, bool bIgnoreUserhash){
	if(!bIgnoreUserhash && HasValidHash() && tocomp->HasValidHash())
	    return !md4cmp(this->GetUserHash(), tocomp->GetUserHash());
	if (HasLowID())
		return ((this->GetUserID() == tocomp->GetUserID()) && (GetServerIP() == tocomp->GetServerIP()) && (this->GetUserPort() == tocomp->GetUserPort()));
	else
		return ((this->GetUserID() == tocomp->GetUserID() && this->GetUserPort() == tocomp->GetUserPort()) || (this->GetIP() && (this->GetIP() == tocomp->GetIP() && this->GetUserPort() == tocomp->GetUserPort())) );
}

/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
void CUpDownClient::AskForDownload()
{
	if (theApp.listensocket->TooManySockets() && !(socket && socket->IsConnected())) {
		if (GetDownloadState() != DS_TOOMANYCONNS) {
			SetDownloadState(DS_TOOMANYCONNS);
		}
		return;
	}
	m_bUDPPending = false;
	m_dwLastAskedTime = ::GetTickCount();
	SetDownloadState(DS_CONNECTING);
	TryToConnect();
}
/* eMule 0.30c implementation, i give it a try (Creteil) END ... */

void CUpDownClient::SendStartupLoadReq()
{
	if (socket==NULL || reqfile==NULL) {
		return;
	}
	SetDownloadState(DS_ONQUEUE);
	CSafeMemFile* dataStartupLoadReq = new CSafeMemFile(16);
	dataStartupLoadReq->Write((const uint8*)reqfile->GetFileHash());
	Packet* packet = new Packet(dataStartupLoadReq);
	packet->opcode = OP_STARTUPLOADREQ;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet, true, true);
	delete dataStartupLoadReq;
}


bool CUpDownClient::IsSourceRequestAllowed()
{
	DWORD dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
	int nTimePassedClient = dwTickCount - GetLastSrcAnswerTime();
	int nTimePassedFile   = dwTickCount - reqfile->GetLastAnsweredTime();
	bool bNeverAskedBefore = (GetLastAskedForSources() == 0);

	UINT uSources = reqfile->GetSourceCount();
	return (
		// if client has the correct extended protocol
		ExtProtocolAvailable() && m_bySourceExchangeVer >= 1 &&
		// AND if we need more sources
		theApp.glob_prefs->GetMaxSourcePerFileSoft() > uSources &&
		// AND if...
		(
		//source is not complete and file is rare, allow once every 10 minutes
		(!m_bCompleteSource &&
		(uSources - reqfile->GetValidSourcesCount() <= RARE_FILE / 4 ||
		uSources <= RARE_FILE * 2) &&
		(bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASK)) ||
		// otherwise, allow every 90 minutes, but only if we haven't
		// asked someone else in last 10 minutes
		((bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASK * reqfile->GetCommonFilePenalty()) &&
		(nTimePassedFile > SOURCECLIENTREASK)))
	);
}

void CUpDownClient::SendFileRequest()
{
	if(!reqfile) {
		return;
	}
	AddAskedCountDown();

	CSafeMemFile dataFileReq(16+16);

	dataFileReq.Write((const uint8*)reqfile->GetFileHash());
	if( GetExtendedRequestsVersion() > 0 ){
		reqfile->WritePartStatus(&dataFileReq);
	}
	if( GetExtendedRequestsVersion() > 1 ){
		reqfile->WriteCompleteSourcesCount(&dataFileReq);		// #zegzav:completesrc (add)
	}
	Packet* packet = new Packet(&dataFileReq);	
	packet->opcode=OP_FILEREQUEST;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet, true);
	
	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_FILEREQUEST with OP_FILEREQANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (reqfile->GetPartCount() > 1){
	    CSafeMemFile dataSetReqFileID(16);
	    dataSetReqFileID.Write((const uint8*)reqfile->GetFileHash());
	    packet = new Packet(&dataSetReqFileID);
	    packet->opcode = OP_SETREQFILEID;
	    theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	    socket->SendPacket(packet, true);
	}

	if( IsEmuleClient() ) {
		SetRemoteQueueFull( true );
		SetRemoteQueueRank(0);
	}	
	
	if(IsSourceRequestAllowed()) {
		reqfile->SetLastAnsweredTimeTimeout();
		Packet* packet = new Packet(OP_REQUESTSOURCES,16,OP_EMULEPROT);
		md4cpy(packet->pBuffer,reqfile->GetFileHash());
		theApp.uploadqueue->AddUpDataOverheadSourceExchange(packet->size);
		socket->SendPacket(packet,true,true);
		SetLastAskedForSources();
		// We need the debug preferences
		#if 0
		if ( theApp.glob_prefs->GetDebugSourceExchange() )
			AddDebugLogLine( false, "Send:Source Request User(%s) File(%s)", GetUserName(), reqfile->GetFileName() );
		#endif
	}	

}

void CUpDownClient::ProcessFileInfo(char* packet,uint32 size)
{
	CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
	uint8 cfilehash[16];
	data->Read(cfilehash);
	wxString Filename;
	data->Read(Filename);
		
	// data.Read(m_strClientFilename.GetBuffer(namelength),namelength);
	// m_strClientFilename.ReleaseBuffer(namelength);

	if (m_pszClientFilename) {
		delete[] m_pszClientFilename;
	}
	m_pszClientFilename = new char[Filename.Length()+1];
	strncpy(m_pszClientFilename, Filename.GetData(), Filename.Length());
	delete data;

	if ( (!reqfile) || memcmp(cfilehash,reqfile->GetFileHash(),16)) {
		throw wxString(wxT("Wrong fileid sent (ProcessFileInfo; reqfile==NULL)"));
	}
	if (md4cmp(cfilehash,reqfile->GetFileHash())) {
		throw wxString(wxT("Wrong fileid sent (ProcessFileInfo; reqfile!=cfilehash)"));
	}
	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_FILEREQUEST with OP_FILEREQANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	
	if (reqfile->GetPartCount() == 1) {
		if (m_abyPartStatus) {
			delete[] m_abyPartStatus;
			m_abyPartStatus = NULL;
		}
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		memset(m_abyPartStatus,1,m_nPartCount);
		m_bCompleteSource = true;

		UpdateDisplayedInfo();
		reqfile->UpdateAvailablePartsCount();

		// even if the file is <= PARTSIZE, we _may_ need the hashset
		// for that file (if the file size == PARTSIZE)
		if (reqfile->hashsetneeded) {
			CMemFile* data = new CMemFile();
			data->Write((const uint8*)reqfile->GetFileHash());
			Packet* packet = new Packet(data);
			packet->opcode = OP_HASHSETREQUEST;
			delete data;
/*	
			Packet* packet = new Packet(OP_HASHSETREQUEST,16);
			md4cpy(packet->pBuffer,reqfile->GetFileHash());
*/
			theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
			socket->SendPacket(packet,true,true);
			SetDownloadState(DS_REQHASHSET);
			m_fHashsetRequesting = 1;
			reqfile->hashsetneeded = false;
		} else {
			SendStartupLoadReq();
		}
		reqfile->NewSrcPartsInfo();
	}
}

void CUpDownClient::ProcessFileStatus(char* packet,uint32 size)
{
	CSafeMemFile data((BYTE*)packet,size);
	uint8 cfilehash[16];
	data.Read(cfilehash);
	
	if ( (!reqfile) || md4cmp(cfilehash,reqfile->GetFileHash())){
		if (reqfile==NULL) {
			throw wxString(wxT("Wrong fileid sent (ProcessFileStatus - reqfile ==  NULL)"));			
		} else {
			throw wxString(wxT("Wrong fileid sent (ProcessFileStatus - reqfile != cfilehash)"));
		}
	}

	uint16 nED2KPartCount;
	data.Read(nED2KPartCount);
	if (m_abyPartStatus){
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	bool bPartsNeeded = false;
	if (!nED2KPartCount){	
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		memset(m_abyPartStatus,1,m_nPartCount);
		bPartsNeeded = true;
		m_bCompleteSource = true;
	} else {
		if (reqfile->GetED2KPartCount() != nED2KPartCount){
			m_nPartCount = 0; // Creteil from 0.30c
			throw wxString(wxT("Wrong part number"));
		}
		m_nPartCount = reqfile->GetPartCount(); // Creteil from 0.30c

		m_bCompleteSource = false;
		m_abyPartStatus = new uint8[m_nPartCount];
		uint16 done = 0;
		uint8 toread;
		while (done != m_nPartCount) {
			data.Read(toread);
			for (sint32 i = 0;i != 8;i++) {
				m_abyPartStatus[done] = ((toread>>i)&1)? 1:0;
				if (m_abyPartStatus[done] && !reqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)) {
					bPartsNeeded = true;
				}
				done++;
				if (done == m_nPartCount) {
					break;
				}
			}
		}
	}
	UpdateDisplayedInfo();
	reqfile->UpdateAvailablePartsCount();
	
	if (!bPartsNeeded) {
		SetDownloadState(DS_NONEEDEDPARTS);
	} else if (reqfile->hashsetneeded) {
		Packet* packet = new Packet(OP_HASHSETREQUEST,16);
		md4cpy(packet->pBuffer,reqfile->GetFileHash());
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet, true, true);
		SetDownloadState(DS_REQHASHSET);
		m_fHashsetRequesting = 1;
		reqfile->hashsetneeded = false;
	} else {
		SendStartupLoadReq();
	}
	reqfile->NewSrcPartsInfo();
}

bool CUpDownClient::AddRequestForAnotherFile(CPartFile* file)
{
	for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos != 0;m_OtherNoNeeded_list.GetNext(pos)) {
		if (m_OtherNoNeeded_list.GetAt(pos) == file) {
			return false;
		}
	}
	for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;m_OtherRequests_list.GetNext(pos)) {
		if (m_OtherRequests_list.GetAt(pos) == file) {
			return false;
		}
	}
	m_OtherRequests_list.AddTail(file);
	file->A4AFsrclist.AddTail(this); // [enkeyDEV(Ottavio84) -A4AF-] Imported from eMule 0.30c (Creteil) ...
	return true;
}

/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
void CUpDownClient::SetDownloadState(uint8 byNewState)
{
	if (m_nDownloadState != byNewState) {
		if (byNewState == DS_DOWNLOADING) {
			reqfile->AddDownloadingSource(this);
		} else if (m_nDownloadState == DS_DOWNLOADING) {
			reqfile->RemoveDownloadingSource(this);
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
			for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;m_DownloadBlocks_list.GetNext(pos)) {
				Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetAt(pos);
				reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
				delete m_DownloadBlocks_list.GetAt(pos);
			}
			m_DownloadBlocks_list.RemoveAll();

			for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;m_PendingBlocks_list.GetNext(pos)) {
				Pending_Block_Struct *pending = m_PendingBlocks_list.GetAt(pos);
				if (reqfile) {
					reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
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
#ifdef DOWNLOADRATE_FILTERED
			kBpsDown = 0.0;
			bytesReceivedCycle = 0;
			msReceivedPrev = 0;
#else
			m_nDownDatarate = 0;
#endif
			if (byNewState == DS_NONE) {
				if (m_abyPartStatus) {
					delete[] m_abyPartStatus;
				}
				m_abyPartStatus = 0;
				m_nPartCount = 0;
			}
			if (socket && byNewState != DS_ERROR) {
				socket->DisableDownloadLimit();
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

void CUpDownClient::ProcessHashSet(char* packet,uint32 size)
{
	if ((!reqfile) || memcmp(packet,reqfile->GetFileHash(),16)) {
		throw wxString(wxT("Wrong fileid sent (ProcessHashSet)"));
	}
	if (!m_fHashsetRequesting) {
		throw wxString(wxT("Received unsolicited hashset, ignoring it."));
	}
	CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
	if (reqfile->LoadHashsetFromFile(data,true)) {
		m_fHashsetRequesting = 0;
	} else {
		reqfile->hashsetneeded = true;
		delete data;	//mf
		throw wxString(wxT("Corrupted or invalid hashset received"));
	}
	delete data;
	SendStartupLoadReq();
}

void CUpDownClient::SendBlockRequests()
{
	m_dwLastBlockReceived = ::GetTickCount();
	if (!reqfile) {
		return;
	}
	if (m_DownloadBlocks_list.IsEmpty()) {
		// Barry - instead of getting 3, just get how many is needed
		uint16 count = 3 - m_PendingBlocks_list.GetCount();
		Requested_Block_Struct** toadd = new Requested_Block_Struct*[count];
		if (reqfile->GetNextRequestedBlock(this,toadd,&count)) {
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
		pblock->bZStreamError = false;
		m_PendingBlocks_list.AddTail(pblock);
	}
	if (m_PendingBlocks_list.IsEmpty()) {
		Packet* packet = new Packet(OP_CANCELTRANSFER,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet,true,true);
		SetDownloadState(DS_NONEEDEDPARTS);
		return;
	}
	Packet* packet = new Packet(OP_REQUESTPARTS,40);
	CMemFile* data = new CMemFile((BYTE*)packet->pBuffer,40);
	data->Write((const uint8*)reqfile->GetFileHash());
	POSITION pos = m_PendingBlocks_list.GetHeadPosition();

	Requested_Block_Struct* block;
	for (uint32 i = 0; i != 3; i++) {
		if (pos) {
			block = m_PendingBlocks_list.GetAt(pos)->block;
			m_PendingBlocks_list.GetNext(pos);
			data->Write(block->StartOffset);
		} else {
			data->Write((uint32)0);
		}
	}
	pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++) {
		if (pos) {
			block = m_PendingBlocks_list.GetAt(pos)->block;
			m_PendingBlocks_list.GetNext(pos);
			data->Write(block->EndOffset+1);
		} else {
			data->Write((uint32)0);
		}
	}
	delete data;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
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

void CUpDownClient::ProcessBlockPacket(char *packet, uint32 size, bool packed)
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
		CSafeMemFile *data = new CSafeMemFile((BYTE*)packet, size);
		uint8 fileID[16];
		data->Read(fileID);

		// Check that this data is for the correct file
		if ((!reqfile) || memcmp(packet, reqfile->GetFileHash(), 16)) {
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
		if ( size != ((nEndPos - nStartPos) + HEADER_SIZE)) {
			throw wxString(wxT("Corrupted or invalid DataBlock received (ProcessBlockPacket)"));
		}
		// Move end back one, should be inclusive
		theApp.UpdateReceivedBytes(size - HEADER_SIZE);
#ifdef DOWNLOADRATE_FILTERED
		bytesReceivedCycle += size - HEADER_SIZE;
#else
		m_nDownDataRateMS += size - HEADER_SIZE;
#endif

		credits->AddDownloaded(size - HEADER_SIZE, GetIP());
		nEndPos--;

		// Loop through to find the reserved block that this is within
		Pending_Block_Struct *cur_block;
		for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL; m_PendingBlocks_list.GetNext(pos)) {
			cur_block = m_PendingBlocks_list.GetAt(pos);
			if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos)) {
				// Found reserved block
				
				if (cur_block->bZStreamError){
					AddDebugLogLine(false, CString(_("Ignoring %u bytes of block %u-%u because of errornous zstream state for file \"%s\"")), size - HEADER_SIZE, nStartPos, nEndPos, reqfile->GetFileName().GetData());
					reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
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
					lenWritten = reqfile->WriteToBuffer(size - HEADER_SIZE, 
					(BYTE *) (packet + HEADER_SIZE), nStartPos, nEndPos,
					cur_block->block );
				} else {
					// Packed
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
					if (result == Z_OK) {
						wxASSERT( (int)lenUnzipped > 0 );
						// Write any unzipped data to disk
						if (lenUnzipped > 0) {
							// Use the current start and end
							// positions for the uncompressed data
							nStartPos = cur_block->block->StartOffset + cur_block->totalUnzipped - lenUnzipped;
							nEndPos = cur_block->block->StartOffset + cur_block->totalUnzipped - 1;

							if (nStartPos > cur_block->block->EndOffset || nEndPos > cur_block->block->EndOffset) {
								AddDebugLogLine(false, CString(_("Corrupted compressed packet for %s received (error %i)")),reqfile->GetFileName().GetData(),666);
								reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
							} else {
								// Write uncompressed data to file
								lenWritten = reqfile->WriteToBuffer(size - HEADER_SIZE,
								unzipped, nStartPos, nEndPos,
								cur_block->block );
							}
						}
					} else {
						CString strZipError;
						if (cur_block->zStream && cur_block->zStream->msg) {
							strZipError.Format(_T(" - %s"), cur_block->zStream->msg);
						} 
						AddDebugLogLine(false, CString(_("Corrupted compressed packet for %s received (error %i)")) + strZipError, reqfile->GetFileName().GetData(), result);
						reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);

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
						cur_block->bZStreamError = true;
					}
					delete [] unzipped;
				}
				// These checks only need to be done if any data was written
				if (lenWritten > 0) {
					m_nTransferedDown += lenWritten;

					// If finished reserved block
					if (nEndPos == cur_block->block->EndOffset) {
						reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
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
		AddDebugLogLine(false, CString(_("Unknown exception in %s: file \"%s\"")), __FUNCTION__, reqfile ? reqfile->GetFileName().GetData() : "?");
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
			CString strZipError;
			if (zS->msg)
				strZipError.Format(_T(" %d '%s'"), err, zS->msg);
			else if (err != Z_OK)
				strZipError.Format(_T(" %d"), err);
			AddDebugLogLine(false,"Unexpected zip error%s in file \"%s\"", strZipError.c_str(), reqfile ? reqfile->GetFileName().GetData() : "?");
		}

		if (err != Z_OK) {
			(*lenUnzipped) = 0;
		}
	} catch (...) {
		AddDebugLogLine(false, _T("Unknown exception in %s: file \"%s\""), __FUNCTION__, reqfile ? reqfile->GetFileName().GetData() : "?");
		err = Z_DATA_ERROR;
	}
	return err;
}


#ifdef DOWNLOADRATE_FILTERED
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
		Packet* packet = new Packet(OP_CANCELTRANSFER,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet,true,true);
		SetDownloadState(DS_ONQUEUE);
	}
		
	return kBpsDown;
}

#else
// Kry - Imported from 0.30d

uint32 CUpDownClient::CalculateDownloadRate(){

	// Patch By BadWolf - Accurate datarate Calculation
	TransferredData newitem = {m_nDownDataRateMS,::GetTickCount()};
	m_AvarageDDR_list.AddTail(newitem);
	m_nSumForAvgDownDataRate += m_nDownDataRateMS;
	m_nDownDataRateMS = 0;

	while (m_AvarageDDR_list.GetCount()>500) {
		m_nSumForAvgDownDataRate -= m_AvarageDDR_list.RemoveHead().datalen;
	}
	
	if(m_AvarageDDR_list.GetCount() > 1){
// COMPUTATION ERRORS: (1) "1000 * m_nSumForAvgDownDataRate" overflows for rates >84kB/s, 
//   would need to be converted to 64 bits.  (2) dwDuration is short by one period (imagine 
//   only two elements in the list to see why) - an initial AddTail in CUpDownClient::Init
//   with zero count and initial timestamp might do the trick if it occurs early enough. (Emilio)
 		DWORD dwDuration = m_AvarageDDR_list.GetTail().timestamp - m_AvarageDDR_list.GetHead().timestamp;
		if (dwDuration)
			m_nDownDatarate = 1000 * m_nSumForAvgDownDataRate / dwDuration;
	
	}
	else
		m_nDownDatarate = 0;
	// END Patch By BadWolf

	m_cShowDR++;
	if (m_cShowDR == 30){
		m_cShowDR = 0;
		UpdateDisplayedInfo();
	}
	if ((::GetTickCount() - m_dwLastBlockReceived) > DOWNLOADTIMEOUT){
		Packet* packet = new Packet(OP_CANCELTRANSFER,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet,true,true);
		SetDownloadState(DS_ONQUEUE);
	}
		
	return m_nDownDatarate;  // in bytes per second
}
// EOI
#endif


uint16 CUpDownClient::GetAvailablePartCount()
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
	m_nRemoteQueueRank = nr;
	UpdateDisplayedInfo();
}

void CUpDownClient::UDPReaskACK(uint16 nNewQR)
{
	m_bUDPPending = false;
	SetRemoteQueueRank(nNewQR);
	m_dwLastAskedTime = ::GetTickCount();
}

void CUpDownClient::UDPReaskFNF()
{
	m_bUDPPending = false;
	theApp.downloadqueue->RemoveSource(this);
	if (!socket) {
		Disconnected();
	}
}

void CUpDownClient::UDPReaskForDownload()
{
	if(!reqfile || m_bUDPPending) {
		return;
	}

	//the line "m_bUDPPending = true;" use to be here

	if(m_nUDPPort != 0 && theApp.glob_prefs->GetUDPPort() != 0 &&
	!theApp.serverconnect->IsLowID() && !HasLowID() && !(socket && socket->IsConnected())) {

		//don't use udp to ask for sources
		if(IsSourceRequestAllowed()) {
			return;
		}
		m_bUDPPending = true;
		Packet* response = new Packet(OP_REASKFILEPING,16,OP_EMULEPROT);
		memcpy(response->pBuffer,reqfile->GetFileHash(),16);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size);
		theApp.clientudp->SendPacket(response,GetIP(),GetUDPPort());
	}
}

// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN

void CUpDownClient::ShowDownloadingParts(CString *partsYN)
{
	Requested_Block_Struct *cur_block;
	int x;

	// Initialise to all N's
	char *n = new char[m_nPartCount+1];
	//_strnset(n, 'N', m_nPartCount);
	memset(n,'N',m_nPartCount);
	n[m_nPartCount] = 0;
	//partsYN->SetString(n, m_nPartCount);
	//partsYN=n;
	*partsYN<<n;
	delete [] n;

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0; m_PendingBlocks_list.GetNext(pos)) {
		cur_block = m_PendingBlocks_list.GetAt(pos)->block;
		x = (cur_block->StartOffset / PARTSIZE);
		partsYN->SetChar(x, 'Y');
	}
}

void CUpDownClient::UpdateDisplayedInfo(bool force)
{
	DWORD curTick = ::GetTickCount();
	if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
		theApp.amuledlg->transferwnd->downloadlistctrl->UpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
}

// void CUpDownClient::SwapToThisFile(CPartFile* file)

/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN */
bool CUpDownClient::DoSwap(CPartFile* SwapTo, bool bRemoveCompletely)
{
	POSITION pos = reqfile->srclists[sourcesslot].Find(this);
	if(pos)	{
		// remove this client from the A4AF list of our new reqfile
		POSITION pos2 = SwapTo->A4AFsrclist.Find(this);
		if (pos2) {
			SwapTo->A4AFsrclist.RemoveAt(pos2);
			theApp.amuledlg->transferwnd->downloadlistctrl->RemoveSource(this,SwapTo);
		}

		reqfile->srclists[sourcesslot].RemoveAt(pos);
		reqfile->IsCountDirty = true;
		reqfile->RemoveDownloadingSource(this);

		if(!bRemoveCompletely) {
			reqfile->A4AFsrclist.AddTail(this);
			if (GetDownloadState() == DS_NONEEDEDPARTS) {
				m_OtherNoNeeded_list.AddTail(reqfile);
			} else {
				m_OtherRequests_list.AddTail(reqfile);
			}
			if (!bRemoveCompletely) {
				theApp.amuledlg->transferwnd->downloadlistctrl->AddSource(reqfile,this,true);
			}
		}
		SetDownloadState(DS_NONE);
		ResetFileStatusInfo();
		m_nRemoteQueueRank = 0;
		
		reqfile->NewSrcPartsInfo();
		reqfile->UpdateAvailablePartsCount();
		reqfile = SwapTo;

		SwapTo->srclists[sourcesslot].AddTail(this);
		SwapTo->IsCountDirty = true;
		theApp.amuledlg->transferwnd->downloadlistctrl->AddSource(SwapTo,this,false);


		return true;
	}
	return false;
}

// IgnoreNoNeeded = will switch to files of which this source has no needed parts (if no better fiels found)
// ignoreSuspensions = ignore timelimit for A4Af jumping
// bRemoveCompletely = do not readd the file which the source is swapped from to the A4AF lists (needed if deleting or stopping a file)
// toFile = Try to swap to this partfile only

// bool CUpDownClient::SwapToAnotherFile()
bool CUpDownClient::SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile)
{
	if (GetDownloadState() == DS_DOWNLOADING) {
		return false;
	}
	CPartFile* SwapTo = NULL;
	CPartFile* cur_file = NULL;
	int cur_prio= -1;
	POSITION finalpos = NULL;
	CTypedPtrList<CPtrList, CPartFile*>* usedList;

	if (!m_OtherRequests_list.IsEmpty()) {
		usedList = &m_OtherRequests_list;
		for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;m_OtherRequests_list.GetNext(pos)) {
			cur_file = m_OtherRequests_list.GetAt(pos);
			if (cur_file != reqfile && theApp.downloadqueue->IsPartFile(cur_file) && !cur_file->IsStopped()
			&& (cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY))
			{
				if (toFile != NULL) {
					if (cur_file == toFile) {
						SwapTo = cur_file;
						finalpos = pos;
						break;
					}
				} else if ( cur_file->GetDownPriority()>cur_prio 
				&& (ignoreSuspensions  || (!ignoreSuspensions && !IsSwapSuspended(cur_file))))
				{
					SwapTo = cur_file;
					cur_prio=cur_file->GetDownPriority();
					finalpos=pos;
					if (cur_prio==PR_HIGH) {
						break;
					}
				}
			}
		}
	}
	if (!SwapTo && bIgnoreNoNeeded) {
		usedList = &m_OtherNoNeeded_list;
		for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos != 0;m_OtherNoNeeded_list.GetNext(pos)) {
			cur_file = m_OtherNoNeeded_list.GetAt(pos);
			if (cur_file != reqfile && theApp.downloadqueue->IsPartFile(cur_file) && !cur_file->IsStopped()
			&& (cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY))
			{
				if (toFile != NULL) {
					if (cur_file == toFile) {
						SwapTo = cur_file;
						finalpos = pos;
						break;
					}
				} else if ( cur_file->GetDownPriority()>cur_prio 
				&& (ignoreSuspensions  || (!ignoreSuspensions && !IsSwapSuspended(cur_file))))
				{
					SwapTo = cur_file;
					cur_prio=cur_file->GetDownPriority();
					finalpos=pos;
					if (cur_prio==PR_HIGH) {
						break;
					}
				}
			}
		}
	}

	if (SwapTo) {
		if (DoSwap(SwapTo,bRemoveCompletely)) {
			usedList->RemoveAt(finalpos);
			return true;
		}
	}

	return false;
}

void CUpDownClient::DontSwapTo(CPartFile* file)
{
	DWORD dwNow = ::GetTickCount();
	for (POSITION pos = m_DontSwap_list.GetHeadPosition(); pos != 0; m_DontSwap_list.GetNext(pos)) {
		if(m_DontSwap_list.GetAt(pos).file == file) {
			m_DontSwap_list.GetAt(pos).timestamp = dwNow ;
			return;
		}
	}
	PartFileStamp newfs = {file, dwNow };
	m_DontSwap_list.AddHead(newfs);
}

bool CUpDownClient::IsSwapSuspended(CPartFile* file)
{
	if (m_DontSwap_list.GetCount()==0) {
		return false;
	}
	for (POSITION pos = m_DontSwap_list.GetHeadPosition(); pos != 0 && m_DontSwap_list.GetCount()>0; m_DontSwap_list.GetNext(pos)) {
		if(m_DontSwap_list.GetAt(pos).file == file) {
			if (::GetTickCount() - m_DontSwap_list.GetAt(pos).timestamp  >= PURGESOURCESWAPSTOP) {
				m_DontSwap_list.RemoveAt(pos);
				return false;
			} else {
				return true;
			}
		} else if (m_DontSwap_list.GetAt(pos).file == NULL) {
			// in which cases should this happen ?
			m_DontSwap_list.RemoveAt(pos);
		}
	}
	return false;
}

/* eMule 0.30c implementation, i give it a try (Creteil) END */
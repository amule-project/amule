//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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

#include <zlib.h>
#include <cmath>		// Needed for std::exp
#include <wx/filename.h>        // Needed for wxFileName::GetPathSeparator()

#include "OtherFunctions.h"	// Needed for nstrdup

#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "ClientCredits.h"	// Needed for CClientCredits
#include "Packet.h"		// Needed for CPacket
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "Preferences.h"	// Needed for CPreferences
#include "OtherStructs.h"	// Needed for Requested_Block_Struct
#include "ServerConnect.h"		// Needed for CServerConnect
#include "PartFile.h"		// Needed for PR_POWERSHARE
#include "KnownFile.h"		// Needed for CKnownFile
#include "KnownFileList.h"		// Needed for CKnownFileLists
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "amule.h"		// Needed for theApp
#include "OPCodes.h"		// Needed for PARTSIZE
#include "BarShader.h"		// Needed for CBarShader
#include "updownclient.h"	// Needed for CUpDownClient
#include "ClientList.h"
#include "Statistics.h"
#include "Logger.h"
#include "Format.h"

#ifndef AMULE_DAEMON
	#include "TransferWnd.h"	// Needed for CTransferWnd
#endif

//	members of CUpDownClient
//	which are mainly used for uploading functions 

#ifndef CLIENT_GUI
uint32 CUpDownClient::GetScore(bool sysvalue, bool isdownloading, bool onlybasevalue) const
{
	//TODO: complete this (friends, uploadspeed, amuleuser etc etc)
	if (m_Username.IsEmpty()) {
		return 0;
	}

	if (credits == 0) {
		return 0;
	}

	CKnownFile* pFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
	if ( !pFile ) {
		return 0;
	}

	// bad clients (see note in function)
	if (credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY)
		return 0;
	// friend slot
	if (IsFriend() && GetFriendSlot() && !HasLowID())
		return 0x0FFFFFFF;

	if (IsBanned())
		return 0;

	if (sysvalue && HasLowID() && !IsConnected()){
		return 0;
	}	

	// TODO coded by tecxx & herbert, one yet unsolved problem here:
	// sometimes a client asks for 2 files and there is no way to decide, which file the 
	// client finally gets. so it could happen that he is queued first because of a 
	// high prio file, but then asks for something completely different.
	int filepriority = 10; // standard
	if(pFile != NULL){ 
		switch(pFile->GetUpPriority()) {
		        case PR_POWERSHARE: //added for powershare (deltaHF)
				filepriority = 2500; 
				break; //end
			case PR_VERYHIGH:
				filepriority = 18;
				break;
			case PR_HIGH:
				filepriority = 9;
				break;
			case PR_LOW:
				filepriority = 6;
				break;
			case PR_VERYLOW:
				filepriority = 2;
				break;
			case PR_NORMAL:
			default:
				filepriority = 7;
				break;
		}
	}
	// calculate score, based on waitingtime and other factors
	float fBaseValue;
	if (onlybasevalue) {
		fBaseValue = 100;
	} else if (!isdownloading) {
		fBaseValue = (float)(::GetTickCount()-GetWaitStartTime())/1000;
	} else {
		// we dont want one client to download forever
		// the first 15 min downloadtime counts as 15 min waitingtime and you get
		// a 15 min bonus while you are in the first 15 min :)
		// (to avoid 20 sec downloads) after this the score won't raise anymore 
		fBaseValue = (float)(m_dwUploadTime-GetWaitStartTime());
		fBaseValue += (float)(::GetTickCount() - m_dwUploadTime > 900000)? 900000:1800000;
		fBaseValue /= 1000;
	}
	
	float modif = credits->GetScoreRatio(GetIP());
	fBaseValue *= modif;
	
	if (!onlybasevalue) {
		fBaseValue *= (float(filepriority)/10.0f);
	}
	if (!isdownloading && !onlybasevalue) {
		if (sysvalue && HasLowID() && !IsConnected()) {
			if (!theApp.serverconnect->IsConnected() || theApp.serverconnect->IsLowID() || theApp.listensocket->TooManySockets()) {
				return 0;
			}
		}
	}
	if( (IsEmuleClient() || GetClientSoft() < 10) && m_byEmuleVersion <= 0x19) {
		fBaseValue *= 0.5f;
	}
	return (uint32)fBaseValue;
}
#endif

// Checks if it is next requested block from another chunk of the actual file or from another file 
// 
// [Returns] 
//   true : Next requested block is from another different chunk or file than last downloaded block 
//   false: Next requested block is from same chunk that last downloaded block 
bool CUpDownClient::IsDifferentPartBlock() const // [Tarod 12/22/2002] 
{ 
	bool different_part = false;
	
	// Check if we have good lists and proceed to check for different chunks
	if (!m_BlockRequests_queue.IsEmpty() && !m_DoneBlocks_list.IsEmpty())
	{
		Requested_Block_Struct* last_done_block = NULL;
		Requested_Block_Struct* next_requested_block = NULL;
		uint32 last_done_part = 0xffffffff;
		uint32 next_requested_part = 0xffffffff;
			
			
		// Get last block and next pending
		last_done_block = (Requested_Block_Struct*)m_DoneBlocks_list.GetHead();
		next_requested_block = (Requested_Block_Struct*)m_BlockRequests_queue.GetHead(); 
			
		// Calculate corresponding parts to blocks
		last_done_part = last_done_block->StartOffset / PARTSIZE;
		next_requested_part = next_requested_block->StartOffset / PARTSIZE; 
             
		// Test is we are asking same file and same part
		if ( last_done_part != next_requested_part)
		{ 
			different_part = true;
			AddDebugLogLineM(false, logClient, wxT("Session ended due to new chunk."));
		}
	
		if (md4cmp(last_done_block->FileID, next_requested_block->FileID) != 0)
		{ 
			different_part = true;
			AddDebugLogLineM(false, logClient, wxT("Session ended due to different file."));
		}
	} 

	return different_part; 
}


bool CUpDownClient::CreateNextBlockPackage()
{
	// time critical
	// check if we should kick this client
	// VQB Full Chunk Trans..
	// If is a friendslot, don't kick
	if ( !GetFriendSlot() ){
		if ( thePrefs::TransferFullChunks() ) {
			// VQB to provide full chunk transfers (modified by Tarod)
			if ( theApp.uploadqueue->CheckForTimeOver(this) || IsDifferentPartBlock()) {
				SetWaitStartTime();
				theApp.uploadqueue->RemoveFromUploadQueue(this);
				theApp.uploadqueue->AddClientToQueue(this);
				return false;
			}
		} else {
			if (theApp.uploadqueue->CheckForTimeOver(this)) {
				// back on the waitqueue
				SetWaitStartTime();
				theApp.uploadqueue->RemoveFromUploadQueue(this);
				theApp.uploadqueue->AddClientToQueue(this);
				return false;
			}
		}
	}
	
	if (m_BlockRequests_queue.IsEmpty()){
		return false;
	}
	CFile file;
	byte* filedata = 0;
	wxString fullname = wxEmptyString;
	try{
		while (!m_BlockRequests_queue.IsEmpty()){
			Requested_Block_Struct* currentblock = m_BlockRequests_queue.GetHead();
			CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(currentblock->FileID);
			if (!srcfile)
				throw wxString(wxT("requested file not found"));

			if (srcfile->IsPartFile() && ((CPartFile*)srcfile)->GetStatus() != PS_COMPLETE){
				fullname = ((CPartFile*)srcfile)->GetFullName();
				fullname.Truncate(fullname.Length()-4);
			} else{
				fullname = srcfile->GetFilePath() + wxFileName::GetPathSeparator() + srcfile->GetFileName();
			}
		
			uint32 togo;
			if (currentblock->StartOffset > currentblock->EndOffset){
				togo = currentblock->EndOffset + (srcfile->GetFileSize() - currentblock->StartOffset);
			}
			else{
				togo = currentblock->EndOffset - currentblock->StartOffset;
				if (srcfile->IsPartFile() && !((CPartFile*)srcfile)->IsComplete(currentblock->StartOffset,currentblock->EndOffset-1))
					throw wxString(wxT("Asked for incomplete block "));
			}

			if( togo > 184320 )
				throw wxString(wxT("Client requested too large of a block."));
			
			if (!srcfile->IsPartFile()){
				if ( !file.Open(fullname,CFile::read) ) {
					// The file was most likely moved/deleted. However it is likely that the
					// same is true for other files, so we recheck all shared files. 
					AddLogLineM( false, CFormat( _("Failed to open shared file (%s), rechecking list of shared files.") ) % srcfile->GetFileName() );
					theApp.sharedfiles->Reload();
					
					throw wxString(wxT("Failed to open requested file: Removing from list of shared files!"));
				}
			
				file.Seek(currentblock->StartOffset);
				
				filedata = new byte[togo+500];
				if (uint32 done = file.Read(filedata,togo) != togo){
					file.Seek(0);
					file.Read(filedata + done,togo-done);
				}
				file.Close();
			}
			else{
				CPartFile* partfile = (CPartFile*)srcfile;
				partfile->m_hpartfile.Seek(currentblock->StartOffset);
				
				filedata = new byte[togo+500];
				if (uint32 done = partfile->m_hpartfile.Read(filedata,togo) != togo){
					partfile->m_hpartfile.Seek(0);
					partfile->m_hpartfile.Read(filedata + done,togo-done);
				}
			}

			SetUploadFileID(srcfile);
			wxString ext = srcfile->GetFileName().Right(4).Lower();
			if (m_byDataCompVer == 1 && (ext != wxT(".zip")) && (ext != wxT(".rar")) && (ext != wxT(".ace")))
				CreatePackedPackets(filedata,togo,currentblock);
			else
				CreateStandartPackets(filedata,togo,currentblock);
			
			// file statistic
			srcfile->statistic.AddTransferred(togo);

			m_DoneBlocks_list.AddHead(m_BlockRequests_queue.RemoveHead());
			delete[] filedata;
			filedata = 0;
		}
	}
	catch (const wxString& error){
		AddDebugLogLineM(false, logClient, wxT("Client '") + GetUserName() + wxT("' caused error while creating packet (") + error + wxT(") - disconnecting client"));
		theApp.uploadqueue->RemoveFromUploadQueue(this);
		if (filedata)
			delete[] filedata;
		return false;
	}
	return true;

}

void CUpDownClient::CreateStandartPackets(const byte* data,uint32 togo, Requested_Block_Struct* currentblock){
	uint32 nPacketSize;

	try {
		CMemFile memfile((BYTE*)data,togo);
		if (togo > 10240) 
			nPacketSize = togo/(uint32)(togo/10240);
		else
			nPacketSize = togo;

		while (togo){
			if (togo < nPacketSize*2)
				nPacketSize = togo;
			togo -= nPacketSize;
			
			CSafeMemFile data(nPacketSize+24);
			data.WriteHash16(GetUploadFileID().GetHash());
			data.WriteUInt32((currentblock->EndOffset - togo) - nPacketSize);
			data.WriteUInt32((currentblock->EndOffset - togo));
			char *tempbuf = new char[nPacketSize];
			memfile.Read(tempbuf, nPacketSize);
			data.Write(tempbuf, nPacketSize);
			delete [] tempbuf;
			CPacket* packet = new CPacket(&data,OP_EDONKEYPROT,OP_SENDINGPART);
			m_BlockSend_queue.AddTail(packet);
		}
	} catch (...) {
		throw wxString(wxT("Caught exception in CUpDownClient::CreateStandartPackets!\n"));
	}
}

void CUpDownClient::CreatePackedPackets(const byte* data,uint32 togo, Requested_Block_Struct* currentblock){
	BYTE* output = new BYTE[togo+300];
	uLongf newsize = togo+300;
	uint16 result = compress2(output,&newsize,data,togo,9);
	if (result != Z_OK || togo <= newsize){
		delete[] output;
		CreateStandartPackets(data,togo,currentblock);
		return;
	}
	
	try {
		CMemFile memfile(output,newsize);
		togo = newsize;
		uint32 nPacketSize;
		if (togo > 10240) 
			nPacketSize = togo/(uint32)(togo/10240);
		else
			nPacketSize = togo;
			
		while (togo){
			if (togo < nPacketSize*2)
				nPacketSize = togo;
			togo -= nPacketSize;

			CSafeMemFile data(nPacketSize+24);
			data.WriteHash16(GetUploadFileID().GetHash());
			data.WriteUInt32(currentblock->StartOffset);
			data.WriteUInt32(newsize);			
			char *tempbuf = new char[nPacketSize];
			memfile.Read(tempbuf, nPacketSize);
			data.Write(tempbuf,nPacketSize);
			delete [] tempbuf;
			CPacket* packet = new CPacket(&data, OP_EMULEPROT, OP_COMPRESSEDPART);
			m_BlockSend_queue.AddTail(packet);
		}
		delete[] output;
	} catch (...) {
		throw wxString(wxT("Caught exception in CUpDownClient::CreatePackedPackets!\n"));
	}
}

void CUpDownClient::ProcessExtendedInfo(const CSafeMemFile *data, CKnownFile *tempreqfile)
{
	try {
		m_requpfile->UpdateUpPartsFrequency( this, false ); // Decrement
		m_upPartStatus.clear();		
		m_nUpPartCount = 0;
		m_nUpCompleteSourcesCount= 0;
		
		if( GetExtendedRequestsVersion() == 0 ) {
			return;
		}
		
		if (data->GetLength() == 16) {
			// to all developers: in the next version the client will be disconnected when causing this error!
			//please fix your protocol implementation (shareaza, xmule, etc)!
			//return;
			// Kry - No mercy since xMule bans aMule and eMule 0.43x
			throw(CInvalidPacket("Wrong size on extended info packet"));
		}
		
		uint16 nED2KUpPartCount = data->ReadUInt16();
		if (!nED2KUpPartCount) {
			m_nUpPartCount = tempreqfile->GetPartCount();
			m_upPartStatus.resize( m_nUpPartCount, 0 );
		} else {
			if (tempreqfile->GetED2KPartCount() != nED2KUpPartCount) {
				//We already checked if we are talking about the same file.. So if we get here, something really strange happened!
				m_nUpPartCount = 0;
				return;
			}
		
			m_nUpPartCount = tempreqfile->GetPartCount();
			m_upPartStatus.resize( m_nUpPartCount, 0 );
		
			try {
				uint16 done = 0;
				while (done != m_nUpPartCount) {
					uint8 toread = data->ReadUInt8();
					for (sint32 i = 0;i != 8;i++){
						m_upPartStatus[done] = (toread>>i)&1;
						//	We may want to use this for another feature..
						//	if (m_upPartStatus[done] && !tempreqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1))
						// bPartsNeeded = true;
						done++;
						if (done == m_nUpPartCount) {
							break;
						}
					}
				}
			} catch ( ... ) {
				// We want the increment the frequency even if we didn't read everything
				m_requpfile->UpdateUpPartsFrequency( this, true ); // Increment
				
				throw;
			}

			if (GetExtendedRequestsVersion() > 1) {
				uint16 nCompleteCountLast = GetUpCompleteSourcesCount();
				uint16 nCompleteCountNew = data->ReadUInt16();
				SetUpCompleteSourcesCount(nCompleteCountNew);
				if (nCompleteCountLast != nCompleteCountNew) {
					tempreqfile->UpdatePartsInfo();
				}
			}
		}
	} catch (const CInvalidPacket& InvalidPacket) {
		wxString error = wxT("CUpDownClient::ProcessExtendedInfo: ");
		if (strlen(InvalidPacket.what())) {
			error += char2unicode(InvalidPacket.what());
		} else {
			error += wxT("Unknown InvalidPacket exception");
		}
		
		throw(error);
	} catch (...) {
		wxString error = wxT("CUpDownClient::ProcessExtendedInfo: Unknown Exception");
		throw(error);
	}
	
	m_requpfile->UpdateUpPartsFrequency( this, true ); // Increment
	
	Notify_QlistRefreshClient(this);
}


void CUpDownClient::ResetUploadFile()
{
	m_requpfile = NULL;
}


void CUpDownClient::SetUploadFileID(CKnownFile* newreqfile)
{
	if ( m_requpfile != newreqfile ) {
		if ( m_requpfile ) {
			m_requpfile->SubQueuedCount();
			m_requpfile->RemoveUploadingClient(this);
			m_requpfile->UpdateUpPartsFrequency( this, false ); // Decrement
		}

		m_requpfile = newreqfile;

		if ( m_requpfile ) {
			m_requpfile->AddQueuedCount();
			m_requpfile->AddUploadingClient(this);
			
			if ( m_requpfileid != m_requpfile->GetFileHash() ) {
				m_requpfileid = m_requpfile->GetFileHash();
				m_upPartStatus.clear();
			} else {
				m_requpfile->UpdateUpPartsFrequency( this, true ); // Increment
			}
		} else {
			m_requpfileid.Clear();
			m_upPartStatus.clear();
		}
	}
}


void CUpDownClient::AddReqBlock(Requested_Block_Struct* reqblock){
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0;m_DoneBlocks_list.GetNext(pos)){
		if (reqblock->StartOffset == m_DoneBlocks_list.GetAt(pos)->StartOffset && reqblock->EndOffset == m_DoneBlocks_list.GetAt(pos)->EndOffset){
			delete reqblock;
			return;
		}
	}
	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition();pos != 0;m_BlockRequests_queue.GetNext(pos)){
		if (reqblock->StartOffset == m_BlockRequests_queue.GetAt(pos)->StartOffset && reqblock->EndOffset == m_BlockRequests_queue.GetAt(pos)->EndOffset){
			delete reqblock;
			return;
		}
	}
	m_BlockRequests_queue.AddTail(reqblock);

}

uint32 CUpDownClient::GetWaitStartTime() const
{
	uint32 dwResult = 0;
	
	if ( credits ) {
		dwResult = credits->GetSecureWaitStartTime(GetIP());
		
		if (dwResult > m_dwUploadTime && IsDownloading()) {
			//this happens only if two clients with invalid securehash are in the queue - if at all
			dwResult = m_dwUploadTime - 1;
		}
	}
		
	return dwResult;
}

void CUpDownClient::SetWaitStartTime() {
	if ( credits ) {
		credits->SetSecWaitStartTime(GetIP());
	}
}

void CUpDownClient::ClearWaitStartTime(){
	if ( credits ) {
		credits->ClearWaitStartTime();
	}
}

uint32 CUpDownClient::SendBlockData(float kBpsToSend){
	uint32	msCur = ::GetTickCount();
	uint32	bytesToSend;
	float	lambdaAvg;		// decay factor for averaging filter

	if (msSentPrev==0) {
		kBpsUp = kBpsToSend;  // initialize averaging filter
		lambdaAvg = 1.0;	// no decay of past value this first round
		bytesToSend = (uint32)(kBpsToSend*102.4);  // assumes 100ms nominal loop time
	}
	else {
		uint32 msElapsed = msCur-msSentPrev;  	// measure time since last block
		lambdaAvg = std::exp(-(float)msElapsed/1000.0);	// averaging time constant 1000ms
		bytesToSend = (uint32)(kBpsToSend*1.024*(msElapsed > 1000 ? 1000.0 : (float)msElapsed));
		// compensates for timer cycles lost due to system load, but avoids spiking of UL rate
	}
	msSentPrev = msCur;

	m_cSendblock++;
	if (m_cSendblock == 30){
		m_cSendblock = 0;
		Notify_UploadCtrlRefreshClient(this);
	}

	kBpsUp *= lambdaAvg;	// 1st part of averaging filter; do it here in case we return "0"
	if (m_socket==NULL || m_socket->IsBusy()
		|| (m_BlockSend_queue.IsEmpty() && !CreateNextBlockPackage()) ) {
		return 0;
	}
	m_nMaxSendAllowed += bytesToSend;
	if (m_BlockSend_queue.GetHead()->GetRealPacketSize() > 0/*m_nMaxSendAllowed * 3*/
		&& m_BlockSend_queue.GetHead()->GetRealPacketSize() > MAXFRAGSIZE*2){
		// splitting packets
			uint32 nSize = m_BlockSend_queue.GetHead()->GetRealPacketSize();
			char* pBuffer = m_BlockSend_queue.GetHead()->DetachPacket();
			delete m_BlockSend_queue.RemoveHead();
				
			uint32 nPos = nSize;
			bool bLast = true;
			while (nPos){
				uint32 nNewSize = (nPos < MAXFRAGSIZE) ? nPos : MAXFRAGSIZE;
				nPos -= nNewSize;
				char* pBuffer2 = new char[nNewSize];
				memcpy(pBuffer2,pBuffer+nPos,nNewSize);
				m_BlockSend_queue.AddHead(new CPacket(pBuffer2,nNewSize,bLast));
				bLast = false;
			}
			delete[] pBuffer;
	}
	while (!m_BlockSend_queue.IsEmpty()
		&& m_BlockSend_queue.GetHead()->GetRealPacketSize() <= m_nMaxSendAllowed){
			
		CPacket* tosend = m_BlockSend_queue.RemoveHead();
		uint32 nBlockSize = tosend->GetRealPacketSize();
		m_nMaxSendAllowed -= nBlockSize;
		SendPacket(tosend,true,false);
		m_nTransferedUp += nBlockSize;
		theApp.statistics->UpdateSentBytes(nBlockSize);
		credits->AddUploaded(nBlockSize, GetIP());
		if (m_BlockSend_queue.IsEmpty())
			CreateNextBlockPackage();
	}
	kBpsUp += kBpsToSend*(1.0-lambdaAvg); // 2nd part of averaging filter
	return bytesToSend;
}


void CUpDownClient::FlushSendBlocks()
{ // call this when you stop upload, or the socket might be not able to send
	while (!m_BlockSend_queue.IsEmpty() && m_BlockSend_queue.GetHead()->IsSplitted() && IsConnected() ) {	
		CPacket* tosend = m_BlockSend_queue.RemoveHead();
		theApp.statistics->AddUpDataOverheadOther(tosend->GetPacketSize());
		SendPacket(tosend,true,false);
	}
}


void CUpDownClient::SendHashsetPacket(const CMD4Hash& forfileid)
{
	CKnownFile* file = theApp.sharedfiles->GetFileByID( forfileid );
	if ( !file ) {
		file = theApp.downloadqueue->GetFileByID(forfileid);

		if ( !file ) {
			AddLogLineM(false, _("Hashset requested for unknown file: ") + forfileid.Encode() );
		
			return;
		} else if ( !file->GetHashCount() ) {
			AddDebugLogLineM(false, logRemoteClient, wxT("Requested hashset could not be found"));

			return;
		}
	} else if ( !file->GetHashCount() ) {
		file = theApp.downloadqueue->GetFileByID(forfileid);
		
		if ( !file || !file->GetHashCount() ) {
			AddDebugLogLineM(false, logRemoteClient, wxT("Requested hashset could not be found"));

			return;
		}
	}
	

	CSafeMemFile* data = new CSafeMemFile();
	data->WriteHash16(file->GetFileHash());
	uint16 parts = file->GetHashCount();
	data->WriteUInt16(parts);
	for (int i = 0; i != parts; i++)
		data->WriteHash16(file->GetPartHash(i));
	CPacket* packet = new CPacket(data);
	delete data;
	packet->SetOpCode(OP_HASHSETANSWER);
	theApp.statistics->AddUpDataOverheadFileRequest(packet->GetPacketSize());
	SendPacket(packet,true,true);
}

void CUpDownClient::ClearUploadBlockRequests(){
	FlushSendBlocks();
	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition();pos != 0; )
		delete m_BlockRequests_queue.GetNext(pos);
	m_BlockRequests_queue.RemoveAll();
	
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0; )
		delete m_DoneBlocks_list.GetNext(pos);
	m_DoneBlocks_list.RemoveAll();
	
	for (POSITION pos = m_BlockSend_queue.GetHeadPosition();pos != 0; )
		delete m_BlockSend_queue.GetNext(pos);
	m_BlockSend_queue.RemoveAll();
}

void CUpDownClient::SendRankingInfo(){
	if (!ExtProtocolAvailable())
		return;
	uint16 nRank = theApp.uploadqueue->GetWaitingPosition(this);
	if (!nRank)
		return;
		
	CSafeMemFile data;
	data.WriteUInt16(nRank);
	// Kry: what are these zero bytes for. are they really correct?
	// Kry - Well, eMule does like that. I guess they're ok.
	data.WriteUInt32(0); data.WriteUInt32(0); data.WriteUInt16(0);
	CPacket* packet = new CPacket(&data,OP_EMULEPROT);
	packet->SetOpCode(OP_QUEUERANKING);
	
	theApp.statistics->AddUpDataOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true,true);
}

void CUpDownClient::SendCommentInfo(CKnownFile* file)
{
	if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1) {
		return;
	}
	m_bCommentDirty = false;

	// Max lenght of comments is 50 chars
	wxString desc = file->GetFileComment().Left(50);
	uint8 rating = file->GetFileRate();
	
	if ( file->GetFileRate() == 0 && desc.IsEmpty() ) {
		return;
	}
	
	CSafeMemFile data;
	data.WriteUInt8(rating);
	data.WriteString(desc, GetUnicodeSupport(), 4 /* size it's uint32 */);
	
	CPacket* packet = new CPacket(&data,OP_EMULEPROT);
	packet->SetOpCode(OP_FILEDESC);
	theApp.statistics->AddUpDataOverheadOther(packet->GetPacketSize());
	SendPacket(packet,true);
}

void  CUpDownClient::UnBan(){
	m_Aggressiveness = 0;
	
	theApp.clientlist->AddTrackClient(this);
	theApp.clientlist->RemoveBannedClient( GetIP() );
	SetUploadState(US_NONE);
	ClearWaitStartTime();
	
	Notify_ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
}

void CUpDownClient::Ban(){
	theApp.clientlist->AddTrackClient(this);
	theApp.clientlist->AddBannedClient( GetIP() );
	
	AddDebugLogLineM( false, logClient, wxT("Client '") + GetUserName() + wxT("' seems to be an aggressive client and is banned from the uploadqueue"));
	
	SetUploadState(US_BANNED);
	
	Notify_ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	Notify_QlistRefreshClient(this);
}

bool CUpDownClient::IsBanned() const
{
	return ( (theApp.clientlist->IsBannedClient(GetIP()) ) && m_nDownloadState != DS_DOWNLOADING);
}

void CUpDownClient::CheckForAggressive()
{
	uint32 cur_time = ::GetTickCount();
	
	// First call, initalize
	if ( !m_LastFileRequest ) {
		m_LastFileRequest = cur_time;
		return;
	}
	
	// Is this an aggressive request?
	if ( ( cur_time - m_LastFileRequest ) < MIN_REQUESTTIME ) {
		m_Aggressiveness += 3;
		
		// Is the client EVIL?
		if ( m_Aggressiveness >= 10 && (!IsBanned() && m_nDownloadState != DS_DOWNLOADING )) {
			AddDebugLogLineM( false, logClient, CFormat( wxT("Aggressive client banned (score: %d): %s -- %s -- %s\n") ) 
				% m_Aggressiveness
				% m_Username
				% m_strModVersion
				% m_clientVerString );
			Ban();
		}
	} else {
		// Polite request, reward client
		if ( m_Aggressiveness )
			m_Aggressiveness--;
	}

	m_LastFileRequest = cur_time;
}

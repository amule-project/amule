//this file is part of aMule
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
#include <cmath>		// Needed for std::exp
#include "otherfunctions.h"	// Needed for nstrdup

#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "ClientCredits.h"	// Needed for CClientCredits
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "packets.h"		// Needed for Packet
#include "QueueListCtrl.h"	// Needed for CQueueListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "Preferences.h"	// Needed for CPreferences
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "otherstructs.h"	// Needed for Requested_Block_Struct
#include "sockets.h"		// Needed for CServerConnect
#include "PartFile.h"		// Needed for PR_POWERSHARE
#include "KnownFile.h"		// Needed for CKnownFile
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "amule.h"		// Needed for theApp
#include "opcodes.h"		// Needed for PARTSIZE
#include "BarShader.h"		// Needed for CBarShader
#include "updownclient.h"	// Needed for CUpDownClient

//	members of CUpDownClient
//	which are mainly used for uploading functions 

CBarShader CUpDownClient::s_UpStatusBar(16);
void CUpDownClient::DrawUpStatusBar(wxMemoryDC* dc, wxRect rect, bool onlygreyrect, bool  bFlat){
	RECT gaprect;
	gaprect.top = rect.y + 2;
	gaprect.bottom = (rect.y+rect.height) - 2;
	gaprect.left = rect.x;//rect->left;
	gaprect.right = (rect.x+rect.width);//rect->right;
	//dc->FillRect(&gaprect,&CBrush(RGB(220,220,220)));
	wxBrush suti(wxColour(220,220,220),wxSOLID);
	dc->SetPen(*wxTRANSPARENT_PEN);
	dc->SetBrush(suti);
	wxRect mygaprect;
	mygaprect.x=gaprect.left; mygaprect.y=gaprect.top;
	mygaprect.width=gaprect.right-gaprect.left;
	mygaprect.height=gaprect.bottom-gaprect.top;
	dc->DrawRectangle(mygaprect);
	dc->SetBrush(*wxWHITE_BRUSH);

	if( onlygreyrect ) {
		return;
	}
	float blockpixel = (float)(rect.width)/((float)(PARTSIZE*(m_nUpPartCount))/1024);
	for (uint32 i = 0;i != m_nUpPartCount;i++){ 
		if (m_abyUpPartStatus[i]){ 
			gaprect.right = rect.x + (uint32)(((float)PARTSIZE*i/1024)*blockpixel);
			gaprect.left  = rect.x + (uint32)((float)((float)PARTSIZE*(i+1)/1024)*blockpixel);
			//dc->FillRect(&gaprect,&CBrush(RGB(0,0,0)));
			dc->SetBrush(*wxBLACK_BRUSH);
			mygaprect.x=gaprect.left; mygaprect.y=gaprect.top;
			mygaprect.width=gaprect.right-gaprect.left;
			mygaprect.height=gaprect.bottom-gaprect.top;
			dc->DrawRectangle(mygaprect);
		}
	}
}
/*
	COLORREF crBoth; 
	COLORREF crNeither; 
	COLORREF crClientOnly; 
	COLORREF crPending;
	COLORREF crNextPending;

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

	s_UpStatusBar.SetFileSize(PARTSIZE*(m_nUpPartCount));
	s_UpStatusBar.SetHeight(rect->bottom - rect->top); 
	s_UpStatusBar.SetWidth(rect->right - rect->left); 
	s_UpStatusBar.Fill(crNeither); 

	if (!onlygreyrect && m_abyUpPartStatus) { 
		for (uint32 i = 0;i != m_nUpPartCount;i++){ 
			if (m_abyUpPartStatus[i]){ 
				s_StatusBar.FillRange(PARTSIZE*(i), PARTSIZE*(i+1), crClientOnly);
			} 
		} 
	} 
	s_StatusBar.Draw(dc, rect->left, rect->top, bFlat); 
} 
*/

uint32 CUpDownClient::GetScore(bool sysvalue, bool isdownloading, bool onlybasevalue)
{
	//TODO: complete this (friends, uploadspeed, amuleuser etc etc)
	if (!m_pszUsername) {
		return 0;
	}

	if (credits == 0) {
		return 0;
	}

	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	if(!currequpfile) {
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

	if (sysvalue && HasLowID() && !(socket && socket->IsConnected())){
		return 0;
	}	

	// TODO coded by tecxx & herbert, one yet unsolved problem here:
	// sometimes a client asks for 2 files and there is no way to decide, which file the 
	// client finally gets. so it could happen that he is queued first because of a 
	// high prio file, but then asks for something completely different.
	int filepriority = 10; // standard
	CKnownFile* pFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
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
		// the first 15 min downloadtime counts as 15 min waitingtime and you get a 15 min bonus while you are in the first 15 min :)
		// (to avoid 20 sec downloads) after this the score won't raise anymore 
		fBaseValue = (float)(m_dwUploadTime-GetWaitStartTime());
		fBaseValue += (float)(::GetTickCount() - m_dwUploadTime > 900000)? 900000:1800000;
		fBaseValue /= 1000;
	}
	//gjn 2003-01-14 added following if statement to get the credit system to work
	if(theApp.glob_prefs->UseCreditSystem()) {
		fBaseValue *= credits->GetScoreRatio(GetIP());
	}
	if (!onlybasevalue) {
		fBaseValue *= (float(filepriority)/10.0f);
	}
	if (!isdownloading && !onlybasevalue) {
		if (sysvalue && HasLowID() && !(socket && socket->IsConnected())) {
			if (!theApp.serverconnect->IsConnected() || theApp.serverconnect->IsLowID() || theApp.listensocket->TooManySockets()) {
				return 0;
			}
		}
	}
	if( (IsEmuleClient() || this->GetClientSoft() < 10) && m_byEmuleVersion <= 0x19) {
		fBaseValue *= 0.5f;
	}
	return (uint32)fBaseValue;
}

// Checks if it is next requested block from another chunk of the actual file or from another file 
// 
// [Returns] 
//   true : Next requested block is from another different chunk or file than last downloaded block 
//   false: Next requested block is from same chunk that last downloaded block 
bool CUpDownClient::IsDifferentPartBlock(void) // [Tarod 12/22/2002] 
{ 
	//printf("entered in : CUpDownClient::IsDifferentPartBlock\n");
	Requested_Block_Struct* last_done_block;
	Requested_Block_Struct* next_requested_block;
	uint32 last_done_part = 0xffffffff;
	uint32 next_requested_part = 0xffffffff;
	
	bool different_part = false;
	
	try {
		// Check if we have good lists and proceed to check for different chunks
		if (!m_BlockRequests_queue.IsEmpty() && !m_DoneBlocks_list.IsEmpty())
		{
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
				theApp.amuledlg->AddDebugLogLine(false, "Session ended due to new chunk.");
			}
			if (memcmp(last_done_block->FileID, next_requested_block->FileID, 16) != 0)
			{ 
				different_part = true;
				theApp.amuledlg->AddDebugLogLine(false, "Session ended due to different file.");
			}
		} 
   	}
   	catch(...)
   	{ 
      		different_part = true; 
   	} 
//	theApp.amuledlg->AddDebugLogLine(false, "Debug: User %s, last_done_part (%u) %s (%u) next_requested_part, sent %u Kbs.", GetUserName(), last_done_part, different_part? "!=": "==", next_requested_part, this->GetTransferedUp() / 1024); 

	return different_part; 
}

bool CUpDownClient::CreateNextBlockPackage(){
	//printf("entered in : CUpDownClient::CreateNextBlockPackage\n");
	// time critical
	// check if we should kick this client
	// VQB Full Chunk Trans..
	// If is a friendslot, don't kick
	if ( !GetFriendSlot() ){
		if (theApp.glob_prefs->TransferFullChunks()) {
			// VQB to provide full chunk transfers (modified by Tarod)
			if ( theApp.uploadqueue->CheckForTimeOver(this) || IsDifferentPartBlock()) {
				SetWaitStartTime();
				theApp.uploadqueue->RemoveFromUploadQueue(this);
				theApp.uploadqueue->AddClientToQueue(this,true);
				return false;
			}
		} else {
			if (theApp.uploadqueue->CheckForTimeOver(this)) {
				// back on the waitqueue
				SetWaitStartTime();
				theApp.uploadqueue->RemoveFromUploadQueue(this);
				theApp.uploadqueue->AddClientToQueue(this,true);
				return false;
			}
		}
	}
	
	if (m_BlockRequests_queue.IsEmpty()){
		return false;
	}
	CFile file;
	byte* filedata = 0;
	char* fullname = 0;
	try{
		while (!m_BlockRequests_queue.IsEmpty()){
			Requested_Block_Struct* currentblock = m_BlockRequests_queue.GetHead();
			CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(currentblock->FileID);
			if (!srcfile)
				throw wxString(wxT("requested file not found"));

			if (srcfile->IsPartFile() && ((CPartFile*)srcfile)->GetStatus() != PS_COMPLETE){
				fullname = nstrdup(((CPartFile*)srcfile)->GetFullName());
				fullname[strlen(fullname)-4] = 0;			
			}
			else{
				fullname = new char[strlen(srcfile->GetPath())+strlen(srcfile->GetFileName().c_str())+10];
				sprintf(fullname,"%s/%s",srcfile->GetPath(),srcfile->GetFileName().GetData());
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
			  if (!file.Open(fullname,CFile::read)) //CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyNone))
					throw wxString(wxT("Failed to open requested file"));
				delete[] fullname;
				fullname = 0;
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
				delete[] fullname;
				fullname = 0;
				partfile->m_hpartfile.Seek(currentblock->StartOffset);
				
				filedata = new byte[togo+500];
				if (uint32 done = partfile->m_hpartfile.Read(filedata,togo) != togo){
					partfile->m_hpartfile.Seek(0);
					partfile->m_hpartfile.Read(filedata + done,togo-done);
				}
			}

			SetUploadFileID(currentblock->FileID);
			if (m_byDataCompVer == 1 && (!strstr(srcfile->GetFileName().c_str(),".zip")) && (!strstr(srcfile->GetFileName().c_str(),".rar")) && (!strstr(srcfile->GetFileName().c_str(),".ace")))
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
	catch(wxString error){
		theApp.amuledlg->AddDebugLogLine(false,CString(_("Client '%s' caused error while creating package (%s) - disconnecting client")),GetUserName(),error.GetData());
		theApp.uploadqueue->RemoveFromUploadQueue(this);
		if (filedata)
			delete[] filedata;
		if (fullname)
			delete[] fullname;
		return false;
	}
	//theApp.amuledlg->AddDebugLogLine(false,"Debug: Packet done. Size: %i",blockpack->GetLength());
	return true;

}

void CUpDownClient::ProcessUpFileStatus(char* packet,uint32 size){
	//printf("entered in : CUpDownClient::ProcessUpFileStatus\n");
	if (m_abyUpPartStatus) {
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;	// added by jicxicmic
	}
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;
	
	/*
	printf("ExtendedRequestsVersion = %i, size = %i\n",GetExtendedRequestsVersion(),size);
	DumpMem(packet,size);
	*/

	if( GetExtendedRequestsVersion() == 0 ) {
		return;	
	}	
	
	if( size == 16 ) {
		return;
	}	
	

	
	CSafeMemFile data((BYTE*)packet,size);
	uchar cfilehash[16];
	data.ReadRaw(cfilehash,16);
	CKnownFile* tempreqfile = theApp.sharedfiles->GetFileByID(cfilehash);
	uint16 nED2KUpPartCount;
	data.Read(nED2KUpPartCount);
	if (!nED2KUpPartCount){
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		memset(m_abyUpPartStatus,0,m_nUpPartCount);
	}
	else{
		if (tempreqfile->GetED2KPartCount() != nED2KUpPartCount){
			m_nUpPartCount = 0;
			return;
		}
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		uint16 done = 0;
		while (done != m_nUpPartCount){
			uint8 toread;
			data.Read(toread);
			for (sint32 i = 0;i != 8;i++){
				m_abyUpPartStatus[done] = ((toread>>i)&1)? 1:0;
//				We may want to use this for another feature..
//				if (m_abyUpPartStatus[done] && !tempreqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1))
//					bPartsNeeded = true;
				done++;
				if (done == m_nUpPartCount)
					break;
			}
		}
		if ((GetExtendedRequestsVersion() > 1) && (data.GetLength() - data.GetPosition() > 1)) {
			uint16 nCompleteCountLast = GetUpCompleteSourcesCount();
			uint16 nCompleteCountNew;
			data.Read(nCompleteCountNew);
			SetUpCompleteSourcesCount(nCompleteCountNew);
			if (nCompleteCountLast != nCompleteCountNew)	{
				tempreqfile->NewAvailPartsInfo();
			}
		}
	}

	theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(this);
}

void CUpDownClient::CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock){
	//printf("entered in : CUpDownClient::CreateStandartPackets\n");
	uint32 nPacketSize;

	CMemFile memfile((BYTE*)data,togo);
	if (togo > 10240) 
		nPacketSize = togo/(uint32)(togo/10240);
	else
		nPacketSize = togo;
	while (togo){
		if (togo < nPacketSize*2)
			nPacketSize = togo;
		togo -= nPacketSize;

		Packet* packet = new Packet(OP_SENDINGPART,nPacketSize+24);
		memcpy(&packet->pBuffer[0],GetUploadFileID(),16);
		uint32 statpos = ENDIAN_SWAP_32((currentblock->EndOffset - togo) - nPacketSize);	
		memcpy(&packet->pBuffer[16],&statpos,4);	
		uint32 endpos = ENDIAN_SWAP_32((currentblock->EndOffset - togo));	
		memcpy(&packet->pBuffer[20],&endpos,4);
		memfile.ReadRaw(&packet->pBuffer[24],nPacketSize);
		m_BlockSend_queue.AddTail(packet);
	}
}

void CUpDownClient::CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock){
	//printf("entered in : CUpDownClient::CreatePackedPackets\n");
	BYTE* output = new BYTE[togo+300];
	uLongf newsize = togo+300;
	uint16 result = compress2(output,&newsize,data,togo,9);
	if (result != Z_OK || togo <= newsize){
		delete[] output;
		CreateStandartPackets(data,togo,currentblock);
		return;
	}
	m_bUsedComprUp = true;
	
	CMemFile memfile(output,newsize);
	uint32 endiannewsize = ENDIAN_SWAP_32(newsize);	
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

		Packet* packet = new Packet(OP_COMPRESSEDPART,nPacketSize+24,OP_EMULEPROT);
		memcpy(&packet->pBuffer[0],GetUploadFileID(),16);
		uint32 statpos = ENDIAN_SWAP_32(currentblock->StartOffset);
		memcpy(&packet->pBuffer[16],&statpos,4);
		memcpy(&packet->pBuffer[20],&endiannewsize,4);
		memfile.ReadRaw(&packet->pBuffer[24],nPacketSize);
		m_BlockSend_queue.AddTail(packet);
	}
	delete[] output;
}

void CUpDownClient::SetUploadFileID(uchar* tempreqfileid){
	CKnownFile* newreqfile = NULL;
	if( tempreqfileid )
		newreqfile = theApp.sharedfiles->GetFileByID(tempreqfileid);
	CKnownFile* oldreqfile = theApp.sharedfiles->GetFileByID(requpfileid);
	if(newreqfile == oldreqfile)
		return;
	if(newreqfile){
		newreqfile->AddQueuedCount();
		newreqfile->AddUploadingClient(this);
		memcpy(requpfileid,tempreqfileid,16);
	}
	else{
		memset(requpfileid, 0, 16);
	}
	if(oldreqfile){
		oldreqfile->SubQueuedCount();
		oldreqfile->RemoveUploadingClient(this);
	}
}

void CUpDownClient::AddReqBlock(Requested_Block_Struct* reqblock){
	//printf("entered in : CUpDownClient::AddReqBlock\n");
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

void CUpDownClient::SetUpStartTime(uint32 dwTime){
	//printf("entered in : CUpDownClient::SetUpStartTime\n");
	if (dwTime)
		m_dwUploadTime = dwTime;
	else
		m_dwUploadTime = ::GetTickCount();
}

void CUpDownClient::SetWaitStartTime(uint32 dwTime){
	//printf("entered in : CUpDownClient::SetWaitStartTime\n");
	if (dwTime)
		m_dwWaitTime = dwTime;
	else
		m_dwWaitTime = ::GetTickCount();
}

uint32 CUpDownClient::SendBlockData(float kBpsToSend){
	//printf("entered in : CUpDownClient::SendBlockData\n");
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
		theApp.amuledlg->transferwnd->uploadlistctrl->RefreshClient(this);
	}

	kBpsUp *= lambdaAvg;	// 1st part of averaging filter; do it here in case we return "0"
	if (socket==NULL || socket->IsBusy()
		|| (m_BlockSend_queue.IsEmpty() && !CreateNextBlockPackage()) ) {
		return 0;
	}
	m_nMaxSendAllowed += bytesToSend;
	if (m_BlockSend_queue.GetHead()->GetRealPacketSize() > 0/*m_nMaxSendAllowed*3*/
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
				m_BlockSend_queue.AddHead(new Packet(pBuffer2,nNewSize,bLast));
				bLast = false;
			}
			delete[] pBuffer;
	}
	while (!m_BlockSend_queue.IsEmpty()
		&& m_BlockSend_queue.GetHead()->GetRealPacketSize() <= m_nMaxSendAllowed){
			
		Packet* tosend = m_BlockSend_queue.RemoveHead();
		uint32 nBlockSize = tosend->GetRealPacketSize();
		m_nMaxSendAllowed -= nBlockSize;
//		theApp.uploadqueue->AddUpDataOverheadOther(0, 24);
		socket->SendPacket(tosend,true,false);
		m_nTransferedUp += nBlockSize;
		theApp.UpdateSentBytes(nBlockSize);
		credits->AddUploaded(nBlockSize, GetIP());
		if (m_BlockSend_queue.IsEmpty())
			CreateNextBlockPackage();
	}
	kBpsUp += kBpsToSend*(1.0-lambdaAvg); // 2nd part of averaging filter
	return bytesToSend;
}

void CUpDownClient::FlushSendBlocks(){ // call this when you stop upload, or the socket might be not able to send
	//printf("entered in : CUpDownClient::FlushSendBlocks\n");
	bool bBreak = false;
	while (!m_BlockSend_queue.IsEmpty() && m_BlockSend_queue.GetHead()->IsSplitted() && socket && socket->IsConnected() && !bBreak ){	
		Packet* tosend = m_BlockSend_queue.RemoveHead();
		//bool bBreak = tosend->IsLastSplitted();
		theApp.uploadqueue->AddUpDataOverheadOther(tosend->size);
		socket->SendPacket(tosend,true,false);
	}
}

void CUpDownClient::SendHashsetPacket(char* forfileid){
	//printf("entered in : CUpDownClient::SendHashsetPacket\n");
	CKnownFile* file = theApp.sharedfiles->GetFileByID((uchar*)forfileid);
	if (!file) {
		theApp.amuledlg->AddLogLine(false, CString(_("requested file not found")));
		return;
	}

	CMemFile* data = new CMemFile();
	data->WriteRaw(file->GetFileHash(),16);
	uint16 parts = file->GetHashCount();
	data->Write(parts);
	for (int i = 0; i != parts; i++)
		data->WriteRaw(file->GetPartHash(i),16);
	Packet* packet = new Packet(data);
	packet->opcode = OP_HASHSETANSWER;
	delete data;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::ClearUploadBlockRequests(){
	//printf("entered in : CUpDownClient::ClearUploadBlockRequests\n");
	FlushSendBlocks();
	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition();pos != 0;m_BlockRequests_queue.GetNext(pos))
		delete m_BlockRequests_queue.GetAt(pos);
	m_BlockRequests_queue.RemoveAll();
	
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0;m_DoneBlocks_list.GetNext(pos))
		delete m_DoneBlocks_list.GetAt(pos);
	m_DoneBlocks_list.RemoveAll();
	
	for (POSITION pos = m_BlockSend_queue.GetHeadPosition();pos != 0;m_BlockSend_queue.GetNext(pos))
		delete m_BlockSend_queue.GetAt(pos);
	m_BlockSend_queue.RemoveAll();
}

void CUpDownClient::SendRankingInfo(){
	//printf("entered in : CUpDownClient::SendRankingInfo\n");
	if (!ExtProtocolAvailable())
		return;
	uint16 nRank = theApp.uploadqueue->GetWaitingPosition(this);
	if (!nRank)
		return;
		
	CMemFile data;
	data.Write(nRank);
	// Kry: what are these zero bytes for. are they really correct?
	// Kry - Well, eMule does like that. I guess they're ok.
	data.Write((uint32)0); data.Write((uint32)0); data.Write((uint16)0);
	Packet* packet = new Packet(&data,OP_EMULEPROT);
	packet->opcode = OP_QUEUERANKING;
//	Packet* packet = new Packet(OP_QUEUERANKING,12,OP_EMULEPROT);
//	memset(packet->pBuffer,0,12);
//	memcpy(packet->pBuffer+0,&nRank,2);
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::SendCommentInfo(CKnownFile *file)
{
	if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1) {
		return;
	}
	m_bCommentDirty = false;

	int8 rating=file->GetFileRate();
	CString desc=file->GetFileComment();
	if(file->GetFileRate() == 0 && desc.IsEmpty()) {
		return;
	}
	CMemFile data;
	data.Write(rating);
	data.Write(desc.Left(128));
	Packet *packet = new Packet(&data,OP_EMULEPROT);
	packet->opcode = OP_FILEDESC;
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true);
}

void  CUpDownClient::AddRequestCount(uchar* fileid){
	//printf("entered in : CUpDownClient::AddRequestCount\n");
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;m_RequestedFiles_list.GetNext(pos)){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetAt(pos);
		if (!memcmp(cur_struct->fileid,fileid,16)){
			if (::GetTickCount() - cur_struct->lastasked < MIN_REQUESTTIME && !GetFriendSlot()){ 
				if (GetDownloadState() != DS_DOWNLOADING)
					cur_struct->badrequests++;
				if (cur_struct->badrequests == BADCLIENTBAN){
					Ban();
				}
			}
			else{
				if (cur_struct->badrequests)
					cur_struct->badrequests--;
			}
			cur_struct->lastasked = ::GetTickCount();
			return;
		}
	}
	Requested_File_Struct* new_struct = new Requested_File_Struct;
	memset(new_struct,0,sizeof(Requested_File_Struct));
	memcpy(new_struct->fileid,fileid,16);
	new_struct->lastasked = ::GetTickCount();
	m_RequestedFiles_list.AddHead(new_struct);
}

void  CUpDownClient::UnBan(){
	//printf("entered in : CUpDownClient::UnBan\n");
	m_bBanned = false;
	m_dwBanTime = 0;
	SetWaitStartTime();
	theApp.uploadqueue->UpdateBanCount();
	theApp.amuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;m_RequestedFiles_list.GetNext(pos)){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetAt(pos);
		cur_struct->badrequests = 0;
		cur_struct->lastasked = 0;	
	}
	//theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(this, true, true);
}

void CUpDownClient::Ban(){
	//printf("entered in : CUpDownClient::Ban\n");
	m_bBanned = true;
	theApp.uploadqueue->UpdateBanCount();
	theApp.amuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	m_dwBanTime = ::GetTickCount();
	theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(this);
	theApp.amuledlg->AddDebugLogLine(false,CString(_("Client '%s' seems to be an aggressive client and is banned from the uploadqueue")),GetUserName());
}

void CUpDownClient::UDPFileReasked(){
	//printf("entered in : CUpDownClient::UDPFileReasked\n");
	AddAskedCount();
	SetLastUpRequest();
	uint16 nRank = theApp.uploadqueue->GetWaitingPosition(this);
	Packet* response = new Packet(OP_REASKACK,2,OP_EMULEPROT);
	memcpy(response->pBuffer,&nRank,2);
	theApp.clientudp->SendPacket(response,GetIP(),GetUDPPort());
}

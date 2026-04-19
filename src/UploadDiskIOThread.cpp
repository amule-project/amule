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

#include "UploadDiskIOThread.h"

#include "updownclient.h"		// Needed for CUpDownClient
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "KnownFile.h"			// Needed for CKnownFile
#include "PartFile.h"			// Needed for CPartFile
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "Packet.h"				// Needed for CPacket
#include "MemFile.h"			// Needed for CMemFile
#include "amule.h"				// Needed for theApp
#include "Logger.h"
#include "OtherFunctions.h"		// Needed for GetFiletype / ftArchive
#include "MD4Hash.h"
#include "ScopedPtr.h"			// Needed for CScopedArray
#include "UploadBandwidthThrottler.h"
#include "Statistics.h"			// Needed for theStats

#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <algorithm>			// Needed for std::min / std::max
#include <zlib.h>

// eMule ref: UploadDiskIOThread.cpp:43-45
#define SLOT_COMPRESSIONCHECK_DATARATE	(1024*150)	// 150 KB/s — above this we may disable compression
#define MAX_FINISHED_REQUESTS_COMPRESSION	15			// max queued finished reads before disabling compression
#define BIGBUFFER_MINDATARATE			(75 * 1024)	// eMule: BIGBUFFER_MINDATARATE


// eMule ref: CUploadDiskIOThread::CUploadDiskIOThread() — line 49
CUploadDiskIOThread::CUploadDiskIOThread()
	: wxThread(wxTHREAD_JOINABLE)
	, m_condition(m_mutex)
{
	m_bRun = false;
	m_bSignalThrottler = false;
	m_bNewBlocksPending = false;
	m_bSocketNeedsPending = false;

	wxMutexLocker lock(m_mutex);
	if (Create() == wxTHREAD_NO_ERROR) {
		Run();
	}
}

CUploadDiskIOThread::~CUploadDiskIOThread()
{
	wxASSERT( !m_bRun );
}


// eMule ref: CUploadDiskIOThread::EndThread() — line 77
void CUploadDiskIOThread::EndThread()
{
	{
		wxMutexLocker lock(m_mutex);
		m_bRun = false;
		m_condition.Signal();
	}
	Wait();	// join — replaces m_eventThreadEnded->Lock()
}


// eMule ref: CUploadDiskIOThread::NewBlockRequestsAvailable() — UploadDiskIOThread.h:55
// Called by main thread when new block requests are added for a client.
// Uses a sticky flag so the signal is not lost if the thread is between iterations
// (not yet sleeping in WaitTimeout). Without the flag, wxCondition::Signal() is a
// pure pulse — it is dropped if no thread is currently waiting.
void CUploadDiskIOThread::NewBlockRequestsAvailable()
{
	wxMutexLocker lock(m_mutex);
	m_bNewBlocksPending = true;
	m_condition.Signal();
}


// eMule ref: CUploadDiskIOThread::SocketNeedsMoreData() — UploadDiskIOThread.h:56
// Called by throttler when it drains a socket and needs more data.
void CUploadDiskIOThread::SocketNeedsMoreData()
{
	wxMutexLocker lock(m_mutex);
	m_bSocketNeedsPending = true;
	m_condition.Signal();
}


// eMule ref: CUploadDiskIOThread::RunInternal() — line 84
void* CUploadDiskIOThread::Entry()
{
	m_bRun = true;

	while (m_bRun)	// eMule ref: line 88
	{
		// eMule ref: lines 92-109 — reset events, lock upload list, iterate clients
		{
			wxMutexLocker uploadLock(theApp->uploadqueue->GetUploadingListLock());
			const CClientRefList& uploadList = theApp->uploadqueue->GetUploadingList();

			for (CClientRefList::const_iterator it = uploadList.begin(); it != uploadList.end(); ++it)
			{
				CUpDownClient* client = it->GetClient();
				if (client != NULL && client->GetSocket() != NULL && client->IsConnected())
				{
					StartCreateNextBlockPackage(client);
				}
			}
		}

		// eMule ref: lines 112-143 — drain pending IO / finished IO
		// Simplified: reads are synchronous, so m_listPendingIO doesn't exist.
		// All completed reads are already in m_listFinishedIO.
		while (!m_listFinishedIO.empty())	// eMule ref: line 142
		{
			ReadRequest_Struct* req = m_listFinishedIO.front();
			m_listFinishedIO.pop_front();
			ReadCompletionRoutine(req);
		}

		// eMule ref: lines 146-149 — signal throttler if we put new data on a socket
		if (m_bSignalThrottler && theApp->uploadBandwidthThrottler != NULL)
		{
			theApp->uploadBandwidthThrottler->NewUploadDataAvailable();
			m_bSignalThrottler = false;
		}

		// eMule ref: line 152-155 — WaitForMultipleObjects(events, 500ms)
		// Replaced with: wxCondition::WaitTimeout(500ms) with sticky-flag check.
		// wxCondition::Signal() is a pure pulse — it is dropped if no thread is
		// currently blocked in WaitTimeout(). The sticky flags (m_bNewBlocksPending,
		// m_bSocketNeedsPending) are set under m_mutex by callers, so we check them
		// before sleeping and skip the wait if a signal arrived since last iteration.
		// 500ms matches eMule's WaitForMultipleObjects timeout.
		{
			wxMutexLocker lock(m_mutex);
			if (m_bRun && !m_bNewBlocksPending && !m_bSocketNeedsPending) {
				m_condition.WaitTimeout(500);
			}
			m_bNewBlocksPending = false;
			m_bSocketNeedsPending = false;
		}
	}

	// Cleanup — eMule ref: lines 157-180
	// No overlapped I/O to cancel. Just clear the open files list.
	for (std::list<OpenFile_Struct*>::iterator it = m_listOpenFiles.begin(); it != m_listOpenFiles.end(); ++it) {
		delete *it;
	}
	m_listOpenFiles.clear();

	// Discard any unprocessed finished reads
	for (std::list<ReadRequest_Struct*>::iterator it = m_listFinishedIO.begin(); it != m_listFinishedIO.end(); ++it) {
		delete *it;
	}
	m_listFinishedIO.clear();

	return NULL;
}


// eMule ref: CUploadDiskIOThread::StartCreateNextBlockPackage() — line 185
void CUploadDiskIOThread::StartCreateNextBlockPackage(CUpDownClient* client)
{
	// eMule ref: lines 188-189 — lock block lists
	wxMutexLocker lockBlockLists(client->m_blockListLock);

	// eMule ref: lines 192-207 — check payload buffer, early return if full
	// eMule ref: lines 193-196 — GetQueueSessionPayloadUp() is probably outdated, so also
	// add the value reported by the socket as sent since the last timer tick.
	// PeekSentPayload() is non-resetting (does not consume the counter used by SendBlockData())
	// and is protected by m_sendLocker. Calling from the disk thread is safe: we hold
	// uploadLock (taken by Entry() before calling this function), which prevents the socket
	// from being freed — disconnect/cleanup requires uploadLock. (eMule ref: line 187)
	sint64 nCurQueueSessionPayloadUp = client->m_nCurQueueSessionPayloadUp;
	CClientTCPSocket* pSock = client->GetSocket();
	if (pSock != NULL)
		nCurQueueSessionPayloadUp += (sint64)pSock->PeekSentPayload();
	sint64 addedPayloadQueueSession = client->m_addedPayloadQueueSession;

	// eMule ref: lines 199-201
	bool bFastUpload = client->GetUploadDatarate() > BIGBUFFER_MINDATARATE;
	const uint32 nBufferLimit = bFastUpload ? ((5 * EMBLOCKSIZE) + 1) : (EMBLOCKSIZE + 1);

	if (client->m_BlockRequests_queue.empty() ||
		(addedPayloadQueueSession > nCurQueueSessionPayloadUp &&
		 (uint32)(addedPayloadQueueSession - nCurQueueSessionPayloadUp) > nBufferLimit))
	{
		return;
	}

	try {
		// eMule ref: lines 211-212 — buffer while below limit
		while (!client->m_BlockRequests_queue.empty() &&
			   (addedPayloadQueueSession <= nCurQueueSessionPayloadUp ||
			    (uint32)(addedPayloadQueueSession - nCurQueueSessionPayloadUp) < nBufferLimit))
		{
			Requested_Block_Struct* currentblock = client->m_BlockRequests_queue.front();

			// eMule ref: lines 215-223 — check file ID switch; defer to main thread
			if (md4cmp(currentblock->FileID, client->GetUploadFileID().GetHash()) != 0)
			{
				AddDebugLogLineN(logClient, wxT("CUploadDiskIOThread::StartCreateNextBlockPackage: Switched fileid, waiting for mainthread"));
				return;
			}

			// eMule ref: lines 227-244 — resolve file from shared list
			CKnownFile* srcfile = theApp->sharedfiles->GetFileByID(CMD4Hash(currentblock->FileID));
			if (srcfile == NULL) {
				throw wxString(wxT("requested file not found"));
			}

			CPartFile* srcPartFile = srcfile->IsPartFile() ? static_cast<CPartFile*>(srcfile) : NULL;

			// eMule ref: lines 247-252 — validate block offsets
			if (currentblock->EndOffset > srcfile->GetFileSize()) {
				throw wxString(CFormat(wxT("Asked for data up to %d beyond end of file (%d)"))
					% currentblock->EndOffset % srcfile->GetFileSize());
			} else if (currentblock->StartOffset > currentblock->EndOffset) {
				throw wxString(CFormat(wxT("Asked for invalid block (start %d > end %d)"))
					% currentblock->StartOffset % currentblock->EndOffset);
			}

			uint64 togo = currentblock->EndOffset - currentblock->StartOffset;
			if (togo > EMBLOCKSIZE * 3) {
				throw wxString(CFormat(wxT("Client requested too large block (%d > %d)"))
					% togo % (EMBLOCKSIZE * 3));
			}

			// eMule ref: lines 256-298 — find/create file struct in m_listOpenFiles
			// In eMule this opens a HANDLE; here we track per-file metadata only.
			// CFileArea opens the file internally as needed.
			OpenFile_Struct* pFileStruct = NULL;
			for (std::list<OpenFile_Struct*>::iterator it = m_listOpenFiles.begin(); it != m_listOpenFiles.end(); ++it)
			{
				if (md4cmp((*it)->ucMD4FileHash, currentblock->FileID) == 0) {
					pFileStruct = *it;
					break;
				}
			}
			if (pFileStruct == NULL)
			{
				pFileStruct = new OpenFile_Struct;
				md4cpy(pFileStruct->ucMD4FileHash, currentblock->FileID);
				pFileStruct->nInUse = 0;
				pFileStruct->bCompress = (GetFiletype(srcfile->GetFileName()) != ftArchive);
				pFileStruct->uFileSize = (uint64)srcfile->GetFileSize();
				m_listOpenFiles.push_back(pFileStruct);
			}

			// eMule ref: lines 301-345 — ReadFile(OVERLAPPED) → replaced with CFileArea::ReadAt()
			// Read is synchronous on this thread; go straight to m_listFinishedIO (no pending list).
			ReadRequest_Struct* req = new ReadRequest_Struct;
			req->pFileStruct = pFileStruct;
			req->pClient = client;
			req->uStartOffset = currentblock->StartOffset;
			req->uEndOffset = currentblock->EndOffset;
			req->pBlock = currentblock;  // snapshot before moving to DoneBlocks_list

			if (srcPartFile) {
				if (!srcPartFile->IsComplete(currentblock->StartOffset, currentblock->EndOffset - 1)) {
					delete req;
					throw wxString(CFormat(wxT("Asked for incomplete block (%d - %d)"))
						% currentblock->StartOffset % (currentblock->EndOffset - 1));
				}
				if (!srcPartFile->ReadData(req->area, currentblock->StartOffset, (uint32)togo)) {
					delete req;
					throw wxString(wxT("Failed to read from requested partfile"));
				}
			} else {
				CFileAutoClose file;
				CPath fullname = srcfile->GetFilePath().JoinPaths(srcfile->GetFileName());
				if (!file.Open(fullname, CFile::read)) {
					AddLogLineN(CFormat(_("Failed to open file (%s), removing from list of shared files.")) % srcfile->GetFileName());
					theApp->sharedfiles->RemoveFile(srcfile);
					delete req;
					throw wxString(wxT("Failed to open requested file"));
				}
				req->area.ReadAt(file, currentblock->StartOffset, (uint32)togo);
			}
			req->area.CheckError();

			pFileStruct->nInUse++;

			// Set upload file ID on the client — mirrors eMule's SetUploadFileID call in the main thread path
			client->SetUploadFileID(srcfile);

			// eMule ref: line 343 — add to m_listFinishedIO (skipping m_listPendingIO)
			m_listFinishedIO.push_back(req);

			// eMule ref: lines 347-349
			addedPayloadQueueSession += togo;
			client->m_addedPayloadQueueSession += togo;
			srcfile->statistic.AddTransferred(togo);
			client->m_DoneBlocks_list.push_front(client->m_BlockRequests_queue.front());
			client->m_BlockRequests_queue.pop_front();
		}
	} catch (const wxString& DEBUG_ONLY(error)) {
		AddDebugLogLineN(logClient, CFormat(wxT("CUploadDiskIOThread: error for client '%s': %s"))
			% client->GetUserName() % error);
		client->m_bIOError = true;
	} catch (const CIOFailureException& error) {
		AddDebugLogLineC(logClient, wxT("CUploadDiskIOThread: IO failure: ") + error.what());
		client->m_bIOError = true;
	} catch (const CEOFException&) {
		AddDebugLogLineN(logClient, wxT("CUploadDiskIOThread: EOF reading block"));
		client->m_bIOError = true;
	}
}


// eMule ref: CUploadDiskIOThread::ReadCompletetionRoutine() — line 369
void CUploadDiskIOThread::ReadCompletionRoutine(ReadRequest_Struct* req)
{
	if (req == NULL) {
		wxASSERT(false);
		return;
	}

	bool bError = false;

	// eMule ref: lines 388-406 — check client is still in upload list
	// Hold uploadLock through SendPacket to prevent the socket from being freed
	// by a concurrent disconnect; matches eMule's design (lock scope to line 482).
	{
		wxMutexLocker uploadLock(theApp->uploadqueue->GetUploadingListLock());
		const CClientRefList& uploadList = theApp->uploadqueue->GetUploadingList();

		bool bFound = false;
		for (CClientRefList::const_iterator it = uploadList.begin(); it != uploadList.end(); ++it)
		{
			if (it->GetClient() == req->pClient) {
				bFound = true;
				break;
			}
		}

		if (!bFound) {
			AddDebugLogLineN(logClient, wxT("CUploadDiskIOThread::ReadCompletionRoutine: Client not found in uploadlist anymore, discarding block"));
			bError = true;
		}

		// eMule ref: lines 408-482 — create packets and send
		if (!bError)
		{
			CUpDownClient* client = req->pClient;
			CClientTCPSocket* pSocket = client->GetSocket();

			// eMule ref: lines 420-422 — check socket still connected
			if (pSocket == NULL || !client->IsConnected()) {
				AddDebugLogLineN(logClient, wxT("CUploadDiskIOThread::ReadCompletionRoutine: Client has no connected socket"));
				bError = true;
				client->m_bIOError = true;
			}

			if (!bError)
			{
				// eMule ref: lines 428-447 — decide compression
				// Disable compression if socket is starved and datarate is high.
				// Once disabled for a client, stays disabled for the session.
				bool bUseCompression = false;
				if (!client->m_bDisableCompression && req->pFileStruct->bCompress && client->m_byDataCompVer == 1)
				{
					if ((sint32)m_listFinishedIO.size() > MAX_FINISHED_REQUESTS_COMPRESSION &&
						theStats::GetUploadRate() > SLOT_COMPRESSIONCHECK_DATARATE)
					{
						client->m_bDisableCompression = true;
					}
					else if (client->GetUploadDatarate() > SLOT_COMPRESSIONCHECK_DATARATE &&
						     pSocket != NULL && !pSocket->HasQueues(true) && !pSocket->IsBusyQuickCheck())
					{
						client->m_bDisableCompression = true;
					}
					else {
						bUseCompression = true;
					}
				}

				// eMule ref: lines 454-470 — CreateStandardPackets() or CreatePackedPackets()
				// Build packets into a local list, then send them out.
				// File ID was set in StartCreateNextBlockPackage when srcfile was resolved.
				CPacketList packetList;
				uint32 datarate = client->GetUploadDatarate();
				if (bUseCompression) {
					CreatePackedPackets(req->area.GetBuffer(),
						req->uStartOffset, req->uEndOffset, packetList,
						req->pFileStruct->ucMD4FileHash, datarate);
				} else {
					CreateStandardPackets(req->area.GetBuffer(),
						req->uStartOffset, req->uEndOffset, packetList,
						req->pFileStruct->ucMD4FileHash, datarate);
				}

				// eMule ref: lines 478-482 — send all packets
				for (CPacketList::iterator it = packetList.begin(); it != packetList.end(); ++it)
				{
					theStats::AddUploadToSoft(client->GetClientSoft(), it->second);
					pSocket->SendPacket(it->first, true, false, it->second);
				}

				// eMule ref: line 471
				m_bSignalThrottler = true;
			}
		}
	} // uploadLock released here

	// eMule ref: line 493 — ReleaseOvOpenFile
	ReleaseOpenFile(req->pFileStruct);
	delete req;
}


// eMule ref: CUploadDiskIOThread::ReleaseOvOpenFile() — line 498
bool CUploadDiskIOThread::ReleaseOpenFile(OpenFile_Struct* pFileStruct)
{
	for (std::list<OpenFile_Struct*>::iterator it = m_listOpenFiles.begin(); it != m_listOpenFiles.end(); ++it)
	{
		if (*it == pFileStruct) {
			pFileStruct->nInUse--;
			if (pFileStruct->nInUse == 0) {
				// eMule: CloseHandle(hFile). We have no persistent handle — CFileArea already closed.
				m_listOpenFiles.erase(it);
				delete pFileStruct;
			}
			return true;
		}
	}
	wxASSERT(false);
	return false;
}


// eMule 0.70b ref: CUploadDiskIOThread::CreateStandardPackets()
void CUploadDiskIOThread::CreateStandardPackets(const uint8_t* buffer, uint64 startOffset, uint64 endOffset, CPacketList& packetList, const uint8_t* fileHash, uint32 uploadDatarate)
{
	uint32 togo = (uint32)(endOffset - startOffset);

	CMemFile memfile(buffer, togo);
	// Adaptive chunk size: scale with per-slot speed, floor 10 KiB, ceil EMBLOCKSIZE.
	// /8 => ~125 ms of data per chunk; enough to saturate a TCP segment burst
	// without making per-packet latency awful on slow peers.
	// uploadDatarate is 0 at session start; the floor keeps behavior sane.
	// Ceiling at EMBLOCKSIZE (180 KiB): one packet per block — no benefit
	// in going higher since the receiver requests blocks of this size.
	const uint32 chunkSize = std::min(std::max(uploadDatarate / 8u, 10240u), (uint32)EMBLOCKSIZE);
	uint32 nPacketSize = (togo <= chunkSize + 2600u) ? togo : chunkSize;

	while (togo){
		if (togo < nPacketSize*2) {
			nPacketSize = togo;
		}

		wxASSERT(nPacketSize);
		togo -= nPacketSize;

		uint64 endpos = (endOffset - togo);
		uint64 startpos = endpos - nPacketSize;

		bool bLargeBlocks = (startpos > 0xFFFFFFFF) || (endpos > 0xFFFFFFFF);

		CMemFile data(nPacketSize + 16 + 2 * (bLargeBlocks ? 8 : 4));
		data.WriteHash(CMD4Hash(fileHash));
		if (bLargeBlocks) {
			data.WriteUInt64(startpos);
			data.WriteUInt64(endpos);
		} else {
			data.WriteUInt32(startpos);
			data.WriteUInt32(endpos);
		}
		char *tempbuf = new char[nPacketSize];
		memfile.Read(tempbuf, nPacketSize);
		data.Write(tempbuf, nPacketSize);
		delete [] tempbuf;
		CPacket* packet = new CPacket(data, (bLargeBlocks ? OP_EMULEPROT : OP_EDONKEYPROT), (bLargeBlocks ? (uint8)OP_SENDINGPART_I64 : (uint8)OP_SENDINGPART));
		theStats::AddUpOverheadFileRequest(16 + 2 * (bLargeBlocks ? 8 : 4));
		packetList.push_back(std::make_pair(packet, nPacketSize));
	}
}


// eMule 0.70b ref: CUploadDiskIOThread::CreatePackedPackets()
void CUploadDiskIOThread::CreatePackedPackets(const uint8_t* buffer, uint64 startOffset, uint64 endOffset, CPacketList& packetList, const uint8_t* fileHash, uint32 uploadDatarate)
{
	uint32 togo = (uint32)(endOffset - startOffset);
	uLongf newsize = togo+300;
	CScopedArray<uint8_t> output(newsize);
	// eMule 0.70b: use compression level 1 instead of 9 — for typical 10240-byte
	// blocks the size difference is small (~4-12%) but level 1 is 1.5-2.5x faster.
	uint16 result = compress2(output.get(), &newsize, buffer, togo, 1);
	if (result != Z_OK || togo <= newsize){
		CreateStandardPackets(buffer, startOffset, endOffset, packetList, fileHash, uploadDatarate);
		return;
	}

	CMemFile memfile(output.get(), newsize);

	uint32 totalPayloadSize = 0;
	uint32 oldSize = togo;
	togo = newsize;
	// Adaptive chunk size — see CreateStandardPackets for rationale.
	const uint32 chunkSize = std::min(std::max(uploadDatarate / 8u, 10240u), (uint32)EMBLOCKSIZE);
	uint32 nPacketSize = (togo <= chunkSize + 2600u) ? togo : chunkSize;

	while (togo) {
		if (togo < nPacketSize*2) {
			nPacketSize = togo;
		}
		togo -= nPacketSize;

		bool isLargeBlock = (startOffset > 0xFFFFFFFF) || (endOffset > 0xFFFFFFFF);

		CMemFile data(nPacketSize + 16 + (isLargeBlock ? 12 : 8));
		data.WriteHash(CMD4Hash(fileHash));
		if (isLargeBlock) {
			data.WriteUInt64(startOffset);
		} else {
			data.WriteUInt32(startOffset);
		}
		data.WriteUInt32(newsize);
		char *tempbuf = new char[nPacketSize];
		memfile.Read(tempbuf, nPacketSize);
		data.Write(tempbuf,nPacketSize);
		delete [] tempbuf;
		CPacket* packet = new CPacket(data, OP_EMULEPROT, (isLargeBlock ? OP_COMPRESSEDPART_I64 : OP_COMPRESSEDPART));

		// approximate payload size
		uint32 payloadSize = nPacketSize*oldSize/newsize;

		if (togo == 0 && totalPayloadSize+payloadSize < oldSize) {
			payloadSize = oldSize-totalPayloadSize;
		}

		totalPayloadSize += payloadSize;

		theStats::AddUpOverheadFileRequest(24);
		packetList.push_back(std::make_pair(packet, payloadSize));
	}
}
// File_checked_for_headers

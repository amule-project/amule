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

#ifndef UPLOADDISKIOTHREAD_H
#define UPLOADDISKIOTHREAD_H

#include <list>
#include <utility>		// Needed for std::pair

#include <wx/thread.h>

#include "Types.h"
#include "FileArea.h"	// Needed for CFileArea

class CPacket;
class CUpDownClient;

// Mirrors eMule's OpenOvFile_Struct (UploadDiskIOThread.h:20-28)
// HANDLE hFile replaced with aMule's file-open-on-demand model:
// we don't cache file handles; CFileArea opens/closes as needed.
struct OpenFile_Struct
{
	uint8    ucMD4FileHash[16];
	uint32   nInUse;
	uint64   uFileSize;
	bool     bCompress;             // true if this file type should be compressed
};

struct Requested_Block_Struct;

// Mirrors eMule's OverlappedEx_Struct (UploadDiskIOThread.h:32-41)
// OVERLAPPED removed — reads are synchronous on this thread via CFileArea.
struct ReadRequest_Struct
{
	OpenFile_Struct*          pFileStruct;
	CUpDownClient*            pClient;
	uint64                    uStartOffset;
	uint64                    uEndOffset;
	Requested_Block_Struct*   pBlock;      // the block this IO is for (set in StartCreateNextBlockPackage)
	CFileArea                 area;        // holds read buffer; replaces BYTE* pBuffer + OVERLAPPED
};

// Packet + payload-size pair, used by the static packet-creation helpers.
// Mirrors eMule's CPacketList + Packet::uStatsPayLoad approach; aMule's CPacket
// has no uStatsPayLoad member so we carry the value alongside the pointer.
typedef std::list< std::pair<CPacket*, uint32> > CPacketList;

// Port of eMule's CUploadDiskIOThread (UploadDiskIOThread.h:48-86).
// Windows primitives replaced with wxWidgets equivalents:
//   CWinThread           -> wxThread (joinable)
//   CEvent               -> wxCondition + wxMutex
//   WaitForMultipleObjects -> wxCondition::WaitTimeout()
//   ReadFile(OVERLAPPED) -> CFileArea::ReadAt() (synchronous, on this thread)
//   HANDLE file handles  -> aMule's CFileAutoClose (open per block)
//   CCriticalSection     -> wxMutex + wxMutexLocker
class CUploadDiskIOThread : public wxThread
{
public:
	CUploadDiskIOThread();
	~CUploadDiskIOThread();

	void EndThread();                  // eMule ref: UploadDiskIOThread.cpp:77
	void NewBlockRequestsAvailable();  // eMule ref: UploadDiskIOThread.h:55
	void SocketNeedsMoreData();        // eMule ref: UploadDiskIOThread.h:56

	// eMule ref: UploadDiskIOThread.h:72-73 — static packet creation helpers
	// uploadDatarate (bytes/s) scales per-packet chunk size (10 KiB floor, 128 KiB ceiling).
	static void CreateStandardPackets(const uint8_t* buffer, uint64 startOffset, uint64 endOffset, CPacketList& packetList, const uint8_t* fileHash, uint32 uploadDatarate = 0);
	static void CreatePackedPackets(const uint8_t* buffer, uint64 startOffset, uint64 endOffset, CPacketList& packetList, const uint8_t* fileHash, uint32 uploadDatarate = 0);

private:
	void* Entry() override;            // replaces RunProc/RunInternal

	void StartCreateNextBlockPackage(CUpDownClient* client);           // eMule ref: line 185
	void ReadCompletionRoutine(ReadRequest_Struct* req);               // eMule ref: line 369
	bool ReleaseOpenFile(OpenFile_Struct* pFileStruct);                // eMule ref: line 498

	volatile bool   m_bRun;            // eMule ref: m_bRun (line 77)
	bool            m_bSignalThrottler;// eMule ref: m_bSignalThrottler (line 78)

	wxMutex         m_mutex;
	wxCondition     m_condition;       // replaces m_eventNewBlockRequests + m_eventSocketNeedsData
	// Sticky flags: set by Signal() callers, consumed by Entry() before sleeping.
	// Prevents lost wakeups when signal arrives before WaitTimeout() is entered.
	bool            m_bNewBlocksPending;   // set by NewBlockRequestsAvailable()
	bool            m_bSocketNeedsPending; // set by SocketNeedsMoreData()
	// m_eventAsyncIOFinished not needed — reads complete synchronously on this thread

	std::list<OpenFile_Struct*>       m_listOpenFiles;   // eMule ref: line 83
	std::list<ReadRequest_Struct*>    m_listFinishedIO;  // eMule ref: line 85
	// m_listPendingIO not needed — reads are synchronous, go straight to m_listFinishedIO
};

#endif
// File_checked_for_headers

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

#include "PartFileWriteThread.h"

#include "PartFile.h"		// Needed for CPartFile, PartFileBufferedData
#include "CFile.h"			// Needed for CIOFailureException
#include "Logger.h"
#include <common/Format.h>	// Needed for CFormat

// eMule ref: CPartFileWriteThread::CPartFileWriteThread() — line 41
CPartFileWriteThread::CPartFileWriteThread()
	: wxThread(wxTHREAD_JOINABLE)
	, m_condition(m_mutex)
{
	m_bRun = false;
	m_bWorkPending = false;

	wxMutexLocker lock(m_mutex);
	if (Create() == wxTHREAD_NO_ERROR) {
		Run();
	}
}


CPartFileWriteThread::~CPartFileWriteThread()
{
	// EndThread() must have been called before destruction.
}


// eMule ref: CPartFileWriteThread::EndThread() — line 62
void CPartFileWriteThread::EndThread()
{
	{
		wxMutexLocker lock(m_mutex);
		m_bRun = false;
		m_bWorkPending = true;
		m_condition.Signal();
	}
	Wait();
}


// eMule ref: CPartFileWriteThread::WakeUpCall() — line 230
// Called by the main thread to queue a write item.
void CPartFileWriteThread::QueueWrite(CPartFile* pFile, PartFileBufferedData* pBuffer)
{
	wxMutexLocker lock(m_mutex);
	m_flushList.push_back(ToWrite{ pFile, pBuffer });
	m_bWorkPending = true;
	m_condition.Signal();
}


// eMule ref: CPartFileWriteThread::RunInternal() — line 69
// Replaces IOCP + overlapped WriteFile with synchronous CFileArea::FlushAt().
// The thread is dedicated to writes, so blocking on disk I/O is acceptable —
// the key win is that the main thread no longer stalls.
void* CPartFileWriteThread::Entry()
{
	m_bRun = true;

	while (m_bRun)
	{
		// Move queued items to a local work list under the lock.
		// This minimises lock hold time — main thread can keep queueing
		// while we process the local list.
		std::list<ToWrite> workList;
		{
			wxMutexLocker lock(m_mutex);
			if (m_bRun && !m_bWorkPending) {
				m_condition.WaitTimeout(500);
			}
			m_bWorkPending = false;
			workList.swap(m_flushList);
		}

		// Process all queued writes synchronously.
		// eMule ref: WriteBuffers() — line 122
		for (std::list<ToWrite>::iterator it = workList.begin();
			 it != workList.end() && m_bRun; ++it)
		{
			PartFileBufferedData* pBuffer = it->pBuffer;
			uint32 lenData = (uint32)(pBuffer->end - pBuffer->start + 1);

			// Synchronous write via CFileArea (replaces eMule's overlapped WriteFile).
			// CFileArea::FlushAt() writes the buffered data at the given offset.
			//
			// Lock m_hpartfileMutex against CPartFileHashThread (see
			// CPartFile::m_hpartfileMutex): with ENABLE_MMAP=OFF the
			// underlying CFileAutoClose::WriteAt does Seek+Write on the
			// shared fd, and HashSinglePart on the hash thread does
			// Seek+Read on the same fd; the two race on file position
			// without the lock.
			//
			// FlushAt can also throw CIOFailureException on a disk-full /
			// EIO / permission failure.  Catching it here keeps the
			// worker thread alive: an unhandled exception in Entry()
			// propagates through wxThreadInternal::PthreadStart() to
			// wxApp::OnUnhandledException(), which std::set_terminate's
			// MuleDebug aborts the process.  On caught failure we mark
			// the buffered item PB_ERROR so the main thread can retry on
			// the next FlushBuffer (where CheckFreeDiskSpace will pause
			// the file if disk is genuinely exhausted).
			bool writeOk = true;
			try {
				std::lock_guard<std::mutex> lock(it->pFile->m_hpartfileMutex);
				pBuffer->area.FlushAt(it->pFile->m_hpartfile, pBuffer->start, lenData);
			} catch (const CIOFailureException& e) {
				AddDebugLogLineC(logPartFile, CFormat(
					"Write thread: I/O failure on '%s' at offset %llu (%u bytes): %s")
					% it->pFile->GetFileName() % pBuffer->start % lenData % e.what());
				writeOk = false;
			}

			// eMule ref: WriteCompletionRoutine line 179 — decrement in write thread
			// so main thread can check m_iWrites at any time.
			--it->pFile->m_iWrites;

			// Mark buffer as written / errored so the main thread can harvest
			// it.  PB_ERROR is handled in FlushBuffer Phase 2 (resets to
			// PB_READY for retry; if the disk is genuinely full the next
			// FlushBuffer's CheckFreeDiskSpace pauses the file before the
			// retry loops).
			// eMule ref: WriteCompletionRoutine — line 182
			pBuffer->flushed = writeOk ? PB_WRITTEN : PB_ERROR;
		}
	}

	return NULL;
}
// File_checked_for_headers

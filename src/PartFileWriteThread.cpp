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
#include "Logger.h"

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
			pBuffer->area.FlushAt(it->pFile->m_hpartfile, pBuffer->start, lenData);

			// eMule ref: WriteCompletionRoutine line 179 — decrement in write thread
			// so main thread can check m_iWrites at any time.
			--it->pFile->m_iWrites;

			// Mark buffer as written so the main thread can harvest it.
			// eMule ref: WriteCompletionRoutine — line 182
			pBuffer->flushed = PB_WRITTEN;
		}
	}

	return NULL;
}
// File_checked_for_headers

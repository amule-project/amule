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

#include "PartFileHashThread.h"

#include <common/Format.h>		// Needed for CFormat

#include "amule.h"			// Needed for theApp
#include "GetTickCount.h"		// Needed for GetTickCountFullRes
#include "Logger.h"
#include "PartFile.h"


// Custom event registration
DEFINE_LOCAL_EVENT_TYPE(wxEVT_PARTFILE_HASH_RESULT)


CPartFileHashThread::CPartFileHashThread()
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


CPartFileHashThread::~CPartFileHashThread()
{
	// EndThread() must have been called before destruction.
}


void CPartFileHashThread::EndThread()
{
	{
		wxMutexLocker lock(m_mutex);
		m_bRun = false;
		m_bWorkPending = true;
		m_condition.Signal();
	}
	Wait();
}


void CPartFileHashThread::QueueHashCheck(CPartFile* pFile, uint16 partNumber,
	bool fromAICHRecoveryDataAvailable)
{
	HashJob job;
	job.pFile = pFile;
	job.partNumber = partNumber;
	job.fileHash = pFile->GetFileHash();
	job.fromAICHRecoveryDataAvailable = fromAICHRecoveryDataAvailable;

	wxMutexLocker lock(m_mutex);
	m_jobList.push_back(job);
	m_bWorkPending = true;
	m_condition.Signal();

	AddDebugLogLineN(logPartFile, CFormat(
		"Hash thread: enqueued part %u for '%s' (queue size %u)")
		% partNumber % pFile->GetFileName() % (uint32)m_jobList.size());
}


void* CPartFileHashThread::Entry()
{
	m_bRun = true;

	AddDebugLogLineN(logPartFile, wxT("Hash thread: started"));

	while (m_bRun)
	{
		// Move queued jobs to a local work list under the lock.
		// Mirrors CPartFileWriteThread's pattern: minimise lock hold
		// time so the main thread can keep enqueueing.
		std::list<HashJob> workList;
		{
			wxMutexLocker lock(m_mutex);
			if (m_bRun && !m_bWorkPending) {
				m_condition.WaitTimeout(500);
			}
			m_bWorkPending = false;
			workList.swap(m_jobList);
		}

		for (std::list<HashJob>::iterator it = workList.begin();
			 it != workList.end() && m_bRun; ++it)
		{
			const uint32 startTick = GetTickCountFullRes();

			// CPartFile::m_pendingHashes was incremented before enqueue
			// and is the gate that ~CPartFile waits on, so the file
			// pointer is guaranteed valid here.
			//
			// Lock m_hpartfileMutex against CPartFileWriteThread: with
			// ENABLE_MMAP=OFF, HashSinglePart's CFileArea::ReadAt does
			// Seek+Read on the same fd that the write thread does
			// Seek+Write on for FlushAt; concurrent execution races
			// on the fd's file position. The quiescent guard at
			// enqueue time only gates dispatch — it does not prevent
			// writes from resuming while the hash thread is still
			// chewing through a backlog (e.g. user pause → drain →
			// resume mid-drain). See CPartFile::m_hpartfileMutex.
			bool ok;
			{
				std::lock_guard<std::mutex> lock(it->pFile->m_hpartfileMutex);
				ok = it->pFile->HashSinglePart(it->partNumber);
			}

			const uint32 elapsedMs = GetTickCountFullRes() - startTick;

			AddDebugLogLineN(logPartFile, CFormat(
				"Hash thread: part %u %s in %u ms for '%s'")
				% it->partNumber
				% (ok ? wxT("ok") : wxT("CORRUPT"))
				% elapsedMs
				% it->pFile->GetFileName());

			// Post result back to the main thread.  Carries fileHash
			// (not pointer) so the handler can drop the event safely if
			// the file was removed between enqueue and dispatch.
			CPartFileHashResultEvent evt(it->fileHash, it->partNumber, ok,
				it->fromAICHRecoveryDataAvailable);
			theApp->AddPendingEvent(evt);

			// Decrement m_pendingHashes here (after work is fully done
			// AND event posted) so ~CPartFile's wait on the counter
			// includes the event-post step.
			--it->pFile->m_pendingHashes;
		}
	}

	AddDebugLogLineN(logPartFile, wxT("Hash thread: exiting"));

	return NULL;
}
// File_checked_for_headers

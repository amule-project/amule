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

#ifndef PARTFILEHASHTHREAD_H
#define PARTFILEHASHTHREAD_H

#include <list>
#include <wx/event.h>
#include <wx/thread.h>

#include "MD4Hash.h"		// Needed for CMD4Hash

class CPartFile;

// Async per-part MD4 verifier.  Mirrors CPartFileWriteThread's structure:
// a single dedicated worker that pops jobs from a queue, runs the work
// synchronously on its own thread, and posts a result event back to the
// main thread.
//
// Why off-main: HashSinglePart reads ~9.28 MB from disk and runs MD4 — on
// a slow disk that's 100-200 ms per part, above CORE_TIMER_PERIOD.  Doing
// it on the main thread starves OnCoreTimer (UI redraws, asio dispatch)
// during pause/resume drain and at any moment several dirty parts have
// accumulated.
//
// Why no throughput regression vs PR #454 (which rejected an async-hash
// thread): the caller's quiescent guard ensures we only enqueue 1 s after
// the last receive, so the worker's read for hashing never competes with
// CPartFileWriteThread's write for the same disk.
struct HashJob
{
	CPartFile *pFile;
	uint16     partNumber;
	CMD4Hash   fileHash;	// captured at enqueue, used for result-event
				// dispatch lookup so a deleted file's events
				// can be dropped safely.
	bool       fromAICHRecoveryDataAvailable;
};


class CPartFileHashThread : public wxThread
{
public:
	CPartFileHashThread();
	~CPartFileHashThread();

	void EndThread();
	void QueueHashCheck(CPartFile* pFile, uint16 partNumber,
		bool fromAICHRecoveryDataAvailable = false);
	bool IsRunning() const { return m_bRun; }

private:
	void* Entry() override;

	volatile bool   m_bRun;
	bool            m_bWorkPending;	// sticky wake flag

	wxMutex         m_mutex;
	wxCondition     m_condition;

	std::list<HashJob> m_jobList;	// protected by m_mutex
};


// Custom event posted from worker to main thread when a single
// HashSinglePart finishes.  Carries the file's CMD4Hash (used for
// safe lookup in CDownloadQueue — the file may have been deleted
// between enqueue and dispatch), the part number, and the result.
extern const wxEventType wxEVT_PARTFILE_HASH_RESULT;

class CPartFileHashResultEvent : public wxEvent
{
public:
	CPartFileHashResultEvent(const CMD4Hash& fileHash, uint16 partNumber,
		bool ok, bool fromAICHRecoveryDataAvailable)
		: wxEvent(0, wxEVT_PARTFILE_HASH_RESULT)
		, m_fileHash(fileHash)
		, m_partNumber(partNumber)
		, m_ok(ok)
		, m_fromAICHRecoveryDataAvailable(fromAICHRecoveryDataAvailable)
	{}

	wxEvent* Clone() const override { return new CPartFileHashResultEvent(*this); }

	const CMD4Hash& FileHash() const { return m_fileHash; }
	uint16 PartNumber() const { return m_partNumber; }
	bool Ok() const { return m_ok; }
	bool FromAICHRecoveryDataAvailable() const { return m_fromAICHRecoveryDataAvailable; }

private:
	CMD4Hash m_fileHash;
	uint16   m_partNumber;
	bool     m_ok;
	bool     m_fromAICHRecoveryDataAvailable;
};


typedef void (wxEvtHandler::*CPartFileHashResultEventFunction)(CPartFileHashResultEvent&);

#define EVT_PARTFILE_HASH_RESULT(func) \
	wx__DECLARE_EVT0(wxEVT_PARTFILE_HASH_RESULT, \
		(wxObjectEventFunction)(CPartFileHashResultEventFunction)& func)


#endif // PARTFILEHASHTHREAD_H
// File_checked_for_headers

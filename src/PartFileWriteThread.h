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

#ifndef PARTFILEWRITETHREAD_H
#define PARTFILEWRITETHREAD_H

#include <list>

#include <wx/thread.h>

class CPartFile;
class PartFileBufferedData;

// eMule ref: PartFile.h:104-108
// Part file buffer write status
#define PB_READY   0
#define PB_PENDING 1
#define PB_ERROR   2
#define PB_WRITTEN 3

// eMule ref: PartFileWriteThread.h:21-25
struct ToWrite
{
	CPartFile           *pFile;
	PartFileBufferedData *pBuffer;
};

// Port of eMule's CPartFileWriteThread (PartFileWriteThread.h:35-67).
// Windows primitives replaced with wxWidgets equivalents:
//   CWinThread             -> wxThread (joinable)
//   IOCP + overlapped I/O  -> synchronous CFileArea::FlushAt() on this thread
//   GetQueuedCompletionStatus -> wxCondition::WaitTimeout()
//   PostQueuedCompletionStatus -> wxCondition::Signal()
//   CCriticalSection       -> wxMutex + wxMutexLocker
class CPartFileWriteThread : public wxThread
{
public:
	CPartFileWriteThread();
	~CPartFileWriteThread();

	void EndThread();                  // eMule ref: PartFileWriteThread.cpp:62
	void QueueWrite(CPartFile* pFile, PartFileBufferedData* pBuffer);
	bool IsRunning() const { return m_bRun; }

private:
	void* Entry() override;

	volatile bool   m_bRun;
	bool            m_bWorkPending;    // sticky wake flag

	wxMutex         m_mutex;
	wxCondition     m_condition;

	std::list<ToWrite> m_flushList;    // protected by m_mutex
};

#endif // PARTFILEWRITETHREAD_H
// File_checked_for_headers

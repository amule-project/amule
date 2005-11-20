//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "Timer.h"		// Interface declaration
#include "InternalEvents.h"	// Needed for wxMuleInternalEvent
#include "GetTickCount.h"	// Needed for GetTickCountFullRes

#include <wx/utils.h>		// Needed for wxSleep

#include <set>

typedef std::set<CTimerThread*> TimerList;

//! List of running times, used to simplify owner-ship issues
TimerList g_timerList;
//! Mutex used to protect access to the timer-list
wxMutex g_timerMutex;


DEFINE_LOCAL_EVENT_TYPE(wxEVT_AMULE_TIMER)

//////////////////////// Timer Thread ////////////////////

class CTimerThread : public wxThread
{
public:
	CTimerThread()
		: wxThread(wxTHREAD_DETACHED)
	{
		// Register the thread
		wxMutexLocker lock(g_timerMutex);
		g_timerList.insert(this);
	}
	
	virtual ~CTimerThread() {
		// Unregister the thread
		wxMutexLocker lock(g_timerMutex);
		g_timerList.erase(this);
	}
	
	void* Entry() {
		wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);

		if (m_oneShot) {
			Sleep(m_period);

			if (!TestDestroy()) {
				wxPostEvent(m_owner, evt);
			}
		} else {
			uint64 code_exec_time = 0;
			
			while (!TestDestroy()) {
				Sleep(m_period - code_exec_time);
				uint64 prev_tick = GetTickCountFullRes();
			
				// Check if the timer was stopped while it slept.
				if (!TestDestroy()) {	
					wxPostEvent(m_owner, evt);
					code_exec_time = (GetTickCountFullRes() - prev_tick);
				}
			}
		}

		return NULL;
	}
	
	unsigned long	m_period;
	bool			m_oneShot;
	wxEvtHandler*	m_owner;
	int				m_id;
};


////////////////////// CTimer ////////////////////////

CTimer::~CTimer()
{
	Stop();
}


CTimer::CTimer(wxEvtHandler* owner, int id)
{
	wxASSERT(owner);
	m_owner = owner;
	m_id = id;
	m_thread = NULL;
}


void CTimer::SetOwner(wxEvtHandler* owner, int id)
{
	m_owner = owner;
	m_id = id;
}


bool CTimer::IsRunning() const
{
	wxMutexLocker lock(g_timerMutex);
	return g_timerList.count(m_thread);
}


bool CTimer::Start(int millisecs, bool oneShot)
{
	wxCHECK_MSG(m_id != -1, false, wxT("Invalid target-ID for timer-events."));
	
	if (!IsRunning()) {
		m_thread = new CTimerThread();
		
		m_thread->m_period	= millisecs;
		m_thread->m_oneShot	= oneShot;
		m_thread->m_owner	= m_owner;
		m_thread->m_id		= m_id;

		if (m_thread->Create() == wxTHREAD_NO_ERROR) {
			if (m_thread->Run() == wxTHREAD_NO_ERROR) {
				return true;
			}
		}

		// Something went wrong ...
		m_thread->Delete();
		m_thread = NULL;
	}

	return false;
}


void CTimer::Stop()
{
	wxMutexLocker lock(g_timerMutex);
	
	// Check if the thread still exists
	if (g_timerList.count(m_thread)) {
		m_thread->Delete();
	}
	
	m_thread = NULL;
}


void CTimer::TerminateTimers()
{
	wxCHECK_RET(wxThread::IsMain(), wxT("Timers must be terminated by main thread."));
	
	// Tell all timers to terminate
	{
		wxMutexLocker lock(g_timerMutex);

		TimerList::iterator it = g_timerList.begin();
		for (; it != g_timerList.end(); ++it) {
			(*it)->Delete();
		}
	}

	while (true) {
		{ 
			wxMutexLocker lock(g_timerMutex);

			if (g_timerList.empty()) {
				// All threads have been deleted
				return;
			}
		}

		// Sleep a bit to avoid clubbering the mutex.
		wxMilliSleep(10);
	}
}


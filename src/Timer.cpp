//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "Timer.h"		// Interface declaration
#include "GetTickCount.h"	// Needed for GetTickCountFullRes


//////////////////////// Timer Thread ////////////////////

class CTimerThread : public wxThread
{
public:
	CTimerThread()
		: wxThread(wxTHREAD_JOINABLE)
	{
	}

	void* Entry() {
		CTimerEvent evt(m_id);

		uint64 lastEvent = GetTickCountFullRes();
		do {
			uint64 now = GetTickCountFullRes();
			uint64 sinceLast = now - lastEvent;
			if (sinceLast > 100 * m_period) {
				// We're way too far behind.  Probably what really happened is
				// the system time was adjusted backwards a bit, or (less
				// likely) the time wrapped past the limit of a uint64.  So,
				// the calculation of sinceLast has produced an absurd value.
				sinceLast = 100 * m_period;
				lastEvent = now - sinceLast;
			}

			unsigned long timeout = ((m_period < sinceLast) ? 0 : (m_period - sinceLast));
			
			// In normal operation, we will never actually acquire the
			// semaphore; we will always timeout.  This is used to
			// implement a Sleep operation which the owning CTimer can
			// interrupt by posting to the semaphore.  So, it follows
			// that if we do acquire the semaphore it means the owner
			// wants us to exit.
			if (m_sleepSemaphore.WaitTimeout(timeout) == wxSEMA_TIMEOUT) {
				// Increment for one event only, so no events can be lost.
				lastEvent += m_period;
					
				wxPostEvent(m_owner, evt);
			} else {
				break;
			}
		} while (!m_oneShot);
		
		return NULL;
	}
	
	unsigned long	m_period;
	bool			m_oneShot;
	wxEvtHandler*	m_owner;
	int				m_id;
	wxSemaphore		m_sleepSemaphore;
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


bool CTimer::IsRunning() const
{
	return (m_thread && m_thread->IsRunning());
}


bool CTimer::Start(int millisecs, bool oneShot)
{
	wxCHECK_MSG(m_id != -1, false, wxT("Invalid target-ID for timer-events."));
	
	// Since this class generally matches wxTimer, calling
	// start on a running timer stops and then restarts it.
	Stop();
	
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
	delete m_thread;
	m_thread = NULL;

	return false;
}


void CTimer::Stop()
{
	if (m_thread) {
		m_thread->m_sleepSemaphore.Post();
		m_thread->Delete();
		m_thread->Wait();
		delete m_thread;
		m_thread = NULL;
	}
}


DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_TIMER);

CTimerEvent::CTimerEvent(int id)
	: wxEvent(id, MULE_EVT_TIMER)
{
}


wxEvent* CTimerEvent::Clone() const
{
	return new CTimerEvent(GetId());
}


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


DEFINE_LOCAL_EVENT_TYPE(wxEVT_AMULE_TIMER)

//////////////////////// Timer Thread ////////////////////

class CTimerThread : public wxThread
{
public:
	CTimerThread()
		: wxThread(wxTHREAD_JOINABLE)
	{
	}
	
	void* Entry() {
		wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);

		if (m_oneShot) {
			Sleep(m_period);

			if (!TestDestroy()) {
				wxPostEvent(m_owner, evt);
			}
		} else {
			uint64 lastEvent = GetTickCountFullRes();
			
			while (!TestDestroy()) {
				uint64 sinceLast = GetTickCountFullRes() - lastEvent;
				if (m_period > sinceLast) {
					// In normal operation, we will never actually acquire the
					// semaphore; we will always timeout.  This is used to
					// implement a Sleep operation which the owning CTimer can
					// interrupt by posting to the semaphore.  So, it follows
					// that if we do acquire the semaphore it means the owner
					// wants us to exit.
					wxSemaError err = m_interruptibleSleepSemaphore.
						WaitTimeout(m_period - sinceLast);
					if (err != wxSEMA_TIMEOUT) {
						break;
					}
				}
				
				// Ensure that no events are discarded
				// by only incrementing for one event.
				lastEvent += m_period;
					
				// Check if the timer was stopped while it slept.
				if (!TestDestroy()) {
					wxPostEvent(m_owner, evt);
				}
			}
		}

		return NULL;
	}
	
	unsigned long	m_period;
	bool			m_oneShot;
	wxEvtHandler*	m_owner;
	int				m_id;
	wxSemaphore		m_interruptibleSleepSemaphore;
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
	if (m_thread) {
		m_thread->m_interruptibleSleepSemaphore.Post();
		m_thread->Delete();
		m_thread->Wait();
		delete m_thread;
		m_thread = NULL;
	}
}

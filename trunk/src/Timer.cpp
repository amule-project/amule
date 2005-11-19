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

class CTimerThread : public wxThread {
	unsigned long m_period;
	bool m_oneShot;
	wxEvtHandler *m_owner;
	int m_id;
	
	void *Entry();

	public:
	CTimerThread(wxEvtHandler *owner, unsigned long period, bool oneShot, int id);
};


CTimerThread::CTimerThread(wxEvtHandler *owner,
	unsigned long period, bool oneShot, int id) : wxThread(wxTHREAD_JOINABLE)
{
	m_owner = owner;
	m_period = period;
	m_oneShot = oneShot;
	m_id = id;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("CTimer::CTimerThread: create failed\n");
	}
}

void* CTimerThread::Entry()
{
	static uint64 prev_tick;
	static uint64 code_exec_time;
	static wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);
	
	if ( m_oneShot ) {
		Sleep(m_period);
		wxPostEvent(m_owner, evt);
	} else {
		while ( !TestDestroy() ) {
			Sleep(m_period-code_exec_time);
			prev_tick = GetTickCountFullRes();
			if ( m_id != -1 ) {
				// Oh?
			}
			wxPostEvent(m_owner, evt);
			code_exec_time = (GetTickCountFullRes() - prev_tick);
		}
	}
	return 0;
}


////////////////////// CTimer ////////////////////////

CTimer::~CTimer()
{
	Stop();
}

CTimer::CTimer(wxEvtHandler *owner, int id)
{
	wxASSERT(owner);
	SetOwner(owner, id);
	thread = 0;
}

void CTimer::SetOwner(wxEvtHandler *owner, int id)
{
	CTimer::owner = owner;
	CTimer::id = id;
}

bool CTimer::IsRunning() const
{
	return thread;
}

bool CTimer::Start( int millisecs, bool oneShot )
{
	if ( thread ) {
		return false;
	} else {
		thread = new CTimerThread(owner, millisecs, oneShot, id);
		thread->Run();
	}
	return true;
}

void CTimer::Stop()
{
	if ( thread ) {
		thread->Delete();
		thread = 0;
	}
}

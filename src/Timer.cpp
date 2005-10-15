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

#include <unistd.h>		// Needed for close(2) and sleep(3)

#include "Timer.h"		// Interface declaration
#include "amule.h"		// Needed for theApp
#include "InternalEvents.h"	// Needed for wxMuleInternalEvent


CTimer::~CTimer()
{
	Stop();
}

CTimer::CTimer(wxEvtHandler *owner, int id)
{
	if ( owner ) {
		SetOwner(owner, id);
	} else {
		SetOwner(&theApp, id);
	}
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

CTimer::CTimerThread::CTimerThread(wxEvtHandler *owner,
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

void *CTimer::CTimerThread::Entry()
{
	if ( m_oneShot ) {
		Sleep(m_period);
		wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);
		wxPostEvent(m_owner, evt);
	} else {
		while ( !TestDestroy() ) {
			Sleep(m_period);
			wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);
			if ( m_id != -1 ) {
			}
			wxPostEvent(m_owner, evt);
		}
	}
	return 0;
}


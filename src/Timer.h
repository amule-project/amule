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

#ifndef TIMER_H
#define TIMER_H

#ifdef AMULE_DAEMON

#define AMULE_TIMER_CLASS CTimer
#define AMULE_TIMER_EVENT_CLASS wxEvent

class wxEvtHandler;

#include <wx/thread.h>

/**
 * Replacement for wxTimer on non-X builds
 */
class CTimer {
	wxEvtHandler *owner;
	int id;
	class CTimerThread : public wxThread {
		unsigned long m_period;
		bool m_oneShot;
		wxEvtHandler *m_owner;
		int m_id;
		
		void *Entry();

		public:
		CTimerThread(wxEvtHandler *owner, unsigned long period, bool oneShot, int id);
	};
	CTimerThread *thread;

	public:
	CTimer(wxEvtHandler *owner = 0, int timerid = -1);
	~CTimer();
	void SetOwner(wxEvtHandler *owner, int id = -1);
	bool Start(int millisecs, bool oneShot = false);
	bool IsRunning() const;
	void Stop();
};

#else /* ! amuled */

#define AMULE_TIMER_CLASS wxTimer
#define AMULE_TIMER_EVENT_CLASS wxTimerEvent

#include <wx/timer.h>

#endif /* amuled / ! amuled */

#endif /* TIMER_H */

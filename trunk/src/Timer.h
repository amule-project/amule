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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef TIMER_H
#define TIMER_H

class wxEvtHandler;
class CTimerThread;

#include <wx/thread.h>

/**
 * Replacement for wxTimer as it doesn't work on non-X builds
 */
class CTimer
{
public:
	CTimer(wxEvtHandler *owner, int timerid = -1);
	~CTimer();

	/**
	 * Starts the timer.
	 *
	 * @param millisecs The frequency of events.
	 * @param oneShot Specifies if only one event should be produced.
	 */
	bool Start(int millisecs, bool oneShot = false);
	
	/** 
	 * Returns true if the timer is running.
	 */
	bool IsRunning() const;

	/**
	 * Stops the timer. 
	 *
	 * Note that this does not delete the actual thread 
	 * immediatly, but no new events will be queued after 
	 * calling this function.
	 */
	void Stop();
	
private:
	CTimerThread* m_thread;
	wxEvtHandler* m_owner;
	int m_id;
};

#endif /* TIMER_H */

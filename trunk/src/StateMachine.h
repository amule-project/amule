//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Marcelo Jimenez (phoenix@amule.org)
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

//
// Generic state machine implementation
//

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <queue>

#include <wx/thread.h>		/* For wxMutex, wxMutexLocker	*/
#include <wx/string.h>		/* For wxString			*/

typedef unsigned int t_sm_state;

typedef unsigned int t_sm_event;

class CStateMachine
{
public:
	CStateMachine(
		const wxString &name,
		const unsigned int maxStates,
		const t_sm_state initialState );
	virtual ~CStateMachine() = 0;
	void Clock();
	void Schedule(t_sm_event event);
	t_sm_state GetState() const			{ return m_state; }
	unsigned int GetClocksInCurrentState() const	{ return m_clocksInCurrentState; }
	virtual t_sm_state next_state(t_sm_event event) = 0;
	virtual void process_state(t_sm_state state, bool entry) = 0;
	
private:
	void flush_queue();

	t_sm_state		m_state;
	wxMutex			m_stateMutex;
	std::queue <t_sm_event>	m_queue;
	wxMutex			m_queueMutex;
	const wxString		m_name;
	const unsigned int	m_maxStates;
	const unsigned int	m_initialState;
	unsigned int		m_clockCounter;
	unsigned int		m_clocksInCurrentState;
};

#endif // STATE_MACHINE_H

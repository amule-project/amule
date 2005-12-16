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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "StateMachine.h"

#include <common/StringFunctions.h>

CStateMachine::CStateMachine(
		const wxString &name,
		const unsigned int maxStates,
		const t_sm_state initialState )
:
m_stateMutex(wxMUTEX_RECURSIVE),
m_queueMutex(wxMUTEX_RECURSIVE),
m_name(name),
m_maxStates(maxStates),
m_initialState(initialState)
{
	m_state = initialState;
	m_clockCounter = 0;
	m_clocksInCurrentState = 0;
}

CStateMachine::~CStateMachine()
{
}

void CStateMachine::Clock()
{
	t_sm_state old_state;
	t_sm_event event;
	bool state_entry;

	old_state = m_state;

	/* Process state change acording to event */
	if (!m_queue.empty()) {
		event = m_queue.front();
		m_queue.pop();
	} else {
		event = 0;
	}
	
	/* State changes can only happen here */
	wxMutexLocker lock(m_stateMutex);
	m_state = next_state( event );

//#if 0
	/* Debug */
	++m_clockCounter;
	state_entry = ( m_state != old_state ) || ( m_clockCounter == 1 );
	if( state_entry )
	{
		m_clocksInCurrentState = 0;
		printf( "%s(%04d): %d -> %d\n",
			(const char *)unicode2char(m_name),
			m_clockCounter, old_state, m_state);
	}
	++m_clocksInCurrentState;
//#endif

	/* Process new state entry */
	if( m_state < m_maxStates )
	{
		/* It should be ok to call Clock() recursively inside this
		 * procedure because state change has already happened. Also
		 * the m_state mutex is recursive. */
		process_state(m_state, state_entry);
	}
}

/* In multithreaded implementations, this must be locked */
void CStateMachine::Schedule(t_sm_event event)
{
	wxMutexLocker lock(m_queueMutex);
	m_queue.push(event);
}

void CStateMachine::flush_queue()
{
	while (!m_queue.empty()) {
		m_queue.pop();
	}
}

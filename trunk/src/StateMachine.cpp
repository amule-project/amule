//
// This file is part of the aMule Project
//
// Copyright (c) 2005 aMule Project ( http://www.amule.org )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include "StateMachine.h"


#include "StringFunctions.h"


StateMachine::StateMachine(
		const wxString &name,
		const unsigned int max_states,
		const t_sm_state initial_state )
:
m_name(name),
m_max_states(max_states),
m_initial_state(initial_state),
m_clock_counter(0)
{
	m_state = initial_state;
}

StateMachine::~StateMachine()
{
}

void StateMachine::Clock()
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
	m_state = next_state( event );

//#if 0
	/* Debug */
	++m_clock_counter;
	state_entry = ( m_state != old_state ) || ( m_clock_counter == 1 );
	if( state_entry )
	{
		m_clocks_in_current_state = 0;
		printf( "%s(%04d): %d -> %d\n",
			unicode2char(m_name), m_clock_counter, old_state, m_state);
	}
	m_clocks_in_current_state++;
//#endif

	/* Process new state entry */
	if( m_state < m_max_states )
	{
		process_state(m_state, state_entry);
	}
}

void StateMachine::Schedule(t_sm_event event)
{
	m_queue.push(event);
}

void StateMachine::flush_queue()
{
	while (!m_queue.empty()) {
		m_queue.pop();
	}
}

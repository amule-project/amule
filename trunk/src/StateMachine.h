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

/**
 * Generic state machine implementation
 */

#ifndef STATE_MACHINE_H

#define STATE_MACHINE_H

#include <queue>
#include <vector>

#include <wx/string.h>

typedef unsigned int t_sm_state;

typedef unsigned int t_sm_event;

typedef void (*state_processor_vector)(bool entry);

class StateMachine
{
public:
	StateMachine(
		wxString &name,
		unsigned int max_states,
		t_sm_state initial_state,
		state_processor_vector *process_state );
	virtual ~StateMachine() = 0;
	void clock();
	void schedule(t_sm_event event);
	virtual t_sm_state next_state(t_sm_event event) = 0;
	
private:
	void reset();
	void flush_queue();

	wxString			m_name;
	unsigned int			m_max_states;
	const unsigned int		m_initial_state;
	t_sm_state			m_state;
	unsigned int			m_clock_counter;
	std::queue <t_sm_event>		m_queue;
	const state_processor_vector	*m_process_state;
};

#endif // STATE_MACHINE_H

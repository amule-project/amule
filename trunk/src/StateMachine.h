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

class StateMachine
{
public:
	StateMachine(
		const wxString &name,
		const unsigned int max_states,
		const t_sm_state initial_state );
	virtual ~StateMachine() = 0;
	void Clock();
	void Schedule(t_sm_event event);
	t_sm_state GetState() const { return m_state; }
	virtual t_sm_state next_state(t_sm_event event) = 0;
	virtual void process_state(t_sm_state state, bool entry) = 0;
	
protected:
	t_sm_state			m_state;
	
private:
	void flush_queue();

	const wxString			m_name;
	const unsigned int		m_max_states;
	const unsigned int		m_initial_state;
	unsigned int			m_clock_counter;
	std::queue <t_sm_event>		m_queue;
};

#endif // STATE_MACHINE_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "UserEvents.h"


#include <common/Format.h>
#include "Logger.h"
#include "Preferences.h"
#include "PartFile.h"
#include "TerminationProcess.h"	// Needed for CTerminationProcess


#include <wx/process.h>


#define USEREVENTS_EVENT(ID, NAME, VARS)	{ wxT(#ID), NAME, false, wxEmptyString, false, wxEmptyString },
static struct {
	const wxString key;
	const wxString name;
	bool core_enabled;
	wxString core_command;
	bool gui_enabled;
	wxString gui_command;
} s_EventList[] = {
	USEREVENTS_EVENTLIST()
	/* This macro expands to initialise the list of user event types. Example:
	   { wxT("NewChatSession"), wxTRANSLATE("New chat session started"), false, wxEmptyString, false, wxEmptyString }, */
};
#undef USEREVENTS_EVENT


#ifdef __WXDEBUG__
inline bool CheckIndex(const unsigned int idx)
{
	return (idx < itemsof(s_EventList));
}
#endif

unsigned int CUserEvents::GetCount()
{
	return itemsof(s_EventList);
}

const wxString& CUserEvents::GetDisplayName(enum EventType event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].name;
}

bool CUserEvents::IsCoreCommandEnabled(enum EventType event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].core_enabled;
}

bool CUserEvents::IsGUICommandEnabled(enum EventType event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].gui_enabled;
}

const wxString& CUserEvents::GetKey(const unsigned int event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].key;
}

bool& CUserEvents::GetCoreEnableVar(const unsigned int event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].core_enabled;
}

wxString& CUserEvents::GetCoreCommandVar(const unsigned int event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].core_command;
}

bool& CUserEvents::GetGUIEnableVar(const unsigned int event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].gui_enabled;
}

wxString& CUserEvents::GetGUICommandVar(const unsigned int event)
{
	wxASSERT(CheckIndex(event));
	return s_EventList[event].gui_command;
}

#define USEREVENTS_EVENT(ID, NAME, VARS)	case CUserEvents::ID: { VARS break; }
#define USEREVENTS_REPLACE_VAR(VAR, DESC, CODE)	command.Replace(wxT("%") VAR, CODE);
static void ExecuteCommand(
	enum CUserEvents::EventType event,
	const void* object,
	const wxString& cmd)
{
	// This variable is needed by the USEREVENTS_EVENTLIST macro.
	wxString command = cmd;
	switch (event) {
		USEREVENTS_EVENTLIST()
		/* This macro expands to handle all user event types. Example:
		   case CUserEvents::NewChatSession: {
		       command.Replace( wxT("%SENDER"), *((wxString*)object) );
			   break;
		   } */
	}
	if (!command.empty()) {
		CTerminationProcess *p = new CTerminationProcess(cmd);
		if (!wxExecute(command, wxEXEC_ASYNC, p)) {
			// If wxExecute fails, we need to delete the CTerminationProcess
			// otherwise it will leak.
			delete p;
			AddLogLineC(CFormat(_("Failed to execute command `%s' on `%s' event.")) %
				command % s_EventList[event].name);
		}
	}
}

void CUserEvents::ProcessEvent(enum EventType event, const void* object)
{
	wxASSERT(CheckIndex(event));
	wxASSERT(object != NULL);

#ifndef CLIENT_GUI
	if (s_EventList[event].core_enabled) {
		ExecuteCommand(event, object, s_EventList[event].core_command);
	}
#endif
#ifndef AMULE_DAEMON
	if (s_EventList[event].gui_enabled) {
		ExecuteCommand(event, object, s_EventList[event].gui_command);
	}
#endif
}

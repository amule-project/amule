//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef USEREVENTS_H
#define USEREVENTS_H

#include <wx/intl.h>		// Needed for wxTRANSLATE

/* Each event will use 5 IDs:
   - the panel that shows the prefs for this event
   - the 'Core command enabled' checkbox
   - the 'Core command' textctrl
   - the 'GUI command enabled' checkbox
   - the 'GUI command' textctrl
*/
#define USEREVENTS_IDS_PER_EVENT	5

const int USEREVENTS_FIRST_ID	=	11500;	/* Some safe GUI ID to start from */

/**
 * Macro listing all the events.
 *
 * This huge macro is expanded 5 times in the sources, each time producing
 * different code. If we decide to get rid of the macro either because of coding style
 * decision, or someone finds a compiler that doesn't support this big macro, we
 * have to maintain these five places in sync. They are:
 * - one in PrefsUnifiedDlg.cpp (EVENT_LIST, PrefsUnifiedDlg::PrefsUnifiedDlg())
 * - one in this header (CUserEvents::EventType)
 * - two in UserEvents.cpp (static struct EventList[]; CUserEvent::ProcessEvent())
 */
#define USEREVENTS_EVENTLIST() \
	USEREVENTS_EVENT(DownloadCompleted, wxTRANSLATE("Download completed"), \
		USEREVENTS_REPLACE_VAR( \
			wxT("FILE"), \
			wxTRANSLATE("The full path to the file."), \
			((CPartFile*)object)->GetFullName() ) \
		USEREVENTS_REPLACE_VAR( \
			wxT("NAME"), \
			wxTRANSLATE("The name of the file without path component."), \
			((CPartFile*)object)->GetFileName() ) \
		USEREVENTS_REPLACE_VAR( \
			wxT("HASH"), \
			wxTRANSLATE("The ed2k hash of the file."), \
			((CPartFile*)object)->GetFileHash().Encode() ) \
		USEREVENTS_REPLACE_VAR( \
			wxT("SIZE"), \
			wxTRANSLATE("The size of the file in bytes."), \
			(wxString)(CFormat(wxT("%llu")) % ((CPartFile*)object)->GetFileSize()) ) \
		USEREVENTS_REPLACE_VAR( \
			wxT("DLACTIVETIME"), \
			wxTRANSLATE("The size of the file in bytes."), \
			CastSecondsToHM(((CPartFile*)object)->GetDlActiveTime()) ) \
	) \
	USEREVENTS_EVENT( \
		NewChatSession, \
		wxTRANSLATE("New chat session started"), \
		USEREVENTS_REPLACE_VAR( \
			wxT("SENDER"), \
			wxTRANSLATE("Message sender."), \
			*((wxString*)object) ) \
	) \
	USEREVENTS_EVENT( \
		OutOfDiskSpace, \
		wxTRANSLATE("Out of space"), \
		USEREVENTS_REPLACE_VAR( \
			wxT("PARTITION"), \
			wxTRANSLATE("Disk partition."), \
			*((wxString*)object) ) \
	) \
	USEREVENTS_EVENT( \
		ErrorOnCompletion, \
		wxTRANSLATE("Error on completion"), \
		USEREVENTS_REPLACE_VAR( \
			wxT("FILE"), \
			wxTRANSLATE("The full path to the file."), \
			((CPartFile*)object)->GetFullName() ) \
	)


#define USEREVENTS_EVENT(ID, NAME, VARS)	ID,

/**
 * Class to handle userspace events.
 *
 * These events that we publish to the user and let him
 * specify a command to be run when one of these events occur.
 */
class CUserEvents {
	friend class CPreferences;
 public:
	//! Event list
	enum EventType {
		USEREVENTS_EVENTLIST()
	};

	/**
	 * Process a user event.
	 *
	 * Notes on the 'object' argument: this should be a pointer to
	 * an object instance, from which all of the replacement texts
	 * can be generated.
	 *
	 * Unfortunately this approach does not provide any type-safety,
	 * a list of string pairs (key, replacement) would be the best.
	 * However, this would need either expanding the macro at all of
	 * the places where CUserEvents::ProcessEvent is called from, or
	 * creating lists of parameters for each event, etc = more lists
	 * to keep in sync manually.
	 */
	static void		ProcessEvent(enum EventType event, const void* object);

	/**
	 * Returns the number of defined user events.
	 */
	static unsigned int	GetCount() __attribute__((__const__));

	/**
	 * Returs the human-readable name of the event.
	 */
	static const wxString&	GetDisplayName(enum EventType event) __attribute__((__pure__));

	/**
	 * Checks whether the core command is enabled.
	 */
	static bool		IsCoreCommandEnabled(enum EventType event) __attribute__((__pure__));

	/**
	 * Checks whether the GUI command is enabled.
	 */
	static bool		IsGUICommandEnabled(enum EventType event) __attribute__((__pure__));

 private:
	// functions for CPreferences
	static const wxString&	GetKey(const unsigned int event) __attribute__((__pure__));
	static bool&		GetCoreEnableVar(const unsigned int event) __attribute__((__pure__));
	static wxString&	GetCoreCommandVar(const unsigned int event) __attribute__((__pure__));
	static bool&		GetGUIEnableVar(const unsigned int event) __attribute__((__pure__));
	static wxString&	GetGUICommandVar(const unsigned int event) __attribute__((__pure__));
};

#undef USEREVENTS_EVENT

#endif /* USEREVENTS_H */

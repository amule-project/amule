//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef INTERNALEVENTS_H
#define INTERNALEVENTS_H


#include <wx/event.h>	// Needed for wxEvent


wxDECLARE_EVENT(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, wxEvent);
wxDECLARE_EVENT(wxEVT_CORE_SOURCE_DNS_DONE, wxEvent);
wxDECLARE_EVENT(wxEVT_CORE_UDP_DNS_DONE, wxEvent);
wxDECLARE_EVENT(wxEVT_CORE_SERVER_DNS_DONE, wxEvent);

class CMuleInternalEvent : public wxEvent
{
public:
	CMuleInternalEvent(int event, int id = wxID_ANY)
		: wxEvent(id, event),
		  m_ptr(NULL),
		  m_value(0),
		  m_commandInt(0)
	{
	}

	wxEvent* Clone(void) const {
		return new CMuleInternalEvent(*this);
	}

	void SetExtraLong(long value) {
		m_value = value;
	}

	long GetExtraLong() {
		return m_value;
	}

	void SetInt(int i) {
		m_commandInt = i;
	}

	long GetInt() const {
		return m_commandInt;
	}

	void SetClientData(void *ptr) {
		m_ptr = ptr;
	}

	void *GetClientData() {
		return m_ptr;
	}

private:
	void*	m_ptr;
	long	m_value;
	int		m_commandInt;
};


typedef void (wxEvtHandler::*MuleInternalEventFunction)(CMuleInternalEvent&);

//! Event-handler for internal core events.
#define EVT_MULE_INTERNAL(event, id, func) \
	wx__DECLARE_EVT1(event, id, wxEVENT_HANDLER_CAST(MuleInternalEventFunction, func))


#endif /* INTERNALEVENTS_H */
// File_checked_for_headers

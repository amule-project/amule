//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, wxEVT_USER_FIRST+HTTP_DOWNLOAD_FINISHED)

DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE, wxEVT_USER_FIRST+SOURCE_DNS_DONE)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE, wxEVT_USER_FIRST+UDP_DNS_DONE)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE, wxEVT_USER_FIRST+SERVER_DNS_DONE)


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

//! Event-handler for completed hashings of new shared files and partfiles.
#define EVT_MULE_INTERNAL(event, id, func) \
	DECLARE_EVENT_TABLE_ENTRY(event, id, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleInternalEventFunction, &func), (wxObject*) NULL),


#endif /* INTERNALEVENTS_H */
// File_checked_for_headers

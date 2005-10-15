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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef INTERNALEVENTS_H
#define INTERNALEVENTS_H


#include <wx/event.h>	// Needed for wxEvent


DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED, wxEVT_USER_FIRST+FILE_HASHING_FINISHED)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN, wxEVT_USER_FIRST+FILE_HASHING_SHUTDOWN)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION, wxEVT_USER_FIRST+FILE_COMPLETION_FINISHED)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, wxEVT_USER_FIRST+HTTP_DOWNLOAD_FINISHED)

DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE, wxEVT_USER_FIRST+SOURCE_DNS_DONE)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_UDP_DNS_DONE, wxEVT_USER_FIRST+UDP_DNS_DONE)
DECLARE_LOCAL_EVENT_TYPE(wxEVT_CORE_SERVER_DNS_DONE, wxEVT_USER_FIRST+SERVER_DNS_DONE)

DECLARE_LOCAL_EVENT_TYPE(wxEVT_AMULE_TIMER, wxEVT_USER_FIRST+EVENT_TIMER)


class wxMuleInternalEvent : public wxEvent {
	void *m_ptr;
	long m_value;
	int  m_commandInt;
	public:
	wxMuleInternalEvent(int id, int event_id) : wxEvent(event_id, id)
	{
	}
	wxMuleInternalEvent(int id) : wxEvent(-1, id)
	{
	}
	wxMuleInternalEvent(int id, void *ptr, long value) : wxEvent(-1, id)
	{
		m_ptr = ptr;
		m_value = value;
	}
	wxEvent *Clone(void) const
	{
		return new wxMuleInternalEvent(*this);
	}
	void SetExtraLong(long value)
	{
		m_value = value;
	}
	long GetExtraLong()
	{
		return m_value;
	}
	void SetInt(int i)
	{
		m_commandInt = i;
	}
	long GetInt() const
	{
		return m_commandInt; 
	}

	void SetClientData(void *ptr)
	{
		m_ptr = ptr;
	}
	void *GetClientData()
	{
		return m_ptr;
	}
};

#endif /* INTERNALEVENTS_H */

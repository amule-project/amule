//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "AsyncDNS.h"	// Interface declaration

#include "InternalEvents.h"	// Needed for wxEVT_*
#include "NetworkFunctions.h" // Needed for StringHosttoUint32
#include "Logger.h"


CAsyncDNS::CAsyncDNS(const wxString& ipName, DnsSolveType type, wxEvtHandler* handler, void* socket)
	: wxThread(wxTHREAD_DETACHED)
{
	m_type = type;
	m_ipName = ipName.wc_str();		// make a deep copy to to circument the thread-unsafe wxString reference counting
	m_socket = socket;
	m_handler = handler;
}


wxThread::ExitCode CAsyncDNS::Entry()
{
	uint32 result = StringHosttoUint32(m_ipName);
	uint32 event_id = 0;
	void* event_data = NULL;

	switch (m_type) {
		case DNS_UDP:
			event_id = wxEVT_CORE_UDP_DNS_DONE;
			event_data = m_socket;
			break;
		case DNS_SOURCE:
			event_id = wxEVT_CORE_SOURCE_DNS_DONE;
			event_data = NULL;
			break;
		case DNS_SERVER_CONNECT:
			event_id = wxEVT_CORE_SERVER_DNS_DONE;
			event_data = m_socket;
			break;
		default:
			AddLogLineN(wxT("WRONG TYPE ID ON ASYNC DNS SOLVING!!!"));
	}

	if (event_id) {
		CMuleInternalEvent evt(event_id);
		evt.SetExtraLong(result);
		evt.SetClientData(event_data);
		wxPostEvent(m_handler,evt);
	}

	return NULL;
}
// File_checked_for_headers

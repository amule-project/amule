//
// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Angel Vidal Veiga - Kry (kry@amule.org)
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "NetworkFunctions.h"
#endif

#include "NetworkFunctions.h"
#ifndef EC_REMOTE
// Not needed for remote apps.

#warning deprecate this: Intenal Events should be on a separate file
#include "amule.h"

CAsyncDNS::CAsyncDNS(const wxString& ipName, DnsSolveType type, void* socket) : wxThread(wxTHREAD_DETACHED)
{
	m_type = type;
	m_ipName = ipName;
	m_socket = socket;
}

wxThread::ExitCode CAsyncDNS::Entry()
{
	printf("Async thread solving a hostname\n");
	uint32 result = StringHosttoUint32(m_ipName);
	printf("Done\n");
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
			printf("WRONG TYPE ID ON ASYNC DNS SOLVING!!!\n");
	}
	
	if (event_id) {
		wxMuleInternalEvent evt(event_id);
		evt.SetExtraLong(result);
		evt.SetClientData(event_data);
		wxPostEvent(&theApp,evt);	
	}
	
	return NULL;
}
#endif /* ! EC_REMOTE */

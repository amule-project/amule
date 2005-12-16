//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal Veiga - Kry (kry@amule.org)
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef ASYNCDNS_H
#define ASYNCDNS_H

#include <wx/string.h>
#include <wx/thread.h>

// Implementation of Asynchronous dns resolving using wxThread 
//	 and internal wxIPV4address handling of dns

class wxEvtHandler;

enum DnsSolveType {
	DNS_UDP,
	DNS_SOURCE,
	DNS_SERVER_CONNECT
};

// Time between DNS solving the same address
// 30 minutes * 60 s/m * 1000 ms/s
#define DNS_SOLVE_TIME 30*60*1000


class CAsyncDNS : public wxThread
{
public:
	CAsyncDNS(const wxString& ipName, DnsSolveType type, wxEvtHandler* handler, void* socket = NULL);
	virtual ExitCode Entry();

private:
	DnsSolveType m_type;
	wxString m_ipName;
	void* m_socket;
	wxEvtHandler* m_handler;
};

#endif // ASYNCDNS_H

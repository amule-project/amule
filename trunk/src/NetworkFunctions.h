// This file is part of the aMule Project.
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

#ifndef NETWORK_FUNCTIONS_H
#define NETWORK_FUNCTIONS_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "NetworkFunctions.h"
#endif

#include "Types.h"		// Needed for uint16 and uint32
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/socket.h>
#include "StringFunctions.h"


/****************************************************/ 
/******************* Inlines ************************/
/****************************************************/

// Network ip/host handling functions
// These functions takes IPs in anti-host order and IPs in anti-host order

inline wxString Uint32toStringIP(uint32 ip)
{
	return wxString::Format(wxT("%u.%u.%u.%u"),(uint8)ip,(uint8)(ip>>8),(uint8)(ip>>16),(uint8)(ip>>24));	
}

inline wxString Uint32_16toStringIP_Port(uint32 ip, uint16 port)
{
	return wxString::Format(wxT("%u.%u.%u.%u:%u"),(uint8)ip,(uint8)(ip>>8),(uint8)(ip>>16),(uint8)(ip>>24),port);	
}

inline uint32 StringIPtoUint32(const wxString &strIP)
{
	uint32 ip[4];
	uint32 ret = 0;
	bool error = false;
	unsigned long u = 0;
	wxString strTmp(strIP);
	for( int i = 0; i < 3; ++i) {
		int j = strTmp.Find(wxT("."));
		if (error = (j == -1)) {
			break;
		}
		if (error = !strTmp.Left(j).ToULong(&u)) {
			break;
		}
		if (error = (u > 255)) {
			break;
		}
		ip[i] = u;
		strTmp = strTmp.Mid(j+1);
	}
	if (!error) {
		error = !strTmp.ToULong(&u);
		if (!error) {
			error = u > 255;
			if (!error) {
				ip[3] = u;
				ret = ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24);
			}
		}
	}
	if (error) {
		printf("Error on ip format!\n");
	}
	
	return ret;
}

inline uint32 StringHosttoUint32(const wxString &Host)
{
	// Why using native things when we have a wrapper for it :)
	wxIPV4address solver;
	solver.Hostname(Host);
	uint32 result = StringIPtoUint32(solver.IPAddress());
	if (result != (uint32)-1) {
		return result;
	} else {
		// This actually happens on wrong hostname
		return 0;
	}
}

/****************************************************/ 
/***************** Non-inlines **********************/
/****************************************************/

/*
 * Note: IP must be in anti-host order (BE on LE platform, LE on BE platform).
 */
bool IsGoodIP( uint32 IP );


#ifndef EC_REMOTE
// Not needed for remote apps.

// Implementation of Asynchronous dns resolving using wxThread 
//	 and internal wxIPV4address handling of dns

enum DnsSolveType {
	DNS_UDP,
	DNS_SOURCE,
	DNS_SERVER_CONNECT
};

class CAsyncDNS : public wxThread
{
public:
	CAsyncDNS(const wxString& ipName, DnsSolveType type, void* socket = NULL);
	virtual ExitCode Entry();

private:
	DnsSolveType m_type;
	wxString m_ipName;
	void* m_socket;
};

#endif /* ! EC_REMOTE */

#endif // NETWORK_FUNCTIONS_H

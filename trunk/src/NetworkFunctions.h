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

#include "types.h"		// Needed for uint16 and uint32
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/socket.h>
#include "StringFunctions.h"

// Foward declaration
class CUDPSocket;


/****************************************************/ 
/******************* Inlines ************************/
/****************************************************/

// Network ip/host handling functions

inline wxString Uint32toStringIP(uint32 ip) {
	return wxString::Format(wxT("%u.%u.%u.%u"),(uint8)ip,(uint8)(ip>>8),(uint8)(ip>>16),(uint8)(ip>>24));	
}

inline wxString Uint32_16toStringIP_Port(uint32 ip, uint16 port) {
	return wxString::Format(wxT("%u.%u.%u.%u:%u"),(uint8)ip,(uint8)(ip>>8),(uint8)(ip>>16),(uint8)(ip>>24),port);	
}

inline uint32 CStringIPtoUint32(const char* str_ip) {
	uint32 ip[4];
	int result = sscanf(str_ip,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);
	if (result==4) {
		return (ip[0] | (ip[1] << 8) | (ip[2] << 16) | (ip[3] << 24));	
	} else {
		return 0; // Error on ip format.
	}
}

inline uint32 StringIPtoUint32(wxString str_ip) {
	return CStringIPtoUint32(unicode2char(str_ip));
}

inline uint32 StringHosttoUint32(wxString Host) {
	// Why using native things when we have a wrapper for it :)
	wxIPV4address solver;
	solver.Hostname(Host);
	return StringIPtoUint32(solver.IPAddress());
}

// Checks an ip to see if it is valid, depending on current preferences.
inline bool IsGoodIP(uint32 nIP)
{
	// always filter following IP's
	// -------------------------------------------
	//	 0.0.0.0
	// 127.*.*.*						localhost

	if (nIP==0 || (uint8)nIP==127)
		return false;

	// filter LAN IP's
	// -------------------------------------------
	//	0.*
	//	10.0.0.0 - 10.255.255.255		class A
	//	172.16.0.0 - 172.31.255.255		class B
	//	192.168.0.0 - 192.168.255.255	class C

	uint8 nFirst = (uint8)nIP;
	uint8 nSecond = (uint8)(nIP >> 8);

	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return false;

	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return false;

	if (nFirst==0 || nFirst==10)
		return false;

	return true;
}

/****************************************************/ 
/***************** Non-inlines **********************/
/****************************************************/


// Implementation of Asynchronous dns resolving using wxThread 
//	 and internal wxIPV4address handling of dns

class CAsyncDNS : public wxThread
{
public:
	CAsyncDNS();
	virtual ExitCode Entry();

	wxString ipName;
	CUDPSocket* socket;
};

#endif // NETWORK_FUNCTIONS_H

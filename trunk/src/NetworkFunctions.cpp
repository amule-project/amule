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

#ifdef __WXMSW__
	#include <winsock.h>
#else
#ifdef __BSD__
       #include <sys/types.h>
#endif /* __BSD__ */
	#include <sys/socket.h>		//
	#include <netinet/in.h>		// Needed for inet_addr, htonl
	#include <arpa/inet.h>		//
#endif



bool StringIPtoUint32(const wxString &strIP, uint32& Ip)
{
	// The current position in the current field, used to detect malformed fields (x.y..z).
	int digit = 0;

	// The current field, used to ensure only IPs that looks like a.b.c.d are supported
	int field = 0;

	// The value of the current field
	int value = 0;

	// Stores the work-value of the IP, reference is not changed unless the str was valid
	uint32 tmp_ip = 0;

	wxString str = strIP.Strip( wxString::both );
	for ( size_t i = 0; i < str.Length(); i++ ) {
		wxChar c = str.GetChar( i );
		
		if ( c >= wxT('0') && c <= wxT('9') ) {
			value = ( value * 10 ) + ( c - wxT('0') );
			++digit;
		} else if ( c == wxT('.') ) {
			if ( digit && value <= 255 ) {
				tmp_ip = tmp_ip | value << ( field * 8 );

				// Rest the current field values
				value = digit = 0;
				++field;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	// Only set the referenced value if it was a valid IP
	if ( field == 3 && digit ) {
		Ip = tmp_ip | value << 24;
		return true;
	}

	return false;
}


uint32 StringHosttoUint32(const wxString &Host)
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




/**
 * Used to store the ranges.
 */
struct IPRange
{
	const wxChar *addr;
	unsigned int mask;
};


const IPRange ranges[] = {
//	Here is reserved blocks from RFC 3330 at http://www.rfc-editor.org/rfc/rfc3330.txt
//
//Address Block               Present Use                           Reference
//---------------------------------------------------------------------------
{ wxT("0.0.0.0"),        8 }, // "This" Network             [RFC1700, page 4]
{ wxT("10.0.0.0"),       8 }, // Private-Use Networks               [RFC1918]
{ wxT("14.0.0.0"),       8 }, // Public-Data Networks     [RFC1700, page 181]
{ wxT("24.0.0.0"),       8 }, // Cable Television Networks                 --
{ wxT("39.0.0.0"),       8 }, // Reserved but subject
                              //    to allocation                   [RFC1797]
{ wxT("127.0.0.0"),      8 }, // Loopback                   [RFC1700, page 5]
{ wxT("128.0.0.0"),     16 }, // Reserved but subject
                              //    to allocation                          --
{ wxT("169.254.0.0"),   16 }, // Link Local                                --
{ wxT("172.16.0.0"),    12 }, // Private-Use Networks               [RFC1918]
{ wxT("191.255.0.0"),   16 }, // Reserved but subject
                              //    to allocation                          --
{ wxT("192.0.0.0"),     24 }, // Reserved but subject          
                              //    to allocation                          --
{ wxT("192.0.2.0"),     24 }, // Test-Net
{ wxT("192.88.99.0"),   24 }, // 6to4 Relay Anycast                 [RFC3068]
{ wxT("192.168.0.0"),   16 }, // Private-Use Networks               [RFC1918]
{ wxT("198.18.0.0"),    15 }, // Network Interconnect
                              //    Device Benchmark Testing        [RFC2544]
{ wxT("223.255.255.0"), 24 }, // Reserved but subject
                              //    to allocation                          --
{ wxT("224.0.0.0"),      4 }, // Multicast                          [RFC3171]
{ wxT("240.0.0.0"),      4 }  // Reserved for Future Use    [RFC1700, page 4]
};


struct filter_st {
	in_addr_t addr;		// Address and mask in anti-host order.
	in_addr_t mask;
};

const int number_of_ranges = sizeof(ranges) / sizeof(IPRange);
static filter_st filters[number_of_ranges];


// This function is used to initialize the IP filter
bool SetupFilter()
{
	for (int i = 0; i < number_of_ranges; ++i) {
		filters[i].addr = StringIPtoUint32( ranges[i].addr );
		filters[i].mask = ~wxUINT32_SWAP_ALWAYS((1 << (32 - ranges[i].mask)) - 1);
	}
	return true;
}


// This is a little trick to ensure that the filter-list is initialized before
// it gets used, while not risking threading problems.
static bool filterSetup = SetupFilter();


bool IsGoodIP(uint32 IP)
{
	for (int i = 0; i < number_of_ranges; ++i) {
		if (((IP ^ filters[i].addr) & filters[i].mask) == 0) {
			return false;
		}
	}

	return true;
}



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


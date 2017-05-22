//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2016 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "NetworkFunctions.h"	// Interface declaration
#include "amuleIPV4Address.h"
#include <common/Macros.h>	// Needed for itemsof()


bool StringIPtoUint32(const wxString &strIP, uint32& Ip)
{
	// The current position in the current field, used to detect malformed fields (x.y..z).
	unsigned digit = 0;

	// The current field, used to ensure only IPs that looks like a.b.c.d are supported
	unsigned field = 0;

	// The value of the current field
	unsigned value = 0;

	// Stores the work-value of the IP, reference is not changed unless the str was valid
	uint32 tmp_ip = 0;

	wxString str = strIP.Strip( wxString::both );
	for (size_t i = 0; i < str.Length(); i++) {
		wxChar c = str.GetChar( i );

		if ( c >= wxT('0') && c <= wxT('9') && (value >> 8) == 0) {
			value = ( value * 10 ) + ( c - wxT('0') );
			++digit;
		} else if ( c == wxT('.') ) {
			if ( digit && (value >> 8) == 0) {
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
	if ( field == 3 && digit && (value >> 8) == 0) {
		Ip = tmp_ip | value << 24;
		return true;
	}

	return false;
}


uint32 StringHosttoUint32(const wxString &Host)
{
	if (Host.IsEmpty()) {
		return 0;
	}
	amuleIPV4Address solver;
	if (solver.Hostname(Host)) {
		uint32 result = StringIPtoUint32(solver.IPAddress());
		if (result != (uint32)-1) {	// should not happen
			return result;
		}
	}
	// This actually happens on wrong hostname
	return 0;
}


struct filter_st {
	uint32 addr;		// Address in anti-host order.
	uint32 mask;		// Mask in anti-host order.
};


static filter_st reserved_ranges[] = {
//	Here are the reserved blocks from RFC 3330 at http://www.rfc-editor.org/rfc/rfc3330.txt
//
//                                         Address Block      Present Use                                     Reference
//---------------------------------------------------------------------------------------------------------------------
	{ 0x00000000, 0x000000ff },	// 0.0.0.0/8          "This" Network                          [RFC1700, page 4]
	// According to RFC3330, 14.* and 24.* must be parsed as normal IPs.
	// { 0x0000000e, 0x000000ff },	// 14.0.0.0/8         Public-Data Networks                  [RFC1700, page 181]
	// { 0x00000018, 0x000000ff },	// 24.0.0.0/8         Cable Television Networks                              --
	{ 0x00000027, 0x000000ff },	// 39.0.0.0/8         Reserved but subject to allocation              [RFC1797]
	{ 0x0000007f, 0x000000ff },	// 127.0.0.0/8        Loopback                                [RFC1700, page 5]
	{ 0x00000080, 0x0000ffff },	// 128.0.0.0/16       Reserved but subject to allocation                     --
	{ 0x0000fea9, 0x0000ffff },	// 169.254.0.0/16     Link Local                                             --
	{ 0x0000ffbf, 0x0000ffff },	// 191.255.0.0/16     Reserved but subject to allocation                     --
	{ 0x000000c0, 0x00ffffff },	// 192.0.0.0/24       Reserved but subject to allocation                     --
	{ 0x000200c0, 0x00ffffff },	// 192.0.2.0/24       Test-Net
	{ 0x006358c0, 0x00ffffff },	// 192.88.99.0/24     6to4 Relay Anycast                              [RFC3068]
	{ 0x000012c6, 0x0000feff },	// 198.18.0.0/15      Network Interconnect Device Benchmark Testing   [RFC2544]
	{ 0x00ffffdf, 0x00ffffff },	// 223.255.255.0/24   Reserved but subject to allocation                     --
	{ 0x000000e0, 0x000000f0 },	// 224.0.0.0/4        Multicast                                       [RFC3171]
	{ 0x000000f0, 0x000000f0 }	// 240.0.0.0/4        Reserved for Future Use                 [RFC1700, page 4]
};


// Private-Use Networks [RFC1918]
static filter_st lan_ranges[] = {
	{ 0x0000000a, 0x000000ff },	// 10.0.0.0/8
	{ 0x000010ac, 0x0000f0ff },	// 172.16.0.0/12
	{ 0x0000a8c0, 0x0000ffff }	// 192.168.0.0/16
};


bool IsGoodIP(uint32 ip, bool filterLAN) throw()
{
	for (unsigned int i = 0; i < itemsof(reserved_ranges); ++i) {
		if (((ip ^ reserved_ranges[i].addr) & reserved_ranges[i].mask) == 0) {
			return false;
		}
	}

	return !(filterLAN && IsLanIP(ip));
}

bool IsLanIP(uint32_t ip) throw()
{
	for (unsigned int i = 0; i < itemsof(lan_ranges); ++i) {
		if (((ip ^ lan_ranges[i].addr) & lan_ranges[i].mask) == 0) {
			return true;
		}
	}
	return false;
}

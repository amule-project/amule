//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2007 Carlo Wood ( carlo@alinoe.com )
// Copyright (c) 2004-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef AMULEIPV4ADDRESS_H
#define AMULEIPV4ADDRESS_H

#include <wx/object.h>			// Needed by wx/sckaddr.h

#include "NetworkFunctions.h"	// Needed for unicode2char


// This is fscking hard to maintain. wxWidgets 2.5.2 has changed internal
// ipaddress structs.
// prevent fscking dns queries
class amuleIPV4Address : public wxIPV4address
{
public:
	amuleIPV4Address() {}
	amuleIPV4Address(const wxIPV4address &a) : wxIPV4address(a) {}

	virtual bool Hostname(const wxString& name) {
		// Some people are sometimes fools.
		if (name.IsEmpty()) {
//			wxASSERT(0);
			return false;
		}
		
		return Hostname(StringIPtoUint32(name));
	}

	virtual bool Hostname(uint32 ip) {
		// Some people are sometimes fools.
		if (!ip) {
//			wxASSERT(0);
			return false;
		}
		
		// We have to take care that wxIPV4address's internals changed on 2.5.2
		return GAddress_INET_SetHostAddress(m_address,wxUINT32_SWAP_ALWAYS(ip))==GSOCK_NOERROR;
	}
};

#endif // AMULEIPV4ADDRESS_H
// File_checked_for_headers

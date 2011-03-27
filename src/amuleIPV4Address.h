//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Carlo Wood ( carlo@alinoe.com )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "NetworkFunctions.h"	// Needed for StringIPtoUint32

class amuleIPV4Address : public wxIPV4address
{
public:
	amuleIPV4Address() {}
	amuleIPV4Address(const wxIPV4address &a) : wxIPV4address(a) {}

	virtual bool Hostname(const wxString& name)
	{
		// Some people are sometimes fools.
		if (name.IsEmpty()) {
			return false;
		}
		
		return wxIPV4address::Hostname(name);
	}

	virtual bool Hostname(uint32 ip) 
	{
		// Some people are sometimes fools.
		if (!ip) {
			return false;
		}
		
		return wxIPV4address::Hostname(Uint32toStringIP(ip));
	}
};

#endif // AMULEIPV4ADDRESS_H
// File_checked_for_headers

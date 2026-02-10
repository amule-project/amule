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

#include "NetworkFunctions.h"	// Needed for Uint32toStringIP

class amuleIPV4Address
{
public:
	amuleIPV4Address();
	amuleIPV4Address(const amuleIPV4Address &a);
	amuleIPV4Address(const class CamuleIPV4Endpoint &ep);
	virtual ~amuleIPV4Address();
	amuleIPV4Address& operator=(const amuleIPV4Address &a);
	amuleIPV4Address& operator=(const class CamuleIPV4Endpoint &a);

	virtual bool Hostname(const wxString& name);

	virtual bool Hostname(uint32 ip)
	{
		// Some people are sometimes fools.
		if (!ip) {
			return false;
		}

		return Hostname(Uint32toStringIP(ip));
	}


	// Set the port to that corresponding to the specified service.
	// Returns true on success, false if something goes wrong (invalid service).
	virtual bool Service(uint16 service);

	// Returns the current service.
	virtual uint16 Service() const;

	// Determines if current address is set to localhost.
	virtual bool IsLocalHost() const;

	// Returns a wxString containing the IP address.
	virtual wxString IPAddress() const;

	// Set address to any of the addresses of the current machine.
	virtual bool AnyAddress();

	const class CamuleIPV4Endpoint & GetEndpoint() const;
	class CamuleIPV4Endpoint & GetEndpoint();

private:
	class CamuleIPV4Endpoint * m_endpoint;
};

#endif // AMULEIPV4ADDRESS_H
// File_checked_for_headers

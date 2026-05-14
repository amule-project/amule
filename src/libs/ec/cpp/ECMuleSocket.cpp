//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
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

#include "ECMuleSocket.h"

#include "../../../amuleIPV4Address.h"
#include "../../../NetworkFunctions.h"

#include <boost/system/error_code.hpp>

//
// CECMuleSocket API - User interface functions
//

CECMuleSocket::CECMuleSocket(bool use_events)
:
CECSocket(use_events)
{
	Notify(use_events);
}

CECMuleSocket::~CECMuleSocket()
{
}

bool CECMuleSocket::ConnectSocket(amuleIPV4Address& address)
{
	return CECSocket::ConnectSocket(StringIPtoUint32(address.IPAddress()),address.Service());
}


bool CECMuleSocket::InternalConnect(uint32_t ip, uint16_t port, bool wait) {
	amuleIPV4Address addr;
	addr.Hostname(Uint32toStringIP(ip));
	addr.Service(port);
	return CLibSocket::Connect(addr, wait);
}

int CECMuleSocket::InternalGetLastError()
{
	switch (LastError()) {
		case boost::system::errc::success:
			return EC_ERROR_NOERROR;
		case boost::system::errc::address_family_not_supported:
		case boost::system::errc::address_in_use:
		case boost::system::errc::address_not_available:
		case boost::system::errc::bad_address:
		case boost::system::errc::invalid_argument:
			return EC_ERROR_INVADDR;
		case boost::system::errc::already_connected:
		case boost::system::errc::connection_already_in_progress:
		case boost::system::errc::not_connected:
			return EC_ERROR_INVOP;
		case boost::system::errc::connection_aborted:
		case boost::system::errc::connection_reset:
		case boost::system::errc::io_error:
		case boost::system::errc::network_down:
		case boost::system::errc::network_reset:
		case boost::system::errc::network_unreachable:
			return EC_ERROR_IOERR;
		case boost::system::errc::connection_refused:
		case boost::system::errc::host_unreachable:
			return EC_ERROR_NOHOST;
		case boost::system::errc::not_a_socket:
			return EC_ERROR_INVSOCK;
		case boost::system::errc::not_enough_memory:
			return EC_ERROR_MEMERR;
		case boost::system::errc::operation_would_block:
			return EC_ERROR_WOULDBLOCK;
		case boost::system::errc::timed_out:
			return EC_ERROR_TIMEDOUT;
		default:
			return EC_ERROR_UNKNOWN;
	}
}

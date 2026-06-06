//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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


// EC-connection keepalive timings. With these values, a half-open
// connection (peer crashed / network blip / FIN lost) is torn down at
// the TCP layer in ~60s instead of sitting idle until the default
// ~2h TCP retransmit timeout, so CECSocket::OnLost fires and the GUI
// can flip to "Connection lost" instead of looking wedged. Same
// constants used by CECServerSocket on the amuled side so detection
// is symmetric. Numbers picked to balance responsiveness against the
// keepalive packet overhead (one probe per 10s after 30s idle).
namespace {
	const int EC_KEEPALIVE_IDLE_SEC      = 30;
	const int EC_KEEPALIVE_INTERVAL_SEC  = 10;
	const int EC_KEEPALIVE_PROBE_COUNT   = 3;
}

bool CECMuleSocket::InternalConnect(uint32_t ip, uint16_t port, bool wait) {
	amuleIPV4Address addr;
	addr.Hostname(Uint32toStringIP(ip));
	addr.Service(port);
	bool ok = CLibSocket::Connect(addr, wait);
	if (ok) {
		// Asio opens the socket fd during connect / async_connect, so
		// setsockopt is valid here regardless of sync vs async mode.
		ApplyEcKeepalive();
	}
	return ok;
}

void CECMuleSocket::ApplyEcKeepalive() {
	CLibSocket::EnableTcpKeepalive(
		EC_KEEPALIVE_IDLE_SEC,
		EC_KEEPALIVE_INTERVAL_SEC,
		EC_KEEPALIVE_PROBE_COUNT);
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

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

#ifdef ASIO_SOCKETS
#include <boost/system/error_code.hpp>
#endif

//-------------------- CECSocketHandler --------------------

#define	EC_SOCKET_HANDLER	(wxID_HIGHEST + 644)

class CECMuleSocketHandler: public wxEvtHandler {
 public:
        CECMuleSocketHandler() {};

 private:
        void SocketHandler(wxSocketEvent& event);

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CECMuleSocketHandler, wxEvtHandler)
        EVT_SOCKET(EC_SOCKET_HANDLER, CECMuleSocketHandler::SocketHandler)
END_EVENT_TABLE()

void CECMuleSocketHandler::SocketHandler(wxSocketEvent& event)
{
        CECSocket *socket = dynamic_cast<CECSocket *>(event.GetSocket());
        wxCHECK_RET(socket, wxT("Socket event with a NULL socket!"));

        switch(event.GetSocketEvent()) {
        case wxSOCKET_LOST:
            socket->OnLost();
            break;
        case wxSOCKET_INPUT:
            socket->OnInput();
            break;
        case wxSOCKET_OUTPUT:
            socket->OnOutput();
            break;
        case wxSOCKET_CONNECTION:
            socket->OnConnect();
            break;

        default:
            // Nothing should arrive here...
            wxFAIL;
            break;
        }
}

static CECMuleSocketHandler	g_ECSocketHandler;

//
// CECMuleSocket API - User interface functions
//

CECMuleSocket::CECMuleSocket(bool use_events)
:
CECSocket(use_events)
{
	if ( use_events ) {
		SetEventHandler(g_ECSocketHandler, EC_SOCKET_HANDLER);
		SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG |
			  wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG);
		Notify(true);
		SetFlags(wxSOCKET_NOWAIT);
	} else {
		SetFlags(wxSOCKET_WAITALL | wxSOCKET_BLOCK);
		Notify(false);
	}
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
#ifdef ASIO_SOCKETS
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
#else
		case wxSOCKET_NOERROR:
			return EC_ERROR_NOERROR;
		case wxSOCKET_INVOP:
			return EC_ERROR_INVOP;
		case wxSOCKET_IOERR:
			return EC_ERROR_IOERR;
		case wxSOCKET_INVADDR:
			return EC_ERROR_INVADDR;
		case wxSOCKET_INVSOCK:
			return EC_ERROR_INVSOCK;
		case wxSOCKET_NOHOST:
			return EC_ERROR_NOHOST;
		case wxSOCKET_INVPORT:
			return EC_ERROR_INVPORT;
		case wxSOCKET_WOULDBLOCK:
			return EC_ERROR_WOULDBLOCK;
		case wxSOCKET_TIMEDOUT:
			return EC_ERROR_TIMEDOUT;
		case wxSOCKET_MEMERR:
			return EC_ERROR_MEMERR;
#endif
		default:
			return EC_ERROR_UNKNOWN;
	}
}

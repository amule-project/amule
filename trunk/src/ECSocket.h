//
// This file is part of the aMule Project.
//
// Copyright (c) 2004 aMule Team (http://www.amule-project.net)
// Copyright (c) Angel Vidal Veiga (kry@users.sourceforge.net)
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


#ifndef ECSOCKET_H
#define ECSOCKET_H

#include <wx/string.h>
#include <wx/socket.h>
#include "types.h"

enum aMuleECSocketType {
	AMULE_EC_CLIENT,
	AMULE_EC_SERVER
};

class CECPacket;

/*! \class ECSocket
 *
 * \brief Socket handler for External Communications (EC).
 *
 * ECSocket takes care of the transmission of EC packets
 *
 * \todo Implement compression.
 */

#ifdef AMULE_DAEMON
class ECSocket : public wxEvtHandler
#else
class ECSocket
#endif
{
	public:
		//
		// Constructors/Destructor
		//
		ECSocket();
		ECSocket(wxSockAddress& address, wxEvtHandler *handler, int id = -1);

		~ECSocket();

		//
		// Base
		//
		bool Destroy() { return m_sock->Destroy(); }
		bool Ok() const { return m_sock->Ok(); }

		//
		// Client
		//
		bool Connect(wxSockAddress& address, bool wait = true) { return ((wxSocketClient *)m_sock)->Connect(address, wait); }
		bool WaitOnConnect(long seconds = -1, long milliseconds = 0) { return ((wxSocketClient *)m_sock)->WaitOnConnect(seconds, milliseconds); }
		bool IsConnected() { return ((wxSocketClient *)m_sock)->IsConnected(); }

		//
		// Server
		//
		wxSocketBase *Accept(bool wait = true) { return ((wxSocketServer *)m_sock)->Accept(wait); }
		bool WaitForAccept(long seconds, long millisecond )
		{
			return ((wxSocketServer *)m_sock)->WaitForAccept(seconds, millisecond);
		}
		//
		// Packet I/O
		//
		CECPacket * ReadPacket(wxSocketBase *sock);
		bool WritePacket(wxSocketBase *sock, const CECPacket *packet);

		// These 4 methods are to be used by CECPacket & CECTag
		bool ReadNumber(wxSocketBase *sock, void *buffer, unsigned int len);
		bool ReadBuffer(wxSocketBase *sock, void *buffer, unsigned int len);

		bool WriteNumber(wxSocketBase *sock, const void *buffer, unsigned int len);
		bool WriteBuffer(wxSocketBase *sock, const void *buffer, unsigned int len);

		//
		// Wrapper functions for client sockets
		//
		CECPacket * ReadPacket(void) { return ReadPacket(m_sock); }
		bool WritePacket(const CECPacket *packet) { return WritePacket(m_sock, packet); }

		bool ReadNumber(void *buffer, unsigned int len) { return ReadNumber(m_sock, buffer, len); }
		bool ReadBuffer(void *buffer, unsigned int len) { return ReadBuffer(m_sock, buffer, len); }

		bool WriteNumber(const void *buffer, unsigned int len) { return WriteNumber(m_sock, buffer, len); }
		bool WriteBuffer(const void *buffer, unsigned int len) { return WriteBuffer(m_sock, buffer, len); }

	private:
		aMuleECSocketType m_type;
		wxSocketBase *m_sock;
};

#endif // ECSOCKET_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal Veiga ( kry@users.sourceforge.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef ECSOCKET_H
#define ECSOCKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ECSocket.h"
#endif

#include <wx/string.h>
#include <wx/socket.h>
#include "Types.h"

enum aMuleECSocketType {
	AMULE_EC_CLIENT,
	AMULE_EC_SERVER
};

class CECPacket;

//
// Socket registry functions
//
void RegisterSocket(wxSocketBase*);
void UnregisterSocket(wxSocketBase*);

/*! \class ECSocket
 *
 * \brief Socket handler for External Communications (EC).
 *
 * ECSocket takes care of the transmission of EC packets
 */

class ECSocket {
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
		bool Destroy() { UnregisterSocket(m_sock); return m_sock->Destroy(); }
		bool Ok() const { return m_sock->Ok(); }
		void SetFlags(wxSocketFlags flags) { if (m_sock) m_sock->SetFlags(flags); }


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
		bool WaitForAccept(long seconds, long millisecond ) { return ((wxSocketServer *)m_sock)->WaitForAccept(seconds, millisecond); }

		//
		// Packet I/O
		//
		CECPacket * ReadPacket(wxSocketBase *sock);
		bool WritePacket(wxSocketBase *sock, const CECPacket *packet);

		// These 4 methods are to be used by CECPacket & CECTag
		bool ReadNumber(wxSocketBase *sock, void *buffer, unsigned int len, void *opaque);
		bool ReadBuffer(wxSocketBase *sock, void *buffer, unsigned int len, void *opaque);

		bool WriteNumber(wxSocketBase *sock, const void *buffer, unsigned int len, void *opaque);
		bool WriteBuffer(wxSocketBase *sock, const void *buffer, unsigned int len, void *opaque);

		//
		// Wrapper functions for client sockets
		//
		CECPacket * ReadPacket(void) { return ReadPacket(m_sock); }
		bool WritePacket(const CECPacket *packet) { return WritePacket(m_sock, packet); }

//		bool ReadNumber(void *buffer, unsigned int len) { return ReadNumber(m_sock, buffer, len); }
//		bool ReadBuffer(void *buffer, unsigned int len) { return ReadBuffer(m_sock, buffer, len); }

//		bool WriteNumber(const void *buffer, unsigned int len) { return WriteNumber(m_sock, buffer, len); }
//		bool WriteBuffer(const void *buffer, unsigned int len) { return WriteBuffer(m_sock, buffer, len); }

	private:
		uint32	ReadFlags(wxSocketBase *);
		bool	WriteFlags(wxSocketBase *, uint32);
		aMuleECSocketType m_type;
		wxSocketBase *m_sock;
};

#endif // ECSOCKET_H

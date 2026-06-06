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

#ifndef ECMULESOCKET_H
#define ECMULESOCKET_H

#include "../../../LibSocket.h"
#include "ECSocket.h"

/*! \class CECMuleSocket
 *
 * \brief Socket handler for External Communications (EC)
 */

class CECMuleSocket : public CECSocket,  public CLibSocket {
public:
	CECMuleSocket(bool use_events);
	virtual ~CECMuleSocket();

	bool ConnectSocket(class amuleIPV4Address& address);

	virtual void OnConnect()	{}					// This is overwritten in RemoteConnect
	virtual void OnConnect(int)	{ OnConnect(); }	// This is called from LibSocketAsio
	virtual void OnSend(int)	{ OnOutput(); }
	virtual void OnReceive(int)	{ OnInput(); }
	// CLibSocket::OnLost(int) fires from the Asio reactor when the
	// peer FIN reaches our kernel (HandleRead returning bytes=0 or
	// an EOF error_code). Forward to the EC-layer CECSocket::OnLost()
	// virtual so CRemoteConnect / CECServerSocket overrides actually
	// run — without this the empty CLibSocket::OnLost(int){} default
	// would swallow the notification and the UI would stay "connected"
	// even after the connection died at the TCP layer.
	//
	// The cast to CECSocket* forces virtual dispatch through the EC
	// vtable, so an instance of CRemoteConnect / CECServerSocket
	// reaches its override.  A qualified `CECSocket::OnLost()` call
	// would bypass virtual dispatch and only run the empty base.
	virtual void OnLost(int)	{ static_cast<CECSocket *>(this)->OnLost(); }

	// Apply EC-tuned TCP keepalive (idle=30s / probe=10s / count=3 →
	// ~60s half-open detection). Called automatically from
	// InternalConnect after a successful client-side connect; subclasses
	// that take over OnConnect (CRemoteConnect) and the server-side
	// accept path (ExternalConn.cpp::OnAccept on amuled) call this
	// explicitly so detection is symmetric on both ends of every EC
	// connection.
	void ApplyEcKeepalive();

private:
	bool InternalConnect(uint32_t ip, uint16_t port, bool wait);

	int InternalGetLastError();

	bool InternalWaitOnConnect(long secs = -1, long msecs = 0) { return CLibSocket::WaitOnConnect(secs,msecs); };
	bool InternalWaitForWrite(long secs = -1, long msecs = 0) { return CLibSocket::WaitForWrite(secs,msecs); };
	bool InternalWaitForRead(long secs = -1, long msecs = 0) { return CLibSocket::WaitForRead(secs,msecs); };

	bool InternalError() { return CLibSocket::LastError() != 0; }
	void InternalClose() { CLibSocket::Close(); }

	uint32 InternalRead(void* ptr, uint32 len)			{ return CLibSocket::Read(ptr, len); };
	uint32 InternalWrite(const void* ptr, uint32 len)	{ return CLibSocket::Write(ptr, len); };

	bool InternalIsConnected() { return CLibSocket::IsConnected(); }
	void InternalDestroy() { CLibSocket::Destroy(); }
};

#endif // ECMULESOCKET_H

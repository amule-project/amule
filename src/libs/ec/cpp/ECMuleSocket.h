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

#ifndef ECMULESOCKET_H
#define ECMULESOCKET_H

#include "../../../LibSocket.h"
#include "ECSocket.h"

/*! \class CECMuleSocket
 *
 * \brief Socket handler for External Communications (EC), wx implementation
 *
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

//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2006 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#include <wx/socket.h>		// Needed for wxSocketClient
#include <ec/ECSocket.h>

/*! \class CECMuleSocket
 *
 * \brief Socket handler for External Communications (EC), wx implementation
 *
 */

class CECMuleSocket : public CECSocket,  public wxSocketClient {
public:
	CECMuleSocket(bool use_events);
	virtual ~CECMuleSocket();

	bool ConnectSocket(wxIPV4address& address);

 private:
 	bool InternalConnect(uint32_t ip, uint16_t port, bool wait);
	
 	int InternalGetLastError();
 
	uint32_t InternalLastCount() { return wxSocketClient::LastCount(); };
	bool InternalWaitOnConnect(long secs = -1, long msecs = 0) { return wxSocketClient::WaitOnConnect(secs,msecs); };
	bool InternalWaitForWrite(long secs = -1, long msecs = 0) { return wxSocketClient::WaitForRead(secs,msecs); };
	bool InternalWaitForRead(long secs = -1, long msecs = 0) { return wxSocketClient::WaitForWrite(secs,msecs); };
	
	bool InternalError() { return wxSocketClient::Error(); }
	void InternalClose() { wxSocketClient::Close(); }
	
	void InternalRead(void* ptr, uint32_t len) { wxSocketClient::Read(ptr, len); };
	void InternalWrite(const void* ptr, uint32_t len) { wxSocketClient::Write(ptr, len); };
	
	bool InternalIsConnected() { return wxSocketClient::IsConnected(); }
	void InternalDestroy() { wxSocketClient::Destroy(); }
};

#endif // ECMULESOCKET_H

//
// This file is part of the aMule project.
//
// Copyright (c) 2004 aMule Project (http://www.amule-prpject.net)
// Copyright (c) 2004 Angel Vidal Veiga (kry@users.sourceforge.net)
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


#include "ECSocket.h"

#include "endianfix.h"		// For EndianSwap
#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR

#include "ECPacket.h"		// Needed for CECPacket

ECSocket::ECSocket()
{
	m_type = AMULE_EC_CLIENT;
	m_sock = new wxSocketClient();
	m_firstMessage = true;
}


ECSocket::ECSocket(wxSockAddress& address, wxEvtHandler *handler, int id)
{
	m_type = AMULE_EC_SERVER;
	m_sock = new wxSocketServer(address, wxSOCKET_WAITALL|wxSOCKET_REUSEADDR);
	if(m_sock->Ok() && handler) {
		// Setup the event handler and subscribe to connection events
		m_sock->SetEventHandler(*handler, id);
		m_sock->SetNotify(wxSOCKET_CONNECTION_FLAG);
		m_sock->Notify(true);
	}
	m_firstMessage = true;
}


ECSocket::~ECSocket()
{
	delete m_sock;
}


bool ECSocket::ReadNumber(wxSocketBase *sock, void *buffer, unsigned int len)
{
	sock->Read(buffer, len);
#if wxBYTE_ORDER != wxLITTLE_ENDIAN
	switch (len) {
		case 2: ENDIAN_SWAP_I_16(buffer); break;
		case 4: ENDIAN_SWAP_I_32(buffer); break;
	}
#endif
	return (sock->LastCount() == len);
}


bool ECSocket::WriteNumber(wxSocketBase *sock, const void *buffer, unsigned int len)
{
#if wxBYTE_ORDER == wxLITTLE_ENDIAN
	sock->Write(buffer, len);
#else
	char tmp[8];

	switch (len) {
		case 1: *((int8 *)tmp) = *((int8 *)buffer); break;
		case 2: *((int16 *)tmp) = ENDIAN_SWAP_16(*((int16 *)buffer));
		case 4: *((int32 *)tmp) = ENDIAN_SWAP_32(*((int32 *)buffer));
	}
	sock->Write(tmp, len);
#endif
	return (sock->LastCount() == len);
}


bool ECSocket::ReadBuffer(wxSocketBase *sock, void *buffer, unsigned int len)
{
	unsigned int msgRemain = len;
	unsigned int LastIO;
	char *iobuf = (char *)buffer;
	bool error = sock->Error();

	while (msgRemain && !error) {
		sock->Read(iobuf, msgRemain);
		LastIO = sock->LastCount();
		error = sock->Error();
		msgRemain -= LastIO;
		iobuf += LastIO;
	}
	return !error;
}


bool ECSocket::WriteBuffer(wxSocketBase *sock, const void *buffer, unsigned int len)
{
	unsigned int msgRemain = len;
	unsigned int LastIO;
	const char *iobuf = (const char *)buffer;
	bool error = sock->Error();

	while(msgRemain && !error) {
		sock->Write(iobuf, msgRemain);
		LastIO = sock->LastCount();
		error = sock->Error();
		msgRemain -= LastIO;
		iobuf += LastIO;
	}
	return !error;
}


bool ECSocket::WritePacket(wxSocketBase *sock, const CECPacket *packet)
{
	uint8 flags = 0x20;

	// TODO: Compression not yet implemented !
	// if (packet.GetPacketLength() > 1024) flags |= 0x01;

	if (!WriteNumber(sock, &flags, 1)) return false;
	return packet->WritePacket(sock, *this);
}


CECPacket * ECSocket::ReadPacket(wxSocketBase *sock)
{
	uint8 flags;

	if (!ReadNumber(sock, &flags, 1)) return NULL;
	if ((flags & 0x60) != 0x20) {
		// Protocol error - other end might use an older protocol
		return NULL;
	}
	if ((flags & 0x01) != 0) {
		// TODO: compression not implemented yet !
	}
	CECPacket *p = new CECPacket(sock, *this);
	if (p->m_error != 0) {
		delete p;
		return NULL;
	} else {
		return p;
	}
}

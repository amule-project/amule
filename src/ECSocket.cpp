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


void ECSocket::Read(wxSocketBase *sock, uint8& i)
{
	sock->Read(&i, 1);
	if (sock->LastCount() < 1) i = 0;
};


void ECSocket::Write(wxSocketBase *sock, const uint8& i)
{
	sock->Write(&i, 1);
};


void ECSocket::Read(wxSocketBase *sock, uint16& i)
{
	sock->Read(&i, 2);
	if (sock->LastCount() < 2) i = 0;
	ENDIAN_SWAP_I_16(i);
};


void ECSocket::Write(wxSocketBase *sock, const uint16& i)
{
	int16 tmp = ENDIAN_SWAP_16(i);
	sock->Write(&tmp, 2);
};


void ECSocket::Read(wxSocketBase *sock, uint32& i)
{
	sock->Read(&i, 4);
	if (sock->LastCount() < 4) i = 0;
	ENDIAN_SWAP_I_32(i);
};


void ECSocket::Write(wxSocketBase *sock, const uint32& i)
{
	int32 tmp = ENDIAN_SWAP_32(i);
	sock->Write(&tmp, 4);
};


#if 0
void ECSocket::Read(wxSocketBase *sock, uint64& i)
{
	sock->Read(&i, 8);
	if (sock->LastCount() < 8) i = 0;
	ENDIAN_SWAP_I_64(i);
};


void ECSocket::Write(wxSocketBase *sock, const uint64& v)
{
	int64 tmp = ENDIAN_SWAP_64(v);
	sock->Write(&tmp, 8);
};
#endif

// Already included via ECPacket.h <- otherfunctions.h
//static wxCSConv aMuleConv(wxT("iso8859-1"));


bool ECSocket::Read(wxSocketBase *sock, wxString& s)
{
	uint32 msgBytes;		// NEVER send/receive types like
	// 'unsigned int' through the network, you MUST NOT assume that
	// the other end has the same size for int.
	Read(sock, msgBytes);
	if (m_firstMessage) {
		// Server side
		if (msgBytes == 0 ||		// Socket error
		        msgBytes == 0x41555448 ||	// 'AUTH' - rc5 client w/o unicode
		        msgBytes == 0x00410055 ||	// little-endian rc5 client w/ unicode
		        msgBytes == 0x41005500 ||	// big-endian rc5 client w/ unicode
		        msgBytes == 0x41636365 ||	// 'Acce' from 'Access Denied' - rc5 server w/o unicode
		        msgBytes == 0x00410063 ||	// little-endian rc5 server w/unicode
		        msgBytes == 0x41006300 ||	// big-endia rc5 server w/ unicode
		        msgBytes == 0x0e004155 ||	// rc6 client with sizeof(int)=2
		        msgBytes > 0x00080000) {	// Unexpected behaviour - message length > 1Mb
			s = wxString(wxT("Access Denied"));	// new client, old (rc5) server
			// we must signal the client the error, and it also does not
			// disturb the other cases
			return false;
		}
		m_firstMessage = false;
	} else {
		if (msgBytes == 0 ||		// Socket error
		        msgBytes > 0x00080000) {	// Don't allow messages larger than 1Mb
			s = wxString(wxT(""));
			return false;
		}
	}
	
	char *utf8 = new char[msgBytes+1];
	utf8[msgBytes] = 0;
	
	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	char *iobuf = utf8;
	bool error = false;
	
	while (msgRemain && !error) {
		sock->Read(iobuf, msgRemain);
		LastIO = sock->LastCount();
		error = sock->Error();
		msgRemain -= LastIO;
		iobuf += LastIO;
	}
	
	// Converts an UTF-8 string to ISO-8859-1
	s = wxString(wxConvUTF8.cMB2WC(utf8), aMuleConv);
	delete [] utf8;
	
	if(error) {
		printf("ECSocket::Read:Error reading wxString Packet!\n");
	}

	return !error;
};


bool ECSocket::Write(wxSocketBase *sock, const wxString& s)
{
	// Converts a string in ISO-8859-1 to wide char, and then
	// converts it to to multi-byte (encoded) UTF-8
	const wxCharBuffer buf = wxConvUTF8.cWC2MB(s.wc_str(aMuleConv));
	const char *utf8 = (const char *)buf;
	// strlen does not like NULL pointers, so we test.
	if (!utf8) {
		printf("ECSocket::Write: Error converting string.\n");
		return false;
	}
	uint32 msgBytes = strlen(utf8);
	Write(sock, msgBytes);

	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	const char *iobuf = utf8;
	bool error = sock->Error();	// We might had an error sending the length !

	while(msgRemain && !error) {
		sock->Write(iobuf, msgRemain);
		LastIO = sock->LastCount();
		error = sock->Error();
		msgRemain -= LastIO;
		iobuf += LastIO;
	}

	return !error;
};


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


//
// Client SendRecvMsg()
//
wxString ECSocket::SendRecvMsg(const wxString &msg)
{
	return SendRecvMsg(m_sock, msg);
}


//
// Server SendRecvMsg()
//
wxString ECSocket::SendRecvMsg(wxSocketBase *sock, const wxString &msg)
{
	bool WriteOK = Write(sock, msg);
	wxString response;
	if (WriteOK) {
		// Wait until data available (will also return if the connection is lost)
		sock->WaitForRead(10);
		if (sock->IsData()) {
			Read(sock, response);
		}
	}
	return response;
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
    return new CECPacket(sock, *this);
}

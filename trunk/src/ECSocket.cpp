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


ECSocket::ECSocket()
{
	m_type = AMULE_EC_CLIENT;
	m_sock = new wxSocketClient();
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
}


ECSocket::~ECSocket()
{
	delete m_sock;
}


bool ECSocket::Read(wxSocketBase *sock, uint8& i)
{
	sock->Read(&i, 1);
	
	return ( sock->LastCount() == 1 ) && !sock->Error();
};
	

bool ECSocket::Write(wxSocketBase *sock, const uint8& i)
{
	sock->Write(&i, 1);
	
	return ( sock->LastCount() == 1 ) && !sock->Error();
};


bool ECSocket::Read(wxSocketBase *sock, uint16& i)
{
	sock->Read(&i, 2);
	ENDIAN_SWAP_I_16(i);
	
	return ( sock->LastCount() == 2 ) && !sock->Error();
};


bool ECSocket::Write(wxSocketBase *sock, const uint16& i)
{
	int16 tmp = ENDIAN_SWAP_16(i);
	sock->Write(&tmp, 2);

	return ( sock->LastCount() == 2 ) && !sock->Error();
};

bool ECSocket::Read(wxSocketBase *sock, uint32& i)
{
	sock->Read(&i, 4);
	ENDIAN_SWAP_I_32(i);
	
	return ( sock->LastCount() == 4 ) && !sock->Error();
};
	
bool ECSocket::Write(wxSocketBase *sock, const uint32& i)
{
	int32 tmp = ENDIAN_SWAP_32(i);
	sock->Write(&tmp, 4);
	
	return ( sock->LastCount() == 4 ) && !sock->Error();
};

#if 0
bool ECSocket::Read(wxSocketBase *sock, uint64& i)
{
	sock->Read(&i, 8);
	ENDIAN_SWAP_I_64(i);

	return ( sock->LastCount() == 8 ) && !sock->Error();
};

bool ECSocket::Write(wxSocketBase *sock, const uint64& v)
{
	int64 tmp = ENDIAN_SWAP_32(v);
	sock->Write(&tmp, 8);

	return ( sock->LastCount() == 8 ) && !sock->Error();
};
#endif

static wxCSConv aMuleConv(wxT("iso8859-1"));

bool ECSocket::Read(wxSocketBase *sock, wxString& s) {
	unsigned int msgBytes = 0;
	
	// Fail if we cant read the string-size
	if ( !Read(sock, msgBytes) )
		return false;
	
	// Fail if the string is abnormally large ( > 1kb )
	if ( msgBytes > 1024 )
		return false;
	
	char *utf8 = new char[msgBytes+1];
	utf8[msgBytes] = 0;
	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	char *iobuf = utf8;
	bool error = false;
	while(msgRemain && !error) {
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

bool ECSocket::Write(wxSocketBase *sock, const wxString& s) {
	// Converts a string in ISO-8859-1 to wide char, and then
	// converts it to to multi-byte (encoded) UTF-8
	const wxCharBuffer buf = wxConvUTF8.cWC2MB(s.wc_str(aMuleConv));
	const char *utf8 = (const char *)buf;
	// strlen does not like NULL pointers, so we test.
	if (!utf8) {
		printf("ECSocket::Write: Error converting string.\n");
		return false;
	}
	unsigned int msgBytes = strlen(utf8);
	
	if ( !Write(sock, msgBytes) )
		return false;

	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	const char *iobuf = utf8;
	bool error = false;
	while(msgRemain && !error) {
		sock->Write(iobuf, msgBytes);
		LastIO = sock->LastCount();
		error = sock->Error();
		msgRemain -= LastIO;
		iobuf += LastIO;
	}
	
	return !error;
};
	
//
// Client SendRecvMsg()
//
wxString ECSocket::SendRecvMsg(const wxString &msg) {
	return SendRecvMsg(m_sock, msg);
}

//
// Server SendRecvMsg()
//
wxString ECSocket::SendRecvMsg(wxSocketBase *sock, const wxString &msg) {
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


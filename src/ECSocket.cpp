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


#include "endianfix.h"


ECSocket::ECSocket()
{
	m_type = AMULE_EC_CLIENT;
	m_sock = new wxSocketClient();
}

ECSocket::ECSocket(wxSockAddress& address, wxSocketFlags flags)
{
	m_type = AMULE_EC_SERVER;
	m_sock = new wxSocketServer(address, flags);
}

void ECSocket::Read(uint8& i) {
	m_sock->Read(&i, 1);
};
	
void ECSocket::Write(const uint8& i) {
	m_sock->Write(&i, 1);
};

void ECSocket::Read(uint16& i) {
	m_sock->Read(&i, 2);
	ENDIAN_SWAP_I_16(i);
};
	
void ECSocket::Write(const uint16& i) {
	int16 tmp = ENDIAN_SWAP_16(i);
	m_sock->Write(&tmp, 2);
};

void ECSocket::Read(uint32& i) {
	m_sock->Read(&i, 4);
	ENDIAN_SWAP_I_32(i);
};
	
void ECSocket::Write(const uint32& i) {
	int32 tmp = ENDIAN_SWAP_32(i);
	m_sock->Write(&tmp, 4);
};

#if 0
void ECSocket::Read(uint64& i) {
	m_sock->Read(&i, 8);
	ENDIAN_SWAP_I_64(i);
};

void ECSocket::Write(const uint64& v) {
	int64 tmp = ENDIAN_SWAP_32(v);
	m_sock->Write(&tmp, 8);
};
#endif
	
void ECSocket::Read(wxString& s) {
	unsigned int msgBytes;
	Read(msgBytes);
	char *utf8 = new char[msgBytes+1];
	utf8[msgBytes] = 0;
	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	char *iobuf = utf8;
	register int i = 0;
	while(msgRemain) {
		m_sock->Read(iobuf, msgRemain);
		LastIO = m_sock->LastCount();
		msgRemain -= LastIO;
		iobuf += LastIO;
		++i;
	}
	s = wxString(wxConvUTF8.cMB2WX(utf8));
//printf("read loop=%d, bytes=%d, original=%d\n", i, msgBytes, s.size());
	delete [] utf8;
	if(m_sock->Error()) {
		printf("Wrong wxString Reading Packet!\n");
	}
};

void ECSocket::Write(const wxString& s) {
	const wxWX2MBbuf buf = wxConvUTF8.cWX2MB(s);
	const char *utf8 = (const char *)buf;
	unsigned int msgBytes = strlen(utf8);
	Write(msgBytes);
	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	const char *iobuf = utf8;
	register int i = 0;
	while(msgRemain) {
		m_sock->Write(iobuf, msgBytes);
		LastIO = m_sock->LastCount();
		msgRemain -= LastIO;
		iobuf += LastIO;
		++i;
	}
//printf("write loop=%d, bytes=%d, original=%d\n", i, msgBytes, s.size());
};
	
wxString ECSocket::SendRecvMsg(const wxString &msg) {
	m_sock->SetFlags(wxSOCKET_WAITALL);
	Write(msg);
	wxString response;
	if (!m_sock->Error()) {
		// Wait until data available (will also return if the connection is lost)
		m_sock->WaitForRead(10);
		if (m_sock->IsData()) {
			Read(response);
		}
	}
	return response;
}


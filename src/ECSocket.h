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

#ifndef AMULE_DAEMON
#define ECSocketBase wxEvtHandler
#endif

class ECSocket  : public ECSocketBase {
public:
	//
	// Constructors/Destructor
	//
	ECSocket();
	ECSocket(wxSockAddress& address, wxEvtHandler& handler, int id = -1);
	~ECSocket();

	//
	// Send/receive string
	//
	wxString SendRecvMsg(const wxString &msg);
	wxString SendRecvMsg(wxSocketBase *sock, const wxString &msg);

	//
	// Base
	//
	bool Destroy()
	{
		return m_sock->Destroy();
	}
	bool Ok() const
	{
		return m_sock->Ok();
	}
	//
	// Client
	//
	bool Connect(wxSockAddress& address, bool wait = true)
	{
		return ((wxSocketClient *)m_sock)->Connect(address, wait);
	}
	
	bool WaitOnConnect(long seconds = -1, long milliseconds = 0)
	{
		return ((wxSocketClient *)m_sock)->WaitOnConnect(seconds, milliseconds);
	}

	bool IsConnected()
	{
		return ((wxSocketClient *)m_sock)->IsConnected();
	}
	
	//
	// Server
	//
	wxSocketBase *Accept(bool wait = true)
	{
		return ((wxSocketServer *)m_sock)->Accept(wait);
	}
	
private:
	// 8 bits
	void Read(wxSocketBase *sock, uint8& i);
	void Write(wxSocketBase *sock, const uint8& i);
	
	// 16 bis
	void Read(wxSocketBase *sock, uint16& v);
	void Write(wxSocketBase *sock, const uint16& i);
	
	// 32 bits
	void Read(wxSocketBase *sock, uint32& v);
	void Write(wxSocketBase *sock, const uint32& i);
#if 0
	// 64 bits
	void Read(wxSocketBase *sock, uint64& i);
	void Write(wxSocketBase *sock, const uint64& i);
#endif
	// String
public:
	void Read(wxSocketBase *sock, wxString& s);
	void Write(wxSocketBase *sock, const wxString& s);
	
private:
	aMuleECSocketType m_type;
	wxSocketBase *m_sock;
};

#endif // ECSOCKET_H


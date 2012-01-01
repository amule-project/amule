//
// This file is part of the aMule Project.
//
// Copyright (c) 2011-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2011-2011 Stu Redman ( admin@amule.org / http://www.amule.org )
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


#ifndef __LIBSOCKET_H__
#define __LIBSOCKET_H__

#ifdef ASIO_SOCKETS

#include <wx/event.h>
#include <wx/thread.h>
#include "Types.h"
#include <wx/socket.h>		// for wxSocketError - remove me
class amuleIPV4Address;

//
// Abstraction class for library TCP socket
// Can be a wxSocket or an ASIO socket
//

//
// Client TCP socket
//
class CLibSocket
{
	friend class CAsioSocketImpl;
	friend class CAsioSocketServerImpl;
public:
	CLibSocket(int flags = 0);
	virtual ~CLibSocket();

	// wx Stuff
	bool	Connect(const amuleIPV4Address& adr, bool wait);
	bool	IsConnected() const;
	bool	IsOk() const;
	bool	GetPeer(amuleIPV4Address& adr);
	void	SetEventHandler(wxEvtHandler& handler, int id);
	uint32	Read(void * buffer, uint32 nbytes);
	uint32	Write(const void * buffer, uint32 nbytes);
	void	Close();
	void	Destroy();

	// Not needed here, we always notify
	void	SetNotify(int) {}
	void	Notify(bool) {}

	bool	Error() const;
	wxSocketError	LastError();


	// lower Prio
	void	Discard() {}	// probably not needed
	void	SetLocal(const amuleIPV4Address& local);

	// not supported
	void SetFlags(int) {}
	bool WaitOnConnect(long, long)	{ return false; }
	bool WaitForWrite(long, long)	{ return false; }
	bool WaitForRead(long, long)	{ return false; }

	// new Stuff

	// Show we're ready for another event
	void	EventProcessed();

	// Get IP of client
	const wxChar * GetIP() const;

	// True if Destroy() has been called for socket
	virtual bool ForDeletion() const { return false; }

	// Handlers
	virtual void OnConnect(int) {}
	virtual void OnSend(int) {}
	virtual void OnReceive(int) {}
	virtual void OnLost() {}

private:
	// Replace the internal socket
	void	LinkSocketImpl(class CAsioSocketImpl *);

	class CAsioSocketImpl * m_aSocket;
	void LastCount();	// No. We don't have this. We return it directly with Read() and Write()
};


//
// TCP socket server
//
class CLibSocketServer
{
public:
	CLibSocketServer(const amuleIPV4Address& adr, int flags);
	virtual ~CLibSocketServer();
	// Accepts an incoming connection request, and creates a new CLibSocket object which represents the server-side of the connection.
	CLibSocket * Accept(bool wait = true);
	// Accept an incoming connection using the specified socket object.
	bool	AcceptWith(CLibSocket & socket, bool wait);

	virtual	void OnAccept() {}

	bool	IsOk() const;

	void	Close();

	// Not needed here
	void	Discard() {}
	void	SetEventHandler(wxEvtHandler& , int ) {}
	void	SetNotify(int) {}
	bool	Notify(bool) { return true; }

	// new Stuff

	// Do we have a socket available if AcceptWith() is called ?
	bool	SocketAvailable();

	// Restart accepting if needed
	bool	RestartAccept();
private:
	class CAsioSocketServerImpl * m_aServer;
};


//
// UDP socket
//
class CLibUDPSocket
{
	friend class CAsioUDPSocketImpl;
public:
	CLibUDPSocket(amuleIPV4Address &address, int flags);
	virtual ~CLibUDPSocket();

	// wx Stuff
	bool	IsOk() const;
	virtual uint32 RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes);
	virtual uint32 SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes);
	bool	Error() const;
	wxSocketError	LastError();
	void	Close();
	void	Destroy();
	void	SetClientData(class CMuleUDPSocket *);

	// Not needed here
	void	SetEventHandler(wxEvtHandler&, int) {}
	void	SetNotify(int) {}
	bool	Notify(bool) { return true; }

private:
	class CAsioUDPSocketImpl * m_aSocket;
	void LastCount();	// block this
};


//
// ASIO event loop
//
class CAsioService : public wxThread 
{
public:
    CAsioService();
    ~CAsioService();
	void Stop();
private:

    void* Entry();

};


#else // ASIO_SOCKETS

// use wx sockets

#include <wx/socket.h>
#include "amuleIPV4Address.h"

class CLibSocket : public wxSocketClient
{
public:
	CLibSocket(wxSocketFlags flags = 0) : wxSocketClient(flags) {}

	// not actually called
	const wxChar * GetIP() const { return wxEmptyString; }
	void	EventProcessed() {}
	// unused Handlers
	virtual void OnConnect(int) {}
	virtual void OnSend(int) {}
	virtual void OnReceive(int) {}
	virtual void OnLost() {}

	uint32 Read(void *buffer, wxUint32 nbytes)
	{
		wxSocketClient::Read(buffer, nbytes);
		return wxSocketClient::LastCount();
	}

	uint32 Write(const void *buffer, wxUint32 nbytes)
	{
		wxSocketClient::Write(buffer, nbytes);
		return wxSocketClient::LastCount();
	}

private:
	void LastCount();	// block this
};


class CLibSocketServer : public wxSocketServer
{
public:
	CLibSocketServer(const amuleIPV4Address &address,	wxSocketFlags flags) : wxSocketServer(address, flags) {}

	CLibSocket * Accept(bool wait) { return (CLibSocket *) wxSocketServer::Accept(wait); }

	bool SocketAvailable() { return true; }

	void RestartAccept() {}
	virtual	void OnAccept() {}
};


class CLibUDPSocket : public wxDatagramSocket
{
public:
	CLibUDPSocket(amuleIPV4Address &address, wxSocketFlags flags) : wxDatagramSocket(address, flags) {}

	virtual uint32 RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes)
	{
		wxDatagramSocket::RecvFrom(addr, buf, nBytes);
		return wxDatagramSocket::LastCount();
	}

	virtual uint32 SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes)
	{
		wxDatagramSocket::SendTo(addr, buf, nBytes);
		return wxDatagramSocket::LastCount();
	}

private:
	void LastCount();	// block this
};


class CAsioService
{
public:
	void Wait() {}
	void Stop() {}
};


#endif

#endif
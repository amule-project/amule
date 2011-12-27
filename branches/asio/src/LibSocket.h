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
public:
	CLibSocket(int flags = 0);
	virtual ~CLibSocket();

	// wx Stuff
	bool	Connect(const amuleIPV4Address& adr, bool wait);
	bool	IsConnected() const;
	bool	IsOk() const;
	bool	GetPeer(amuleIPV4Address& adr);
	void	Destroy();
	void	SetEventHandler(wxEvtHandler& handler, int id);
//	uint32	LastCount() const;  // No. We don't have this. We return it directly with Read() and Write()
	uint32	Read(void * buffer, uint32 nbytes);
	uint32	Write(const void * buffer, uint32 nbytes);
	void	Close();

	// Replace these
	void	SetNotify(int);
	bool	Notify(bool);

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

	// Replace the internal socket
	void	LinkSocketImpl(class CAsioSocketImpl *);

	// Get IP of client
	const wxChar * GetIP() const;

	// True if Destroy() has been called for socket
	virtual bool ForDeletion() const { return false; }

	// Handlers
	virtual void OnConnect(int) {}
	virtual void OnSend(int) {}
	virtual void OnReceive(int) {}

private:
	class CAsioSocketImpl * m_aSocket;
	void LastCount();	// block this
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


class CAsioService
{
public:
	void Wait() {}
	void Stop() {}
};


#endif

#endif
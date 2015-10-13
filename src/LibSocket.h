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

#ifdef HAVE_CONFIG_H
#	include "config.h"		// Needed for ASIO_SOCKETS
#endif

#ifdef ASIO_SOCKETS

#include <wx/event.h>
#include <wx/thread.h>
#include "Types.h"
class amuleIPV4Address;

// Socket flags (unused in ASIO implementation, just provide the names)
enum {
	MULE_SOCKET_NONE,
	MULE_SOCKET_NOWAIT_READ,
	MULE_SOCKET_NOWAIT_WRITE,
	MULE_SOCKET_NOWAIT,
	MULE_SOCKET_WAITALL_READ,
	MULE_SOCKET_WAITALL_WRITE,
	MULE_SOCKET_WAITALL,
	MULE_SOCKET_BLOCK,
	MULE_SOCKET_REUSEADDR,
	MULE_SOCKET_BROADCAST,
	MULE_SOCKET_NOBIND
};
typedef int muleSocketFlags;

// Socket events (used for proxy notification)
enum {
	MULE_SOCKET_CONNECTION,
	MULE_SOCKET_INPUT,
	MULE_SOCKET_OUTPUT,
	MULE_SOCKET_LOST
};

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
	void	Notify(bool);
	bool	Connect(const amuleIPV4Address& adr, bool wait);
	bool	IsConnected() const;
	bool	IsOk() const;
	void	SetLocal(const amuleIPV4Address& local);
	uint32	Read(void * buffer, uint32 nbytes);
	uint32	Write(const void * buffer, uint32 nbytes);
	void	Close();
	void	Destroy();

	// Not needed here, we always notify the same way
	void	SetNotify(int) {}
	void	SetEventHandler(wxEvtHandler&, int) {}

	// Get last error, 0 == no error
	int		LastError() const;

	// not supported
	void SetFlags(int) {}
	void Discard() {}
	bool WaitOnConnect(long, long)	{ return true; }
	bool WaitForWrite(long, long)	{ return true; }
	bool WaitForRead(long, long)	{ return true; }

	// new Stuff

	// Check if socket is currently blocking for read or write
	bool	BlocksRead() const;
	bool	BlocksWrite() const;

	// Show we're ready for another event
	void	EventProcessed();

	// Get IP of client
	const wxChar * GetIP() const;

	// True if Destroy() has been called for socket
	bool	IsDestroying() const;

	// Get/set proxy state
	bool GetProxyState() const;
	void SetProxyState(bool state, const amuleIPV4Address * adr = 0);

	// Get peer address (better API than wx)
	wxString	GetPeer();
	uint32		GetPeerInt();

	// Handlers
	virtual void OnConnect(int) {}
	virtual void OnSend(int) {}
	virtual void OnReceive(int) {}
	virtual void OnLost() {}
	virtual void OnProxyEvent(int) {}

private:
	// Replace the internal socket
	void	LinkSocketImpl(class CAsioSocketImpl *);

	class CAsioSocketImpl * m_aSocket;
	void LastCount();	// No. We don't have this. We return it directly with Read() and Write()
	bool Error() const;	// Only use LastError
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
	int		LastError() const;
	void	Close();
	void	Destroy();
	void	SetClientData(class CMuleUDPSocket *);

	// Not needed here
	void	SetEventHandler(wxEvtHandler&, int) {}
	void	SetNotify(int) {}
	bool	Notify(bool) { return true; }

	// Check if socket is currently blocking for write
	// Well - we apparently have block in wx. At least we handle it in MuleUDPSocket.
	// But this makes no sense. We send a packet to an IP in background.
	// Either this works after some time, or not. But there is no block.
	bool	BlocksWrite() const { return false; }

private:
	class	CAsioUDPSocketImpl * m_aSocket;
	void	LastCount();	// block this
	bool	Error() const;	// Only use LastError
};


//
// ASIO event loop
//
class CAsioService
{
public:
    CAsioService();
    ~CAsioService();
	void Stop();
private:
	static const int m_numberOfThreads;
	class CAsioServiceThread * m_threads;
};


#else /* ASIO_SOCKETS */

// use wx sockets

#include <wx/socket.h>
#include "amuleIPV4Address.h"

typedef wxSocketFlags muleSocketFlags;

// Socket flags
#define MULE_SOCKET_NONE		wxSOCKET_NONE
#define MULE_SOCKET_NOWAIT_READ		wxSOCKET_NOWAIT_READ
#define MULE_SOCKET_NOWAIT_WRITE	wxSOCKET_NOWAIT_WRITE
#define MULE_SOCKET_NOWAIT		wxSOCKET_NOWAIT
#define MULE_SOCKET_WAITALL_READ	wxSOCKET_WAITALL_READ
#define MULE_SOCKET_WAITALL_WRITE	wxSOCKET_WAITALL_WRITE
#define MULE_SOCKET_WAITALL		wxSOCKET_WAITALL
#define MULE_SOCKET_BLOCK		wxSOCKET_BLOCK
#define MULE_SOCKET_REUSEADDR		wxSOCKET_REUSEADDR
#define MULE_SOCKET_BROADCAST		wxSOCKET_BROADCAST
#define MULE_SOCKET_NOBIND		wxSOCKET_NOBIND

// Socket events
#define MULE_SOCKET_CONNECTION		wxSOCKET_CONNECTION
#define MULE_SOCKET_INPUT		wxSOCKET_INPUT
#define MULE_SOCKET_OUTPUT		wxSOCKET_OUTPUT
#define MULE_SOCKET_LOST		wxSOCKET_LOST

class CLibSocket : public wxSocketClient
{
public:
	CLibSocket(wxSocketFlags flags = 0) : wxSocketClient(flags), m_isDestroying(false) {}

	// not actually called
	const wxChar * GetIP() const { return wxEmptyString; }
	void EventProcessed() {}
	bool GetProxyState() const { return false; }
	// unused Handlers
	virtual void OnConnect(int) {}
	virtual void OnSend(int) {}
	virtual void OnReceive(int) {}
	virtual void OnLost() {}
	virtual void OnProxyEvent(int) {}

	// methods using amuleIPV4Address
	bool Connect(amuleIPV4Address& adr, bool wait);		// Yes. adr is not const.
	bool GetPeer(amuleIPV4Address& adr);
	void SetLocal(amuleIPV4Address& local);				// Same here.

	// Get last error, 0 == no error
	// BLOCK is also not an error!
	int	LastError() const
	{
		int ret = 0;
		if (wxSocketClient::Error()) {
			ret = wxSocketClient::LastError();
			if (ret == wxSOCKET_WOULDBLOCK) {
				ret = 0;
			}
		}
		return ret;
	}

	// Check if socket is currently blocking for read or write
	bool	BlocksRead() const
	{
		return wxSocketClient::Error() && wxSocketClient::LastError() == wxSOCKET_WOULDBLOCK;
	}

	bool	BlocksWrite() const { return BlocksRead(); }	// no difference here

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

	void	Destroy()
	{
		if (!m_isDestroying) {
			m_isDestroying = true;
			SetNotify(0);
			Notify(false);
			Close(); // Destroy is suposed to call Close(), but.. it doesn't hurt.
			wxSocketClient::Destroy();
		}
	}

	bool	IsDestroying() const { return m_isDestroying; }

	// Get peer address (better API than wx)
	wxString	GetPeer()
	{
		wxIPV4address adr;
		wxSocketClient::GetPeer(adr);
		return adr.IPAddress();
	}

	uint32		GetPeerInt() { return StringIPtoUint32(GetPeer()); }

private:
	bool	m_isDestroying;		// true if Destroy() was called

	void LastCount();	// block this
	bool Error() const;	// Only use LastError
};


class CLibSocketServer : public wxSocketServer
{
public:
	CLibSocketServer(const amuleIPV4Address &address, wxSocketFlags flags);

	CLibSocket * Accept(bool wait) { return static_cast<CLibSocket*>(wxSocketServer::Accept(wait)); }

	bool SocketAvailable() { return true; }

	virtual	void OnAccept() {}
};


class CLibUDPSocket : public wxDatagramSocket
{
public:
	CLibUDPSocket(amuleIPV4Address &address, wxSocketFlags flags);

	virtual uint32 RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes);

	virtual uint32 SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes);

	// Get last error, 0 == no error
	int	LastError() const
	{
		int ret = 0;
		if (wxDatagramSocket::Error()) {
			ret = wxDatagramSocket::LastError();
			if (ret == wxSOCKET_WOULDBLOCK) {
				ret = 0;
			}
		}
		return ret;
	}

	// Check if socket is currently blocking for write
	// I wonder if this EVER returns true (see Asio)
	bool	BlocksWrite() const
	{
		return wxDatagramSocket::Error() && wxDatagramSocket::LastError() == wxSOCKET_WOULDBLOCK;
	}

private:
	void LastCount();	// block this
	bool Error() const;	// Only use LastError
};


class CAsioService
{
public:
	void Wait() {}
	void Stop() {}
};

#endif /* ASIO_SOCKETS */

#endif /* __LIBSOCKET_H__ */

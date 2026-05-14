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

#include "Types.h"
#include <memory>		// shared_ptr for CAsioUDPSocketImpl ownership
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
	// Replace the internal socket. Takes ownership of the passed shared_ptr.
	void	LinkSocketImpl(std::shared_ptr<class CAsioSocketImpl>);

	// shared_ptr so the asio impl can outlive this wrapper for as long as
	// any in-flight async callback still holds a shared_from_this() ref.
	// Required to fix the wake-from-sleep use-after-free crash (issue #384).
	std::shared_ptr<class CAsioSocketImpl> m_aSocket;
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
	bool	Notify(bool) { return true; }

	// new Stuff

	// Do we have a socket available if AcceptWith() is called ?
	bool	SocketAvailable();
private:
	// shared_ptr for the same reason as CLibSocket::m_aSocket — pending
	// async_accept completions must keep the impl alive past wrapper death.
	std::shared_ptr<class CAsioSocketServerImpl> m_aServer;
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
	bool	Notify(bool) { return true; }

	// Check if socket is currently blocking for write
	// Well - we apparently have block in wx. At least we handle it in MuleUDPSocket.
	// But this makes no sense. We send a packet to an IP in background.
	// Either this works after some time, or not. But there is no block.
	bool	BlocksWrite() const { return false; }

private:
	// shared_ptr so the asio impl can outlive this wrapper for as long as
	// any in-flight async callback still holds a shared_from_this() ref.
	// Required to fix the wake-from-sleep use-after-free crash (issue #384).
	std::shared_ptr<class CAsioUDPSocketImpl> m_aSocket;
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



#endif /* __LIBSOCKET_H__ */

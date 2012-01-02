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

#ifdef ASIO_SOCKETS

#ifdef _MSC_VER
#define _WIN32_WINNT 0x0501		// Boost complains otherwise
#endif

// Windows requires that Boost headers are included before wx headers.
// This works if precompiled headers are disabled for this file.

#define BOOST_ALL_NO_LIB

#include <boost/asio.hpp>
#include <boost/bind.hpp>

//
// Do away with building Boost.System, adding lib paths...
// Just include the single file and be done.
//
#include <boost/../libs/system/src/error_code.cpp>

#include "LibSocket.h"
#include <wx/socket.h>		// wxSocketError
#include <wx/thread.h>		// wxMutex
#include <wx/intl.h>		// _()
#include <common/Format.h>	// Needed for CFormat
#include "Logger.h"
#include "GuiEvents.h"
#include "amuleIPV4Address.h"
#include "MuleUDPSocket.h"
#include "OtherFunctions.h"	// DeleteContents

using namespace boost::asio;
using namespace boost::system;	// for error_code
static 	io_service s_io_service;

/**
 * ASIO Client TCP socket implementation
 */

class CAsioSocketImpl
{
public:
	CAsioSocketImpl(CLibSocket * libSocket) : 
		m_libSocket(libSocket),
		m_timer(s_io_service)
	{
		m_OK = false;
		m_readBuffer = NULL;
		m_readBufferSize = 0;
		m_readPending = false;
		m_readBufferContent = 0;
		m_eventPending = false;
		m_port = 0;
		m_sendBuffer = NULL;
		m_connected = false;
		m_closed = false;
		m_dying = false;
		m_IP = wxT("?");
		ClearError();
		m_socket = new ip::tcp::socket(s_io_service);

		// Set socket to non blocking
		m_socket->non_blocking();
	}

	~CAsioSocketImpl()
	{
		delete[] m_readBuffer;
		delete[] m_sendBuffer;
	}

	bool Connect(const amuleIPV4Address& adr, bool wait)
	{
		SetIpString(adr.IPAddress());
		m_port = adr.Service();
		m_closed = false;
		m_OK = false;
		AddDebugLogLineF(logAsio, CFormat(wxT("Connect %s %p")) % m_IP % this);
		ip::tcp::endpoint endpoint(ip::address_v4::from_string(adr.GetStrIP()), adr.Service());

		if (wait) {
			wxFAIL;	// we're not supposed to do that
			error_code ec;
			m_socket->connect(endpoint, ec);
			m_OK = !ec;
			m_connected = m_OK;
			return m_OK;
		} else {
			m_socket->async_connect(endpoint, 
				boost::bind(& CAsioSocketImpl::HandleConnect, this, placeholders::error));
			// m_OK and return are false because we are not connected yet
			return false;
		}
	}

	bool IsConnected() const
	{
		return m_connected;
	}

	// For wxSocketClient, Ok won't return true unless the client is connected to a server.
	bool IsOk() const 
	{
		return m_OK;
	}

	// Returns true if an error occurred in the last IO operation.
	bool Error() const
	{
		return m_Error;
	}

	// Returns the actual error code
	wxSocketError LastError() const
	{
		return m_ErrorCode;
	}

	// Problem: wx sends an event when data gets available, so first there is an event, then Read() is called
	// Asio can read async with callback, so you first read, then you get an event.
	// Strategy:
	// - Read some data in background into a buffer
	// - Callback posts event when something is there
	// - Read data from buffer
	// - If data is exhausted, start reading more in background
	// - If not, post another event (making sure events don't pile up though)
	uint32 Read(char * buf, uint32 bytesToRead)
	{
		if (bytesToRead == 0) {			// huh?
			return 0;
		}

		if (m_Error && m_ErrorCode != wxSOCKET_WOULDBLOCK) {
			AddDebugLogLineF(logAsio, CFormat(wxT("Read1 %s %d - Error")) % m_IP % bytesToRead);
			return 0;
		}

		if (m_readPending					// Background read hasn't completed.
			|| m_readBufferContent == 0) {	// shouldn't be if it's not pending
			
			m_Error = true;
			m_ErrorCode = wxSOCKET_WOULDBLOCK;
			AddDebugLogLineF(logAsio, CFormat(wxT("Read1 %s %d - Block")) % m_IP % bytesToRead);
			return 0;
		}

		ResetBlock();	// shouldn't be needed

		// Read from our buffer
		uint32 readCache = std::min(m_readBufferContent, bytesToRead);
		memcpy(buf, m_readBufferPtr, readCache);
		m_readBufferContent	-= readCache;
		m_readBufferPtr		+= readCache;

		AddDebugLogLineF(logAsio, CFormat(wxT("Read2 %s %d - %d")) % m_IP % bytesToRead % readCache);
		if (m_readBufferContent) {
			// Data left, post another event
			PostReadEvent(1);
		} else {
			// Nothing left, read more
			StartBackgroundRead();
		}
		return readCache;
	}


	// Make a copy of the data and send it in background
	// - unless a background send is already going on
	uint32 Write(const void * buf, uint32 nbytes)
	{
		if (m_sendBuffer) {
			m_Error = true;
			m_ErrorCode = wxSOCKET_WOULDBLOCK;
			AddDebugLogLineF(logAsio, CFormat(wxT("Write blocks %d %p %s")) % nbytes % m_sendBuffer % m_IP);
			return 0;
		}
		AddDebugLogLineF(logAsio, CFormat(wxT("Write %d %s")) % nbytes % m_IP);
		m_sendBuffer = new char[nbytes];
		memcpy(m_sendBuffer, buf, nbytes);
		s_io_service.dispatch(boost::bind(& CAsioSocketImpl::DispatchWrite, this, nbytes));
		ClearError();
		return nbytes;
	}

	void Close()
	{
		if (!m_closed) {
			m_closed = true;
			m_connected = false;
			if (s_io_service.stopped()) {
				DispatchClose();
			} else {
				s_io_service.dispatch(boost::bind(& CAsioSocketImpl::DispatchClose, this));
			}
		}
	}


	void Destroy()
	{
		if (m_dying) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Destroy() already dying socket %p %p %s")) % m_libSocket % this % m_IP);
			return;
		}
		m_dying = true;
		AddDebugLogLineF(logAsio, CFormat(wxT("Destroy() %p %p %s")) % m_libSocket % this % m_IP);
		Close();
		if (s_io_service.stopped()) {
			HandleDestroy();
		} else {
			// Close prevents creation of any more callbacks, but does not clear any callbacks already
			// sitting in Asio's event queue (I have seen such a crash).
			// So create a delay timer so they can be called until core is notified.
			m_timer.expires_from_now(boost::posix_time::seconds(1));
			m_timer.async_wait(boost::bind(& CAsioSocketImpl::HandleDestroy, this));
		}
	}


	bool GetPeer(amuleIPV4Address& adr)
	{
		return adr.Hostname(m_IP)
				&& adr.Service(m_port);
	}


	void SetLocal(const amuleIPV4Address& local)
	{
		/* That's useless. Why bind a client socket to a local address? 
		   All you get is: Can't bind socket to local endpoint 192.168.178.34:21746 : Address already in use
		error_code ec;
		if (!m_socket->is_open()) {
			// Socket is usually still closed when this is called
			m_socket->open(boost::asio::ip::tcp::v4(), ec);
			if (ec) {
				AddDebugLogLineC(logAsio, CFormat(wxT("Can't open socket : %s")) % ec.message());
			}
		}
		ip::tcp::endpoint endpoint(ip::address_v4::from_string(local.GetStrIP()), local.Service());
		m_socket->bind(endpoint, ec);
		if (ec) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Can't bind socket to local endpoint %s:%d : %s")) 
				% local.IPAddress() % local.Service() % ec.message());
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("Bound socket to local endpoint %s:%d")) % local.IPAddress() % local.Service());
		}
		*/
	}


	void EventProcessed()
	{
		m_eventPending = false;
	}

	void SetWrapSocket(CLibSocket * socket)
	{
		m_libSocket = socket;
		// Also do some setting up
		m_OK = true;
		m_connected = true;
		// Start reading
		StartBackgroundRead();
	}

	void UpdateIP()
	{
		error_code ec;
		ip::tcp::endpoint endpoint = m_socket->remote_endpoint(ec);
		if (SetError(ec)) {
			AddDebugLogLineN(logAsio, CFormat(wxT("UpdateIP failed %p %s")) % this % ec.message());
			return;
		}
		ip::address adr = endpoint.address();
		SetIpString(wxString(adr.to_string().c_str(), wxConvUTF8));
		m_port = endpoint.port();
		AddDebugLogLineF(logAsio, CFormat(wxT("UpdateIP %s %d %p")) % m_IP % m_port % this);
	}

	const wxChar * GetIP() const { return m_IP; }
	uint16 GetPort() const { return m_port; }

	ip::tcp::socket & GetAsioSocket()
	{
		return * m_socket;
	}

private:
	//
	// Dispatch handlers
	// Access to m_socket is all bundled in the thread running s_io_service to avoid 
	// concurrent access to the socket from several threads.
	// So once things are running (after connect), all access goes through one of these handlers.
	//
	void DispatchClose()
	{
		error_code ec;
		m_socket->close(ec);
		if (ec) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Close error %s %s")) % m_IP % ec.message());
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("Closed %s")) % m_IP);
		}
	}

	void DispatchBackgroundRead()
	{
		m_socket->async_read_some(null_buffers(), 
			boost::bind(& CAsioSocketImpl::HandleRead, this, placeholders::error));
		//m_socket->async_read_some(buffer(m_readBuffer, c_readBufferSize), 
		//	boost::bind(& CAsioSocketImpl::HandleRead, this, placeholders::error, placeholders::bytes_transferred));
	}

	void DispatchWrite(uint32 nbytes)
	{
		async_write(*m_socket, buffer(m_sendBuffer, nbytes),
			boost::bind(& CAsioSocketImpl::HandleSend, this, placeholders::error, placeholders::bytes_transferred));
	}

	//
	// Completion handlers for async requests
	//

	void HandleConnect(const error_code& err)
	{
		m_OK = !err;
		AddDebugLogLineF(logAsio, CFormat(wxT("HandleConnect %d %s")) % m_OK % m_IP);
		if (m_libSocket->ForDeletion()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("HandleConnect: socket pending for deletion %s")) % m_IP);
		} else {
			CoreNotify_LibSocketConnect(m_libSocket, err.value());
			if (m_OK) {
				// After connect also send a OUTPUT event to show data is available
				CoreNotify_LibSocketSend(m_libSocket, 0);
				// Start reading
				StartBackgroundRead();
				m_connected = true;
			}
		}
	}

	void HandleSend(const error_code& err, size_t bytes_transferred)
	{
		delete[] m_sendBuffer;
		m_sendBuffer = NULL;

		if (m_libSocket->ForDeletion()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("HandleSend: socket pending for deletion %s")) % m_IP);
		} else {
			if (SetError(err)) {
				AddDebugLogLineN(logAsio, CFormat(wxT("HandleSend Error %d %s")) % bytes_transferred % m_IP);
				PostLostEvent();
			} else {
				AddDebugLogLineF(logAsio, CFormat(wxT("HandleSend %d %s")) % bytes_transferred % m_IP);
				ResetBlock();
				CoreNotify_LibSocketSend(m_libSocket, m_ErrorCode);
			}
		}
	}

	void HandleRead(const error_code & ec)
	{
		if (SetError(ec)) {
			// This is what we get in Windows when a connection gets closed from remote.
			AddDebugLogLineN(logAsio, CFormat(wxT("HandleReadError %s %s")) % m_IP % ec.message());
			PostLostEvent();
			return;
		}

		error_code ec2;
		uint32 avail = m_socket->available(ec2);
		if (SetError(ec2)) {
			AddDebugLogLineN(logAsio, CFormat(wxT("HandleReadError available %d %s %s")) % avail % m_IP % ec2.message());
			PostLostEvent();
			return;
		}
		if (avail == 0) {
			// This is what we get in Linux when a connection gets closed from remote.
			AddDebugLogLineF(logAsio, CFormat(wxT("HandleReadError nothing available %s")) % m_IP);
			SetError();
			PostLostEvent();
			return;
		}
		AddDebugLogLineF(logAsio, CFormat(wxT("HandleRead %d %s")) % avail % m_IP);

		// adjust (or create) our read buffer
		if (m_readBufferSize < avail) {
			delete[] m_readBuffer;
			m_readBuffer = new char[avail];
			m_readBufferSize = avail;
		}
		m_readBufferPtr = m_readBuffer;

		// read available data
		m_readBufferContent = m_socket->read_some(buffer(m_readBuffer, avail), ec2);
		if (SetError(ec2) || m_readBufferContent == 0) {
			AddDebugLogLineN(logAsio, CFormat(wxT("HandleReadError read %d %s %s")) % m_readBufferContent % m_IP % ec2.message());
			PostLostEvent();
			return;
		}

		m_readPending = false;
		if (m_libSocket->ForDeletion()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("HandleRead: socket pending for deletion %s")) % m_IP);
		} else {
			ResetBlock();
			PostReadEvent(2);
		}
	}

	void HandleDestroy()
	{
		AddDebugLogLineF(logAsio, CFormat(wxT("HandleDestroy() %p %p %s")) % m_libSocket % this % m_IP);
		CoreNotify_LibSocketDestroy(m_libSocket);
	}


	//
	// Other functions
	//

	void StartBackgroundRead()
	{
		m_readPending = true;
		m_readBufferContent = 0;
		s_io_service.dispatch(boost::bind(& CAsioSocketImpl::DispatchBackgroundRead, this));
	}

	void PostReadEvent(int from)
	{
		if (!m_eventPending) {
			AddDebugLogLineF(logAsio, CFormat(wxT("Post read event %d %s")) % from % m_IP);
			m_eventPending = true;
			CoreNotify_LibSocketReceive(m_libSocket, m_ErrorCode);
		}
	}

	void PostLostEvent()
	{
		if (!m_libSocket->ForDeletion()) {
			CoreNotify_LibSocketLost(m_libSocket);
		}
	}

	void ClearError()
	{
		m_Error = false;
		m_ErrorCode = wxSOCKET_NOERROR;
	}

	void SetError()
	{
		m_Error = true;
		m_ErrorCode = wxSOCKET_IOERR;
	}

	bool SetError(const error_code & err)
	{
		if (err) {
			SetError();
		} else {
			ClearError();
		}
		return m_Error;
	}

	void ResetBlock()
	{
		if (m_ErrorCode == wxSOCKET_WOULDBLOCK) {
			ClearError();
		}
	}

	//
	// Access to even const & wxString is apparently not thread-safe.
	// Locks are set/removed in wx and reference counts can go astray.
	// So store our IP string in a wxString which is used nowhere.
	// Store a pointer to its string buffer as well and use THAT everywhere.
	//
	void SetIpString (const wxString & ip)
	{
		m_IPstring = ip;
		m_IP = m_IPstring.c_str();
	}

	CLibSocket	*	m_libSocket;
	ip::tcp::socket	*	m_socket;
	wxString		m_IPstring;			// remote IP
	const wxChar *	m_IP;				
	uint16			m_port;				// remote port
	bool			m_OK;
	bool			m_Error;
	wxSocketError	m_ErrorCode;
	char *			m_readBuffer;
	uint32			m_readBufferSize;
	char *			m_readBufferPtr;
	bool			m_readPending;
	uint32			m_readBufferContent;
	bool			m_eventPending;
	char *			m_sendBuffer;
	deadline_timer	m_timer;
	bool			m_connected;
	bool			m_closed;
	bool			m_dying;
};


/**
 * Library socket wrapper
 */

CLibSocket::CLibSocket(int /* flags */)
{
	m_aSocket = new CAsioSocketImpl(this);
}


CLibSocket::~CLibSocket()
{
	AddDebugLogLineF(logAsio, CFormat(wxT("~CLibSocket() %p %p %s")) % this % m_aSocket % m_aSocket->GetIP());
	delete m_aSocket;
}


bool CLibSocket::Connect(const amuleIPV4Address& adr, bool wait)
{
	return m_aSocket->Connect(adr, wait);
}


bool CLibSocket::IsConnected() const
{
	return m_aSocket->IsConnected();
}


bool CLibSocket::IsOk() const
{
	return m_aSocket->IsOk();
}


bool CLibSocket::Error() const
{
	return m_aSocket->Error();
}


bool CLibSocket::GetPeer(amuleIPV4Address& adr)
{
	return m_aSocket->GetPeer(adr);
}


void CLibSocket::Destroy()
{
	m_aSocket->Destroy();
}


void CLibSocket::SetEventHandler(wxEvtHandler& , int )
{
}


uint32 CLibSocket::Read(void * buffer, uint32 nbytes)
{
	return m_aSocket->Read((char *) buffer, nbytes);
}


uint32 CLibSocket::Write(const void * buffer, uint32 nbytes)
{
	return m_aSocket->Write(buffer, nbytes);
}


void CLibSocket::Close()
{
	m_aSocket->Close();
}


wxSocketError CLibSocket::LastError()
{
	return m_aSocket->LastError();
}


void CLibSocket::SetLocal(const amuleIPV4Address& local)
{
	m_aSocket->SetLocal(local);
}



// new Stuff

void CLibSocket::EventProcessed()
{
	m_aSocket->EventProcessed();
}


void CLibSocket::LinkSocketImpl(class CAsioSocketImpl * socket)
{
	delete m_aSocket;
	m_aSocket = socket;
	m_aSocket->SetWrapSocket(this);
}


const wxChar * CLibSocket::GetIP() const
{
	return m_aSocket->GetIP();
}


/**
 * ASIO TCP socket server
 */

class CAsioSocketServerImpl : public ip::tcp::acceptor
{
public:
	CAsioSocketServerImpl(const amuleIPV4Address & adr, CLibSocketServer * libSocketServer) 
		: ip::tcp::acceptor(s_io_service),
		  m_libSocketServer(libSocketServer)
	{
		m_ok = false;
		m_currentSocket = NULL;
		m_socketAvailable = false;
		m_acceptError = false;

		try {
			ip::address_v4 ipadr = ip::address_v4::from_string(adr.GetStrIP());
			ip::tcp::endpoint endpoint(ipadr, adr.Service());
			open(endpoint.protocol());
			set_option(ip::tcp::acceptor::reuse_address(true));
			bind(endpoint);
			listen();
			StartAccept();
			m_ok = true;
			AddDebugLogLineN(logAsio, CFormat(wxT("CAsioSocketServerImpl bind to %s %d")) % adr.IPAddress() % adr.Service());
		} catch (system_error err) {
			AddDebugLogLineC(logAsio, CFormat(wxT("CAsioSocketServerImpl bind to %s %d failed - %s")) % adr.IPAddress() % adr.Service() % err.code().message());
		}
	}

	~CAsioSocketServerImpl()
	{
	}

	// For wxSocketServer, Ok will return true if the server could bind to the specified address and is already listening for new connections.
	bool IsOk() const { return m_ok; }

	void Close() { close();	}

	bool AcceptWith(CLibSocket & socket)
	{
		if (!m_socketAvailable) {
			AddDebugLogLineF(logAsio, wxT("AcceptWith: nothing there"));
			return false;
		}

		// return the socket we received
		socket.LinkSocketImpl(m_currentSocket);

		// check if we have another socket ready for reception
		m_currentSocket = new CAsioSocketImpl(NULL);
		error_code ec;
		// async_accept does not work if server is non-blocking
		// temporarily switch it to non-blocking
		non_blocking(true);
		// we are set to non-blocking, so this returns right away
		accept(m_currentSocket->GetAsioSocket(), ec);
		// back to blocking
		non_blocking(false);
		if (ec) {
			// nothing there
			m_socketAvailable = false;
			// start getting another one
			StartAccept();
			AddDebugLogLineF(logAsio, wxT("AcceptWith: ok, getting another socket in background"));
		} else {
			// we got another socket right away
			m_socketAvailable = true;	// it is already true, but this improves readability
			m_currentSocket->UpdateIP();
			AddDebugLogLineF(logAsio, wxT("AcceptWith: ok, another socket is available"));
		}

		return true;
	}

	bool SocketAvailable() const { return m_socketAvailable; }

	bool RestartAccept()
	{
		if (!m_acceptError) {
			return false;
		}
		m_acceptError = false;
		AddDebugLogLineN(logAsio, wxT("RestartAccept after error"));
		StartAccept();
		return true;
	}

private:

	void StartAccept()
	{
		if (!m_currentSocket) {
			m_currentSocket = new CAsioSocketImpl(NULL);
		}
		async_accept(m_currentSocket->GetAsioSocket(), boost::bind(& CAsioSocketServerImpl::HandleAccept, this, placeholders::error));
	}

	void HandleAccept(const error_code& error)
	{
		if (error) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Error in HandleAccept: %s")) % error.message());
			// Try again
			delete m_currentSocket;
			m_currentSocket = NULL;
			m_acceptError = true;
		} else {
			m_currentSocket->UpdateIP();
			AddDebugLogLineN(logAsio, CFormat(wxT("HandleAccept received a connection from %s:%d")) 
				% m_currentSocket->GetIP() % m_currentSocket->GetPort());
			m_socketAvailable = true;
			CoreNotify_ServerTCPAccept(m_libSocketServer);
		}
	}

	// The wrapper object
	CLibSocketServer * m_libSocketServer;
	// Startup ok
	bool m_ok;
	// Accept error
	bool m_acceptError;
	// The last socket that connected to us
	CAsioSocketImpl * m_currentSocket;
	// Is there a socket available?
	bool m_socketAvailable;
};


CLibSocketServer::CLibSocketServer(const amuleIPV4Address& adr, int /* flags */)
{
	m_aServer = new CAsioSocketServerImpl(adr, this);
}


CLibSocketServer::~CLibSocketServer()
{
	delete m_aServer;
}


// Accepts an incoming connection request, and creates a new CLibSocket object which represents the server-side of the connection.
// Only used in CamuleApp::ListenSocketHandler() and we don't get there.
CLibSocket * CLibSocketServer::Accept(bool /* wait */)
{
	wxFAIL;
	return NULL;
}


// Accept an incoming connection using the specified socket object.
bool CLibSocketServer::AcceptWith(CLibSocket & socket, bool wait)
{
	wxASSERT(!wait);
	return m_aServer->AcceptWith(socket);
}


bool CLibSocketServer::IsOk() const
{
	return m_aServer->IsOk();
}


void CLibSocketServer::Close()
{
	m_aServer->Close();
}


bool CLibSocketServer::SocketAvailable()
{
	return m_aServer->SocketAvailable();
}


bool CLibSocketServer::RestartAccept()
{
	return m_aServer->RestartAccept();
}



/**
 * ASIO UDP socket implementation
 */

class CAsioUDPSocketImpl
{
private:
	// UDP data block
	class CUDPData {
	public:
		char * buffer;
		uint32 size;
		amuleIPV4Address ipadr;

		CUDPData(const void * src, uint32 _size, amuleIPV4Address adr) :
			size(_size), ipadr(adr)
		{
			buffer = new char[size];
			memcpy(buffer, src, size);
		}

		~CUDPData()
		{
			delete[] buffer;
		}
	};

public:
	CAsioUDPSocketImpl(const amuleIPV4Address &address, int /* flags */, CLibUDPSocket * libSocket) :
		m_libSocket(libSocket),
		m_timer(s_io_service)
	{
		m_muleSocket = NULL;
		m_readBuffer = new char[CMuleUDPSocket::UDP_BUFFER_SIZE];
		m_OK = true;

		try {
			ip::address_v4 addr = ip::address_v4::from_string(address.GetStrIP());
			ip::udp::endpoint endpoint(addr, address.Service());
			m_socket = new ip::udp::socket(s_io_service, endpoint);
			AddDebugLogLineN(logAsio, CFormat(wxT("Created UDP socket %s %d")) % address.IPAddress() % address.Service());
			StartBackgroundRead();
		} catch (system_error err) {
			AddLogLineC(CFormat(wxT("Error creating UDP socket %s %d : %s")) % address.IPAddress() % address.Service() % err.code().message());
			m_socket = NULL;
			m_OK = false;
		}
	}

	~CAsioUDPSocketImpl()
	{
		AddDebugLogLineF(logAsio, wxT("UDP ~CAsioUDPSocketImpl"));
		delete m_socket;
		delete[] m_readBuffer;
		DeleteContents(m_receiveBuffers);
	}

	void SetClientData(CMuleUDPSocket * muleSocket)
	{
		AddDebugLogLineF(logAsio, wxT("UDP SetClientData"));
		m_muleSocket = muleSocket;
	}

	uint32 RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes)
	{
		CUDPData * recdata;
		{
			wxMutexLocker lock(m_receiveBuffersLock);
			if (m_receiveBuffers.empty()) {
				AddDebugLogLineN(logAsio, wxT("UDP RecvFromError no data"));
				return 0;
			}
			recdata = * m_receiveBuffers.begin();
			m_receiveBuffers.pop_front();
		}
		uint32 read = recdata->size;
		if (read > nBytes) {
			// should not happen
			AddDebugLogLineN(logAsio, CFormat(wxT("UDP RecvFromError too much data %d")) % read);
			read = nBytes;
		}
		memcpy(buf, recdata->buffer, read);
		addr = recdata->ipadr;
		delete recdata;
		return read;
	}

	uint32 SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes)
	{
		// Collect data, make a copy of the buffer's content
		CUDPData * recdata = new CUDPData(buf, nBytes, addr);
		AddDebugLogLineF(logAsio, CFormat(wxT("UDP SendTo %d to %s")) % nBytes % addr.IPAddress());
		s_io_service.dispatch(boost::bind(& CAsioUDPSocketImpl::DispatchSendTo, this, recdata));
		return nBytes;
	}

	bool IsOk() const
	{
		return m_OK;
	}

	void Close()
	{
		if (s_io_service.stopped()) {
			DispatchClose();
		} else {
			s_io_service.dispatch(boost::bind(& CAsioUDPSocketImpl::DispatchClose, this));
		}
	}

	void Destroy()
	{
		AddDebugLogLineF(logAsio, CFormat(wxT("Destroy() %p %p")) % m_libSocket % this);
		Close();
		if (s_io_service.stopped()) {
			HandleDestroy();
		} else {
			// Close prevents creation of any more callbacks, but does not clear any callbacks already
			// sitting in Asio's event queue (I have seen such a crash).
			// So create a delay timer so they can be called until core is notified.
			m_timer.expires_from_now(boost::posix_time::seconds(1));
			m_timer.async_wait(boost::bind(& CAsioUDPSocketImpl::HandleDestroy, this));
		}
	}


private:
	//
	// Dispatch handlers
	// Access to m_socket is all bundled in the thread running s_io_service to avoid 
	// concurrent access to the socket from several threads.
	// So once things are running (after connect), all access goes through one of these handlers.
	//
	void DispatchClose()
	{
		error_code ec;
		m_socket->close(ec);
		if (ec) {
			AddDebugLogLineC(logAsio, CFormat(wxT("UDP Close error %s")) % ec.message());
		} else {
			AddDebugLogLineF(logAsio, wxT("UDP Closed"));
		}
	}

	void DispatchSendTo(CUDPData * recdata)
	{
		ip::address_v4 addr = ip::address_v4::from_string(recdata->ipadr.GetStrIP());
		ip::udp::endpoint endpoint(addr, recdata->ipadr.Service());

		AddDebugLogLineF(logAsio, CFormat(wxT("UDP DispatchSendTo %d to %s:%d")) % recdata->size 
			% endpoint.address().to_string() % endpoint.port());
		m_socket->async_send_to(buffer(recdata->buffer, recdata->size), endpoint,
			boost::bind(& CAsioUDPSocketImpl::HandleSendTo, this, placeholders::error, placeholders::bytes_transferred, recdata));
	}

	//
	// Completion handlers for async requests
	//

	void HandleRead(const error_code & ec, size_t received)
	{
		if (ec) {
			AddDebugLogLineN(logAsio, CFormat(wxT("UDP HandleReadError %s")) % ec.message());
		} else if (received == 0) {
			AddDebugLogLineF(logAsio, wxT("UDP HandleReadError nothing available"));
		} else if (m_muleSocket == NULL) {
			AddDebugLogLineN(logAsio, wxT("UDP HandleReadError no handler"));
		} else {

			amuleIPV4Address ipadr;
			ipadr.Hostname(m_receiveEndpoint.address().to_string());
			uint16 port = m_receiveEndpoint.port();
			ipadr.Service(port);
			AddDebugLogLineF(logAsio, CFormat(wxT("UDP HandleRead %d %s:%d")) % received % ipadr.IPAddress() % port);

			// create our read buffer
			CUDPData * recdata = new CUDPData(m_readBuffer, received, ipadr);
			{
				wxMutexLocker lock(m_receiveBuffersLock);
				m_receiveBuffers.push_back(recdata);
			}
			CoreNotify_UDPSocketReceive(m_muleSocket);
		}
		StartBackgroundRead();
	}

	void HandleSendTo(const error_code & ec, size_t sent, CUDPData * recdata)
	{
		if (ec) {
			AddDebugLogLineN(logAsio, CFormat(wxT("UDP HandleSendToError %s")) % ec.message());
		} else if (sent != recdata->size) {
			AddDebugLogLineN(logAsio, CFormat(wxT("UDP HandleSendToError tosend: %d sent %d")) % recdata->size % sent);
		}
		if (m_muleSocket == NULL) {
			AddDebugLogLineN(logAsio, wxT("UDP HandleSendToError no handler"));
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("UDP HandleSendTo %d to %s")) % sent % recdata->ipadr.IPAddress());
			CoreNotify_UDPSocketSend(m_muleSocket);
		}
		delete recdata;
	}

	void HandleDestroy()
	{
		AddDebugLogLineF(logAsio, CFormat(wxT("HandleDestroy() %p %p")) % m_libSocket % this);
		delete m_libSocket;
	}

	//
	// Other functions
	//

	void StartBackgroundRead()
	{
		m_socket->async_receive_from(buffer(m_readBuffer, CMuleUDPSocket::UDP_BUFFER_SIZE), m_receiveEndpoint,
			boost::bind(& CAsioUDPSocketImpl::HandleRead, this, placeholders::error, placeholders::bytes_transferred));
	}

	CLibUDPSocket *		m_libSocket;
	ip::udp::socket *	m_socket;
	CMuleUDPSocket *	m_muleSocket;
	bool				m_OK;
	deadline_timer		m_timer;

	// One fix receive buffer
	char *				m_readBuffer;
	// and a list of dynamic buffers. UDP data may be coming in faster
	// than the main loop can handle it.
	std::list<CUDPData *>	m_receiveBuffers;
	wxMutex				m_receiveBuffersLock;

	// Address of last reception
	ip::udp::endpoint	m_receiveEndpoint;
};


/**
 * Library UDP socket wrapper
 */

CLibUDPSocket::CLibUDPSocket(amuleIPV4Address &address, int flags)
{
	m_aSocket = new CAsioUDPSocketImpl(address, flags, this);
}


CLibUDPSocket::~CLibUDPSocket()
{
	AddDebugLogLineF(logAsio, CFormat(wxT("~CLibUDPSocket() %p %p")) % this % m_aSocket);
	delete m_aSocket;
}


bool CLibUDPSocket::IsOk() const
{
	return m_aSocket->IsOk();
}


uint32 CLibUDPSocket::RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes)
{
	return m_aSocket->RecvFrom(addr, buf, nBytes);
}


uint32 CLibUDPSocket::SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes)
{
	return m_aSocket->SendTo(addr, buf, nBytes);
}


void CLibUDPSocket::SetClientData(CMuleUDPSocket * muleSocket)
{
	m_aSocket->SetClientData(muleSocket);
}


bool CLibUDPSocket::Error() const
{
	return !IsOk();
}


wxSocketError CLibUDPSocket::LastError()
{
	return wxSOCKET_NOERROR;
}


void CLibUDPSocket::Close()
{
	m_aSocket->Close();
}


void CLibUDPSocket::Destroy()
{
	m_aSocket->Destroy();
}


/**
 * CAsioService - ASIO event loop thread
 */

/**
 * The constructor starts the thread.
 */
CAsioService::CAsioService()
		: wxThread( wxTHREAD_JOINABLE )
{
	Create();
	Run();
}


/**
 * The destructor stops the thread. If the thread has already stoppped, destructor does nothing.
 */
CAsioService::~CAsioService()
{
}


void CAsioService::Stop()
{
	s_io_service.stop();
}


void * CAsioService::Entry()
{
	AddLogLineNS(_("Asio thread started"));
	io_service::work worker(s_io_service);		// keep io_service running
	s_io_service.run();
	AddDebugLogLineN(logAsio, wxT("Asio thread stopped"));

	return NULL;
}

#endif

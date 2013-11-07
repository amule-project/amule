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

#ifdef HAVE_CONFIG_H
#	include "config.h"		// Needed for HAVE_BOOST_SOURCES
#endif

#ifdef _MSC_VER
#define _WIN32_WINNT 0x0501		// Boost complains otherwise
#endif

// Windows requires that Boost headers are included before wx headers.
// This works if precompiled headers are disabled for this file.

#define BOOST_ALL_NO_LIB

// Suppress warning caused by faulty boost/preprocessor/config/config.hpp in Boost 1.49
#if defined __GNUC__ && ! defined __GXX_EXPERIMENTAL_CXX0X__ && __cplusplus < 201103L
	#define BOOST_PP_VARIADICS 0
#endif

#include <algorithm>	// Needed for std::min - Boost up to 1.54 fails to compile with MSVC 2013 otherwise

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/version.hpp>

//
// Do away with building Boost.System, adding lib paths...
// Just include the single file and be done.
//
#ifdef HAVE_BOOST_SOURCES
#	include <boost/../libs/system/src/error_code.cpp>
#else
#	include <boost/system/error_code.hpp>
#endif

#include "LibSocket.h"
#include <wx/thread.h>		// wxMutex
#include <wx/intl.h>		// _()
#include <common/Format.h>	// Needed for CFormat
#include "Logger.h"
#include "GuiEvents.h"
#include "amuleIPV4Address.h"
#include "MuleUDPSocket.h"
#include "OtherFunctions.h"	// DeleteContents, MuleBoostVersion
#include "ScopedPtr.h"

using namespace boost::asio;
using namespace boost::system;	// for error_code
static 	io_service s_io_service;

// Number of threads in the Asio thread pool
const int CAsioService::m_numberOfThreads = 4;

/**
 * ASIO Client TCP socket implementation
 */

class CamuleIPV4Endpoint : public ip::tcp::endpoint {
public:
	CamuleIPV4Endpoint() {}

	CamuleIPV4Endpoint(const CamuleIPV4Endpoint & impl) : ip::tcp::endpoint(impl) {}
	CamuleIPV4Endpoint(const ip::tcp::endpoint & ep) { * this = ep; }
	CamuleIPV4Endpoint(const ip::udp::endpoint & ep) { address(ep.address()); port(ep.port()); }

	const CamuleIPV4Endpoint& operator = (const ip::tcp::endpoint & ep)
	{
		* (ip::tcp::endpoint *) this = ep;
		return *this;
	}
};

class CAsioSocketImpl
{
public:
	// cppcheck-suppress uninitMemberVar m_readBufferPtr
	CAsioSocketImpl(CLibSocket * libSocket) :
		m_libSocket(libSocket),
		m_strand(s_io_service),
		m_timer(s_io_service)
	{
		m_OK = false;
		m_blocksRead = false;
		m_blocksWrite = false;
		m_ErrorCode = 0;
		m_readBuffer = NULL;
		m_readBufferSize = 0;
		m_readPending = false;
		m_readBufferContent = 0;
		m_eventPending = false;
		m_port = 0;
		m_sendBuffer = NULL;
		m_connected = false;
		m_closed = false;
		m_isDestroying = false;
		m_proxyState = false;
		m_notify = true;
		m_sync = false;
		m_IP = wxT("?");
		m_IPint = 0;
		m_socket = new ip::tcp::socket(s_io_service);

		// Set socket to non blocking
		m_socket->non_blocking();
	}

	~CAsioSocketImpl()
	{
		delete[] m_readBuffer;
		delete[] m_sendBuffer;
	}

	void Notify(bool notify)
	{
		m_notify = notify;
	}

	bool Connect(const amuleIPV4Address& adr, bool wait)
	{
		if (!m_proxyState) {
			SetIp(adr);
		}
		m_port = adr.Service();
		m_closed = false;
		m_OK = false;
		m_sync = !m_notify;		// set this once for the whole lifetime of the socket
		AddDebugLogLineF(logAsio, CFormat(wxT("Connect %s %p")) % m_IP % this);

		if (wait || m_sync) {
			error_code ec;
			m_socket->connect(adr.GetEndpoint(), ec);
			m_OK = !ec;
			m_connected = m_OK;
			return m_OK;
		} else {
			m_socket->async_connect(adr.GetEndpoint(),
				m_strand.wrap(boost::bind(& CAsioSocketImpl::HandleConnect, this, placeholders::error)));
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

	bool IsDestroying() const
	{
		return m_isDestroying;
	}

	// Returns the actual error code
	int LastError() const
	{
		return m_ErrorCode;
	}

	// Is reading blocked?
	bool BlocksRead() const
	{
		return m_blocksRead;
	}

	// Is writing blocked?
	bool BlocksWrite() const
	{
		return m_blocksWrite;
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

		if (m_sync) {
			return ReadSync(buf, bytesToRead);
		}

		if (m_ErrorCode) {
			AddDebugLogLineF(logAsio, CFormat(wxT("Read1 %s %d - Error")) % m_IP % bytesToRead);
			return 0;
		}

		if (m_readPending					// Background read hasn't completed.
			|| m_readBufferContent == 0) {	// shouldn't be if it's not pending

			m_blocksRead = true;
			AddDebugLogLineF(logAsio, CFormat(wxT("Read1 %s %d - Block")) % m_IP % bytesToRead);
			return 0;
		}

		m_blocksRead = false;	// shouldn't be needed

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
		if (m_sync) {
			return WriteSync(buf, nbytes);
		}

		if (m_sendBuffer) {
			m_blocksWrite = true;
			AddDebugLogLineF(logAsio, CFormat(wxT("Write blocks %d %p %s")) % nbytes % m_sendBuffer % m_IP);
			return 0;
		}
		AddDebugLogLineF(logAsio, CFormat(wxT("Write %d %s")) % nbytes % m_IP);
		m_sendBuffer = new char[nbytes];
		memcpy(m_sendBuffer, buf, nbytes);
		m_strand.dispatch(boost::bind(& CAsioSocketImpl::DispatchWrite, this, nbytes));
		m_ErrorCode = 0;
		return nbytes;
	}


	void Close()
	{
		if (!m_closed) {
			m_closed = true;
			m_connected = false;
			if (m_sync || s_io_service.stopped()) {
				DispatchClose();
			} else {
				m_strand.dispatch(boost::bind(& CAsioSocketImpl::DispatchClose, this));
			}
		}
	}


	void Destroy()
	{
		if (m_isDestroying) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Destroy() already dying socket %p %p %s")) % m_libSocket % this % m_IP);
			return;
		}
		m_isDestroying = true;
		AddDebugLogLineF(logAsio, CFormat(wxT("Destroy() %p %p %s")) % m_libSocket % this % m_IP);
		Close();
		if (m_sync || s_io_service.stopped()) {
			HandleDestroy();
		} else {
			// Close prevents creation of any more callbacks, but does not clear any callbacks already
			// sitting in Asio's event queue (I have seen such a crash).
			// So create a delay timer so they can be called until core is notified.
			m_timer.expires_from_now(boost::posix_time::seconds(1));
			m_timer.async_wait(m_strand.wrap(boost::bind(& CAsioSocketImpl::HandleDestroy, this)));
		}
	}


	wxString GetPeer()
	{
		return m_IP;
	}

	uint32 GetPeerInt()
	{
		return m_IPint;
	}

	//
	// Bind socket to local endpoint if user wants to choose the local address
	//
	void SetLocal(const amuleIPV4Address& local)
	{
		error_code ec;
		if (!m_socket->is_open()) {
			// Socket is usually still closed when this is called
			m_socket->open(boost::asio::ip::tcp::v4(), ec);
			if (ec) {
				AddDebugLogLineC(logAsio, CFormat(wxT("Can't open socket : %s")) % ec.message());
			}
		}
		//
		// We are using random (OS-defined) local ports.
		// To set a constant output port, first call
		// m_socket->set_option(socket_base::reuse_address(true));
		// and then set the endpoint's port to it.
		//
		CamuleIPV4Endpoint endpoint(local.GetEndpoint());
		endpoint.port(0);
		m_socket->bind(endpoint, ec);
		if (ec) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Can't bind socket to local endpoint %s : %s"))
				% local.IPAddress() % ec.message());
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("Bound socket to local endpoint %s")) % local.IPAddress());
		}
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

	bool UpdateIP()
	{
		error_code ec;
		amuleIPV4Address addr = CamuleIPV4Endpoint(m_socket->remote_endpoint(ec));
		if (SetError(ec)) {
			AddDebugLogLineN(logAsio, CFormat(wxT("UpdateIP failed %p %s")) % this % ec.message());
			return false;
		}
		SetIp(addr);
		m_port = addr.Service();
		AddDebugLogLineF(logAsio, CFormat(wxT("UpdateIP %s %d %p")) % m_IP % m_port % this);
		return true;
	}

	const wxChar * GetIP() const { return m_IP; }
	uint16 GetPort() const { return m_port; }

	ip::tcp::socket & GetAsioSocket()
	{
		return * m_socket;
	}

	bool GetProxyState() const { return m_proxyState; }

	void SetProxyState(bool state, const amuleIPV4Address * adr)
	{
		m_proxyState = state;
		if (state) {
			// Start. Get the true IP for logging.
			wxASSERT(adr);
			SetIp(*adr);
			AddDebugLogLineF(logAsio, CFormat(wxT("SetProxyState to proxy %s")) % m_IP);
		} else {
			// Transition from proxy to normal mode
			AddDebugLogLineF(logAsio, CFormat(wxT("SetProxyState to normal %s")) % m_IP);
			m_ErrorCode = 0;
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
			AddDebugLogLineC(logAsio, CFormat(wxT("Close error %s %s")) % m_IP % ec.message());
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("Closed %s")) % m_IP);
		}
	}

	void DispatchBackgroundRead()
	{
		AddDebugLogLineF(logAsio, CFormat(wxT("DispatchBackgroundRead %s")) % m_IP);
		m_socket->async_read_some(null_buffers(),
			m_strand.wrap(boost::bind(& CAsioSocketImpl::HandleRead, this, placeholders::error)));
	}

	void DispatchWrite(uint32 nbytes)
	{
		async_write(*m_socket, buffer(m_sendBuffer, nbytes),
			m_strand.wrap(boost::bind(& CAsioSocketImpl::HandleSend, this, placeholders::error, placeholders::bytes_transferred)));
	}

	//
	// Completion handlers for async requests
	//

	void HandleConnect(const error_code& err)
	{
		m_OK = !err;
		AddDebugLogLineF(logAsio, CFormat(wxT("HandleConnect %d %s")) % m_OK % m_IP);
		if (m_isDestroying) {
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

		if (m_isDestroying) {
			AddDebugLogLineF(logAsio, CFormat(wxT("HandleSend: socket pending for deletion %s")) % m_IP);
		} else {
			if (SetError(err)) {
				AddDebugLogLineN(logAsio, CFormat(wxT("HandleSend Error %d %s")) % bytes_transferred % m_IP);
				PostLostEvent();
			} else {
				AddDebugLogLineF(logAsio, CFormat(wxT("HandleSend %d %s")) % bytes_transferred % m_IP);
				m_blocksWrite = false;
				CoreNotify_LibSocketSend(m_libSocket, m_ErrorCode);
			}
		}
	}

	void HandleRead(const error_code & ec)
	{
		if (m_isDestroying) {
			AddDebugLogLineF(logAsio, CFormat(wxT("HandleRead: socket pending for deletion %s")) % m_IP);
		}

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
		m_blocksRead = false;
		PostReadEvent(2);
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
		m_strand.dispatch(boost::bind(& CAsioSocketImpl::DispatchBackgroundRead, this));
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
		if (!m_isDestroying && !m_closed) {
			CoreNotify_LibSocketLost(m_libSocket);
		}
	}

	void SetError()
	{
		m_ErrorCode = 2;
	}

	bool SetError(const error_code & err)
	{
		if (err) {
			SetError();
		} else {
			m_ErrorCode = 0;
		}
		return m_ErrorCode != 0;
	}

	//
	// Synchronous sockets (amulecmd)
	//
	uint32 ReadSync(char * buf, uint32 bytesToRead)
	{
		error_code ec;
		uint32 received = read(*m_socket, buffer(buf, bytesToRead), ec);
		SetError(ec);
		return received;
	}

	uint32 WriteSync(const void * buf, uint32 nbytes)
	{
		error_code ec;
		uint32 sent = write(*m_socket, buffer(buf, nbytes), ec);
		SetError(ec);
		return sent;
	}

	//
	// Access to even const & wxString is apparently not thread-safe.
	// Locks are set/removed in wx and reference counts can go astray.
	// So store our IP string in a wxString which is used nowhere.
	// Store a pointer to its string buffer as well and use THAT everywhere.
	//
	void SetIp(const amuleIPV4Address& adr)
	{
		m_IPstring = adr.IPAddress();
		m_IP = m_IPstring.c_str();
		m_IPint = StringIPtoUint32(m_IPstring);
	}

	CLibSocket	*	m_libSocket;
	ip::tcp::socket	*	m_socket;
										// remote IP
	wxString		m_IPstring;			// as String (use nowhere because of threading!)
	const wxChar *	m_IP;				// as char*  (use in debug logs)
	uint32			m_IPint;			// as int
	uint16			m_port;				// remote port
	bool			m_OK;
	int				m_ErrorCode;
	bool			m_blocksRead;
	bool			m_blocksWrite;
	char *			m_readBuffer;
	uint32			m_readBufferSize;
	char *			m_readBufferPtr;
	bool			m_readPending;
	uint32			m_readBufferContent;
	bool			m_eventPending;
	char *			m_sendBuffer;
	io_service::strand	m_strand;		// handle synchronisation in io_service thread pool
	deadline_timer	m_timer;
	bool			m_connected;
	bool			m_closed;
	bool			m_isDestroying;		// true if Destroy() was called
	bool			m_proxyState;
	bool			m_notify;			// set by Notify()
	bool			m_sync;				// copied from !m_notify on Connect()
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


wxString CLibSocket::GetPeer()
{
	return m_aSocket->GetPeer();
}


uint32 CLibSocket::GetPeerInt()
{
	return m_aSocket->GetPeerInt();
}


void CLibSocket::Destroy()
{
	m_aSocket->Destroy();
}


bool CLibSocket::IsDestroying() const
{
	return m_aSocket->IsDestroying();
}


void CLibSocket::Notify(bool notify)
{
	m_aSocket->Notify(notify);
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


int CLibSocket::LastError() const
{
	return m_aSocket->LastError();
}


void CLibSocket::SetLocal(const amuleIPV4Address& local)
{
	m_aSocket->SetLocal(local);
}



// new Stuff

bool CLibSocket::BlocksRead() const
{
	return m_aSocket->BlocksRead();
}


bool CLibSocket::BlocksWrite() const
{
	return m_aSocket->BlocksWrite();
}


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


bool CLibSocket::GetProxyState() const
{
	return m_aSocket->GetProxyState();
}


void CLibSocket::SetProxyState(bool state, const amuleIPV4Address * adr)
{
	m_aSocket->SetProxyState(state, adr);
}


/**
 * ASIO TCP socket server
 */

class CAsioSocketServerImpl : public ip::tcp::acceptor
{
public:
	CAsioSocketServerImpl(const amuleIPV4Address & adr, CLibSocketServer * libSocketServer)
		: ip::tcp::acceptor(s_io_service),
		  m_libSocketServer(libSocketServer),
		  m_currentSocket(NULL),
		  m_strand(s_io_service)
	{
		m_ok = false;
		m_socketAvailable = false;

		try {
			open(adr.GetEndpoint().protocol());
			set_option(ip::tcp::acceptor::reuse_address(true));
			bind(adr.GetEndpoint());
			listen();
			StartAccept();
			m_ok = true;
			AddDebugLogLineN(logAsio, CFormat(wxT("CAsioSocketServerImpl bind to %s %d")) % adr.IPAddress() % adr.Service());
		} catch (const system_error& err) {
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
		socket.LinkSocketImpl(m_currentSocket.release());

		// check if we have another socket ready for reception
		m_currentSocket.reset(new CAsioSocketImpl(NULL));
		error_code ec;
		// async_accept does not work if server is non-blocking
		// temporarily switch it to non-blocking
		non_blocking(true);
		// we are set to non-blocking, so this returns right away
		accept(m_currentSocket->GetAsioSocket(), ec);
		// back to blocking
		non_blocking(false);
		if (ec || !m_currentSocket->UpdateIP()) {
			// nothing there
			m_socketAvailable = false;
			// start getting another one
			StartAccept();
			AddDebugLogLineF(logAsio, wxT("AcceptWith: ok, getting another socket in background"));
		} else {
			// we got another socket right away
			m_socketAvailable = true;	// it is already true, but this improves readability
			AddDebugLogLineF(logAsio, wxT("AcceptWith: ok, another socket is available"));
			// aMule actually doesn't need a notification as it polls the listen socket.
			// amuleweb does need it though
			CoreNotify_ServerTCPAccept(m_libSocketServer);
		}

		return true;
	}

	bool SocketAvailable() const { return m_socketAvailable; }

private:

	void StartAccept()
	{
		m_currentSocket.reset(new CAsioSocketImpl(NULL));
		async_accept(m_currentSocket->GetAsioSocket(), 
			m_strand.wrap(boost::bind(& CAsioSocketServerImpl::HandleAccept, this, placeholders::error)));
	}

	void HandleAccept(const error_code& error)
	{
		if (error) {
			AddDebugLogLineC(logAsio, CFormat(wxT("Error in HandleAccept: %s")) % error.message());
		} else {
			if (m_currentSocket->UpdateIP()) {
				AddDebugLogLineN(logAsio, CFormat(wxT("HandleAccept received a connection from %s:%d"))
					% m_currentSocket->GetIP() % m_currentSocket->GetPort());
				m_socketAvailable = true;
				CoreNotify_ServerTCPAccept(m_libSocketServer);
				return;
			} else {
				AddDebugLogLineN(logAsio, wxT("Error in HandleAccept: invalid socket"));
			}
		}
		// We were not successful. Try again.
		// Post the request to the event queue to make sure it doesn't get called immediately.
		m_strand.post(boost::bind(& CAsioSocketServerImpl::StartAccept, this));
	}

	// The wrapper object
	CLibSocketServer * m_libSocketServer;
	// Startup ok
	bool m_ok;
	// The last socket that connected to us
	CScopedPtr<CAsioSocketImpl> m_currentSocket;
	// Is there a socket available?
	bool m_socketAvailable;
	io_service::strand	m_strand;		// handle synchronisation in io_service thread pool
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
		m_strand(s_io_service),
		m_timer(s_io_service),
		m_address(address)
	{
		m_muleSocket = NULL;
		m_socket = NULL;
		m_readBuffer = new char[CMuleUDPSocket::UDP_BUFFER_SIZE];
		m_OK = true;
		CreateSocket();
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
		m_strand.dispatch(boost::bind(& CAsioUDPSocketImpl::DispatchSendTo, this, recdata));
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
			m_strand.dispatch(boost::bind(& CAsioUDPSocketImpl::DispatchClose, this));
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
			m_timer.async_wait(m_strand.wrap(boost::bind(& CAsioUDPSocketImpl::HandleDestroy, this)));
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
		ip::udp::endpoint endpoint(recdata->ipadr.GetEndpoint().address(), recdata->ipadr.Service());

		AddDebugLogLineF(logAsio, CFormat(wxT("UDP DispatchSendTo %d to %s:%d")) % recdata->size
			% endpoint.address().to_string() % endpoint.port());
		m_socket->async_send_to(buffer(recdata->buffer, recdata->size), endpoint,
			m_strand.wrap(boost::bind(& CAsioUDPSocketImpl::HandleSendTo, this, placeholders::error, placeholders::bytes_transferred, recdata)));
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

			amuleIPV4Address ipadr = amuleIPV4Address(CamuleIPV4Endpoint(m_receiveEndpoint));
			AddDebugLogLineF(logAsio, CFormat(wxT("UDP HandleRead %d %s:%d")) % received % ipadr.IPAddress() % ipadr.Service());

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

	void CreateSocket()
	{
		try {
			delete m_socket;
			ip::udp::endpoint endpoint(m_address.GetEndpoint().address(), m_address.Service());
			m_socket = new ip::udp::socket(s_io_service, endpoint);
			AddDebugLogLineN(logAsio, CFormat(wxT("Created UDP socket %s %d")) % m_address.IPAddress() % m_address.Service());
			StartBackgroundRead();
		} catch (const system_error& err) {
			AddLogLineC(CFormat(wxT("Error creating UDP socket %s %d : %s")) % m_address.IPAddress() % m_address.Service() % err.code().message());
			m_socket = NULL;
			m_OK = false;
		}
	}

	void StartBackgroundRead()
	{
		m_socket->async_receive_from(buffer(m_readBuffer, CMuleUDPSocket::UDP_BUFFER_SIZE), m_receiveEndpoint,
			m_strand.wrap(boost::bind(& CAsioUDPSocketImpl::HandleRead, this, placeholders::error, placeholders::bytes_transferred)));
	}

	CLibUDPSocket *		m_libSocket;
	ip::udp::socket *	m_socket;
	CMuleUDPSocket *	m_muleSocket;
	bool				m_OK;
	io_service::strand	m_strand;		// handle synchronisation in io_service thread pool
	deadline_timer		m_timer;
	amuleIPV4Address	m_address;

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


int CLibUDPSocket::LastError() const
{
	return !IsOk();
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

class CAsioServiceThread : public wxThread {
public:
	CAsioServiceThread() : wxThread(wxTHREAD_JOINABLE)
	{
		static int count = 0;
		m_threadNumber = ++count;
		Create();
		Run();
	}

	void * Entry()
	{
		AddLogLineNS(CFormat(_("Asio thread %d started")) % m_threadNumber);
		io_service::work worker(s_io_service);		// keep io_service running
		s_io_service.run();
		AddDebugLogLineN(logAsio, CFormat(wxT("Asio thread %d stopped")) % m_threadNumber);

		return NULL;
	}

private:
	int m_threadNumber;
};

/**
 * The constructor starts the thread.
 */
CAsioService::CAsioService()
{
	m_threads = new CAsioServiceThread[m_numberOfThreads];
}


CAsioService::~CAsioService()
{
}


void CAsioService::Stop()
{
	if (!m_threads) {
		return;
	}
	s_io_service.stop();
	// Wait for threads to exit
	for (int i = 0; i < m_numberOfThreads; i++) {
		CAsioServiceThread * t = m_threads + i;
		t->Wait();
	}
	delete[] m_threads;
	m_threads = 0;
}





/**
 * amuleIPV4Address
 */

amuleIPV4Address::amuleIPV4Address()
{
	m_endpoint = new CamuleIPV4Endpoint();
}

amuleIPV4Address::amuleIPV4Address(const amuleIPV4Address &a)
{
	*this = a;
}

amuleIPV4Address::amuleIPV4Address(const CamuleIPV4Endpoint &ep)
{
	*this = ep;
}

amuleIPV4Address::~amuleIPV4Address()
{
	delete m_endpoint;
}

amuleIPV4Address& amuleIPV4Address::operator=(const amuleIPV4Address &a)
{
	m_endpoint = new CamuleIPV4Endpoint(* a.m_endpoint);
	return *this;
}

amuleIPV4Address& amuleIPV4Address::operator=(const CamuleIPV4Endpoint &ep)
{
	m_endpoint = new CamuleIPV4Endpoint(ep);
	return *this;
}

bool amuleIPV4Address::Hostname(const wxString& name)
{
	if (name.IsEmpty()) {
		return false;
	}
	// This is usually just an IP.
	std::string sname(unicode2char(name));
	error_code ec;
	ip::address_v4 adr = ip::address_v4::from_string(sname, ec);
	if (!ec) {
		m_endpoint->address(adr);
		return true;
	}
	AddDebugLogLineN(logAsio, CFormat(wxT("Hostname(\"%s\") failed, not an IP address %s")) % name % ec.message());

	// Try to resolve (sync). Normally not required. Unless you type in your hostname as "local IP address" or something.
	error_code ec2;
	ip::tcp::resolver res(s_io_service);
	// We only want to get IPV4 addresses.
	ip::tcp::resolver::query query(ip::tcp::v4(), sname, "");
	ip::tcp::resolver::iterator endpoint_iterator = res.resolve(query, ec2);
	if (ec2) {
		AddDebugLogLineN(logAsio, CFormat(wxT("Hostname(\"%s\") resolve failed: %s")) % name % ec2.message());
		return false;
	}
	if (endpoint_iterator == ip::tcp::resolver::iterator()) {
		AddDebugLogLineN(logAsio, CFormat(wxT("Hostname(\"%s\") resolve failed: no address found")) % name);
		return false;
	}
	m_endpoint->address(endpoint_iterator->endpoint().address());
	AddDebugLogLineN(logAsio, CFormat(wxT("Hostname(\"%s\") resolved to %s")) % name % IPAddress());
	return true;
}

bool amuleIPV4Address::Service(uint16 service)
{
	if (service == 0) {
		return false;
	}
	m_endpoint->port(service);
	return true;
}

uint16 amuleIPV4Address::Service() const
{
	return m_endpoint->port();
}

bool amuleIPV4Address::IsLocalHost() const
{
	return m_endpoint->address().is_loopback();
}

wxString amuleIPV4Address::IPAddress() const
{
	return CFormat(wxT("%s")) % m_endpoint->address().to_string();
}

// "Set address to any of the addresses of the current machine."
// This just sets the address to 0.0.0.0 .
// wx does the same.
bool amuleIPV4Address::AnyAddress()
{
	m_endpoint->address(ip::address_v4::any());
	AddDebugLogLineN(logAsio, CFormat(wxT("AnyAddress: set to %s")) % IPAddress());
	return true;
}

const CamuleIPV4Endpoint & amuleIPV4Address::GetEndpoint() const
{
	return * m_endpoint;
}

CamuleIPV4Endpoint & amuleIPV4Address::GetEndpoint()
{
	return * m_endpoint;
}


//
// Notification stuff
//
namespace MuleNotify
{

	void LibSocketConnect(CLibSocket * socket, int error)
	{
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketConnect Destroying %s %d")) % socket->GetIP() % error);
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketConnect Proxy %s %d")) % socket->GetIP() % error);
			socket->OnProxyEvent(wxSOCKET_CONNECTION);
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketConnect %s %d")) %socket->GetIP() % error);
			socket->OnConnect(error);
		}
	}

	void LibSocketSend(CLibSocket * socket, int error)
	{
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketSend Destroying %s %d")) % socket->GetIP() % error);
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketSend Proxy %s %d")) % socket->GetIP() % error);
			socket->OnProxyEvent(wxSOCKET_OUTPUT);
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketSend %s %d")) % socket->GetIP() % error);
			socket->OnSend(error);
		}
	}

	void LibSocketReceive(CLibSocket * socket, int error)
	{
		socket->EventProcessed();
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketReceive Destroying %s %d")) % socket->GetIP() % error);
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketReceive Proxy %s %d")) % socket->GetIP() % error);
			socket->OnProxyEvent(wxSOCKET_INPUT);
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketReceive %s %d")) % socket->GetIP() % error);
			socket->OnReceive(error);
		}
	}

	void LibSocketLost(CLibSocket * socket)
	{
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketLost Destroying %s")) % socket->GetIP());
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketLost Proxy %s")) % socket->GetIP());
			socket->OnProxyEvent(wxSOCKET_LOST);
		} else {
			AddDebugLogLineF(logAsio, CFormat(wxT("LibSocketLost %s")) % socket->GetIP());
			socket->OnLost();
		}
	}

	void LibSocketDestroy(CLibSocket * socket)
	{
		AddDebugLogLineF(logAsio, CFormat(wxT("LibSocket_Destroy %s")) % socket->GetIP());
		delete socket;
	}

	void ProxySocketEvent(CLibSocket * socket, int evt)
	{
		AddDebugLogLineF(logAsio, CFormat(wxT("ProxySocketEvent %s %d")) % socket->GetIP() % evt);
		socket->OnProxyEvent(evt);
	}

	void ServerTCPAccept(CLibSocketServer * socketServer)
	{
		AddDebugLogLineF(logAsio, wxT("ServerTCP_Accept"));
		socketServer->OnAccept();
	}

} // namespace MuleNotify

//
// Initialize MuleBoostVersion
//
wxString MuleBoostVersion = CFormat(wxT("%d.%d")) % (BOOST_VERSION / 100000) % (BOOST_VERSION / 100 % 1000);

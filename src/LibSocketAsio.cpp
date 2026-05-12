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

#include "config.h"		// Needed for HAVE_BOOST_SOURCES


#ifdef _MSC_VER
#define _WIN32_WINNT 0x0501	// Boost complains otherwise
#endif

// Windows requires that Boost headers are included before wx headers.
// This works if precompiled headers are disabled for this file.

#define BOOST_ALL_NO_LIB

// Suppress warning caused by faulty boost/preprocessor/config/config.hpp in Boost 1.49
#if defined __GNUC__ && ! defined __GXX_EXPERIMENTAL_CXX0X__ && __cplusplus < 201103L
	#define BOOST_PP_VARIADICS 0
#endif

#include <algorithm>	// Needed for std::min - Boost up to 1.54 fails to compile with MSVC 2013 otherwise
#include <atomic>
#include <chrono>

// Trip the compile if we accidentally pull a deprecated Asio API back in.
#define BOOST_ASIO_NO_DEPRECATED
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
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
#include "OtherFunctions.h"	// DeleteContents
#include "ScopedPtr.h"
#include <common/Macros.h>

#ifndef __WINDOWS__
#include <fcntl.h>		// FD_CLOEXEC
#endif

using namespace boost::asio;
using namespace boost::system;	// for error_code
static io_context s_io_service;

//
// Mark a freshly-created socket close-on-exec so subprocesses launched
// via wxExecute() (preview-with-vlc, etc.) don't inherit and pin our
// listen / UDP file descriptors. Without this, vlc keeps the bind alive
// after aMule exits, and the next aMule start fails with
// "Address already in use" until the user kills vlc (#172).
//
// No-op on Windows: WinSock SOCKET handles are non-inheritable by
// default unless the parent passes bInheritHandle=TRUE to CreateProcess,
// which wxExecute does not do.
//
template <typename Handle>
static inline void SetCloexecOnSocket(Handle native)
{
#ifndef __WINDOWS__
	int flags = ::fcntl(native, F_GETFD, 0);
	if (flags != -1) {
		::fcntl(native, F_SETFD, flags | FD_CLOEXEC);
	}
#else
	(void) native;
#endif
}

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

// See the comment above CAsioUDPSocketImpl for the rationale on enable_shared_from_this:
// pending asio completion handlers must keep the impl alive past the wrapper's
// death (and past the old 1-second-timer guard that did not survive the time
// jump on wake-from-sleep — issue #384).
class CAsioSocketImpl : public std::enable_shared_from_this<CAsioSocketImpl>
{
	// Buffer size posted with async_read_some.  256 KB lets a fast peer
	// fill the buffer with several TCP segments at once on POSIX (epoll/
	// kqueue), and is the IOCP-native WSARecv buffer on Windows.  Bigger
	// keeps per-byte event-loop overhead small without blowing memory.
	static constexpr uint32 READ_CHUNK = 256 * 1024;

public:
	// cppcheck-suppress uninitMemberVar m_readBufferPtr
	CAsioSocketImpl(CLibSocket * libSocket) :
		m_libSocket(libSocket),
		m_strand(s_io_service)
	{
		m_OK = false;
		m_blocksRead = false;
		m_blocksWrite.store(false, std::memory_order_relaxed);
		m_ErrorCode = 0;
		m_readBuffer = NULL;
		m_readBufferSize = 0;
		m_readPending = false;
		m_readBufferContent = 0;
		m_eventPending = false;
		m_port = 0;
		m_sendBuffer.store(nullptr, std::memory_order_relaxed);
		m_connected = false;
		m_closed = false;
		m_destroying.store(false, std::memory_order_relaxed);
		m_proxyState = false;
		m_notify = true;
		m_sync = false;
		m_IP = L"?";
		m_IPint = 0;
		m_socket = new ip::tcp::socket(s_io_service);

		// Set socket to non blocking
		m_socket->non_blocking();
	}

	~CAsioSocketImpl()
	{
		delete[] m_readBuffer;
		delete[] m_sendBuffer.load();
		delete m_socket;
	}

	// Called by the wrapper's destructor (or by LinkSocketImpl when swapping
	// us out) to detach the back-pointer so any callback that fires after
	// the wrapper is gone no-ops its CoreNotify_* branch instead of
	// dereferencing freed memory. Atomic so the wrapper-side (any thread)
	// and the strand-side reads don't need an external lock.
	void OnWrapperGone()
	{
		m_libSocket.store(nullptr, std::memory_order_release);
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
		AddDebugLogLineF(logAsio, CFormat("Connect %s %p") % m_IP % this);

		if (wait || m_sync) {
			error_code ec;
			m_socket->connect(adr.GetEndpoint(), ec);
			m_OK = !ec;
			m_connected = m_OK;
			return m_OK;
		} else {
			auto self = shared_from_this();
			m_socket->async_connect(adr.GetEndpoint(),
				bind_executor(m_strand, [self](const error_code& ec) { self->HandleConnect(ec); }));
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
		return m_destroying.load(std::memory_order_acquire);
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
		return m_blocksWrite.load(std::memory_order_acquire);
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
			AddDebugLogLineF(logAsio, CFormat("Read1 %s %d - Error") % m_IP % bytesToRead);
			return 0;
		}

		if (m_readPending					// Background read hasn't completed.
			|| m_readBufferContent == 0) {	// shouldn't be if it's not pending

			m_blocksRead = true;
			AddDebugLogLineF(logAsio, CFormat("Read1 %s %d - Block") % m_IP % bytesToRead);
			return 0;
		}

		m_blocksRead = false;	// shouldn't be needed

		// Read from our buffer
		uint32 readCache = std::min(m_readBufferContent, bytesToRead);
		memcpy(buf, m_readBufferPtr, readCache);
		m_readBufferContent	-= readCache;
		m_readBufferPtr		+= readCache;

		AddDebugLogLineF(logAsio, CFormat("Read2 %s %d - %d") % m_IP % bytesToRead % readCache);
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

		if (m_sendBuffer.load(std::memory_order_acquire)) {
			m_blocksWrite.store(true, std::memory_order_relaxed);
			AddDebugLogLineF(logAsio, CFormat("Write blocks %d %p %s") % nbytes % m_sendBuffer.load() % m_IP);
			return 0;
		}
		AddDebugLogLineF(logAsio, CFormat("Write %d %s") % nbytes % m_IP);
		char* newBuf = new char[nbytes];
		memcpy(newBuf, buf, nbytes);
		m_sendBuffer.store(newBuf, std::memory_order_release);
		auto self = shared_from_this();
		dispatch(m_strand, [self, newBuf, nbytes]() { self->DispatchWrite(newBuf, nbytes); });
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
				auto self = shared_from_this();
				dispatch(m_strand, [self]() { self->DispatchClose(); });
			}
		}
	}


	// See the parallel comment on CAsioUDPSocketImpl::Destroy(). The TCP path
	// has identical wake-from-sleep risk; the fix is identical too — drop the
	// 1-second-timer band-aid in favour of shared_from_this lifetime.
	//
	// TCP routes wrapper deletion through CoreNotify_LibSocketDestroy (rather
	// than deleting inline like UDP) because TCP wrappers are reachable from
	// many parts of the core; the GUI-thread delete preserves the existing
	// thread affinity for that cleanup.
	void Destroy()
	{
		if (m_destroying.exchange(true, std::memory_order_acq_rel)) {
			CLibSocket * w = m_libSocket.load(std::memory_order_acquire);
			AddDebugLogLineC(logAsio, CFormat("Destroy() already dying socket %p %p %s") % w % this % m_IP);
			return;
		}
		CLibSocket * wrapper = m_libSocket.load(std::memory_order_acquire);
		AddDebugLogLineF(logAsio, CFormat("Destroy() %p %p %s") % wrapper % this % m_IP);
		Close();

		auto self = shared_from_this();
		auto teardown = [self]() {
			// Null the back-pointer before notifying so any callback that
			// fires after this point sees null and skips its CoreNotify_*.
			CLibSocket * w = self->m_libSocket.exchange(nullptr,
				std::memory_order_acq_rel);
			if (w) {
				CoreNotify_LibSocketDestroy(w);
			}
		};

		if (m_sync || s_io_service.stopped()) {
			teardown();
		} else {
			post(m_strand, teardown);
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
				AddDebugLogLineC(logAsio, CFormat("Can't open socket : %s") % ec.message());
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
			AddDebugLogLineC(logAsio, CFormat("Can't bind socket to local endpoint %s : %s")
				% local.IPAddress() % ec.message());
		} else {
			AddDebugLogLineF(logAsio, CFormat("Bound socket to local endpoint %s") % local.IPAddress());
		}
	}


	void EventProcessed()
	{
		m_eventPending = false;
	}

	void SetWrapSocket(CLibSocket * socket)
	{
		m_libSocket.store(socket, std::memory_order_release);
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
			AddDebugLogLineN(logAsio, CFormat("UpdateIP failed %p %s") % this % ec.message());
			return false;
		}
		SetIp(addr);
		m_port = addr.Service();
		AddDebugLogLineF(logAsio, CFormat("UpdateIP %s %d %p") % m_IP % m_port % this);
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
			AddDebugLogLineF(logAsio, CFormat("SetProxyState to proxy %s") % m_IP);
		} else {
			// Transition from proxy to normal mode
			AddDebugLogLineF(logAsio, CFormat("SetProxyState to normal %s") % m_IP);
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
			AddDebugLogLineC(logAsio, CFormat("Close error %s %s") % m_IP % ec.message());
		} else {
			AddDebugLogLineF(logAsio, CFormat("Closed %s") % m_IP);
		}
	}

	void DispatchBackgroundRead()
	{
		AddDebugLogLineF(logAsio, CFormat("DispatchBackgroundRead %s") % m_IP);
		// Why async_read_some and not async_wait(wait_read): on Windows
		// boost.asio implements async_wait via its select_reactor (a single
		// select() loop in a dedicated thread) because IOCP has no native
		// "ready notification" without a buffer.  async_read_some maps to
		// WSARecv on Windows (IOCP-native) and to epoll/kqueue on POSIX, so
		// it is the fast path on every platform.
		if (m_readBufferSize < READ_CHUNK) {
			delete[] m_readBuffer;
			m_readBuffer = new char[READ_CHUNK];
			m_readBufferSize = READ_CHUNK;
		}
		auto self = shared_from_this();
		m_socket->async_read_some(buffer(m_readBuffer, m_readBufferSize),
			bind_executor(m_strand, [self](const error_code& ec, std::size_t n) { self->HandleRead(ec, n); }));
	}

	// The buffer pointer is passed explicitly so each HandleSend knows
	// which buffer it owns and must delete.  m_sendBuffer only tracks the
	// currently-in-flight write and is cleared by HandleSend when the send
	// completes — it cannot be used to identify the buffer to free.
	void DispatchWrite(char* sendBuffer, uint32 nbytes)
	{
		auto self = shared_from_this();
		async_write(*m_socket, buffer(sendBuffer, nbytes),
			bind_executor(m_strand, [self, sendBuffer](const error_code& ec, std::size_t n) { self->HandleSend(sendBuffer, ec, n); }));
	}

	//
	// Completion handlers for async requests
	//

	void HandleConnect(const error_code& err)
	{
		m_OK = !err;
		AddDebugLogLineF(logAsio, CFormat("HandleConnect %d %s") % m_OK % m_IP);
		CLibSocket * wrapper = m_libSocket.load(std::memory_order_acquire);
		if (!wrapper) {
			AddDebugLogLineF(logAsio, CFormat("HandleConnect: wrapper gone %s") % m_IP);
		} else {
			CoreNotify_LibSocketConnect(wrapper, err.value());
			if (m_OK) {
				// After connect also send a OUTPUT event to show data is available
				CoreNotify_LibSocketSend(wrapper, 0);
				// Start reading
				StartBackgroundRead();
				m_connected = true;
			}
		}
	}

	void HandleSend(char* sentBuffer, const error_code& err, size_t bytes_transferred)
	{
		delete[] sentBuffer;
		// Atomically clear m_sendBuffer only if it still points to the buffer
		// we just finished sending.  A racing Write() on the throttler thread
		// may have already swapped in a new buffer.
		m_sendBuffer.compare_exchange_strong(sentBuffer, nullptr, std::memory_order_release);

		CLibSocket * wrapper = m_libSocket.load(std::memory_order_acquire);
		if (!wrapper) {
			AddDebugLogLineF(logAsio, CFormat("HandleSend: wrapper gone %s") % m_IP);
		} else {
			if (SetError(err)) {
				AddDebugLogLineN(logAsio, CFormat("HandleSend Error %d %s") % bytes_transferred % m_IP);
				PostLostEvent();
			} else {
				AddDebugLogLineF(logAsio, CFormat("HandleSend %d %s") % bytes_transferred % m_IP);
				m_blocksWrite.store(false, std::memory_order_release);
				CoreNotify_LibSocketSend(wrapper, m_ErrorCode);
			}
		}
	}

	void HandleRead(const error_code & ec, size_t bytes_transferred)
	{
		if (!m_libSocket.load(std::memory_order_acquire)) {
			AddDebugLogLineF(logAsio, CFormat("HandleRead: wrapper gone %s") % m_IP);
		}

		if (SetError(ec)) {
			// This is what we get in Windows when a connection gets closed from remote.
			AddDebugLogLineN(logAsio, CFormat("HandleReadError %s %s") % m_IP % ec.message());
			PostLostEvent();
			return;
		}

		if (bytes_transferred == 0) {
			AddDebugLogLineF(logAsio, CFormat("HandleReadError nothing available %s") % m_IP);
			SetError();
			PostLostEvent();
			return;
		}

		AddDebugLogLineF(logAsio, CFormat("HandleRead %zu %s") % bytes_transferred % m_IP);
		m_readBufferPtr = m_readBuffer;
		m_readBufferContent = (uint32)bytes_transferred;

		m_readPending = false;
		m_blocksRead = false;
		PostReadEvent(2);
	}


	//
	// Other functions
	//

	void StartBackgroundRead()
	{
		m_readPending = true;
		m_readBufferContent = 0;
		auto self = shared_from_this();
		dispatch(m_strand, [self]() { self->DispatchBackgroundRead(); });
	}

	void PostReadEvent(int DEBUG_ONLY(from) )
	{
		if (!m_eventPending) {
			AddDebugLogLineF(logAsio, CFormat("Post read event %d %s") % from % m_IP);
			m_eventPending = true;
			CLibSocket * wrapper = m_libSocket.load(std::memory_order_acquire);
			if (wrapper) {
				CoreNotify_LibSocketReceive(wrapper, m_ErrorCode);
			}
		}
	}

	void PostLostEvent()
	{
		CLibSocket * wrapper = m_libSocket.load(std::memory_order_acquire);
		if (wrapper && !m_destroying.load(std::memory_order_acquire) && !m_closed) {
			CoreNotify_LibSocketLost(wrapper);
		}
	}

	void SetError()
	{
		m_ErrorCode = 2;
	}

	bool SetError(const error_code & err)
	{
		m_ErrorCode = err.value();
		return m_ErrorCode != errc::success;
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

	// Atomic so OnWrapperGone() (called from the wrapper's dtor on any
	// thread) and the strand-side load in Destroy() can both touch it
	// without an external lock.
	std::atomic<CLibSocket *>	m_libSocket;
	ip::tcp::socket	*	m_socket;
										// remote IP
	wxString		m_IPstring;			// as String (use nowhere because of threading!)
	const wxChar *	m_IP;				// as char*  (use in debug logs)
	uint32			m_IPint;			// as int
	uint16			m_port;				// remote port
	bool			m_OK;
	int				m_ErrorCode;
	bool			m_blocksRead;
	char *			m_readBuffer;
	uint32			m_readBufferSize;
	char *			m_readBufferPtr;
	bool			m_readPending;
	uint32			m_readBufferContent;
	bool			m_eventPending;
	std::atomic<char*>	m_sendBuffer;	// atomic: shared between throttler thread (Write) and ASIO thread (HandleSend)
	std::atomic<bool>	m_blocksWrite;	// atomic: shared between throttler thread (BlocksWrite) and ASIO thread (HandleSend)
	io_context::strand	m_strand;		// handle synchronisation in io_service thread pool
	bool			m_connected;
	bool			m_closed;
	std::atomic<bool>	m_destroying;	// set once Destroy() has been called
	bool			m_proxyState;
	bool			m_notify;			// set by Notify()
	bool			m_sync;				// copied from !m_notify on Connect()
};


/**
 * Library socket wrapper
 */

CLibSocket::CLibSocket(int /* flags */)
{
	// make_shared so the impl can later use shared_from_this() inside async
	// callbacks. The TCP impl's ctor does not start any async ops, so no
	// post-construction Init() call is needed; async work starts in Connect().
	m_aSocket = std::make_shared<CAsioSocketImpl>(this);
}


CLibSocket::~CLibSocket()
{
	AddDebugLogLineF(logAsio, CFormat("~CLibSocket() %p %p %s") % this % m_aSocket.get() % m_aSocket->GetIP());
	// Detach the back-pointer first so any callbacks that fire after the
	// wrapper is gone don't dereference us. The impl itself stays alive as
	// long as any callback still holds a shared_from_this() ref; once the
	// last drops, the impl destructs cleanly and frees the asio socket.
	if (m_aSocket) {
		m_aSocket->OnWrapperGone();
	}
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


void CLibSocket::LinkSocketImpl(std::shared_ptr<class CAsioSocketImpl> socket)
{
	// Detach the back-pointer on the outgoing impl before swapping it out;
	// any in-flight callback that still holds a shared_from_this() ref on
	// it will then no-op its notify branch instead of touching us.
	if (m_aSocket) {
		m_aSocket->OnWrapperGone();
	}
	m_aSocket = std::move(socket);
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

// See the parallel comment on CAsioUDPSocketImpl. Same lifetime fix applied
// here so a pending async_accept completion can't fire on a freed acceptor
// impl after the wrapper has been deleted.
class CAsioSocketServerImpl : public ip::tcp::acceptor,
	public std::enable_shared_from_this<CAsioSocketServerImpl>
{
public:
	CAsioSocketServerImpl(const amuleIPV4Address & adr, CLibSocketServer * libSocketServer)
		: ip::tcp::acceptor(s_io_service),
		  m_libSocketServer(libSocketServer),
		  m_strand(s_io_service),
		  m_address(adr)
	{
		m_ok = false;
		m_socketAvailable = false;
	}

	~CAsioSocketServerImpl()
	{
	}

	// Init() runs the bind/listen/StartAccept sequence after the managing
	// shared_ptr is in place — StartAccept captures shared_from_this(), and
	// that's only valid post-construction.
	void Init()
	{
		try {
			open(m_address.GetEndpoint().protocol());
			SetCloexecOnSocket(native_handle());
			set_option(ip::tcp::acceptor::reuse_address(true));
			bind(m_address.GetEndpoint());
			listen();
			StartAccept();
			m_ok = true;
			AddDebugLogLineN(logAsio, CFormat("CAsioSocketServerImpl bind to %s %d") % m_address.IPAddress() % m_address.Service());
		} catch (const system_error& err) {
			AddDebugLogLineC(logAsio, CFormat("CAsioSocketServerImpl bind to %s %d failed - %s") % m_address.IPAddress() % m_address.Service() % err.code().message());
		}
	}

	// Detach the back-pointer to the wrapper so any in-flight async_accept
	// completion that fires after the wrapper has been deleted no-ops its
	// CoreNotify_ServerTCPAccept branch instead of dereferencing freed memory.
	void OnWrapperGone()
	{
		m_libSocketServer.store(nullptr, std::memory_order_release);
	}

	// For wxSocketServer, Ok will return true if the server could bind to the specified address and is already listening for new connections.
	bool IsOk() const { return m_ok; }

	void Close() { close();	}

	bool AcceptWith(CLibSocket & socket)
	{
		if (!m_socketAvailable) {
			AddDebugLogLineF(logAsio, "AcceptWith: nothing there");
			return false;
		}

		// return the socket we received
		socket.LinkSocketImpl(std::move(m_currentSocket));

		// check if we have another socket ready for reception
		m_currentSocket = std::make_shared<CAsioSocketImpl>(nullptr);
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
			AddDebugLogLineF(logAsio, "AcceptWith: ok, getting another socket in background");
		} else {
			// we got another socket right away
			m_socketAvailable = true;	// it is already true, but this improves readability
			AddDebugLogLineF(logAsio, "AcceptWith: ok, another socket is available");
			// aMule actually doesn't need a notification as it polls the listen socket.
			// amuleweb does need it though
			CLibSocketServer * w = m_libSocketServer.load(std::memory_order_acquire);
			if (w) {
				CoreNotify_ServerTCPAccept(w);
			}
		}

		return true;
	}

	bool SocketAvailable() const { return m_socketAvailable; }

private:

	void StartAccept()
	{
		m_currentSocket = std::make_shared<CAsioSocketImpl>(nullptr);
		auto self = shared_from_this();
		async_accept(m_currentSocket->GetAsioSocket(),
			bind_executor(m_strand, [self](const error_code& ec) { self->HandleAccept(ec); }));
	}

	void HandleAccept(const error_code& error)
	{
		if (error) {
			AddDebugLogLineC(logAsio, CFormat("Error in HandleAccept: %s") % error.message());
		} else {
			if (m_currentSocket->UpdateIP()) {
				AddDebugLogLineN(logAsio, CFormat("HandleAccept received a connection from %s:%d")
					% m_currentSocket->GetIP() % m_currentSocket->GetPort());
				m_socketAvailable = true;
				CLibSocketServer * w = m_libSocketServer.load(std::memory_order_acquire);
				if (w) {
					CoreNotify_ServerTCPAccept(w);
				}
				return;
			} else {
				AddDebugLogLineN(logAsio, "Error in HandleAccept: invalid socket");
			}
		}
		// We were not successful. Try again.
		// Post the request to the event queue to make sure it doesn't get called immediately.
		auto self = shared_from_this();
		post(m_strand, [self]() { self->StartAccept(); });
	}

	// The wrapper object. Atomic for the same reason as CAsioSocketImpl::m_libSocket.
	std::atomic<CLibSocketServer *> m_libSocketServer;
	// Startup ok
	bool m_ok;
	// The last socket that connected to us
	std::shared_ptr<CAsioSocketImpl> m_currentSocket;
	// Is there a socket available?
	bool m_socketAvailable;
	io_context::strand	m_strand;		// handle synchronisation in io_service thread pool
	// Bind address. Stored so Init() can run after construction (the
	// shared-from-this contract needs make_shared to complete before any
	// async ops start).
	amuleIPV4Address m_address;
};


CLibSocketServer::CLibSocketServer(const amuleIPV4Address& adr, int /* flags */)
{
	// make_shared so the impl can use shared_from_this() inside its
	// async_accept callbacks. Init() runs the bind/listen/StartAccept
	// sequence after the managing shared_ptr is in place.
	m_aServer = std::make_shared<CAsioSocketServerImpl>(adr, this);
	m_aServer->Init();
}


CLibSocketServer::~CLibSocketServer()
{
	if (m_aServer) {
		m_aServer->OnWrapperGone();
	}
	// shared_ptr drops automatically; impl stays alive via callback self refs
	// until the last in-flight async_accept completion drains.
}


// Accepts an incoming connection request, and creates a new CLibSocket object which represents the server-side of the connection.
// Only used in CamuleApp::ListenSocketHandler() and we don't get there.
CLibSocket * CLibSocketServer::Accept(bool /* wait */)
{
	wxFAIL;
	return NULL;
}


// Accept an incoming connection using the specified socket object.
bool CLibSocketServer::AcceptWith(CLibSocket & socket, bool WXUNUSED_UNLESS_DEBUG(wait) )
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

// Wake-from-sleep crash (issue #384) was caused by asio completion handlers
// firing on a freed CAsioUDPSocketImpl: pending async_receive_from ops survive
// a long suspend, complete on wake, and re-enter HandleRead → StartBackgroundRead
// after the impl has been destroyed by the post-resume socket-recreation path.
// The old 1-second-timer guard in Destroy() did not survive the time jump.
//
// Fix: enable_shared_from_this. Each async callback captures
// [self = shared_from_this()], keeping the impl alive until the last in-flight
// callback drops its ref. The wrapper's raw back-pointer m_libSocket is atomic
// and nulled on the strand during Destroy(), so callbacks that fire after the
// wrapper has been notified-destroyed silently no-op instead of dereferencing
// freed memory.
class CAsioUDPSocketImpl : public std::enable_shared_from_this<CAsioUDPSocketImpl>
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
		m_address(address)
	{
		m_muleSocket = NULL;
		m_socket = NULL;
		m_readBuffer = new char[CMuleUDPSocket::UDP_BUFFER_SIZE];
		m_OK = true;
		m_destroying.store(false, std::memory_order_relaxed);
		// CreateSocket() must run after construction completes — it calls
		// StartBackgroundRead() which captures shared_from_this(), and that
		// requires a managing shared_ptr to already exist. The wrapper calls
		// Init() right after make_shared.
	}

	~CAsioUDPSocketImpl()
	{
		AddDebugLogLineF(logAsio, "UDP ~CAsioUDPSocketImpl");
		delete m_socket;
		delete[] m_readBuffer;
		DeleteContents(m_receiveBuffers);
	}

	// Called by the wrapper after make_shared so StartBackgroundRead() can
	// safely call shared_from_this().
	void Init()
	{
		CreateSocket();
	}

	// Called by the wrapper's destructor (or by the destroy chain) to detach
	// the back-pointer so any still-in-flight callbacks no-op the notify path
	// instead of touching the freed wrapper. Atomic so it's safe to call from
	// any thread without an external lock.
	void OnWrapperGone()
	{
		m_libSocket.store(nullptr, std::memory_order_release);
	}

	void SetClientData(CMuleUDPSocket * muleSocket)
	{
		AddDebugLogLineF(logAsio, "UDP SetClientData");
		m_muleSocket = muleSocket;
	}

	uint32 RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes)
	{
		CUDPData * recdata;
		{
			wxMutexLocker lock(m_receiveBuffersLock);
			if (m_receiveBuffers.empty()) {
				AddDebugLogLineN(logAsio, "UDP RecvFromError no data");
				return 0;
			}
			recdata = * m_receiveBuffers.begin();
			m_receiveBuffers.pop_front();
		}
		uint32 read = recdata->size;
		if (read > nBytes) {
			// should not happen
			AddDebugLogLineN(logAsio, CFormat("UDP RecvFromError too much data %d") % read);
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
		AddDebugLogLineF(logAsio, CFormat("UDP SendTo %d to %s") % nBytes % addr.IPAddress());
		auto self = shared_from_this();
		dispatch(m_strand, [self, recdata]() { self->DispatchSendTo(recdata); });
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
			auto self = shared_from_this();
			dispatch(m_strand, [self]() { self->DispatchClose(); });
		}
	}

	// Destroy() schedules a single strand task that closes the socket, nulls
	// the back-pointer, and deletes the wrapper. The impl itself stays alive
	// as long as any in-flight async callback holds a shared_from_this() ref
	// — it dies cleanly when the last drains, with no risk of a pending
	// completion firing on freed memory.
	//
	// Note: unlike the TCP path which posts CoreNotify_LibSocketDestroy to
	// route the wrapper delete through the GUI thread, UDP deletes the
	// wrapper directly. By contract the caller (CMuleUDPSocket) has already
	// nulled its pointer before calling Destroy(), so nothing else is
	// expected to reach the wrapper.
	void Destroy()
	{
		if (m_destroying.exchange(true, std::memory_order_acq_rel)) {
			// Already destroying; no-op so callers can be sloppy about it.
			return;
		}
		CLibUDPSocket * wrapper = m_libSocket.load(std::memory_order_acquire);
		AddDebugLogLineF(logAsio, CFormat("Destroy() %p %p") % wrapper % this);

		auto self = shared_from_this();
		auto teardown = [self]() {
			// Null the back-pointer before deleting the wrapper so any
			// callback that fires after this point sees null and skips
			// its notify branch.
			CLibUDPSocket * w = self->m_libSocket.exchange(nullptr,
				std::memory_order_acq_rel);
			if (self->m_socket) {
				error_code ec;
				self->m_socket->close(ec);
			}
			if (w) {
				// Wrapper dtor drops its shared_ptr<impl>; we still hold
				// 'self' here so the impl stays alive until all queued
				// callbacks drain.
				delete w;
			}
		};

		if (s_io_service.stopped()) {
			// Service stopped (shutdown): run the teardown inline; no
			// pending callbacks to wait for.
			teardown();
		} else {
			post(m_strand, teardown);
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
			AddDebugLogLineC(logAsio, CFormat("UDP Close error %s") % ec.message());
		} else {
			AddDebugLogLineF(logAsio, "UDP Closed");
		}
	}

	void DispatchSendTo(CUDPData * recdata)
	{
		ip::udp::endpoint endpoint(recdata->ipadr.GetEndpoint().address(), recdata->ipadr.Service());

		AddDebugLogLineF(logAsio, CFormat("UDP DispatchSendTo %d to %s:%d") % recdata->size
			% endpoint.address().to_string() % endpoint.port());
		auto self = shared_from_this();
		m_socket->async_send_to(buffer(recdata->buffer, recdata->size), endpoint,
			bind_executor(m_strand, [self, recdata](const error_code& ec, std::size_t sent) { self->HandleSendTo(ec, sent, recdata); }));
	}

	//
	// Completion handlers for async requests
	//

	void HandleRead(const error_code & ec, size_t received)
	{
		if (ec) {
			AddDebugLogLineN(logAsio, CFormat("UDP HandleReadError %s") % ec.message());
		} else if (received == 0) {
			AddDebugLogLineF(logAsio, "UDP HandleReadError nothing available");
		} else if (m_muleSocket == NULL) {
			AddDebugLogLineN(logAsio, "UDP HandleReadError no handler");
		} else {

			amuleIPV4Address ipadr = amuleIPV4Address(CamuleIPV4Endpoint(m_receiveEndpoint));
			AddDebugLogLineF(logAsio, CFormat("UDP HandleRead %d %s:%d") % received % ipadr.IPAddress() % ipadr.Service());

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
			AddDebugLogLineN(logAsio, CFormat("UDP HandleSendToError %s") % ec.message());
		} else if (sent != recdata->size) {
			AddDebugLogLineN(logAsio, CFormat("UDP HandleSendToError tosend: %d sent %d") % recdata->size % sent);
		}
		if (m_muleSocket == NULL) {
			AddDebugLogLineN(logAsio, "UDP HandleSendToError no handler");
		} else {
			AddDebugLogLineF(logAsio, CFormat("UDP HandleSendTo %d to %s") % sent % recdata->ipadr.IPAddress());
			CoreNotify_UDPSocketSend(m_muleSocket);
		}
		delete recdata;
	}

	//
	// Other functions
	//

	void CreateSocket()
	{
		try {
			delete m_socket;
			ip::udp::endpoint endpoint(m_address.GetEndpoint().address(), m_address.Service());
			// Open + bind in two steps so we can mark the fd close-on-exec
			// before bind, matching the TCP acceptor path. Single-arg
			// ctor + open() is the documented Asio idiom for "create
			// without binding".
			m_socket = new ip::udp::socket(s_io_service);
			m_socket->open(endpoint.protocol());
			SetCloexecOnSocket(m_socket->native_handle());
			m_socket->bind(endpoint);
			AddDebugLogLineN(logAsio, CFormat("Created UDP socket %s %d") % m_address.IPAddress() % m_address.Service());
			StartBackgroundRead();
		} catch (const system_error& err) {
			AddLogLineC(CFormat("Error creating UDP socket %s %d : %s") % m_address.IPAddress() % m_address.Service() % err.code().message());
			m_socket = NULL;
			m_OK = false;
		}
	}

	void StartBackgroundRead()
	{
		// Skip if Destroy() has already nulled the socket via the strand
		// teardown lambda. Without this guard the impl's last self ref
		// (held by the in-flight async_receive_from completion that
		// brought us here) would try to re-queue a recv on a closed-and-
		// nulled socket.
		if (!m_socket || m_destroying.load(std::memory_order_acquire)) {
			return;
		}
		auto self = shared_from_this();
		m_socket->async_receive_from(buffer(m_readBuffer, CMuleUDPSocket::UDP_BUFFER_SIZE), m_receiveEndpoint,
			bind_executor(m_strand, [self](const error_code& ec, std::size_t n) { self->HandleRead(ec, n); }));
	}

	// Atomic so OnWrapperGone() (called from the wrapper's dtor on any
	// thread) and the strand-side load in Destroy() can both touch it
	// without an external lock.
	std::atomic<CLibUDPSocket *>	m_libSocket;
	ip::udp::socket *	m_socket;
	CMuleUDPSocket *	m_muleSocket;
	bool				m_OK;
	std::atomic<bool>	m_destroying;	// set once Destroy() has been called
	io_context::strand	m_strand;		// handle synchronisation in io_service thread pool
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
	// make_shared must run to completion (so a shared_ptr exists to manage
	// the object) before Init() — Init triggers async_receive_from whose
	// completion handler captures shared_from_this(), and that requires a
	// managing shared_ptr to already be in place.
	m_aSocket = std::make_shared<CAsioUDPSocketImpl>(address, flags, this);
	m_aSocket->Init();
}


CLibUDPSocket::~CLibUDPSocket()
{
	AddDebugLogLineF(logAsio, CFormat("~CLibUDPSocket() %p %p") % this % m_aSocket.get());
	// Detach the back-pointer first so any callbacks that fire after the
	// wrapper is gone don't dereference us. The impl itself stays alive as
	// long as any callback still holds a shared_from_this() ref; once the
	// last drops, the impl destructs cleanly and frees the asio socket.
	if (m_aSocket) {
		m_aSocket->OnWrapperGone();
	}
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
		auto worker = make_work_guard(s_io_service);		// keep io_service running
		s_io_service.run();
		AddDebugLogLineN(logAsio, CFormat("Asio thread %d stopped") % m_threadNumber);

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
	ip::address_v4 adr = ip::make_address_v4(sname, ec);
	if (!ec) {
		m_endpoint->address(adr);
		return true;
	}
	AddDebugLogLineN(logAsio, CFormat("Hostname(\"%s\") failed, not an IP address %s") % name % ec.message());

	// Try to resolve (sync). Normally not required. Unless you type in your hostname as "local IP address" or something.
	error_code ec2;
	ip::tcp::resolver res(s_io_service);
	// We only want to get IPV4 addresses.
	ip::tcp::resolver::results_type endpoint_iterator = res.resolve(sname, "", ec2);
	if (ec2) {
		AddDebugLogLineN(logAsio, CFormat("Hostname(\"%s\") resolve failed: %s") % name % ec2.message());
		return false;
	}
	if (endpoint_iterator == ip::tcp::resolver::results_type()) {
		AddDebugLogLineN(logAsio, CFormat("Hostname(\"%s\") resolve failed: no address found") % name);
		return false;
	}
	m_endpoint->address(endpoint_iterator.begin()->endpoint().address());
	AddDebugLogLineN(logAsio, CFormat("Hostname(\"%s\") resolved to %s") % name % IPAddress());
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
	return CFormat("%s") % m_endpoint->address().to_string();
}

// "Set address to any of the addresses of the current machine."
// This just sets the address to 0.0.0.0 .
// wx does the same.
bool amuleIPV4Address::AnyAddress()
{
	m_endpoint->address(ip::address_v4::any());
	AddDebugLogLineN(logAsio, CFormat("AnyAddress: set to %s") % IPAddress());
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
			AddDebugLogLineF(logAsio, CFormat("LibSocketConnect Destroying %s %d") % socket->GetIP() % error);
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketConnect Proxy %s %d") % socket->GetIP() % error);
			socket->OnProxyEvent(MULE_SOCKET_CONNECTION);
		} else {
			AddDebugLogLineF(logAsio, CFormat("LibSocketConnect %s %d") %socket->GetIP() % error);
			socket->OnConnect(error);
		}
	}

	void LibSocketSend(CLibSocket * socket, int error)
	{
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketSend Destroying %s %d") % socket->GetIP() % error);
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketSend Proxy %s %d") % socket->GetIP() % error);
			socket->OnProxyEvent(MULE_SOCKET_OUTPUT);
		} else {
			AddDebugLogLineF(logAsio, CFormat("LibSocketSend %s %d") % socket->GetIP() % error);
			socket->OnSend(error);
		}
	}

	void LibSocketReceive(CLibSocket * socket, int error)
	{
		socket->EventProcessed();
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketReceive Destroying %s %d") % socket->GetIP() % error);
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketReceive Proxy %s %d") % socket->GetIP() % error);
			socket->OnProxyEvent(MULE_SOCKET_INPUT);
		} else {
			AddDebugLogLineF(logAsio, CFormat("LibSocketReceive %s %d") % socket->GetIP() % error);
			socket->OnReceive(error);
		}
	}

	void LibSocketLost(CLibSocket * socket)
	{
		if (socket->IsDestroying()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketLost Destroying %s") % socket->GetIP());
		} else if (socket->GetProxyState()) {
			AddDebugLogLineF(logAsio, CFormat("LibSocketLost Proxy %s") % socket->GetIP());
			socket->OnProxyEvent(MULE_SOCKET_LOST);
		} else {
			AddDebugLogLineF(logAsio, CFormat("LibSocketLost %s") % socket->GetIP());
			socket->OnLost();
		}
	}

	void LibSocketDestroy(CLibSocket * socket)
	{
		AddDebugLogLineF(logAsio, CFormat("LibSocket_Destroy %s") % socket->GetIP());
		delete socket;
	}

	void ProxySocketEvent(CLibSocket * socket, int evt)
	{
		AddDebugLogLineF(logAsio, CFormat("ProxySocketEvent %s %d") % socket->GetIP() % evt);
		socket->OnProxyEvent(evt);
	}

	void ServerTCPAccept(CLibSocketServer * socketServer)
	{
		AddDebugLogLineF(logAsio, "ServerTCP_Accept");
		socketServer->OnAccept();
	}

	void UDPSocketSend(CMuleUDPSocket * socket)
	{
		AddDebugLogLineF(logAsio, "UDPSocketSend");
		socket->OnSend(0);
	}

	void UDPSocketReceive(CMuleUDPSocket * socket)
	{
		AddDebugLogLineF(logAsio, "UDPSocketReceive");
		socket->OnReceive(0);
	}


} // namespace MuleNotify

//
// Initialize MuleBoostVersion
//
wxString MuleBoostVersion = CFormat("%d.%d") % (BOOST_VERSION / 100000) % (BOOST_VERSION / 100 % 1000);

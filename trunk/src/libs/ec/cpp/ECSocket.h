//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2006 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#ifndef ECSOCKET_H
#define ECSOCKET_H


#include <deque>	// Needed for std::deque
#include <memory>	// Needed for std::auto_ptr	// Do_not_auto_remove (mingw-gcc-3.4.5)
#include <string>
#include <vector>

#include <zlib.h>	// Needed for packet (de)compression
#include <inttypes.h>	// Needed for uint32_t

#include <wx/defs.h>	// Needed for wx/debug.h
#include <wx/debug.h>	// Needed for wxASSERT

enum ECSocketErrors {
	EC_ERROR_NOERROR,
	EC_ERROR_INVOP,
	EC_ERROR_IOERR,
	EC_ERROR_INVADDR,
	EC_ERROR_INVSOCK,
	EC_ERROR_NOHOST,
	EC_ERROR_INVPORT,
	EC_ERROR_WOULDBLOCK,
	EC_ERROR_TIMEDOUT,
	EC_ERROR_MEMERR,
	EC_ERROR_DUMMY,
	EC_ERROR_UNKNOWN
};

class CECPacket;
class CQueuedData;

/*! \class CECSocket
 *
 * \brief Socket handler for External Communications (EC).
 *
 * CECSocket takes care of the transmission of EC packets
 */

class CECSocket{
	friend class CECPacket;
	friend class CECTag;

private:
	static const unsigned int EC_SOCKET_BUFFER_SIZE = 2048;
	const bool m_use_events;
	
	// Output related data
	std::deque<CQueuedData *> m_output_queue;

	// zlib (deflation) buffers
	std::vector<unsigned char> m_in_ptr;
	std::vector<unsigned char> m_out_ptr;
	std::auto_ptr<CQueuedData> m_curr_rx_data;
	std::auto_ptr<CQueuedData> m_curr_tx_data;
	
	// This transfer only
	uint32_t m_rx_flags;
	uint32_t m_tx_flags;
	uint32_t m_my_flags;
	size_t m_bytes_needed;
	bool m_in_header;
	
	
	uint32_t m_curr_packet_len;
	z_stream m_z;
	
public:
	CECSocket(bool use_events);
	virtual ~CECSocket();

	bool ConnectSocket(uint32_t ip, uint16_t port);

	void CloseSocket() { InternalClose(); }

	/**
	 * Sends an EC packet and returns immediately.
	 *
	 * @param packet The CECPacket packet to be sent.
	 *
	 * This is an asynchronous call, the function returns
	 * immediately and the packet is sent on idle time.
	 *
	 * @note It's the caller's responsibilty to \c delete
	 * the \e packet.
	 */
	void SendPacket(const CECPacket *packet);

	/**
	 * Sends an EC packet and waits for a reply.
	 *
	 * @param request The CECPacket packet to be sent.
	 * @return The reply packet for the request.
	 *
	 * Unlike SendPacket(), this call is synchronous and blocking.
	 * The packet is sent immediately (or at least as soon as possible),
	 * and the function does not return until a reply is received,
	 * or a timeout encountered.
	 *
	 * The returned packet will be allocated on the heap with \c new,
	 * or \c NULL is returned in case of an error (timeout).
	 *
	 * @note It's the caller's responsibilty to \c delete both
	 * request and reply.
	 *
	 * @note OnPacketReceived() won't be called for packets
	 * received via this function.
	 */
	const CECPacket *SendRecvPacket(const CECPacket *request);

	/**
	 * Event handler function called when a new packet is received.
	 *
	 * @param packet The packet that has been received.
	 * @return The reply packet or \c NULL if no reply needed.
	 *
	 * In this function the application should process the received
	 * packet, and create a reply if necessary. The reply must be allocated
	 * on the heap with \c new. If no reply is necessary, the return
	 * value of the function should be \c NULL. The library will \c delete
	 * both packets.
	 *
	 * @note This function won't be called for packets received via the
	 * SendRecvPacket() function.
	 */
	virtual const CECPacket *OnPacketReceived(const CECPacket *packet);

	/**
	 * Get a message describing the error.
	 *
	 * @param error The code of the error for which a message should be returned.
	 * @return The text descibing the error.
	 */
	virtual std::string	GetLastErrorMsg();

	/**
	 * Error handler.
	 *
	 * This function is called when an error occurs. Use GetLastError() and
	 * GetErrorMsg() to find out the nature of the error.
	 *
	 * The default error handler prints out an error message in debug builds,
	 * and destroys the socket.
	 */
	virtual void OnError();


	/**
	 * Socket lost event handler.
	 *
	 * This function is called when the socket is lost (either because of a network
	 * failure or because the remote end closed the socket gracefully).
	 *
	 * The default handler destroys the socket.
	 */
	virtual void OnLost();

	/**
	 * Event handler for connection events.
	 *
	 * This function is called when a connection attempt succeeds. When CECSocket
	 * is compiled with ECSOCKET_USE_EVENTS == 0, WaitOnConnect() should be called
	 * for this to work.
	 */
	virtual void OnConnect();

	void OnInput();
	void OnOutput();

	bool WouldBlock() { return InternalGetLastError() == EC_ERROR_WOULDBLOCK; } 
	bool GotError() { return InternalGetLastError() != EC_ERROR_NOERROR; } 

	void SocketRead(void* ptr, uint32_t len) { InternalRead(ptr,len); }
	void SocketWrite(const void* ptr, uint32_t len) { InternalWrite(ptr,len); }
	bool SocketError() { return InternalError() && GotError(); }

	uint32_t GetLastCount() { return InternalLastCount(); }
	bool WaitSocketConnect(long secs = -1, long msecs = 0) { return InternalWaitOnConnect(secs,msecs); }
	bool WaitSocketWrite(long secs = -1, long msecs = 0) { return InternalWaitForWrite(secs,msecs); }
	bool WaitSocketRead(long secs = -1, long msecs = 0) { return InternalWaitForRead(secs,msecs); }	
	
	bool IsSocketConnected() { return InternalIsConnected(); }
	
	void DestroySocket() { return InternalDestroy(); }
	
 private:
	const CECPacket *ReadPacket();
	void WritePacket(const CECPacket *packet);

	// These 4 methods are to be used by CECPacket & CECTag
	bool	ReadNumber(void *buffer, size_t len);
	bool	ReadBuffer(void *buffer, size_t len);

	bool	WriteNumber(const void *buffer, size_t len);
	bool	WriteBuffer(const void *buffer, size_t len);

	// Internal stuff
	bool	FlushBuffers();

	size_t	ReadBufferFromSocket(void *buffer, size_t len);
	void	WriteBufferToSocket(const void *buffer, size_t len);

 	/* virtuals */
		virtual bool InternalConnect(uint32_t ip, uint16_t port, bool wait) = 0;
	
		virtual uint32_t InternalLastCount() = 0;
		virtual bool InternalWaitOnConnect(long secs = -1, long msecs = 0) = 0;
		virtual bool InternalWaitForWrite(long secs = -1, long msecs = 0) = 0;
		virtual bool InternalWaitForRead(long secs = -1, long msecs = 0) = 0;
	
		virtual int InternalGetLastError() = 0;
	
		virtual void InternalClose() = 0;
		virtual bool InternalError() = 0;
		virtual void InternalRead(void* ptr, uint32_t len) = 0;
		virtual void InternalWrite(const void* ptr, uint32_t len) = 0;
		
		virtual bool InternalIsConnected() = 0;
		virtual void InternalDestroy() = 0;
};


class CQueuedData
{
	std::vector<unsigned char> m_data;
	unsigned char *m_rd_ptr, *m_wr_ptr;
public:
	CQueuedData(size_t len)
	:
	m_data(len)
	{
		m_rd_ptr = m_wr_ptr = &m_data[0];
	}
	
	~CQueuedData() {}
	
	void Rewind()
	{
		m_rd_ptr = m_wr_ptr = &m_data[0];
	}
	
	void Write(const void *data, size_t len)
	{
		wxASSERT(len <= m_data.size());
		memcpy(m_wr_ptr, data, len);
		m_wr_ptr += len;
	}

	void WriteAt(const void *data, size_t len, size_t off)
	{
		wxASSERT(off + len <= m_data.size());
		memcpy(&m_data[0] + off, data, len);
	}

	void Read(void *data, size_t len)
	{
		wxASSERT(len <= GetUnreadDataLength());
		memcpy(data, m_rd_ptr, len);
		m_rd_ptr += len;
	}

	/*
	 * Pass pointers to zlib. From now on, no Read() calls are allowed
	 */
	void ToZlib(z_stream &m_z)
	{
		m_z.avail_in = GetUnreadDataLength();
		m_z.next_in = m_rd_ptr;
	}
	
	void WriteToSocket(CECSocket *sock)
	{
		wxASSERT(m_rd_ptr < &m_data[0] + m_data.size());
		wxASSERT(m_wr_ptr <= &m_data[0] + m_data.size());
		sock->SocketWrite(m_rd_ptr, m_wr_ptr - m_rd_ptr);
		m_rd_ptr += sock->GetLastCount();
	}

	void ReadFromSocket(CECSocket *sock, int len)
	{
		wxASSERT(m_wr_ptr + len <= &m_data[0] + m_data.size());
		sock->SocketRead(m_wr_ptr, len);
		m_wr_ptr += sock->GetLastCount();
	}
	
	size_t ReadFromSocketAll(CECSocket *sock, size_t len);
	
	size_t GetLength()	{ return m_data.size(); }
	size_t GetDataLength()	{ return m_wr_ptr - &m_data[0]; }
	size_t GetRemLength()	{ return m_data.size() - GetDataLength(); }
	size_t GetUnreadDataLength() { return m_wr_ptr - m_rd_ptr; }
	
};

#endif // ECSOCKET_H

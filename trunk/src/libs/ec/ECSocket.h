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

#include <wx/socket.h>		// Needed for wxSocketClient
#include <wx/string.h>		// Needed for wxString
#include <wx/thread.h>		// Needed for MT-Safe API.
#include "Types.h"

#include "zlib.h"			// Needed for packet (de)compression

class CECPacket;

#include <deque>			// Needed for std::deque
#include <set>				// Needed for std::set
#include <utility>			// Needed for std::pair

//void DumpMemToStr(const void *buff, int n);

class CQueuedData {
		unsigned char *m_data, *m_rd_ptr, *m_wr_ptr;
		size_t m_len;
	public:
		CQueuedData(size_t len)
		{
			m_data = new unsigned char [len];
			m_rd_ptr = m_wr_ptr = m_data;
			m_len = len;
		}
		
		~CQueuedData()
		{
			delete [] m_data;
		}
		
		void Rewind()
		{
			m_rd_ptr = m_wr_ptr = m_data;
		}
		
		void Write(const void *data, size_t len)
		{
			memcpy(m_wr_ptr, data, len);
			m_wr_ptr += len;
		}

		void WriteAt(const void *data, size_t len, size_t off)
		{
			memcpy(m_data + off, data, len);
		}

		void Read(void *data, size_t len)
		{
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
		
		void WriteToSocket(wxSocketBase *sock)
		{
			sock->Write(m_rd_ptr, m_wr_ptr - m_rd_ptr);
			m_rd_ptr += sock->LastCount();
		}

		void ReadFromSocket(wxSocketBase *sock, int len)
		{
			sock->Read(m_wr_ptr, len);
			m_wr_ptr += sock->LastCount();
		}
		
		size_t ReadFromSocketAll(wxSocketBase *sock, size_t len);
		
		size_t GetLength()	{ return m_len; }
		size_t GetDataLength()	{ return m_wr_ptr - m_data; }
		size_t GetRemLength()	{ return m_len - GetDataLength(); }
		size_t GetUnreadDataLength() { return m_wr_ptr - m_rd_ptr; }
		
		//
		// Dump mem in dword format
//		void DumpMem()
//		{
//			DumpMemToStr(m_data, GetDataLength());
//			printf("RD ptr @ offset %04x\n", m_rd_ptr - m_data);
//		}
};


/*! \class CECSocket
 *
 * \brief Socket handler for External Communications (EC).
 *
 * CECSocket takes care of the transmission of EC packets
 */

class CECSocket : public wxSocketClient {
	friend class CECPacket;
	friend class CECTag;
	friend class CECSocketHandler;

 public:
	CECSocket(bool use_events);
	virtual ~CECSocket();

	bool Connect(wxSockAddress& address);

	/**
	 * Destroy socket.
	 *
	 * This function does the same as wxSocketBase::Destroy(),
	 * except that it won't immediately delete the object if it
	 * can't be scheduled for deletion, instead just marks it as
	 * being deleted. Use IsDestroying() to check whether the socket
	 * is marked to be deleted.
	 *
	 * @param raiseLostEvent When \c true, OnLost() will be called for
	 * the first Destroy() call on the socket.
	 */
	void Destroy();

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
	wxString	GetErrorMsg(wxSocketError error);

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

	bool m_use_events;
	
	// Output related data
	std::deque<CQueuedData*> m_output_queue;

	size_t m_bytes_needed;
	bool m_in_header;
	
	// zlib (deflation) buffers
	unsigned char *m_in_ptr, *m_out_ptr;
	
	CQueuedData *m_curr_rx_data, *m_curr_tx_data;
	
	// This transfer only
	uint32 m_rx_flags, m_tx_flags;
	uint32 m_my_flags;
	
	uint32 m_curr_packet_len;
	
	z_stream m_z;
};

#endif // ECSOCKET_H

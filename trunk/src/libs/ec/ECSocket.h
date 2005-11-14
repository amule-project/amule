//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal Veiga ( kry@users.sourceforge.net )
// Copyright (c) 2005 Dévai Tamás ( gonosztopi@amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef ECSOCKET_H
#define ECSOCKET_H

#include <wx/socket.h>		// Needed for wxSocketClient
#include <wx/string.h>		// Needed for wxString
#include <wx/thread.h>		// Needed for MT-Safe API.
#include "Types.h"

#include "zlib.h"			// Needed for packet (de)compression

// By default we use event driven sockets.
#ifndef ECSOCKET_USE_EVENTS
#	define ECSOCKET_USE_EVENTS	1
#endif


#if ECSOCKET_USE_EVENTS
#include <deque>			// Needed for std::deque
#include <set>				// Needed for std::set
#include <utility>			// Needed for std::pair

class CQueuedData;

#endif /* ECSOCKET_USE_EVENTS */


class CECPacket;


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
	CECSocket();
	virtual ~CECSocket();

#if !ECSOCKET_USE_EVENTS
	// TODO: Get rid of this
	bool	WaitOnConnect(long seconds = -1, long milliseconds = 0)
		{
			if (wxSocketClient::WaitOnConnect(seconds, milliseconds)) {
				OnConnect();
				return true;
			}
			return false;
		}
#endif

	 /**
	 * Close the socket.
	 *
	 * This is just an extension to wxSocketBase::Close(),
	 * to call OnClose().
	 */
	bool	Close()
		{
			OnClose();
			return wxSocketClient::Close();
		}

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
	void	Destroy(bool raiseLostEvent = false);

	/**
	 * Check whether the socket is marked to be deleted.
	 *
	 * @return \c true if Destroy() was called on the socket, but the
	 * socket isn't yet deleted.
	 */
	bool	IsDestroying() { return m_destroying; }

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
	void		SendPacket(const CECPacket *packet);

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

#if ECSOCKET_USE_EVENTS

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
	virtual const CECPacket *OnPacketReceived(const CECPacket *WXUNUSED(packet)) { return NULL; }

#endif /* ECSOCKET_USE_EVENTS */

	/**
	 * Get the last error code.
	 *
	 * @return The error code of the last error happened.
	 */
	wxSocketError	GetLastError() { return m_lastError; }

	/**
	 * Get a message describing the error.
	 *
	 * @param error The code of the error for which a message should be returned.
	 * @return The text descibing the error.
	 */
	wxString	GetErrorMsg(wxSocketError error);

	/**
	 * Event handler for connection events.
	 *
	 * This function is called when a connection attempt succeeds. When CECSocket
	 * is compiled with ECSOCKET_USE_EVENTS == 0, WaitOnConnect() should be called
	 * for this to work.
	 */
	virtual void	OnConnect() {}

	/**
	 * Error handler.
	 *
	 * This function is called when an error occurs. Use GetLastError() and
	 * GetErrorMsg() to find out the nature of the error.
	 *
	 * The default error handler prints out an error message in debug builds,
	 * and destroys the socket.
	 */
	virtual void	OnError();

	/**
	 * Close event handler.
	 *
	 * This function is called when the connection is closed (i.e. Close() was
	 * called). Use this to clean up anything you did in OnConnect().
	 */
	virtual void	OnClose() {};

	/**
	 * Socket lost event handler.
	 *
	 * This function is called when the socket is lost (either because of a network
	 * failure or because the remote end closed the socket gracefully).
	 *
	 * The default handler destroys the socket.
	 */
	virtual void	OnLost();

 private:
#if ECSOCKET_USE_EVENTS
	typedef std::pair<const CECPacket *, uint32>	packet_desc;

	// Packet I/O
	void	ReadPacket();
	void	WritePacket(packet_desc);
#else
	const CECPacket *ReadPacket();
	void	WritePacket(const CECPacket *packet);
#endif

	// These 4 methods are to be used by CECPacket & CECTag
	bool	ReadNumber(void *buffer, size_t len);
	bool	ReadBuffer(void *buffer, size_t len);

	bool	WriteNumber(const void *buffer, size_t len);
	bool	WriteBuffer(const void *buffer, size_t len);

	// Internal stuff
	bool	FlushBuffers();
	void	InitBuffers();

	uint32	ReadFlags();
	void	WriteFlags(uint32);		

	size_t	ReadBufferFromSocket(void *buffer, size_t required_len, size_t max_len);
	void	WriteBufferToSocket(const void *buffer, size_t len);

	wxSocketError	m_lastError;
	bool		m_destroying;

#if ECSOCKET_USE_EVENTS

	// Event handlers
	void	DoConnect()	{ m_canSend = true; OnConnect(); }
	void	DoInput();
	void	DoOutput();
	void	DoLost()	{ m_canSend = false; OnLost(); }
	void	DoSendPacket();
	void	DoReceivePacket();
	void	HandlePacket();

	bool	CanReadNBytes(size_t len);

	uint32	m_id;

	wxCRIT_SECT_DECLARE_MEMBER(m_cs_packet_out);
	wxCRIT_SECT_DECLARE_MEMBER(m_cs_id_list);
	wxCRIT_SECT_DECLARE_MEMBER(m_cs_waited_packets);

	// Input related data
	bool	m_canUseIDs;
	std::deque<CQueuedData*>	m_input_queue;
	std::deque<packet_desc>		m_input_packet_queue;

	std::set<uint32>		m_waited_ids;
	std::set<uint32>		m_abandoned_ids;
	std::deque<packet_desc>		m_waited_packets;

	// Output related data
	bool	m_canSend;
	std::deque<CQueuedData*>	m_output_queue;
	std::deque<packet_desc>		m_output_packet_queue;

#endif /* ECSOCKET_USE_EVENTS */

	// Transfer specific data
#if wxUSE_THREADS
	bool	m_transfer_in_progress;
	wxMutex	m_transfer_lock;
#endif
	// Common between all transfers
	bool			m_firsttransfer;
	uint32			m_my_flags;
	uint32			m_accepts;
	unsigned char *		m_in_ptr;
	unsigned char *		m_out_ptr;
	// This transfer only
	uint32			m_used_flags;
	z_stream		m_z;
};

#endif // ECSOCKET_H

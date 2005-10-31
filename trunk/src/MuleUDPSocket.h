//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef MULEUDPSOCKET_H
#define MULEUDPSOCKET_H


#include "Types.h"				// Needed for uint16 and uint32
#include "ThrottledSocket.h"	// Needed for ThrottledControlSocket
#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include <list>


class CDatagramSocketProxy;
class CProxyData;
class CPacket;


/***
 * This class provides a UBT governed UDP-socket.
 *
 * The CMuleUDPSocket are created with the NOWAIT option and
 * handle both INPUT and OUTPUT events.
 * 
 * The following additional features are provided compared to CDatagramSocketProxy:
 *  - Goverened by the UBT.
 *  - Automatic sending/receiving of packets.
 *  - Fallover recovery for when a socket becomes invalid (error 4).
 *
 * @see ThrottledControlSocket
 * @see CDatagramSocketProxy
 */
class CMuleUDPSocket : public ThrottledControlSocket
{
public:
	/**
	 * Opens a UDP socket at the specified address.
	 *
	 * @param name Name used when logging events.
	 * @param id The ID used for events.
	 * @param address The address where the socket will listen.
	 * @param ProxyData ProxyData assosiated with the socket.
	 */
	CMuleUDPSocket(const wxString& name, int id, const amuleIPV4Address& address, const CProxyData* ProxyData = NULL);
	
	/**
	 * Destructor, safely closes the socket if opened.
	 */
	virtual ~CMuleUDPSocket();

	
	/**
	 * Opens the socket.
	 *
	 * The socket is bound to the address specified in
	 * the constructor.
	 */
	void Open();
	
	/**
	 * Closes the socket.
	 *
	 * The socket can be reopened by calling Open.
	 */
	void Close();
	
	
	/**
	 * This function is called by aMule when the socket may send.
	 */
	virtual void	OnSend(int errorCode);

	/**
	 * This function is called by aMule when there are data to be received.
	 */
	virtual void	OnReceive(int errorCode);
	

	/**
	 * Queues a packet for sending.
	 *
	 * @param packet The packet to send.
	 * @param IP The target IP address.
	 * @param port The target port.
	 *
	 * Note that CMuleUDPSocket takes ownership of the packet.
	 */
	void	SendPacket(CPacket* packet, uint32 IP, uint16 port);


	/**
	 * Returns true if the socket is Ok, false otherwise.
	 *
	 * @see wxSocketBase::Ok
	 */
	bool	Ok();

protected:
	/**
	 * This function is called when a packet has been received.
	 *
	 * @param addr The address from where data was received.
	 * @param buffer The data that has been received.
	 * @param length The length of the data buffer.
	 */
	virtual void OnPacketReceived(amuleIPV4Address& addr, byte* buffer, size_t length) = 0;

	
	/** See ThrottledControlSocket::SendControlData */
	SocketSentBytes  SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize);
	    
private:
	/**
	 * Sends a packet to the specified address.
	 *
	 * @param The data to be sent.
	 * @param length the length of the data buffer.
	 * @param ip The target ip address.
	 * @param port The target port.
	 */
	bool	SendTo(char* buffer, uint32 length, uint32 ip, uint16 port);

	
	//! Specifies if the last write attempt would cause the socket to block.
	bool					m_busy;
	//! The name of the socket, used for debugging messages.
	wxString				m_name;
	//! The socket-ID, used for event-handling.
	int						m_id;
	//! The address at which the socket is currently bound.
	amuleIPV4Address		m_addr;
	//! Proxy settings used by the socket ...
	const CProxyData*		m_proxy;
	//! Mutex needed due to the use of the UBT.
	wxMutex					m_mutex;
	//! The currently opened socket, if any.
	CDatagramSocketProxy*	m_socket;

	
	//! Storage struct used for queueing packets.
	struct UDPPack
	{
		//! The packet, which at this point is owned by CMuleUDPSocket.
		CPacket*	packet;
		//! The timestamp of when the packet was queued.
		uint32		time;
		//! Target IP address.
		uint32		IP;
		//! Target port.
		uint16		port;
	};
	
	//! The queue of packets waiting to be sent.
	std::list<UDPPack> m_queue;
};

#endif // CLIENTUDPSOCKET_H

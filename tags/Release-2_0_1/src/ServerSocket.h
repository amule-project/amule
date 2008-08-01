//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

//
// Client to Server communication
//

#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ServerSocket.h"
#endif

#include "Types.h"		// Needed for int8 and int32
#include "EMSocket.h"		// Needed for CEMSocket
#include "ServerConnect.h"

//------------------------------------------------------------------------------
// CServerSocketHandler
//------------------------------------------------------------------------------

#ifdef AMULE_DAEMON
#define SERVER_SOCK_HANDLER_BASE wxThread
#else
#define SERVER_SOCK_HANDLER_BASE wxEvtHandler
#endif

class CServerSocketHandler: public SERVER_SOCK_HANDLER_BASE
{
public:
	CServerSocketHandler(CServerSocket *socket = NULL);

public:
#ifdef AMULE_DAEMON
	void *Entry();
	CServerSocket *m_socket;
#else
private:
	void ServerSocketHandler(wxSocketEvent& event);
	DECLARE_EVENT_TABLE()
#endif
};

//------------------------------------------------------------------------------
// CServerSocket
//------------------------------------------------------------------------------

class CServer;

class CServerSocket : public CEMSocket
{
	DECLARE_DYNAMIC_CLASS(CServerSocket)
	friend class CServerConnect;
	CServerSocket() {};
	
public:
	CServerSocket(CServerConnect* in_serverconnect, const CProxyData *ProxyData = NULL);
	virtual ~CServerSocket();

	void	ConnectToServer(CServer* server);
	sint8	GetConnectionState()		{ return connectionstate; } 
 	uint32  GetLastTransmission() const	{ return m_dwLastTransmission; }
	wxString info;

 public:
	void	OnClose(wxSocketError nErrorCode);
	void	OnConnect(wxSocketError nErrorCode);
	void	OnReceive(wxSocketError nErrorCode);
	void	OnError(wxSocketError nErrorCode);
	bool	PacketReceived(CPacket* packet);
	bool	SendPacket(CPacket* packet, bool delpacket = true,bool controlpacket = true);
	bool	IsSolving() const { return m_IsSolving;};
 	void	OnHostnameResolved(uint32 ip);
 	CServer *GetServerConnected() const { return serverconnect->GetCurrentServer(); }
	
#ifdef AMULE_DAEMON
	bool Connect(wxIPV4address &addr, bool wait);
#endif

private:
	bool	ProcessPacket(const char* packet, uint32 size, int8 opcode);
	void	SetConnectionState(sint8 newstate);
	CServerConnect*	serverconnect; 
	sint8	connectionstate;
	CServer*	cur_server;
	bool	headercomplete;
	int32	sizetoget;
	int32	sizereceived;
	char*	rbuffer;
	bool	m_bIsDeleting;	// true: socket is already in deletion phase, don't destroy it in ::StopConnectionTry
	uint32	m_dwLastTransmission;

	bool m_IsSolving;

    CServerSocketHandler* my_handler;
	
};

#endif // SERVERSOCKET_H
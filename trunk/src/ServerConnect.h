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

#ifndef SERVERCONNECT_H
#define SERVERCONNECT_H

#include <wx/defs.h>		// Needed before any other wx/*.h

#include "Types.h"		// Needed for int8, uint8, uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include "Timer.h"		// Needed for CTimer

#include <map>			// Needed for std::map

class CServerList;
class CServerSocket;
class CServer;
class CPacket;
class CServerUDPSocket;

#define CS_FATALERROR	-5
#define CS_DISCONNECTED	-4
#define CS_SERVERDEAD	-3
#define	CS_ERROR	-2
#define CS_SERVERFULL	-1
#define	CS_NOTCONNECTED	0
#define	CS_CONNECTING	1
#define	CS_CONNECTED	2
#define	CS_WAITFORLOGIN	3
#define CS_RETRYCONNECTTIME  30 // seconds

class CServerConnect {
public:
	CServerConnect(CServerList* in_serverlist, amuleIPV4Address &address);
	~CServerConnect();
	
	void	ConnectionFailed(CServerSocket* sender);
	void	ConnectionEstablished(CServerSocket* sender);
	
	void	ConnectToAnyServer(bool prioSort = true);
	void	ConnectToServer(CServer* toconnect, bool multiconnect = false);
	void	StopConnectionTry();
	void	CheckForTimeout();
	
	// safe socket closure and destruction
	void	DestroySocket(CServerSocket* pSck);
	bool	SendPacket(CPacket* packet,bool delpacket = true, CServerSocket* to = 0);

	// Creteil Begin
	bool	IsUDPSocketAvailable() const { return serverudpsocket != NULL; }
	// Creteil End

	bool	SendUDPPacket(CPacket* packet,CServer* host, bool delpacket = false );
	bool	Disconnect();
	bool	IsConnecting()	{ return connecting; }
	bool	IsConnected()	{ return connected; }
	uint32	GetClientID()	{ return clientid; }
	CServer*GetCurrentServer();
	uint32	clientid;
	uint8	pendingConnects;
	bool	IsLowID()	{ return ::IsLowID(clientid); }
	void	SetClientID(uint32 newid);
	bool	IsLocalServer(uint32 dwIP, uint16 nPort);
	void	TryAnotherConnectionrequest();
	bool	IsSingleConnect()	{ return singleconnecting; }
	void	KeepConnectionAlive();	

private:
	bool	connecting;
	bool	singleconnecting;
	bool	connected;
	int8	max_simcons;
	CServerSocket*	connectedsocket;
	CServerList*	used_list;
	CServerUDPSocket*	serverudpsocket;
	
	// list of currently opened sockets
	CTypedPtrList<CPtrList, CServerSocket*>	m_lstOpenSockets;
	CTimer	m_idRetryTimer;

	std::map<uint32, CServerSocket*> connectionattemps;
};

#endif // SERVERCONNECT_H

//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// Client to Server communication

#ifndef SOCKETS_H
#define SOCKETS_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/timer.h>		// Needed for wxTimer

#include "types.h"		// Needed for int8, uint8, uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "mfc.h"		// Needed for CMap

class CPreferences;
class CServerList;
class CServerSocket;
class CServer;
class Packet;
class CUDPSocket;

#define CS_FATALERROR	-5
#define CS_DISCONNECTED	-4
#define CS_SERVERDEAD	-3
#define	CS_ERROR		-2
#define CS_SERVERFULL	-1
#define	CS_NOTCONNECTED	0
#define	CS_CONNECTING	1
#define	CS_CONNECTED	2
#define	CS_WAITFORLOGIN	3
#define CS_RETRYCONNECTTIME  30 // seconds

class CServerConnect {
public:
	CServerConnect(CServerList* in_serverlist, CPreferences* in_prefs);
	~CServerConnect();
	void	ConnectionFailed(CServerSocket* sender);
	void	ConnectionEstablished(CServerSocket* sender);
	
	void	ConnectToAnyServer() {ConnectToAnyServer(0,true,true);}
	void	ConnectToAnyServer(uint32 startAt,bool prioSort=false,bool isAuto=true);
	void	ConnectToServer(CServer* toconnect, bool multiconnect=false);
	void	StopConnectionTry();
	//static  VOID CALLBACK RetryConnectCallback(HWND hWnd, unsigned int nMsg, unsigned int nId, DWORD dwTime);

	void	CheckForTimeout();
	void	DestroySocket(CServerSocket* pSck);	// safe socket closure and destruction
	bool	SendPacket(Packet* packet,bool delpacket = true, CServerSocket* to = 0);

	// Creteil Begin
	bool	IsUDPSocketAvailable() const { return udpsocket != NULL; }
	// Creteil End

	bool	SendUDPPacket(Packet* packet,CServer* host, bool delpacket = false );
	bool	Disconnect();
	bool	IsConnecting()	{return connecting;}
	bool	IsConnected()	{return connected;}
	uint32	GetClientID()		{return clientid;}
	CServer*	GetCurrentServer();
	uint32	clientid;
	uint8	pendingConnects;
	bool	IsLowID()		{return (clientid < 16777216);}
	void	SetClientID(uint32 newid);
	bool	IsLocalServer(uint32 dwIP, uint16 nPort);
	void	TryAnotherConnectionrequest();
	bool	IsSingleConnect()	{return singleconnecting;}
	void  KeepConnectionAlive();	
	void	InitLocalIP();
	uint32	GetLocalIP()		{return m_nLocalIP;}
	

private:
	bool	connecting;
	bool	singleconnecting;
	bool	connected;
	int8	max_simcons;
	uint32 lastStartAt;
	CPreferences*	app_prefs;
	CServerSocket*	connectedsocket;
	CServerList*	used_list;
	CUDPSocket*		udpsocket;
	CTypedPtrList<CPtrList,void*>		m_lstOpenSockets;	// list of currently opened sockets
	//unsigned int		m_idRetryTimer;
	wxTimer m_idRetryTimer;
	uint32	m_nLocalIP;

	CMap<DWORD ,DWORD&,CServerSocket*,CServerSocket*> connectionattemps;
};

#endif // SOCKETS_H

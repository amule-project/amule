//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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
// Handling incoming connections (up or downloadrequests)
//

#ifndef LISTENSOCKET_H
#define LISTENSOCKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ListenSocket.h"
#endif

#include "Types.h"		// Needed for uint8, uint16, uint32 and uint64
#include "EMSocket.h"		// Needed for CEMSocket
#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR
#include "amuleIPV4Address.h"
#include "Proxy.h"

#include <wx/dynarray.h>

#include <set> 
#include <list>

#ifdef AMULE_DAEMON
#define CLIENT_REQ_SOCK_HANDLER_BASE wxThread
#else
#define CLIENT_REQ_SOCK_HANDLER_BASE wxEvtHandler
#endif

//------------------------------------------------------------------------------
// CClientReqSocketHandler
//------------------------------------------------------------------------------

class CClientReqSocket;

class CClientReqSocketHandler: public CLIENT_REQ_SOCK_HANDLER_BASE
{
public:
	CClientReqSocketHandler(CClientReqSocket* socket = NULL);

#ifdef AMULE_DAEMON
public:
	// lfroen: for some reason wx can't Wait for detached threads
	~CClientReqSocketHandler();

private:
	CClientReqSocket* m_socket;
	void *Entry();
#else
private:
	void ClientReqSocketHandler(wxSocketEvent& event);
	DECLARE_EVENT_TABLE()
#endif
};

//------------------------------------------------------------------------------
// CClientReqSocket
//------------------------------------------------------------------------------

WX_DECLARE_OBJARRAY(wxString, ArrayOfwxStrings);

class CUpDownClient;
class CPacket;
class CTimerWnd;

enum LastActionType {
	ACTION_NONE,
	ACTION_CONNECT,
	ACTION_SEND,
	ACTION_RECEIVE
};

class CClientReqSocket : public CEMSocket
{
friend class CClientReqSocketHandler;

	DECLARE_DYNAMIC_CLASS(CClientReqSocket)

public:
	CClientReqSocket(CUpDownClient* in_client = 0, const CProxyData *ProxyData = NULL);	
	virtual ~CClientReqSocket();
	virtual	void 	OnInit();
	virtual	bool 	Close(); /*	{return wxSocketBase::Close();}*/
	virtual	bool 	Connect(amuleIPV4Address addr, bool wait);
	bool		Create();
	void		Disconnect(const wxString& strReason);

	void		ResetTimeOutTimer();
	bool		CheckTimeOut();
	void		Safe_Delete();

	bool		deletethis; // 0.30c (Creteil), set as bool

	void		OnConnect(int nErrorCode);
	void		OnSend(int nErrorCode);
	void		OnReceive(int nErrorCode);
	
	LastActionType	last_action;	
	void		RepeatLastAction();

	void		OnClose(int nErrorCode);
	void		OnError(int nErrorCode);
	
	uint32		timeout_timer;
	bool		hotrank;

	void		SetClient(CUpDownClient* client) { m_client = client; }
	CUpDownClient* GetClient() { return m_client; }
#ifdef AMULE_DAEMON
	void Destroy();
#endif
protected:
	bool	 PacketReceived(CPacket* packet);

private:
	CUpDownClient*	m_client;

//	void	Delete_Timed();
	bool	ProcessPacket(const char *packet, uint32 size, uint8 opcode);
	bool	ProcessExtPacket(const char *packet, uint32 size, uint8 opcode);
	bool	IsMessageFiltered(const wxString& Message, CUpDownClient* client);

	CClientReqSocketHandler* my_handler;
#ifdef AMULE_DAEMON
	wxMutex handler_exit;
#endif
};

#ifdef AMULE_DAEMON
class CSocketGlobalThread : public wxThread {
	void *Entry();
	
	std::set<CClientReqSocket *> socket_list;
public:
	CSocketGlobalThread(/*CListenSocket *socket*/);
	void AddSocket(CClientReqSocket* sock);
	void RemoveSocket(CClientReqSocket* sock);
};
#endif

// CListenSocket command target
class CListenSocket : public CSocketServerProxy
#ifdef AMULE_DAEMON
, public wxThread
#endif
{
#ifdef AMULE_DAEMON
	void *Entry();
	CSocketGlobalThread global_sock_thread;
#endif
public:
	CListenSocket(wxIPaddress &addr, const CProxyData *ProxyData = NULL);
	~CListenSocket();
	bool	StartListening();
	void	StopListening();
	void OnAccept(int nErrorCode);
	void	Process();
	void	RemoveSocket(CClientReqSocket* todel);
	void	AddSocket(CClientReqSocket* toadd);
	uint32	GetOpenSockets()		{return socket_list.size();}
	void	KillAllSockets();
	bool	TooManySockets(bool bIgnoreInterval = false);
	uint32	GetMaxConnectionReached()	{return maxconnectionreached;}
	bool    IsValidSocket(CClientReqSocket* totest);
	void	AddConnection();
	void	RecalculateStats();
	void	ReStartListening();
	void	UpdateConnectionsStatus();
	
	float	GetMaxConperFiveModifier();
	uint32	GetPeakConnections()		{ return peakconnections; }
	uint32	GetTotalConnectionChecks()	{ return totalconnectionchecks; }
	float	GetAverageConnections()		{ return averageconnections; }
	uint32	GetActiveConnections()		{ return activeconnections; }
	
	bool	OnShutdown() { return shutdown;}
	
private:
	
	typedef std::set<CClientReqSocket *> SocketSet;
	SocketSet socket_list;
	
	bool bListening;
	bool shutdown;
	
	uint16 m_OpenSocketsInterval;
	uint32 maxconnectionreached;
	uint16	m_ConnectionStates[3];
	uint16	m_nPeningConnections;
	uint32	peakconnections;
	uint32	totalconnectionchecks;
	float	averageconnections;
	uint32	activeconnections;
};


#endif // LISTENSOCKET_H

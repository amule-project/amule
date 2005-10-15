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
// Handling incoming connections (up or downloadrequests)
//

#ifndef LISTENSOCKET_H
#define LISTENSOCKET_H

#include "Types.h"		// Needed for uint8, uint16, uint32 and uint64
#include "EMSocket.h"		// Needed for CEMSocket
#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR
#include "amuleIPV4Address.h"
#include "Proxy.h"

#include <wx/dynarray.h>

#include <set> 
#include <list>

//------------------------------------------------------------------------------
// CClientReqSocketHandler
//------------------------------------------------------------------------------

class CClientReqSocket;

class CClientReqSocketHandler: public wxEvtHandler
{
public:
	CClientReqSocketHandler(CClientReqSocket* socket = NULL);

private:
	void ClientReqSocketHandler(wxSocketEvent& event);
	DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
// CClientReqSocket
//------------------------------------------------------------------------------

WX_DECLARE_OBJARRAY(wxString, ArrayOfwxStrings);

class CUpDownClient;
class CPacket;
class CTimerWnd;


class CClientReqSocket : public CEMSocket
{
	DECLARE_DYNAMIC_CLASS(CClientReqSocket)
	friend class CClientReqSocketHandler;
public:
	CClientReqSocket(CUpDownClient* in_client = 0, const CProxyData *ProxyData = NULL);	
	virtual ~CClientReqSocket();
	
	void		Disconnect(const wxString& strReason);

	void		ResetTimeOutTimer();
	bool		CheckTimeOut();

	void		Safe_Delete();

	bool		deletethis; // 0.30c (Creteil), set as bool

	void		OnConnect(int nErrorCode);
	void		OnSend(int nErrorCode);
	void		OnReceive(int nErrorCode);
	
	void		OnClose(int nErrorCode);
	void		OnError(int nErrorCode);
	
	uint32		timeout_timer;

	void		SetClient(CUpDownClient* client);
	CUpDownClient* GetClient() { return m_client; }
	
	virtual void SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0);
    virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend);
    virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend);

protected:
	virtual bool PacketReceived(CPacket* packet);

private:
	CUpDownClient*	m_client;

//	void	Delete_Timed();
	bool	ProcessPacket(const char *packet, uint32 size, uint8 opcode);
	bool	ProcessExtPacket(const char *packet, uint32 size, uint8 opcode);
	bool	IsMessageFiltered(const wxString& Message, CUpDownClient* client);

	CClientReqSocketHandler* my_handler;
};


// CListenSocket command target
class CListenSocket : public CSocketServerProxy
{
public:
	CListenSocket(wxIPaddress &addr, const CProxyData *ProxyData = NULL);
	~CListenSocket();
	bool	StartListening();
	void	StopListening();
	void	OnAccept(int nErrorCode);
	void	Process();
	void	RemoveSocket(CClientReqSocket* todel);
	void	AddSocket(CClientReqSocket* toadd);
	uint32	GetOpenSockets()		{return socket_list.size();}
	void	KillAllSockets();
	bool	TooManySockets(bool bIgnoreInterval = false);
	bool    IsValidSocket(CClientReqSocket* totest);
	void	AddConnection();
	void	RecalculateStats();
	void	ReStartListening();
	void	UpdateConnectionsStatus();
	
	float	GetMaxConperFiveModifier();
	uint32	GetTotalConnectionChecks()	{ return totalconnectionchecks; }
	float	GetAverageConnections()		{ return averageconnections; }
	
	bool	OnShutdown() { return shutdown;}
	
private:
	
	typedef std::set<CClientReqSocket *> SocketSet;
	SocketSet socket_list;
	
	bool bListening;
	bool shutdown;
	
	uint16	m_OpenSocketsInterval;
	uint16	m_ConnectionStates[3];
	uint16	m_nPeningConnections;
	uint32	totalconnectionchecks;
	float	averageconnections;
};


#endif // LISTENSOCKET_H

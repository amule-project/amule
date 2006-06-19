//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

//
// Handling incoming connections (up or downloadrequests)
//

#ifndef LISTENSOCKET_H
#define LISTENSOCKET_H

#include "Proxy.h"		// Needed fot CProxyData, CSocketServerProxy

#include <set> 

class CClientTCPSocket;

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
	void	RemoveSocket(CClientTCPSocket* todel);
	void	AddSocket(CClientTCPSocket* toadd);
	uint32	GetOpenSockets()		{return socket_list.size();}
	void	KillAllSockets();
	bool	TooManySockets(bool bIgnoreInterval = false);
	bool    IsValidSocket(CClientTCPSocket* totest);
	void	AddConnection();
	void	RecalculateStats();
	void	ReStartListening();
	void	UpdateConnectionsStatus();
	
	float	GetMaxConperFiveModifier();
	uint32	GetTotalConnectionChecks()	{ return totalconnectionchecks; }
	float	GetAverageConnections()		{ return averageconnections; }
	
	bool	OnShutdown() { return shutdown;}
	
private:
	
	typedef std::set<CClientTCPSocket *> SocketSet;
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
// File_checked_for_headers

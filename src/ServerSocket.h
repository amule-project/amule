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
// Client to Server communication
//

#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include "EMSocket.h"		// Needed for CEMSocket
#include "ServerConnect.h"

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

	void	ConnectToServer(CServer* server, bool bNoCrypt = false);
	sint8	GetConnectionState()	const	{ return connectionstate; } 
 	uint32  GetLastTransmission() const	{ return m_dwLastTransmission; }
	wxString info;

	void	OnClose(wxSocketError nErrorCode);
	void	OnConnect(wxSocketError nErrorCode);
	void	OnReceive(wxSocketError nErrorCode);
	void	OnError(wxSocketError nErrorCode);
	bool	PacketReceived(CPacket* packet);
	void 	SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0);
	bool	IsSolving() const { return m_IsSolving;};
 	void	OnHostnameResolved(uint32 ip);
 	CServer *GetServerConnected() const { return serverconnect->GetCurrentServer(); }
	
	uint32 GetServerIP() const;
	
private:
	bool	ProcessPacket(const byte* packet, uint32 size, int8 opcode);
	void	SetConnectionState(sint8 newstate);
	CServerConnect*	serverconnect; 
	sint8	connectionstate;
	CServer*	cur_server;
	bool m_bNoCrypt;
	bool	headercomplete;
	int32	sizetoget;
	int32	sizereceived;
	char*	rbuffer;
	bool	m_bIsDeleting;	// true: socket is already in deletion phase, don't destroy it in ::StopConnectionTry
	uint32	m_dwLastTransmission;

	bool m_IsSolving;

};

#endif // SERVERSOCKET_H
// File_checked_for_headers

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

#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include "types.h"		// Needed for int8 and int32
#include "EMSocket.h"		// Needed for CEMSocket
#include "CString.h"		// Needed for CString

class CServer;

#ifndef ID_SOKETTI
#define ID_SOKETTI 7772
#endif

class CServerSocket : public CEMSocket
{
  DECLARE_DYNAMIC_CLASS(CServerSocket)
    friend class CServerConnect;

    CServerSocket() {};
public:
	CServerSocket(CServerConnect* in_serverconnect);
	~CServerSocket();

	void	ConnectToServer(CServer* server);
	sint8	GetConnectionState()	{return connectionstate;} 
 	DWORD   GetLastTransmission() const { return m_dwLastTransmission; }
	CString info;

 public:
	void	OnClose(int nErrorCode);
	void	OnConnect(int nErrorCode);
	void	OnReceive(int nErrorCode);
	void	OnError(int nErrorCode);
	void	PacketReceived(Packet* packet);
	bool   SendPacket(Packet* packet, bool delpacket = true,bool controlpacket = true);
private:
	bool	ProcessPacket(char* packet, uint32 size, int8 opcode);
	void	SetConnectionState(sint8 newstate);
	CServerConnect*	serverconnect; 
	sint8	connectionstate;
	CServer*	cur_server;
	bool	headercomplete;
	int32	sizetoget;
	int32	sizereceived;
	char*	rbuffer;
	bool	m_bIsDeleting;	// true: socket is already in deletion phase, don't destroy it in ::StopConnectionTry
    DWORD	m_dwLastTransmission;
};

#endif // SERVERSOCKET_H

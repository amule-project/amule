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

#ifndef CLIENTTCPSOCKET_H
#define CLIENTTCPSOCKET_H

#include "EMSocket.h"		// Needed for CEMSocket

class CProxyData;

//------------------------------------------------------------------------------
// CClientTCPSocket
//------------------------------------------------------------------------------

class CUpDownClient;
class CPacket;
class CTimerWnd;

class CClientTCPSocket : public CEMSocket
{
	DECLARE_DYNAMIC_CLASS(CClientTCPSocket)
public:
	CClientTCPSocket(CUpDownClient* in_client = NULL, const CProxyData *ProxyData = NULL);	
	virtual ~CClientTCPSocket();
	
	void		Disconnect(const wxString& strReason);

	bool		InitNetworkData();

	bool		CheckTimeOut();

	void		Safe_Delete();

	bool		ForDeletion() const { return m_ForDeletion; }

	void		OnConnect(int nErrorCode);
	void		OnSend(int nErrorCode);
	void		OnReceive(int nErrorCode);
	
	void		OnClose(int nErrorCode);
	void		OnError(int nErrorCode);
	
	uint32		GetRemoteIP() const { return m_remoteip; }
		
	CUpDownClient* GetClient() { return m_client; }
	
	virtual void SendPacket(CPacket* packet, bool delpacket = true, bool controlpacket = true, uint32 actualPayloadSize = 0);
    virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend);
    virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend);

protected:
	virtual bool PacketReceived(CPacket* packet);

private:
	CUpDownClient*	m_client;

	bool	ProcessPacket(const byte* packet, uint32 size, uint8 opcode);
	bool	ProcessExtPacket(const byte* packet, uint32 size, uint8 opcode);
	bool	ProcessED2Kv2Packet(const byte* packet, uint32 size, uint8 opcode);
	bool	IsMessageFiltered(const wxString& Message, CUpDownClient* client);
	void	ResetTimeOutTimer();
	void	SetClient(CUpDownClient* client);

	bool	m_ForDeletion; // 0.30c (Creteil), set as bool
	uint32	timeout_timer;
	uint32	m_remoteip;
};

#endif // CLIENTTCPSOCKET_H
// File_checked_for_headers

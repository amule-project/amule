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

#ifndef CLIENTUDPSOCKET_H
#define CLIENTUDPSOCKET_H

#include <wx/thread.h>

#include "Types.h"		// Needed for uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "Proxy.h"		// Needed for CDatagramSocketProxy and amuleIPV4Address
#include "ThrottledSocket.h"


class CPacket;

// CClientUDPSocket command target

class CClientUDPSocket : public CDatagramSocketProxy, public ThrottledControlSocket
{
	DECLARE_DYNAMIC_CLASS(CClientUDPSocket)
	CClientUDPSocket() : CDatagramSocketProxy(useless2) {};
	
public:
	CClientUDPSocket(amuleIPV4Address &address, const CProxyData *ProxyData = NULL);
	virtual ~CClientUDPSocket();
	bool	SendPacket(CPacket* packet, uint32 dwIP, uint16 nPort);
    SocketSentBytes  SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize); // ZZ:UploadBandWithThrottler (UDP)
	bool	IsBusy()	{return m_bWouldBlock;}

protected:
	bool	ProcessPacket(byte* packet, int16 size, int8 opcode, uint32 host, uint16 port);
	
public:
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);
	
private:
	bool	SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort);
	bool	m_bWouldBlock;

	struct UDPPack
	{
		CPacket*	packet;
		uint32		dwTime;
		uint32		dwIP;
		uint16		nPort;
	};
	
	CList<UDPPack> controlpacket_queue;
	amuleIPV4Address  useless2;

	wxMutex m_sendLocker;

};

#endif // CLIENTUDPSOCKET_H

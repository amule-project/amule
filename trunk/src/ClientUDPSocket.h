//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef CLIENTUDPSOCKET_H
#define CLIENTUDPSOCKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ClientUDPSocket.h"
#endif

#include "types.h"		// Needed for uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "Proxy.h"		// Needed for wxDatagramSocketProxy and amuleIPV4Address

class Packet;

#pragma pack(1)
struct UDPPack {
	Packet* packet;
	uint32 dwIP;
	uint16 nPort;
	int    trial;
};
#pragma pack()

// CClientUDPSocket command target

class CClientUDPSocket : public wxDatagramSocketProxy
#ifdef AMULE_DAEMON
, public wxThread
#endif
{
	DECLARE_DYNAMIC_CLASS(CClientUDPSocket)
	CClientUDPSocket() : wxDatagramSocketProxy(useless2) {};
	
public:
	CClientUDPSocket(amuleIPV4Address &address, const wxProxyData *ProxyData = NULL);
	virtual ~CClientUDPSocket();
	bool	SendPacket(Packet* packet, uint32 dwIP, uint16 nPort);
	bool	IsBusy()	{return m_bWouldBlock;}

protected:
	bool	ProcessPacket(char* packet, int16 size, int8 opcode, uint32 host, uint16 port);
	
public:
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);
	void 	ReceiveAndDiscard();
	
private:
	void	ClearQueues();	
	bool	SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort);
	bool	m_bWouldBlock;

	CTypedPtrList<CPtrList, UDPPack*> controlpacket_queue;
	amuleIPV4Address  useless2;
#ifdef AMULE_DAEMON
	void *Entry();
#endif
};

#endif // CLIENTUDPSOCKET_H

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

#ifndef CLIENTUDPSOCKET_H
#define CLIENTUDPSOCKET_H

#include <wx/socket.h>		// Needed for wxDatagramSocket and wxIPV4address

#include "types.h"		// Needed for uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

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

class CClientUDPSocket : public wxDatagramSocket
{
  DECLARE_DYNAMIC_CLASS(CClientUDPSocket)
    CClientUDPSocket() : wxDatagramSocket(useless2) {};
public:
	CClientUDPSocket(wxIPV4address address);
	virtual ~CClientUDPSocket();
	bool	SendPacket(Packet* packet, uint32 dwIP, uint16 nPort);
	bool	IsBusy()	{return m_bWouldBlock;}
	bool	Create();
protected:
	bool	ProcessPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port);
	
 public:
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);
private:
	void	ClearQueues();	
	bool	SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort);
	bool	m_bWouldBlock;

	CTypedPtrList<CPtrList, UDPPack*> controlpacket_queue;
	wxIPV4address  useless2;
};

#endif // CLIENTUDPSOCKET_H

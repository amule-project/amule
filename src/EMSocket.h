// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef EMSOCKET_H
#define EMSOCKET_H

#include <wx/socket.h>		// Needed for wxSocketClient

#include "types.h"		// Needed for uint8 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

class Packet;

#define ERR_WRONGHEADER		0x01
#define ERR_TOOBIG		0x02

#define	ES_DISCONNECTED		0xFF
#define	ES_NOTCONNECTED		0x00
#define	ES_CONNECTED		0x01

#define PACKET_HEADER_SIZE	6

//WX_DECLARE_LIST(Packet,PacketListL);

class CEMSocket :
	public wxSocketClient
{
  DECLARE_DYNAMIC_CLASS(CEMSocket)
    
public:
	CEMSocket(void);
	~CEMSocket(void);
	bool	SendPacket(Packet* packet, bool delpacket = true,bool controlpacket = true);// controlpackets have a higher priority
	bool	IsBusy()	{return sendbuffer;}
	bool	IsConnected() { return byConnected==ES_CONNECTED;};
	uint8	GetConState()	{return byConnected;}
	void	SetDownloadLimit(uint32 limit);
	void	DisableDownloadLimit();
	bool	AsyncSelect(long lEvent);
	//protected:
 public:
	virtual void	PacketReceived(Packet* packet)		{}
	virtual void	OnError(int nErrorCode)				{}
	virtual void	OnClose(int nErrorCode);
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);
	uint8	byConnected;

	private:
	void	ClearQueues();	
	int		Send(char* lpBuf,int nBufLen,int nFlags = 0);

	uint32	downloadlimit;
	bool	limitenabled;
	bool	pendingOnReceive;

	// Download partial header
	char*	pendingHeader[PACKET_HEADER_SIZE];      
	uint32	pendingHeaderSize;

	// Download partial packet
	Packet* pendingPacket;
	uint32  pendingPacketSize;

/*  Removed since 0.30d import of cpu-less download
	char*	readbuf;
	uint32  readbuf_size;
	char	header[6];
*/

	char*	sendbuffer;
	uint32	sendblen;
	uint32	sent;
	bool	m_bLinkedPackets;

	CList<Packet*, Packet*> controlpacket_queue;
	CList<Packet*, Packet*> standartpacket_queue;
};

#endif // EMSOCKET_H

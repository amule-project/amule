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

#ifndef EMSOCKET_H
#define EMSOCKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "EMSocket.h"
#endif

#include "Proxy.h"		// Needed for CSocketClientProxy
#include <wx/event.h>

#include "Types.h"		// Needed for uint8 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

#ifdef __DEBUG__
	#include "amule.h"
#endif

class CPacket;

#define ERR_WRONGHEADER		0x01
#define ERR_TOOBIG		0x02

#define	ES_DISCONNECTED		0xFF
#define	ES_NOTCONNECTED		0x00
#define	ES_CONNECTED		0x01

#define PACKET_HEADER_SIZE	6

class CEMSocket : public CSocketClientProxy
{
  DECLARE_DYNAMIC_CLASS(CEMSocket)
    
public:
	CEMSocket(const CProxyData *ProxyData = NULL);
	virtual ~CEMSocket(void);
	bool	SendPacket(CPacket* packet, bool delpacket = true,bool controlpacket = true);// controlpackets have a higher priority
	bool	IsBusy()	{return sendbuffer;}
	bool	IsConnected() { return byConnected==ES_CONNECTED;};
	uint8	GetConState()	{return byConnected;}
	void	SetDownloadLimit(uint32 limit);
	void	DisableDownloadLimit();
	void	Destroy();
	bool OnDestroy() { return DoingDestroy; };
	//protected:
	// this functions are public on our code because of the amuleDlg::socketHandler
	virtual void	OnError(int WXUNUSED(nErrorCode)) { };
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);
	
 protected:

	virtual bool	PacketReceived(CPacket* WXUNUSED(packet)) { return false; };

	virtual void	OnClose(int nErrorCode);
	uint8	byConnected;

	bool RecievePending() { return (limitenabled && (downloadlimit == 0)); }
private:
	void	ClearQueues();	
	int		Send(char* lpBuf,int nBufLen,int nFlags = 0);

	uint32	downloadlimit;
	bool	limitenabled;
	bool	pendingOnReceive;

	// Download partial header
	char	pendingHeader[PACKET_HEADER_SIZE];      
	uint32	pendingHeaderSize;

	// Download partial packet
	CPacket* pendingPacket;
	uint32  pendingPacketSize;

	char*	sendbuffer;
	uint32	sendblen;
	uint32	sent;
	bool	m_bLinkedPackets;
	bool		DoingDestroy;  	
	
	CList<CPacket*, CPacket*> controlpacket_queue;
	CList<CPacket*, CPacket*> standartpacket_queue;
	
};

#endif // EMSOCKET_H

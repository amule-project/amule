//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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

#ifndef SERVERUDPSOCKET_H
#define SERVERUDPSOCKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "UDPSocket.h"
#endif

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
	#include <netinet/in.h>	// Needed for struct sockaddr_in
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h

#include "Types.h"		// Needed for uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "Proxy.h"		// Needed for CDatagramSocketProxy and amuleIPV4Address

class CPacket;
class CServer;
class CSafeMemFile;

#define WM_DNSLOOKUPDONE WM_USER+280

struct ServerUDPPacket {
	CPacket*	packet;
	CServer*	server;
};

// Client to Server communication

class CServerUDPSocket : public CDatagramSocketProxy
#ifdef AMULE_DAEMON
, wxThread
#endif
{
	friend class CServerConnect;

public:
	CServerUDPSocket(
		CServerConnect* in_serverconnect,
		amuleIPV4Address &addr,
		const CProxyData *ProxyData = NULL);
	~CServerUDPSocket();

	void	SendPacket(CPacket* packet,CServer* host);
	void	OnHostnameResolved(uint32 ip);

	virtual void OnReceive(int nErrorCode);
 	int DoReceive(amuleIPV4Address& addr, char* buffer, uint32 max_size);

private:

	void	SendBuffer();
	void	ProcessPacket(CSafeMemFile& packet, int16 size, int8 opcode, const wxString& host, uint16 port);

	amuleIPV4Address m_SaveAddr;

	CServerConnect*	serverconnect;
	char*	sendbuffer;
	uint32	sendblen;
	CServer* cur_server;
	CTypedPtrList<CPtrList, ServerUDPPacket*> server_packet_queue;
#ifdef AMULE_DAEMON
	void *Entry();
#endif
};

#endif // SERVERUDPSOCKET_H

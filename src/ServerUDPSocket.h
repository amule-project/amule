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

#ifndef SERVERUDPSOCKET_H
#define SERVERUDPSOCKET_H

#include "Types.h"				// Needed for uint16 and uint32
#include "Proxy.h"				// Needed for CDatagramSocketProxy and amuleIPV4Address

#include <list>


class CPacket;
class CServer;
class CMemFile;
class CServerConnect;


class CServerUDPSocket : public CDatagramSocketProxy
{
	friend class CServerConnect;

public:
	CServerUDPSocket(amuleIPV4Address& addr, const CProxyData* ProxyData = NULL);
	~CServerUDPSocket();

	void	SendPacket(CPacket* packet, CServer* host, bool delPacket);
	void	OnHostnameResolved(uint32 ip);

	virtual void OnReceive(int nErrorCode);

private:
	void	SendBuffer();
	void	ProcessPacket(CMemFile& packet, int16 size, int8 opcode, const wxString& host, uint16 port);
 	int		DoReceive(amuleIPV4Address& addr, char* buffer, uint32 max_size);

	struct ServerUDPPacket {
		CPacket*	packet;
		uint32		ip;
		uint16		port;
		wxString	addr;
	};

	typedef std::list<ServerUDPPacket> PacketList;
	PacketList m_queue;
};

#endif // SERVERUDPSOCKET_H

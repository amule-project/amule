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

#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
	#include <netinet/in.h>	// Needed for struct sockaddr_in
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/socket.h>		// Needed for wxDatagramSocket

#include "types.h"		// Needed for uint16 and uint32

class Packet;
class CServer;

#define WM_DNSLOOKUPDONE WM_USER+280

// Client to Server communication

#if 0
class CUDPSocketWnd : public CWnd {
  
// Construction
public:
	CUDPSocketWnd();
	CUDPSocket* m_pOwner;
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnDNSLookupDone(WPARAM wParam,LPARAM lParam);
private:
	
};
#endif

class CUDPSocket : public wxDatagramSocket //CAsyncSocket
{
	friend class CServerConnect;
	DECLARE_DYNAMIC_CLASS(CUDPSocket);

	CUDPSocket():wxDatagramSocket(useless) {};
public:
	CUDPSocket(CServerConnect* in_serverconnect,wxIPV4address& addr);
	~CUDPSocket();
	//bool	Create();
	void	SendPacket(Packet* packet,CServer* host);
	void DnsLookupDone(struct sockaddr_in* addr);
	//void	DnsLookupDone(WPARAM wp, LPARAM lp);
protected:
	void	AsyncResolveDNS(LPCTSTR lpszHostAddress, unsigned int nHostPort);
	//HANDLE	DnsTaskHandle;					// dns lookup handle
	
 public:
	virtual void OnReceive(int nErrorCode);
private:
	LPCTSTR m_lpszHostAddress;
	unsigned int m_nHostPort;
	//HWND m_hWndResolveMessage;	// where to send WM_DNSRESOLVED
	//SOCKADDR_IN m_SaveAddr;
	struct sockaddr_in m_SaveAddr;
	//CUDPSocketWnd m_udpwnd;

	void SendBuffer();
	bool	ProcessPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port);
	bool	ProcessExtPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port);
	CServerConnect*	serverconnect;
	char*	sendbuffer;
	uint32	sendblen;
	CServer* cur_server;
	char	DnsHostBuffer[1024]; //MAXGETHOSTSTRUCT];	// dns lookup structure
	wxIPV4address useless;
};

#endif // UDPSOCKET_H

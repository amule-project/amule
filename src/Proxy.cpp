/*
 * This file is part of the aMule project.
 *
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/******************************************************************************/

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "Proxy.h"
#endif

#include "Proxy.h"		/* for Interface		*/


#include <netinet/in.h>		/* for htons()			*/


#include "opcodes.h"		/* for PROXY_SOCKET_HANDLER	*/
#include "NetworkFunctions.h"	/* for CStringIPtoUint32()	*/
#include "otherfunctions.h"	/* for EncodeBase64()		*/


/******************************************************************************/

BEGIN_EVENT_TABLE(wxProxyEventHandler, wxEvtHandler)
	EVT_SOCKET(PROXY_SOCKET_HANDLER, wxProxyEventHandler::m_ProxySocketHandler)
END_EVENT_TABLE()

wxProxyEventHandler::wxProxyEventHandler(wxSocketProxy *WXUNUSED(parent))
{
}

void wxProxyEventHandler::m_ProxySocketHandler(wxSocketEvent& event)
{
	switch(event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		break;
	case wxSOCKET_LOST:
		break;
	case wxSOCKET_CONNECTION:
		break;
	default:
		break;
	}
}

/******************************************************************************/

wxProxyData::wxProxyData()
{
	Empty();
}

wxProxyData::wxProxyData(
	bool		ProxyEnable,
	wxProxyType	ProxyType,
	const wxString	&ProxyHostName,
	unsigned short	ProxyPort,
	bool		EnablePassword,
	const wxString	&UserName,
	const wxString	&Password)
{
	m_ProxyEnable	= ProxyEnable;
	m_ProxyType	= ProxyType;
	m_ProxyHostName	= ProxyHostName;
	m_ProxyPort	= ProxyPort;
	/* This flag is currently not used. The first authentication method 
	 * tryed is No-Authentication, the second is Username/Password. If 
	 * there is no username/password in wxProxyData, a NULL 
	 * username/password is sent. That will probably lead to a failure,
	 * but at least we tryed. Maybe this behaviour could be altered later.
	 */
	m_EnablePassword= EnablePassword;
	m_UserName	= UserName;
	m_Password	= Password;
}

void wxProxyData::Empty()
{
	m_ProxyEnable = false;
	m_ProxyHostName.Clear();
	m_ProxyPort = 0;
	m_ProxyType = wxPROXY_NONE;
	m_EnablePassword = false;
	m_UserName.Clear();
	m_Password.Clear();
}

/******************************************************************************/

wxSocketProxy::wxSocketProxy(const wxProxyData *ProxyData)
{
	SetProxyData(ProxyData);
}

void wxSocketProxy::SetProxyData(const wxProxyData *ProxyData)
{
	if (ProxyData) {
		m_ProxyData = *ProxyData;
		m_ProxyAddress.Hostname(m_ProxyData.m_ProxyHostName);
		m_ProxyAddress.Service(m_ProxyData.m_ProxyPort);
	} else {
		m_ProxyData.Empty();
	}
}

bool wxSocketProxy::Start(wxIPaddress &address, enum wxProxyCommand cmd, wxSocketClient *socket)
{
	bool ok = false;

	m_ProxyBoundAddress = NULL;
printf("wxSocketProxy::Start\nHostname Orig:%s, IPAddr:%s, Port:%d\n",
unicode2char(address.Hostname()),
unicode2char(address.IPAddress()),
address.Service());
	m_ProxyClientSocket = socket;
	m_ProxyClientSocket->SaveState();
#if 0
	m_ProxyClientSocket->SetEventHandler(
		wxProxyEventHandler, PROXY_SOCKET_HANDLER);
	m_ProxyClientSocket->SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	m_ProxyClientSocket->Notify(true);
#endif
	m_ProxyClientSocket->Notify(false);
	m_ProxyClientSocket->Connect(m_ProxyAddress, false);
	if (m_ProxyClientSocket->WaitOnConnect(10,0) )
	{
		if (m_ProxyClientSocket->IsConnected()) {
			// Prepare for transmition
			m_ProxyClientSocket->SetFlags(wxSOCKET_WAITALL);

			/* Call the proxy stuff routine */
			switch(m_ProxyData.m_ProxyType)
			{
			case wxPROXY_NONE:
				ok = false;
				break;
				
			case wxPROXY_SOCKS4:
				ok = DoSocks4(address, cmd);
				break;
				
			case wxPROXY_SOCKS5:
				ok = DoSocks5(address, cmd);
				break;
				
			case wxPROXY_HTTP:
				ok = DoHttp(address, cmd);
				break;
			}
		}
	}
	m_ProxyClientSocket->RestoreState();
if (m_ProxyBoundAddress) {
printf("Proxy Bound Address: IP:%s, Port:%u, ok:%d\n",
unicode2char(GetProxyBoundAddress().IPAddress()),
GetProxyBoundAddress().Service(), ok);
} else {
printf("Failed to bind proxy address, ok=%d\n", ok);
}
	
	return ok;
}

bool wxSocketProxy::DoSocks4(wxIPaddress &address, wxProxyCommand cmd)
{
	bool ok =
		DoSocks4Request(address, cmd) &&
		DoSocks4Reply();
	if (ok) {
		switch(cmd)
		{
		case wxPROXY_CMD_CONNECT:
			ok = DoSocks4CmdConnect();
			break;
		
		case wxPROXY_CMD_BIND:
			ok = DoSocks4CmdBind();
			break;
		
		case wxPROXY_CMD_UDP_ASSOCIATE:
			// No UDP in SOCKS4
			ok = false;
			break;
		}
	}
	return ok;
}

bool wxSocketProxy::DoSocks4Request(wxIPaddress &address, wxProxyCommand cmd)
{
	// Prepare the request command buffer
	m_buffer[0] = SOCKS4_VERSION;
	switch (cmd) {
	case wxPROXY_CMD_CONNECT:
		m_buffer[1] = SOCKS4_CMD_CONNECT;
		break;
		
	case wxPROXY_CMD_BIND:
		m_buffer[1] = SOCKS4_CMD_BIND;
		break;
		
	case wxPROXY_CMD_UDP_ASSOCIATE:
		/* Not supported, cannot happen */
		return false;
	}
	*((uint16 *)(m_buffer+2)) = htons(address.Service());
	*((uint32 *)(m_buffer+4)) = StringIPtoUint32(address.IPAddress());
	unsigned int OffsetUser = 8;
	unsigned char LenUser = m_ProxyData.m_UserName.Len();
	memcpy(m_buffer+OffsetUser, unicode2char(m_ProxyData.m_UserName),
		LenUser);
	unsigned int LenPacket = 1 + 1 + 2 + 4 + LenUser + 1 ;
	
	// Send the command packet
	m_ProxyClientSocket->Write(m_buffer, LenPacket);

	// Check the if the write operation succeded
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket;

	return ok;
}

bool wxSocketProxy::DoSocks4Reply(void)
{
	// Receive the server's reply
	unsigned int LenPacket = 8;
	m_ProxyClientSocket->Read(m_buffer, LenPacket);
	m_LastReply = m_buffer[1];

	// Process the server's reply
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket &&
		m_buffer[0] == SOCKS4_VERSION &&
		m_buffer[1] == SOCKS4_REPLY_GRANTED;
	if (ok) {
		// Read BND.PORT
		const unsigned int Port_offset = 2;
		m_ProxyBoundAddressIPV4.Service(ntohs(
			*((uint16 *)(m_buffer+Port_offset)) ));
		// Read BND.ADDR
		const unsigned int Addr_offset = 4;
		m_ProxyBoundAddressIPV4.Hostname(Uint32toStringIP(
			*((uint32 *)(m_buffer+Addr_offset)) ));
		m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
	}

	return ok;
}

bool wxSocketProxy::DoSocks4CmdConnect(void)
{
	// Nothing to do here.
	
	return true;
}

bool wxSocketProxy::DoSocks4CmdBind(void)
{
	// TODO
	bool ok = false;
	
	return ok;
}

bool wxSocketProxy::DoSocks5(wxIPaddress &address, wxProxyCommand cmd)
{
	// Use the short circuit evaluation
	bool ok = 
		DoSocks5Authentication() && 
		DoSocks5Request(address, cmd) &&
		DoSocks5Reply();
	if (ok) {
		switch(cmd)
		{
		case wxPROXY_CMD_CONNECT:
			ok = DoSocks5CmdConnect();
			break;
		
		case wxPROXY_CMD_BIND:
			ok = DoSocks5CmdBind();
			break;
		
		case wxPROXY_CMD_UDP_ASSOCIATE:
			ok = DoSocks5CmdUDPAssociate();
			break;
		}
	}

	return ok;
}

bool wxSocketProxy::DoSocks5Authentication(void)
{
	// Prepare the authentication method negotiation packet
	m_buffer[0] = SOCKS5_VERSION;
	m_buffer[1] = 2; // Number of supported methods
	//m_buffer[1] = 3; // Number of supported methods
	m_buffer[2] = SOCKS5_AUTH_METHOD_NO_AUTH_REQUIRED;
	m_buffer[3] = SOCKS5_AUTH_METHOD_USERNAME_PASSWORD;
	m_buffer[4] = SOCKS5_AUTH_METHOD_GSSAPI;
	unsigned int LenPacket = 4;
	//unsigned int LenPacket = 5;

	// Send the authentication method negotiation packet
	m_ProxyClientSocket->Write(m_buffer, LenPacket);	
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket;
	if (ok) {
		// Receive the method selection message
		LenPacket = 2;
		m_ProxyClientSocket->Read(m_buffer, LenPacket);
		m_LastReply = m_buffer[1];
		ok =	!m_ProxyClientSocket->Error() &&
			m_ProxyClientSocket->LastCount() == LenPacket &&
			m_buffer[0] == SOCKS5_VERSION;
		if (ok) {
			// Process the result
			switch (m_LastReply)
			{
			case SOCKS5_AUTH_METHOD_NO_AUTH_REQUIRED:
				break;
			
			case SOCKS5_AUTH_METHOD_GSSAPI:
				ok = DoSocks5AuthenticationGSSAPI();
				break;
			
			case SOCKS5_AUTH_METHOD_USERNAME_PASSWORD:
				ok = DoSocks5AuthenticationUsernamePassword();
				break;
			
			case SOCKS5_AUTH_METHOD_NO_ACCEPTABLE_METHODS:
			default:
				ok = false;
				break;
			}
		}
	}
		
	return ok;
}

bool wxSocketProxy::DoSocks5AuthenticationGSSAPI(void)
{
	// TODO or not TODO? That is the question...
	return false;
}

bool wxSocketProxy::DoSocks5AuthenticationUsernamePassword(void)
{
	unsigned char LenUser = m_ProxyData.m_UserName.Len();
	unsigned char LenPassword = m_ProxyData.m_Password.Len();
	unsigned int LenPacket = 1 + 1 + LenUser + 1 + LenPassword;
	unsigned int OffsetUser = 2;
	unsigned int OffsetPassword = OffsetUser + LenUser + 1;
	
	// Prepare username/password buffer
	m_buffer[0] = SOCKS5_VERSION;
	m_buffer[OffsetUser-1] = LenUser;
	memcpy(m_buffer+OffsetUser, unicode2char(m_ProxyData.m_UserName),
		LenUser);
	m_buffer[OffsetPassword-1] = LenPassword;
	memcpy(m_buffer+OffsetPassword, unicode2char(m_ProxyData.m_Password),
		LenPassword);

	// Send the username/password packet
	m_ProxyClientSocket->Write(m_buffer, LenPacket);
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket;
	if (ok) {
		// Receive the server's authentication response
		LenPacket = 2;
		m_ProxyClientSocket->Read(m_buffer, LenPacket);
		m_LastReply = m_buffer[1];

		// Process the server's reply
		ok = 	!m_ProxyClientSocket->Error() &&
			m_ProxyClientSocket->LastCount() == LenPacket &&
			m_buffer[0] == SOCKS5_VERSION &&
			m_buffer[1] == SOCKS5_REPLY_SUCCEED;
	}
	
	return ok;
}

bool wxSocketProxy::DoSocks5Request(wxIPaddress &address, wxProxyCommand cmd)
{
	// Prepare the request command buffer
	m_buffer[0] = SOCKS5_VERSION;
	switch (cmd) {
	case wxPROXY_CMD_CONNECT:
		m_buffer[1] = SOCKS5_CMD_CONNECT;
		break;
		
	case wxPROXY_CMD_BIND:
		m_buffer[1] = SOCKS5_CMD_BIND;
		break;
		
	case wxPROXY_CMD_UDP_ASSOCIATE:
		m_buffer[1] = SOCKS5_CMD_UDP_ASSOCIATE;
		break;
	}
	m_buffer[2] = SOCKS5_RSV;
	m_buffer[3] = SOCKS5_ATYP_IPV4_ADDRESS;
	*((uint32 *)(m_buffer+4)) = StringIPtoUint32(address.IPAddress());
	*((uint16 *)(m_buffer+8)) = htons(address.Service());
	unsigned int LenPacket = 10;
	
	// Send the command packet
	m_ProxyClientSocket->Write(m_buffer, LenPacket);

	// Check the if the write operation succeded
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket;

	return ok;
}

bool wxSocketProxy::DoSocks5Reply(void)
{
	// Receive the server's reply -- read 4 bytes
	unsigned int LenPacket = 4;
	m_ProxyClientSocket->Read(m_buffer, LenPacket);
	m_LastReply = m_buffer[1];
	m_AddressType = m_buffer[3];

	// Process the server's reply
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket &&
		m_buffer[0] == SOCKS5_VERSION &&
		m_buffer[1] == SOCKS5_REPLY_SUCCEED;
	if (ok) {
		// Read BND.ADDR
		unsigned int Port_offset = 0;
		switch(m_AddressType) {
		case SOCKS5_ATYP_IPV4_ADDRESS:
		{
			const unsigned int Addr_offset = 4;
			Port_offset = 8;
			LenPacket = 4;
			m_ProxyClientSocket->Read(m_buffer+Addr_offset, LenPacket);
			ok = 	!m_ProxyClientSocket->Error() &&
				m_ProxyClientSocket->LastCount() == LenPacket;
			if (ok) {
				wxString strAddr = Uint32toStringIP(
					*((uint32 *)(m_buffer+Addr_offset)) );
				ok = m_ProxyBoundAddressIPV4.Hostname(strAddr);
				m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
			}
			break;
		}
		case SOCKS5_ATYP_DOMAINNAME:
		{
			// Read the size
			LenPacket = 1;
			m_ProxyClientSocket->Read(m_buffer+4, LenPacket);
			ok = 	!m_ProxyClientSocket->Error() &&
				m_ProxyClientSocket->LastCount() == LenPacket;
			if (ok) {
				LenPacket = m_buffer[4];
				const unsigned int Addr_offset = 5;
				Port_offset = Addr_offset + LenPacket;
				// Read the address
				m_ProxyClientSocket->Read(m_buffer+Addr_offset,
					LenPacket);
				ok =	!m_ProxyClientSocket->Error() &&
					m_ProxyClientSocket->LastCount() == LenPacket;
				if (ok) {
					m_buffer[Port_offset] = 0;
					m_ProxyBoundAddressIPV4.Hostname(
						char2unicode(m_buffer+Addr_offset));
					m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
				}
			}
			break;
		}
		case SOCKS5_ATYP_IPV6_ADDRESS:
		{
			const unsigned int Addr_offset = 4;
			Port_offset = 20;
			LenPacket = 16;
			m_ProxyClientSocket->Read(m_buffer+Addr_offset, LenPacket);
			ok =	!m_ProxyClientSocket->Error() &&
				m_ProxyClientSocket->LastCount() == LenPacket;
			// TODO
			// IPV6 not yet implemented in wx
			//m_ProxyBoundAddress.Hostname(Uint128toStringIP(
			//	*((uint128 *)(m_buffer+Addr_offset)) ));
			//m_ProxyBoundAddress = &m_ProxyBoundAddressIPV6;
			ok = false;
			break;
		}
		}
		if (ok) {
			// Read BND.PORT
			LenPacket = 2;
			m_ProxyClientSocket->Read(m_buffer+Port_offset, LenPacket);
			ok =	!m_ProxyClientSocket->Error() &&
				m_ProxyClientSocket->LastCount() == LenPacket &&
				m_ProxyBoundAddress->Service(ntohs(
					*((uint16 *)(m_buffer+Port_offset)) ));
		}
	}

	return ok;
}

bool wxSocketProxy::DoSocks5CmdConnect(void)
{
	// Nothing to do here.
	
	return true;
}

bool wxSocketProxy::DoSocks5CmdBind(void)
{
	// TODO
	bool ok = false;
	
	return ok;
}

bool wxSocketProxy::DoSocks5CmdUDPAssociate(void)
{
	// TODO
	bool ok = false;
	
	return ok;
}

bool wxSocketProxy::DoHttp(wxIPaddress &address, wxProxyCommand cmd)
{
	// Use the short circuit evaluation
	int ok = 
		DoHttpRequest(address, cmd) &&
		DoHttpReply();
	if (ok) {
		switch(cmd)
		{
		case wxPROXY_CMD_CONNECT:
			ok = DoHttpCmdConnect();
			break;
		
		case wxPROXY_CMD_BIND:
			ok = false;
			break;
		
		case wxPROXY_CMD_UDP_ASSOCIATE:
			ok = false;
			break;
		}
	}

	return ok;
}

bool wxSocketProxy::DoHttpRequest(wxIPaddress &address, wxProxyCommand cmd)
{
	// Prepare the request command buffer
	wxCharBuffer buf(unicode2charbuf(address.IPAddress()));
	const char *host = (const char *)buf;
	uint16 port = address.Service();
	wxString UserPass = m_ProxyData.m_UserName + wxT(":") + m_ProxyData.m_Password;
	wxString UserPassEncoded =
		otherfunctions::EncodeBase64(m_buffer, wxPROXY_BUFFER_SIZE);
	wxString msg;
	switch (cmd) {
	case wxPROXY_CMD_CONNECT:
		msg = wxString::Format(
			wxT(
			"CONNECT %s:%d HTTP/1.1\r\n"
			"Host: %s:%d\r\n"
			"Proxy-Authorization: Basic %s\r\n"),
			host, port, host, port, unicode2char(UserPassEncoded));
		break;
		
	case wxPROXY_CMD_BIND:
		/* This is not possible */
		return false;
		
	case wxPROXY_CMD_UDP_ASSOCIATE:
		/* This is not possible */
		return false;
	}
	
	// Send the command packet
	unsigned int LenPacket = msg.Len();
	memcpy(m_buffer, unicode2char(msg), LenPacket+1);
	m_ProxyClientSocket->Write(m_buffer, LenPacket);

	// Check the if the write operation succeded
	bool ok =
		!m_ProxyClientSocket->Error() &&
		m_ProxyClientSocket->LastCount() == LenPacket;

	return ok;
}

bool wxSocketProxy::DoHttpReply(void)
{
	// TODO
	
	return false;
}

bool wxSocketProxy::DoHttpCmdConnect(void)
{
	// Nothing to do here.
	
	return true;
}

/******************************************************************************/

wxSocketClientProxy::wxSocketClientProxy(
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
wxSocketClient(flags),
m_SocketProxy(ProxyData)
{
	m_UseProxy = ProxyData != NULL;
}

/*
 * Notice! These includes are here as long as it is impossible to retrieve 
 * the event handler from the socket. They should be removed. For now,
 * please leave it here.
 */
#include "ListenSocket.h"	// For CClientReqSocketHandler
#include "ServerSocket.h"	// For CServerSocketHandler

bool wxSocketClientProxy::Connect(wxIPaddress &address, bool wait)
{
	bool ok;
	
	if (m_UseProxy) {
		ok = m_SocketProxy.Start(address, wxPROXY_CMD_CONNECT, this);
#ifndef AMULE_DAEMON
		/* If proxy is beeing used, CServerSocketHandler will not receive a 
		 * wxSOCKET_CONNECTION event, because the connection has already 
		 * started with the proxy. So we must add a wxSOCKET_CONNECTION
		 * event to make things go undetected. A wxSOCKET_OUTPUT event is
		 * also necessary to start sending data to the server. */
		if(m_UseProxy) {
			wxSocketEvent e(SERVERSOCKET_HANDLER);
			e.m_event = wxSOCKET_CONNECTION;
			e.SetEventObject(this);
			CClientReqSocket *s1 = wxDynamicCastThis(CClientReqSocket);
			if (s1) {
				CClientReqSocketHandler *h = s1->GetEventHandler();
				h->AddPendingEvent(e);
				e.m_event = wxSOCKET_OUTPUT;
				h->AddPendingEvent(e);
				goto end;
			}
			CServerSocket *s2 = wxDynamicCastThis(CServerSocket);
			if (s2) {
				CServerSocketHandler *h = s2->GetEventHandler();
				h->AddPendingEvent(e);
				e.m_event = wxSOCKET_OUTPUT;
				h->AddPendingEvent(e);
				goto end;
			}
		}
#endif
	} else {
		ok = wxSocketClient::Connect(address, wait);
	}

#ifndef AMULE_DAEMON
end:
#endif
	return ok;
}

void wxSocketClientProxy::SetProxyData(const wxProxyData *ProxyData)
{
	m_UseProxy = ProxyData != NULL && ProxyData->m_ProxyEnable;
	m_SocketProxy.SetProxyData(ProxyData);
}

/******************************************************************************/

wxSocketServerProxy::wxSocketServerProxy(
	wxIPaddress &address,
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
wxSocketServer(address, flags),
m_SocketProxy(ProxyData)
{
	m_UseProxy = ProxyData != NULL && ProxyData->m_ProxyEnable;
	if (m_UseProxy) {
		/* Maybe some day when socks6 is out... :) */
	}
}

void wxSocketServerProxy::SetProxyData(const wxProxyData *ProxyData)
{
	m_UseProxy = ProxyData != NULL && ProxyData->m_ProxyEnable;
	m_SocketProxy.SetProxyData(ProxyData);
}

/******************************************************************************/

#if !wxCHECK_VERSION(2,5,3)
IMPLEMENT_ABSTRACT_CLASS(wxDatagramSocketProxy,wxDatagramSocket)
#endif

wxDatagramSocketProxy::wxDatagramSocketProxy(
	wxIPaddress &address, wxSocketFlags flags, const wxProxyData *ProxyData)
:
wxDatagramSocket(address, flags),
m_SocketProxy(ProxyData)
{
	m_UseProxy = ProxyData != NULL && ProxyData->m_ProxyEnable;
	bool ok;
	
	if (m_UseProxy) {
		// Create the TCP socket to talk to the proxy
		m_ProxySocket = new wxSocketClient(wxSOCKET_NOWAIT);
		ok = m_SocketProxy.Start(address, wxPROXY_CMD_UDP_ASSOCIATE, m_ProxySocket);
	} else {
	}
	m_LastUDPOperation = wxUDP_OPERATION_NONE;
}

wxDatagramSocketProxy::~wxDatagramSocketProxy()
{
	if (m_UseProxy) {
		// From RFC-1928:
		// "A UDP association terminates when the TCP connection that the
		// UDP ASSOCIATE request arrived terminates."
#ifndef AMULE_DAEMON
		m_ProxySocket->Destroy();
#else
		delete m_ProxySocket;
#endif
	} else {
	}
}

void wxDatagramSocketProxy::SetProxyData(const wxProxyData *ProxyData)
{
	m_UseProxy = ProxyData != NULL && ProxyData->m_ProxyEnable;
	m_SocketProxy.SetProxyData(ProxyData);
}

wxDatagramSocket &wxDatagramSocketProxy::RecvFrom(
	wxSockAddress &addr, void* buf, wxUint32 nBytes )
{
	m_LastUDPOperation = wxUDP_OPERATION_RECV_FROM;
	if (m_UseProxy) {
		if (m_UDPSocketOk) {
			char *bufUDP = new char[nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD];
			wxDatagramSocket::RecvFrom(m_SocketProxy.GetProxyBoundAddress(), bufUDP, nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD);
			unsigned int offset;
			
			switch (m_SocketProxy.m_buffer[3]) {
			case SOCKS5_ATYP_IPV4_ADDRESS:
				offset = wxPROXY_UDP_OVERHEAD_IPV4;
				break;
				
			case SOCKS5_ATYP_DOMAINNAME:
				offset = wxPROXY_UDP_OVERHEAD_DOMAIN_NAME;
				break;
				
			case SOCKS5_ATYP_IPV6_ADDRESS:
				offset = wxPROXY_UDP_OVERHEAD_IPV6;
				break;
				
			default:
				/* Error! */
				offset = 0;
				break;
			}
			memcpy(buf, bufUDP + offset, nBytes);
		}
	} else {
		wxDatagramSocket::RecvFrom(addr, buf, nBytes);
	}
	
	return *this;
}

wxDatagramSocket &wxDatagramSocketProxy::SendTo(
	wxIPaddress &addr, const void* buf, wxUint32 nBytes )
{
	m_LastUDPOperation = wxUDP_OPERATION_SEND_TO;
	m_LastUDPOverhead = wxPROXY_UDP_OVERHEAD_IPV4;
	if (m_UseProxy) {
		if (m_UDPSocketOk) {
			m_SocketProxy.m_buffer[0] = SOCKS5_RSV;	// Reserved
			m_SocketProxy.m_buffer[1] = SOCKS5_RSV;	// Reserved
			m_SocketProxy.m_buffer[2] = 0;		// FRAG
			m_SocketProxy.m_buffer[3] = SOCKS5_ATYP_IPV4_ADDRESS;
			*((uint32 *)(m_SocketProxy.m_buffer+4)) = StringIPtoUint32(addr.IPAddress());
			*((uint16 *)(m_SocketProxy.m_buffer+8)) = htons(addr.Service());
			memcpy(m_SocketProxy.m_buffer + wxPROXY_UDP_OVERHEAD_IPV4, buf, nBytes);
			nBytes += wxPROXY_UDP_OVERHEAD_IPV4;
			
			wxDatagramSocket::SendTo(m_SocketProxy.GetProxyBoundAddress(), m_SocketProxy.m_buffer, nBytes);
		}
	} else {
		wxDatagramSocket::SendTo(addr, buf, nBytes);
	}
	
	return *this;
}

wxUint32 wxDatagramSocketProxy::LastCount(void) const
{
	wxUint32 ret;

	if (m_UseProxy) {	
		switch (m_LastUDPOperation) {
		case wxUDP_OPERATION_RECV_FROM:
		case wxUDP_OPERATION_SEND_TO:
			ret = Ok() ? wxDatagramSocket::LastCount() - m_LastUDPOverhead : 0;
			break;
			
		case wxUDP_OPERATION_NONE:
		default:
			ret = 0;
			break;
		
		}
	} else {
		ret = wxDatagramSocket::LastCount();
	}
	
	return ret;
}

/******************************************************************************/

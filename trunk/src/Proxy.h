/*
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

#ifndef __PROXY_H__

#define __PROXY_H__


/******************************************************************************/


#include <wx/socket.h>		// For wxSocket*
#include <wx/string.h>		// For wxString

#include "amuleIPV4Address.h"	// For amuleIPV4address

/******************************************************************************/

/*
 * SOCKS4 protocol implementation according to:
 * - "SOCKS: A protocol for TCP proxy across firewalls":
 *   amule-root/docs/socks4.protocol
 */
const unsigned char SOCKS4_VERSION = 0x04;

const unsigned char SOCKS4_CMD_CONNECT	= 0x01;
const unsigned char SOCKS4_CMD_BIND	= 0x02;

const unsigned char SOCKS4_REPLY_GRANTED			= 90;
const unsigned char SOCKS4_REPLY_FAILED				= 91;
const unsigned char SOCKS4_REPLY_FAILED_NO_IDENTD		= 92;
const unsigned char SOCKS4_REPLY_FAILED_DIFFERENT_USERIDS	= 93;

/*
 * SOCKS5 protocol implementation according to:
 * - RFC-1928: SOCKS Protocol Version 5
 * - RFC-1929: Username/Password Authentication for SOCKS V5
 *
 * Also, for the future :) :
 * - RFC-1961: GSS-API Authentication Method for SOCKS Version 5
 * - RFC-1508: Generic Security Service Application Program Interface
 * - RFC-1509: Genecic Security Service API: C-bindings
 *   
 */

const unsigned char SOCKS5_VERSION = 0x05;

const unsigned char SOCKS5_AUTH_METHOD_NO_AUTH_REQUIRED		= 0x00;
const unsigned char SOCKS5_AUTH_METHOD_GSSAPI			= 0x01;
const unsigned char SOCKS5_AUTH_METHOD_USERNAME_PASSWORD	= 0x02;
const unsigned char SOCKS5_AUTH_METHOD_NO_ACCEPTABLE_METHODS	= 0xFF;

const unsigned char SOCKS5_CMD_CONNECT		= 0x00;
const unsigned char SOCKS5_CMD_BIND		= 0x01;
const unsigned char SOCKS5_CMD_UDP_ASSOCIATE	= 0x02;

const unsigned char SOCKS5_RSV = 0x00;

const unsigned char SOCKS5_ATYP_IPV4_ADDRESS	= 0x01;
const unsigned char SOCKS5_ATYP_DOMAINNAME	= 0x03;
const unsigned char SOCKS5_ATYP_IPV6_ADDRESS	= 0x04;

const unsigned char SOCKS5_REPLY_SUCCEED		= 0x00;
const unsigned char SOCKS5_REPLY_GENERAL_SERVER_FAILURE	= 0x01;
const unsigned char SOCKS5_REPLY_CONNECTION_NOT_ALLOWED	= 0x02;
const unsigned char SOCKS5_REPLY_NETWORK_UNREACHABLE	= 0x03;
const unsigned char SOCKS5_REPLY_HOST_UNREACHABLE	= 0x04;
const unsigned char SOCKS5_REPLY_CONNECTION_REFUSED	= 0x05;
const unsigned char SOCKS5_REPLY_TTL_EXPIRED		= 0x06;
const unsigned char SOCKS5_REPLY_COMMAND_NOT_SUPPORTED	= 0x07;
const unsigned char SOCKS5_REPLY_ATYP_NOT_SUPPORTED	= 0x08;

/******************************************************************************/

enum wxProxyType {
	wxPROXY_SOCKS4,
	wxPROXY_SOCKS5
};

enum wxProxyCommand {
	wxPROXY_CMD_CONNECT,
	wxPROXY_CMD_BIND,
	wxPROXY_CMD_UDP_ASSOCIATE
};

class wxProxyData
{
public:
	wxString ProxyHostName;
	unsigned short ProxyPort;
	wxProxyType ProxyType;
	wxString Username;
	wxString Password;
};

/******************************************************************************/

#if !wxCHECK_VERSION(2,5,1)
	#define wxIPaddress amuleIPV4Address
#endif

const unsigned int wxPROXY_BUFFER_SIZE = 1024;

class wxProxyEventHandler;
class wxSocketProxy
{
public:
	/* Constructor */
	wxSocketProxy(const wxProxyData *ProxyData);
	/* Destructor */
	~wxSocketProxy();
	
	/* Interface */
	void		SetProxyData(const wxProxyData *ProxyData);
	bool		Start(wxIPaddress &address, enum wxProxyCommand cmd);
	wxIPaddress	&GetTargetAddress(void) { return *m_TargetAddress; }
	unsigned char	GetLastReply(void) { return m_LastReply; }

private:
	/* SOCKS4 */
	bool DoSocks4(wxIPaddress &address, wxProxyCommand cmd);
	bool DoSocks4Request(wxIPaddress &address, unsigned char cmd);
	bool DoSocks4Reply(void);
	bool DoSocks4CmdConnect(void);
	bool DoSocks4CmdBind(void);
	
	/* SOCKS5 */
	bool DoSocks5(wxIPaddress &address, wxProxyCommand cmd);
	bool DoSocks5Authentication(void);
	bool DoSocks5AuthenticationUsernamePassword(void);
	bool DoSocks5AuthenticationGSSAPI(void);
	bool DoSocks5Request(wxIPaddress &address, unsigned char cmd);
	bool DoSocks5Reply(void);
	bool DoSocks5CmdConnect(void);
	bool DoSocks5CmdBind(void);
	bool DoSocks5CmdUDPAssociate(void);

	/* HTTP */
	bool DoHttp(wxIPaddress &address, wxProxyCommand cmd);
	bool DoHttpRequest(wxIPaddress &address, unsigned char cmd);
	bool DoHttpReply(void);
	bool DoHttpCmdConnect(void);
	
	wxProxyData		m_ProxyData;
	char			m_buffer[wxPROXY_BUFFER_SIZE];
	amuleIPV4Address	m_ProxyAddress;
	wxSocketClient		*m_ProxyClientSocket;
	wxIPaddress		*m_TargetAddress;
	amuleIPV4Address	m_TargetAddressIPV4;
	//wxIPV6address		m_TargetAddressIPV6;
	unsigned char		m_LastReply;
	unsigned char		m_AddressType;
};

/******************************************************************************/

class wxProxyEventHandler : public wxEvtHandler
{
public:
	wxProxyEventHandler(wxSocketProxy *parent);

private:
	void m_ProxySocketHandler(wxSocketEvent& event);
	wxSocketClient 	*m_ProxySocket;

	DECLARE_EVENT_TABLE();
};

/******************************************************************************/

class wxSocketClientProxy : public wxSocketClient
{
public:
	/* Constructor */
	wxSocketClientProxy(
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL);
		
	/* Interface */
	bool Connect(wxIPaddress &address, bool wait);
	void SetProxyData(const wxProxyData *ProxyData);
	
private:
	wxSocketProxy	m_SocketProxy;
	bool 		m_UseProxy;
};

/******************************************************************************/

class wxSocketServerProxy
{
public:
	/* Constructor */
	wxSocketServerProxy(
		wxIPaddress &address,
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL);
		
	/* Destructor */
	~wxSocketServerProxy();
		
	/* Interface */
	void SetProxyData(const wxProxyData *ProxyData);
	
	/* wxSocketServer interface */
	wxSocketBase *Accept(bool wait = true) { return m_SocketServer->Accept(wait); }
	bool AcceptWith(wxSocketBase& socket, bool wait = true) { return m_SocketServer->AcceptWith(socket, wait); }
	void Close(void) { m_SocketServer->Close(); }
	wxSocketBase& Discard(void) { return m_SocketServer->Discard(); }
	void Notify(bool notify) { m_SocketServer->Notify(notify); }
	bool Ok() const	{ return m_SocketServer->Ok(); }
	void SetEventHandler(wxEvtHandler& handler, int id = -1) { m_SocketServer->SetEventHandler(handler, id); }
	void SetNotify(wxSocketEventFlags flags) { m_SocketServer->SetNotify(flags); }
	bool WaitForAccept(long seconds = -1, long millisecond = 0) { return m_SocketServer->WaitForAccept(seconds, millisecond); }
	
private:
	wxSocketProxy	m_SocketProxy;
	bool 		m_UseProxy;
	wxSocketServer	*m_SocketServer;
};

/******************************************************************************/

#endif /* __PROXY_H__ */


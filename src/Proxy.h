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

#ifndef __PROXY_H__
#define __PROXY_H__


/******************************************************************************/

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Proxy.h"
#endif

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

const unsigned char SOCKS5_CMD_CONNECT		= 0x01;
const unsigned char SOCKS5_CMD_BIND		= 0x02;
const unsigned char SOCKS5_CMD_UDP_ASSOCIATE	= 0x03;

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

//
// These constants must match the integer values saved in the configuration file
//
enum wxProxyType {
	wxPROXY_NONE = -1,
	wxPROXY_SOCKS5 = 0,
	wxPROXY_SOCKS4 = 1,
	wxPROXY_HTTP = 2
};

enum wxProxyCommand {
	wxPROXY_CMD_CONNECT,
	wxPROXY_CMD_BIND,
	wxPROXY_CMD_UDP_ASSOCIATE
};

class wxProxyData
{
public:
	wxProxyData();
	wxProxyData(
		bool		ProxyEnable,
		wxProxyType	ProxyType,
		const wxString	&ProxyHostName,
		unsigned short	ProxyPort,
		bool		EnablePassword,
		const wxString	&UserName,
		const wxString	&Password
	);
	void Empty();

public:
	bool		m_ProxyEnable;
	wxString	m_ProxyHostName;
	unsigned short	m_ProxyPort;
	wxProxyType	m_ProxyType;
	bool		m_EnablePassword;
	wxString	m_UserName;
	wxString	m_Password;
};

/******************************************************************************/

#if !wxCHECK_VERSION(2,5,1)
	#define wxIPaddress wxIPV4address
#endif

const unsigned int wxPROXY_BUFFER_SIZE = 1024;

class wxProxyEventHandler;
class wxSocketProxy
{
public:
	/* Constructor */
	wxSocketProxy(const wxProxyData *ProxyData);
	
	/* Interface */
	void		SetProxyData(const wxProxyData *ProxyData);
	bool		Start(wxIPaddress &address, enum wxProxyCommand cmd, wxSocketClient *socket);
	wxIPaddress &GetProxyBoundAddress(void) const { return *m_ProxyBoundAddress; }
	unsigned char	GetLastReply(void) const { return m_LastReply; }

private:
	/* SOCKS4 */
	bool DoSocks4(wxIPaddress &address, wxProxyCommand cmd);
	bool DoSocks4Request(wxIPaddress &address, wxProxyCommand cmd);
	bool DoSocks4Reply(void);
	bool DoSocks4CmdConnect(void);
	bool DoSocks4CmdBind(void);
	
	/* SOCKS5 */
	bool DoSocks5(wxIPaddress &address, wxProxyCommand cmd);
	bool DoSocks5Authentication(void);
	bool DoSocks5AuthenticationUsernamePassword(void);
	bool DoSocks5AuthenticationGSSAPI(void);
	bool DoSocks5Request(wxIPaddress &address, wxProxyCommand cmd);
	bool DoSocks5Reply(void);
	bool DoSocks5CmdConnect(void);
	bool DoSocks5CmdBind(void);
	bool DoSocks5CmdUDPAssociate(void);

	/* HTTP */
	bool DoHttp(wxIPaddress &address, wxProxyCommand cmd);
	bool DoHttpRequest(wxIPaddress &address, wxProxyCommand cmd);
	bool DoHttpReply(void);
	bool DoHttpCmdConnect(void);
	
public:
	char			m_buffer[wxPROXY_BUFFER_SIZE];
	
private:
	wxProxyData		m_ProxyData;
	amuleIPV4Address	m_ProxyAddress;
	wxSocketClient		*m_ProxyClientSocket;
	wxIPaddress		*m_ProxyBoundAddress;
	amuleIPV4Address	m_ProxyBoundAddressIPV4;
	//wxIPV6address		m_ProxyBoundAddressIPV6;
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
	bool UseProxy(void) { return m_UseProxy; }
	
private:
	wxSocketProxy	m_SocketProxy;
	bool 		m_UseProxy;
};

/******************************************************************************/

class wxSocketServerProxy : public wxSocketServer
{
public:
	/* Constructor */
	wxSocketServerProxy(
		wxIPaddress &address,
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL);
		
	/* Interface */
	void SetProxyData(const wxProxyData *ProxyData);
	
private:
	wxSocketProxy	m_SocketProxy;
	bool 		m_UseProxy;
};

/******************************************************************************/

enum wxUDPOperation {
	wxUDP_OPERATION_NONE,
	wxUDP_OPERATION_RECV_FROM,
	wxUDP_OPERATION_SEND_TO
};

const unsigned int wxPROXY_UDP_OVERHEAD_IPV4 		= 10;
const unsigned int wxPROXY_UDP_OVERHEAD_DOMAIN_NAME	= 262;
const unsigned int wxPROXY_UDP_OVERHEAD_IPV6		= 20;
const unsigned int wxPROXY_UDP_MAXIMUM_OVERHEAD		= wxPROXY_UDP_OVERHEAD_DOMAIN_NAME;

class wxDatagramSocketProxy : public wxDatagramSocket
{
#if !wxCHECK_VERSION(2,5,3)
	DECLARE_CLASS(wxDatagramSocketProxy)
#endif
public:
	/* Constructor */
	wxDatagramSocketProxy(
		wxIPaddress &address,
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL);
	
	/* Destructor */
	~wxDatagramSocketProxy();
	
	/* Interface */
	void SetProxyData(const wxProxyData *ProxyData);
	
	/* wxDatagramSocket Interface */
	wxDatagramSocket& RecvFrom(
		wxSockAddress& addr, void* buf, wxUint32 nBytes );
	wxDatagramSocket& SendTo(
		wxIPaddress& addr, const void* buf, wxUint32 nBytes );
	wxUint32 LastCount(void) const;
	
private:
	wxSocketProxy	m_SocketProxy;
	bool 		m_UseProxy;
	wxSocketClient	*m_ProxySocket;
	enum wxUDPOperation m_LastUDPOperation;
	unsigned int	m_LastUDPOverhead;
};

/******************************************************************************/

#endif /* __PROXY_H__ */

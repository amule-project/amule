/*
 * This file is part of the aMule project.
 *
 * Copyright (C) 2004-2005 aMule Team (http://www.amule.org)
 * Copyright (C) 2004-2005 Marcelo Jimenez (phoenix@amule.org)
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
#include "StateMachine.h"	// For StateMachine

#if !wxCHECK_VERSION(2,5,1)
	#define wxIPaddress wxIPV4address
#endif

/******************************************************************************/

/*
 * SOCKS4 protocol implementation according to:
 * - "SOCKS: A protocol for TCP proxy across firewalls":
 *   amule-root/docs/socks4.protocol
 */
const unsigned char SOCKS4_VERSION = 0x04;

const unsigned char SOCKS4_CMD_CONNECT	= 0x01;
const unsigned char SOCKS4_CMD_BIND	= 0x02;

const unsigned char SOCKS4_REPLY_CODE				= 0;
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

const unsigned char SOCKS5_AUTH_VERSION_USERNAME_PASSWORD	= 0x01;

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

//------------------------------------------------------------------------------
// wxProxyType
//------------------------------------------------------------------------------

/*
 * These constants must match the integer values saved in the configuration file,
 * DO NOT CHANGE THIS ORDER!!!
 */
enum wxProxyType {
	wxPROXY_NONE = -1,
	wxPROXY_SOCKS5,
	wxPROXY_SOCKS4,
	wxPROXY_HTTP
};

//------------------------------------------------------------------------------
// wxProxyData
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// ProxyEventHandler
//------------------------------------------------------------------------------

class ProxyEventHandler :
#ifndef AMULE_DAEMON
public wxEvtHandler
#else
public wxThread
#endif
{
public:
	ProxyEventHandler();

#ifndef AMULE_DAEMON
private:
	void ProxySocketHandler(wxSocketEvent &event);
	DECLARE_EVENT_TABLE();
#else
public:
	~ProxyEventHandler();

private:
	void *Entry();
#endif
};

//------------------------------------------------------------------------------
// ProxyStateMachine
//------------------------------------------------------------------------------
/* This size is just to be a little bit greater than the UDP buffer used in aMule.
 * Proxy protocol needs much less than this. 1024 would be ok. Other options are
 * - Default ethernet MTU - Eth-II - IP - UDP: 1,514 - 14 - 20 - 8 = 1472 bytes;
 * - Default token ring MTU 4,202 - overheads = ??.
 * It would be really more efficient if the final object was less than 
 * a page (4096 bytes) in size.
 */
//const unsigned int wxPROXY_BUFFER_SIZE = 1024;
const unsigned int wxPROXY_BUFFER_SIZE = 5*1024;

enum wxProxyCommand {
	wxPROXY_CMD_CONNECT,
	wxPROXY_CMD_BIND,
	wxPROXY_CMD_UDP_ASSOCIATE
};

enum ProxyState {
	PROXY_STATE_START = 0,
	PROXY_STATE_END = 1
};

class ProxyStateMachine : public StateMachine
{
public:
	ProxyStateMachine(
		wxString name,
		const unsigned int max_states,
		const wxProxyData &ProxyData,
		wxProxyCommand cmd);
	virtual ~ProxyStateMachine();
	static wxString	&NewName(wxString &s, wxProxyCommand cmd);
	
	/* Interface */
	bool		Start(const wxIPaddress &PeerAddress, wxSocketClient *ProxyClientSocket);
	t_sm_state	HandleEvent(t_sm_event event);
	void		AddDummyEvent();
	void		ReactivateSocket();
	char 		*GetBuffer() const			{ return (char *)m_buffer; }
	wxIPaddress	&GetProxyBoundAddress(void) const	{ return *m_ProxyBoundAddress; }
	unsigned char	GetLastReply(void) const		{ return m_LastReply; }
	bool		IsEndState() const			{ return GetState() == PROXY_STATE_END; }

protected:
	wxSocketBase		&ProxyWrite(wxSocketBase &socket, const void *buffer, wxUint32 nbytes);
	wxSocketBase		&ProxyRead(wxSocketBase &socket, void *buffer);
#ifndef AMULE_DAEMON
	bool			CanReceive() const	{ return m_CanReceive; };
	bool			CanSend() const		{ return m_CanSend; };
#else
	bool			CanReceive() const	{ return true; };
	bool			CanSend() const		{ return true; };
#endif
	//
	// Initialized at constructor
	//
	const wxProxyData	&m_ProxyData;
	wxProxyCommand		m_ProxyCommand;
	//
	// Member variables
	//
	char			m_buffer[wxPROXY_BUFFER_SIZE];
	bool			m_IsLost;
	bool			m_IsConnected;
	bool			m_CanReceive;
	bool			m_CanSend;
	bool			m_ok;
	unsigned int		m_LastRead;
	unsigned int		m_LastWritten;
	wxSocketError		m_LastError;
	//
	// Will be initialized at Start()
	//
	wxIPaddress		*m_PeerAddress;
	wxSocketClient		*m_ProxyClientSocket;	
	wxIPaddress		*m_ProxyBoundAddress;
	amuleIPV4Address	m_ProxyBoundAddressIPV4;
	//wxIPV6address		m_ProxyBoundAddressIPV6;
	//
	// Temporary variables
	//
	unsigned char		m_LastReply;
	unsigned int		m_PacketLenght;
};

//------------------------------------------------------------------------------
// Socks5StateMachine
//------------------------------------------------------------------------------
class Socks5StateMachine;
typedef void (Socks5StateMachine::*Socks5StateProcessor)(bool entry);
class Socks5StateMachine : public ProxyStateMachine
{
private:
	static const unsigned int SOCKS5_MAX_STATES = 14;

	enum Socks5State {
		SOCKS5_STATE_START = PROXY_STATE_START,
		SOCKS5_STATE_END = PROXY_STATE_END,
		SOCKS5_STATE_SEND_QUERY_AUTHENTICATION_METHOD,
		SOCKS5_STATE_RECEIVE_AUTHENTICATION_METHOD,
		SOCKS5_STATE_PROCESS_AUTHENTICATION_METHOD,
		SOCKS5_STATE_SEND_AUTHENTICATION_GSSAPI,
		SOCKS5_STATE_RECEIVE_AUTHENTICATION_GSSAPI,
		SOCKS5_STATE_PROCESS_AUTHENTICATION_GSSAPI,
		SOCKS5_STATE_SEND_AUTHENTICATION_USERNAME_PASSWORD,
		SOCKS5_STATE_RECEIVE_AUTHENTICATION_USERNAME_PASSWORD,
		SOCKS5_STATE_PROCESS_AUTHENTICATION_USERNAME_PASSWORD,
		SOCKS5_STATE_SEND_COMMAND_REQUEST,
		SOCKS5_STATE_RECEIVE_COMMAND_REPLY,
		SOCKS5_STATE_PROCESS_COMMAND_REPLY
	};

public:
	/* Constructor */
	Socks5StateMachine(
		const wxProxyData &ProxyData,
		wxProxyCommand ProxyCommand);
	void process_state(t_sm_state state, bool entry);
	t_sm_state next_state(t_sm_event event);
	
private:
	/* State Processors */
	void process_start(bool entry);
	void process_send_query_authentication_method(bool entry);
	void process_receive_authentication_method(bool entry);
	void process_process_authentication_method(bool entry);
	void process_send_authentication_gssapi(bool entry);
	void process_receive_authentication_gssapi(bool entry);
	void process_process_authentication_gssapi(bool entry);
	void process_send_authentication_username_password(bool entry);
	void process_receive_authentication_username_password(bool entry);
	void process_process_authentication_username_password(bool entry);
	void process_send_command_request(bool entry);
	void process_receive_command_reply(bool entry);
	void process_process_command_reply(bool entry);
	void process_end(bool entry);
	/* Private Vars */
	Socks5StateProcessor m_process_state[SOCKS5_MAX_STATES];
};

//------------------------------------------------------------------------------
// Socks4StateMachine
//------------------------------------------------------------------------------
class Socks4StateMachine;
typedef void (Socks4StateMachine::*Socks4StateProcessor)(bool entry);
class Socks4StateMachine : public ProxyStateMachine
{
private:
	static const unsigned int SOCKS4_MAX_STATES = 5;

	enum Socks4State {
		SOCKS4_STATE_START = PROXY_STATE_START,
		SOCKS4_STATE_END = PROXY_STATE_END,
		SOCKS4_STATE_SEND_COMMAND_REQUEST,
		SOCKS4_STATE_RECEIVE_COMMAND_REPLY,
		SOCKS4_STATE_PROCESS_COMMAND_REPLY
	};

public:
	/* Constructor */
	Socks4StateMachine(
		const wxProxyData &ProxyData,
		wxProxyCommand ProxyCommand);
	void process_state(t_sm_state state, bool entry);
	t_sm_state next_state(t_sm_event event);
	
private:
	/* State Processors */
	void process_start(bool entry);
	void process_send_command_request(bool entry);
	void process_receive_command_reply(bool entry);
	void process_process_command_reply(bool entry);
	void process_end(bool entry);
	/* Private Vars */
	Socks4StateProcessor m_process_state[SOCKS4_MAX_STATES];
};

//------------------------------------------------------------------------------
// HttpStateMachine
//------------------------------------------------------------------------------
class HttpStateMachine;
typedef void (HttpStateMachine::*HttpStateProcessor)(bool entry);
class HttpStateMachine : public ProxyStateMachine
{
private:
	static const unsigned int HTTP_MAX_STATES = 5;

	enum HttpState {
		HTTP_STATE_START = PROXY_STATE_START,
		HTTP_STATE_END = PROXY_STATE_END,
		HTTP_STATE_SEND_COMMAND_REQUEST,
		HTTP_STATE_RECEIVE_COMMAND_REPLY,
		HTTP_STATE_PROCESS_COMMAND_REPLY
	};

public:
	/* Constructor */
	HttpStateMachine(
		const wxProxyData &ProxyData,
		wxProxyCommand ProxyCommand);
	void process_state(t_sm_state state, bool entry);
	t_sm_state next_state(t_sm_event event);
	
private:
	/* State Processors */
	void process_start(bool entry);
	void process_send_command_request(bool entry);
	void process_receive_command_reply(bool entry);
	void process_process_command_reply(bool entry);
	void process_end(bool entry);
	/* Private Vars */
	HttpStateProcessor m_process_state[HTTP_MAX_STATES];
};

//------------------------------------------------------------------------------
// amuleProxyClientSocket
//------------------------------------------------------------------------------

class wxDatagramSocketProxy;

class amuleProxyClientSocket : public wxSocketClient
{
friend class ProxyEventHandler;
public:
	/* Constructor */
	amuleProxyClientSocket(
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL,
		wxProxyCommand ProxyCommand = wxPROXY_CMD_CONNECT,
		wxDatagramSocketProxy *UDPSocket = NULL);
	
	/* Destructor */
	~amuleProxyClientSocket();
	
	/* Interface */
	void		SetProxyData(const wxProxyData *ProxyData);
	bool		GetUseProxy() const	{ return m_UseProxy; }
	char 		*GetBuffer() const	{ return m_ProxyStateMachine->GetBuffer(); }
	wxIPaddress	&GetProxyBoundAddress(void) const
						{ return m_ProxyStateMachine->GetProxyBoundAddress(); }
	bool Start(const wxIPaddress &PeerAddress);
	bool ProxyIsCapableOf(wxProxyCommand ProxyCommand) const;
	bool ProxyNegotiationIsOver() const	{ return m_ProxyStateMachine->IsEndState(); }
	wxDatagramSocketProxy *GetUDPSocket() const { return m_UDPSocket; }
	
private:
	bool			m_UseProxy;
	wxProxyData		m_ProxyData;
	amuleIPV4Address	m_ProxyAddress;
	ProxyStateMachine	*m_ProxyStateMachine;
	wxDatagramSocketProxy	*m_UDPSocket;
};

//------------------------------------------------------------------------------
// wxSocketClientProxy
//------------------------------------------------------------------------------

class wxSocketClientProxy : public amuleProxyClientSocket
{
public:
	/* Constructor */
	wxSocketClientProxy(
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL);
		
	/* Interface */
	bool Connect(wxIPaddress &address, bool wait);
};

//------------------------------------------------------------------------------
// wxSocketServerProxy
//------------------------------------------------------------------------------

class wxSocketServerProxy : public wxSocketServer
{
public:
	/* Constructor */
	wxSocketServerProxy(
		wxIPaddress &address,
		wxSocketFlags flags = wxSOCKET_NONE,
		const wxProxyData *ProxyData = NULL);
		
	/* Interface */
	
private:
};

//------------------------------------------------------------------------------
// wxDatagramSocketProxy
//------------------------------------------------------------------------------

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
	DECLARE_ABSTRACT_CLASS(wxDatagramSocketProxy)
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
	void SetUDPSocketOk() { m_UDPSocketOk = true; }
	
	/* wxDatagramSocket Interface */
	virtual wxDatagramSocket& RecvFrom(
		wxSockAddress& addr, void* buf, wxUint32 nBytes );
	virtual wxDatagramSocket& SendTo(
		wxIPaddress& addr, const void* buf, wxUint32 nBytes );
	virtual wxUint32 LastCount(void) const;
	
private:
	bool			m_UDPSocketOk;
	amuleProxyClientSocket	m_ProxyTCPSocket;
	enum wxUDPOperation	m_LastUDPOperation;
	unsigned int		m_LastUDPOverhead;
};

/******************************************************************************/

#endif /* __PROXY_H__ */

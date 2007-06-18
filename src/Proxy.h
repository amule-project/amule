//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2007 Marcelo Jimenez ( phoenix@amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef __PROXY_H__
#define __PROXY_H__


#include "amuleIPV4Address.h"	// For amuleIPV4address
#include "StateMachine.h"	// For CStateMachine

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
 * - RFC-1929: username/password Authentication for SOCKS V5
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
// CProxyType
//------------------------------------------------------------------------------

/*
 * These constants must match the integer values saved in the configuration file,
 * DO NOT CHANGE THIS ORDER!!!
 */
enum CProxyType {
	PROXY_NONE = -1,
	PROXY_SOCKS5,
	PROXY_SOCKS4,
	PROXY_HTTP,
	PROXY_SOCKS4a,
};


//------------------------------------------------------------------------------
// CProxyData
//------------------------------------------------------------------------------
/**
 * The ProxyData class will hold information about the proxy server to be used.
 */
class CProxyData
{
public:
	/**
	 * Default constructor.
	 */
	CProxyData();
	/**
	 * Constructor.
	 * 
	 * @param proxyEnable	Whether proxy is enabled or not.
	 * @param proxyType	The type of the proxy server.
	 * @param proxyHostName	The proxy host name or IP address.
	 * @param proxyPort	The proxy port number.
	 * @param enablePassword Whether authentication should be performed.
	 * @param userName	The user name to authenticate to the server.
	 * @param password	The password to authenticate to the server.
	 */
	CProxyData(
		bool		proxyEnable,
		CProxyType	proxyType,
		const wxString	&proxyHostName,
		unsigned short	proxyPort,
		bool		enablePassword,
		const wxString	&userName,
		const wxString	&password
	);
	/**
	 * Clears the object contents.
	 */
	void Clear();

public:
	//! Whether proxy is enabled or not.
	bool		m_proxyEnable;
	//! The type of the proxy server.
	CProxyType	m_proxyType;
	//! The proxy host name or IP address.
	wxString	m_proxyHostName;
	//! The proxy port number.
	unsigned short	m_proxyPort;
	//! Whether authentication should be performed.
	bool		m_enablePassword;
	//! The user name to authenticate to the server.
	wxString	m_userName;
	//! The password to authenticate to the server.
	wxString	m_password;
};

//------------------------------------------------------------------------------
// CProxyEventHandler
//------------------------------------------------------------------------------
/**
 * Event handler object used during proxy negotiation.
 */
class CProxyEventHandler : public wxEvtHandler {
public:
	/**
	 * Constructor.
	 */
	CProxyEventHandler();

private:
	/**
	 * Event handler function.
	 */
	void ProxySocketHandler(wxSocketEvent &event);
	DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
// CProxyStateMachine
//------------------------------------------------------------------------------
/* This size is just to be a little bit greater than the UDP buffer used in aMule.
 * Proxy protocol needs much less than this. 1024 would be ok. Other options are
 * - Default ethernet MTU - Eth-II - IP - UDP: 1,514 - 14 - 20 - 8 = 1472 bytes;
 * - Default token ring MTU 4,202 - overheads = ??.
 * It would be really more efficient if the final object was less than 
 * a page (4096 bytes) in size.
 */
//const unsigned int PROXY_BUFFER_SIZE = 1024;
const unsigned int PROXY_BUFFER_SIZE = 5*1024;

enum CProxyCommand {
	PROXY_CMD_CONNECT,
	PROXY_CMD_BIND,
	PROXY_CMD_UDP_ASSOCIATE
};

enum CProxyState {
	PROXY_STATE_START = 0,
	PROXY_STATE_END = 1
};

/**
 * The ProxyStateMachine class is the ancestor of all proxy classes.
 *
 * CProxyStateMachine will do all the common work that a proxy class must do
 * and provide the necessary variables.
 */
class CProxyStateMachine : public CStateMachine
{
public:
	/**
	 * Constructor.
	 *
	 * @param name		The name of the state machine. For debug messages only.
	 * @param max_states	The maximum number of states that this machine will have.
	 * @param proxyData	The necessary proxy information.
	 * @param cmd		The type of proxy command to run.
	 */
	CProxyStateMachine(
		wxString name,
		const unsigned int max_states,
		const CProxyData &proxyData,
		CProxyCommand cmd);
	/**
	 * Destructor.
	 */
	virtual ~CProxyStateMachine();
	/**
	 * Adds a small string to the state machine name, containing the proxy command.
	 *
	 * @param s	The original state machine name.
	 * @param cmd	The proxy command.
	 */
	static wxString	&NewName(wxString &s, CProxyCommand cmd);
	
	/* Interface */
	bool		Start(const wxIPaddress &peerAddress, wxSocketClient *proxyClientSocket);
	t_sm_state	HandleEvent(t_sm_event event);
	void		AddDummyEvent();
	void		ReactivateSocket();
	char 		*GetBuffer() const			{ return (char *)m_buffer; }
	wxIPaddress	&GetProxyBoundAddress(void) const	{ return *m_proxyBoundAddress; }
	unsigned char	GetLastReply(void) const		{ return m_lastReply; }
	bool		IsEndState() const			{ return GetState() == PROXY_STATE_END; }

protected:
	wxSocketBase		&ProxyWrite(wxSocketBase &socket, const void *buffer, wxUint32 nbytes);
	wxSocketBase		&ProxyRead(wxSocketBase &socket, void *buffer);
#ifndef AMULE_DAEMON
	bool			CanReceive() const	{ return m_canReceive; };
	bool			CanSend() const		{ return m_canSend; };
#else
	bool			CanReceive() const	{ return true; };
	bool			CanSend() const		{ return true; };
#endif
	//
	// Initialized at constructor
	//
	const CProxyData	&m_proxyData;
	CProxyCommand		m_proxyCommand;
	//
	// Member variables
	//
	char			m_buffer[PROXY_BUFFER_SIZE];
	bool			m_isLost;
	bool			m_isConnected;
	bool			m_canReceive;
	bool			m_canSend;
	bool			m_ok;
	unsigned int		m_lastRead;
	unsigned int		m_lastWritten;
	wxSocketError		m_lastError;
	//
	// Will be initialized at Start()
	//
	wxIPaddress		*m_peerAddress;
	wxSocketClient		*m_proxyClientSocket;	
	wxIPaddress		*m_proxyBoundAddress;
	amuleIPV4Address	m_proxyBoundAddressIPV4;
	//wxIPV6address		m_proxyBoundAddressIPV6;
	//
	// Temporary variables
	//
	unsigned char		m_lastReply;
	unsigned int		m_packetLenght;
};

//------------------------------------------------------------------------------
// CSocks5StateMachine
//------------------------------------------------------------------------------
class CSocks5StateMachine;
typedef void (CSocks5StateMachine::*Socks5StateProcessor)(bool entry);
class CSocks5StateMachine : public CProxyStateMachine
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
	CSocks5StateMachine(
		const CProxyData &proxyData,
		CProxyCommand proxyCommand);
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
	wxString m_state_name[SOCKS5_MAX_STATES];
};

//------------------------------------------------------------------------------
// CSocks4StateMachine
//------------------------------------------------------------------------------
class CSocks4StateMachine;
typedef void (CSocks4StateMachine::*Socks4StateProcessor)(bool entry);
class CSocks4StateMachine : public CProxyStateMachine
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
	CSocks4StateMachine(
		const CProxyData &proxyData,
		CProxyCommand proxyCommand);
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
	wxString m_state_name[SOCKS4_MAX_STATES];
};

//------------------------------------------------------------------------------
// CHttpStateMachine
//------------------------------------------------------------------------------
class CHttpStateMachine;
typedef void (CHttpStateMachine::*HttpStateProcessor)(bool entry);
class CHttpStateMachine : public CProxyStateMachine
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
	CHttpStateMachine(
		const CProxyData &proxyData,
		CProxyCommand proxyCommand);
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
	wxString m_state_name[HTTP_MAX_STATES];
};

//------------------------------------------------------------------------------
// CProxySocket
//------------------------------------------------------------------------------

class CDatagramSocketProxy;

class CProxySocket : public wxSocketClient
{
friend class CProxyEventHandler;
public:
	/* Constructor */
	CProxySocket(
		wxSocketFlags flags = wxSOCKET_NONE,
		const CProxyData *proxyData = NULL,
		CProxyCommand proxyCommand = PROXY_CMD_CONNECT,
		CDatagramSocketProxy *udpSocket = NULL);
	
	/* Destructor */
	~CProxySocket();
	
	/* I know, this is not very good, because SetEventHandler is not
	 * virtual in wxSocketBase, but I need to GetEventHandler in Proxy.cpp,
	 * so...
	 */
	void SetEventHandler(wxEvtHandler &handler, int id = wxID_ANY)
	{
		m_socketEventHandler = &handler;
		m_socketEventHandlerId = id;
		wxSocketClient::SetEventHandler(handler, id);
	}
	wxEvtHandler *GetEventHandler(void)	const { return m_socketEventHandler; }
	int GetEventHandlerId(void)		const { return m_socketEventHandlerId; }
	void SaveEventHandler(void)
	{
		m_savedSocketEventHandler = m_socketEventHandler;
		m_savedSocketEventHandlerId = m_socketEventHandlerId;
	}
	void RestoreEventHandler(void)
	{
		m_socketEventHandler = m_savedSocketEventHandler;
		m_socketEventHandlerId = m_savedSocketEventHandlerId;
		SetEventHandler(*m_socketEventHandler, m_socketEventHandlerId);
	}
	
	/* Interface */
	void		SetProxyData(const CProxyData *proxyData);
	bool		GetUseProxy() const	{ return m_useProxy; }
	char 		*GetBuffer() const	{ return m_proxyStateMachine->GetBuffer(); }
	wxIPaddress	&GetProxyBoundAddress(void) const
						{ return m_proxyStateMachine->GetProxyBoundAddress(); }
	bool Start(const wxIPaddress &peerAddress);
	bool ProxyIsCapableOf(CProxyCommand proxyCommand) const;
	bool ProxyNegotiationIsOver() const	{ return m_proxyStateMachine->IsEndState(); }
	CDatagramSocketProxy *GetUDPSocket() const { return m_udpSocket; }
	
private:
	bool			m_useProxy;
	CProxyData		m_proxyData;
	amuleIPV4Address	m_proxyAddress;
	CProxyStateMachine	*m_proxyStateMachine;
	CDatagramSocketProxy	*m_udpSocket;
	wxEvtHandler		*m_socketEventHandler;
	int			m_socketEventHandlerId;
	wxEvtHandler		*m_savedSocketEventHandler;
	int			m_savedSocketEventHandlerId;
};

//------------------------------------------------------------------------------
// CSocketClientProxy
//------------------------------------------------------------------------------

class CSocketClientProxy : public CProxySocket
{
public:
	/* Constructor */
	CSocketClientProxy(
		wxSocketFlags flags = wxSOCKET_NONE,
		const CProxyData *proxyData = NULL);
		
	/* Interface */
	bool Connect(wxIPaddress &address, bool wait);
	CSocketClientProxy& Read(void *buffer, wxUint32 nbytes);
	CSocketClientProxy& Write(const void *buffer, wxUint32 nbytes);

private:
	wxMutex			m_socketLocker;
};

//------------------------------------------------------------------------------
// CSocketServerProxy
//------------------------------------------------------------------------------

class CSocketServerProxy : public wxSocketServer
{
public:
	/* Constructor */
	CSocketServerProxy(
		wxIPaddress &address,
		wxSocketFlags flags = wxSOCKET_NONE,
		const CProxyData *proxyData = NULL);
		
	/* Interface */
	CSocketServerProxy& Read(void *buffer, wxUint32 nbytes);
	CSocketServerProxy& Write(const void *buffer, wxUint32 nbytes);
	
private:
	wxMutex			m_socketLocker;
};

//------------------------------------------------------------------------------
// CDatagramSocketProxy
//------------------------------------------------------------------------------

enum UDPOperation {
	UDP_OPERATION_NONE,
	UDP_OPERATION_RECV_FROM,
	UDP_OPERATION_SEND_TO
};

const unsigned int PROXY_UDP_OVERHEAD_IPV4 		= 10;
const unsigned int PROXY_UDP_OVERHEAD_DOMAIN_NAME	= 262;
const unsigned int PROXY_UDP_OVERHEAD_IPV6		= 20;
const unsigned int PROXY_UDP_MAXIMUM_OVERHEAD		= PROXY_UDP_OVERHEAD_DOMAIN_NAME;

class CDatagramSocketProxy : public wxDatagramSocket
{
public:
	/* Constructor */
	CDatagramSocketProxy(
		wxIPaddress &address,
		wxSocketFlags flags = wxSOCKET_NONE,
		const CProxyData *proxyData = NULL);
	
	/* Destructor */
	~CDatagramSocketProxy();
	
	/* Interface */
	void SetUDPSocketOk() { m_udpSocketOk = true; }
	
	/* wxDatagramSocket Interface */
	virtual wxDatagramSocket& RecvFrom(
		wxSockAddress& addr, void* buf, wxUint32 nBytes );
	virtual wxDatagramSocket& SendTo(
		wxIPaddress& addr, const void* buf, wxUint32 nBytes );
	virtual wxUint32 LastCount(void) const;
	
private:
	bool			m_udpSocketOk;
	CProxySocket		m_proxyTCPSocket;
	enum UDPOperation	m_lastUDPOperation;
	unsigned int		m_lastUDPOverhead;
	wxMutex			m_socketLocker;
};

/******************************************************************************/

#endif /* __PROXY_H__ */

// File_checked_for_headers

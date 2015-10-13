//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
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

#include "Proxy.h"		/* for Interface		*/

#include <common/EventIDs.h>

#include "ArchSpecific.h"	/* for ENDIAN_HTONS()		*/
#include "Logger.h"		/* for AddDebugLogLineN		*/
#include "OtherFunctions.h"	/* for EncodeBase64()		*/
#include <common/StringFunctions.h>	/* for unicode2char */
#include "GuiEvents.h"

//------------------------------------------------------------------------------
// CProxyData
//------------------------------------------------------------------------------

CProxyData::CProxyData()
{
	Clear();
}

CProxyData::CProxyData(
	bool		proxyEnable,
	CProxyType	proxyType,
	const wxString	&proxyHostName,
	unsigned short	proxyPort,
	bool		enablePassword,
	const wxString	&userName,
	const wxString	&password)
:
m_proxyEnable(proxyEnable),
m_proxyType(proxyType),
m_proxyHostName(proxyHostName),
m_proxyPort(proxyPort),
/*
 * The flag m_enablePassword is currently not used. The first
 * authentication method tryed is No-Authentication, the second
 * is username/password. If there is no username/password in
 * CProxyData, a NULL username/password is sent. That will probably
 * lead to a failure, but at least we tryed. Maybe this behaviour
 * could be altered later.
 */
m_enablePassword(enablePassword),
m_userName(userName),
m_password(password)
{
}

void CProxyData::Clear()
{
	m_proxyEnable = false;
	m_proxyType = PROXY_NONE;
	m_proxyHostName.Clear();
	m_proxyPort = 0;
	m_enablePassword = false;
	m_userName.Clear();
	m_password.Clear();
}

#ifndef CLIENT_GUI

#include <typeinfo> // Do_not_auto_remove (NetBSD, older gccs)

//------------------------------------------------------------------------------
// ProxyEventHandler
//------------------------------------------------------------------------------

CProxyEventHandler::CProxyEventHandler()
{
}

BEGIN_EVENT_TABLE(CProxyEventHandler, wxEvtHandler)
	EVT_SOCKET(ID_PROXY_SOCKET_EVENT, CProxyEventHandler::ProxySocketHandler)
END_EVENT_TABLE()

//
// THE one and only Event Handler
//
static CProxyEventHandler g_proxyEventHandler;

void CProxyEventHandler::ProxySocketHandler(wxSocketEvent& event)
{
	CProxySocket *sock = dynamic_cast<CProxySocket *>(event.GetSocket());
	if (sock) {
		sock->m_proxyStateMachine->Schedule(event.GetSocketEvent());
		sock->m_proxyStateMachine->Clock();
	} else {
		// we're doomed :)
	}
}

//
// In Asio mode the event handler is:
//
void CProxySocket::OnProxyEvent(int evt)
{
	m_proxyStateMachine->Schedule(evt);
	m_proxyStateMachine->Clock();
}

//------------------------------------------------------------------------------
// CProxyStateMachine
//------------------------------------------------------------------------------

CProxyStateMachine::CProxyStateMachine(
		wxString name,
		const unsigned int max_states,
		const CProxyData &proxyData,
		CProxyCommand proxyCommand)
:
CStateMachine(NewName(name, proxyCommand), max_states, PROXY_STATE_START),
m_proxyData(proxyData),
m_proxyCommand(proxyCommand),
m_isLost(false),
m_isConnected(false),
m_canReceive(false),
m_canSend(false),
m_ok(true),
m_lastRead(0),
// Will be initialized at Start()
m_peerAddress(NULL),
m_proxyClientSocket(NULL),
m_proxyBoundAddress(NULL),
// Temporary variables
m_lastReply(0),
m_packetLenght(0)
{
}

CProxyStateMachine::~CProxyStateMachine()
{
	delete m_peerAddress;
}

wxString &CProxyStateMachine::NewName(wxString &s, CProxyCommand proxyCommand)
{
	switch (proxyCommand) {
	case PROXY_CMD_CONNECT:
		s += wxT("-CONNECT");
		break;

	case PROXY_CMD_BIND:
		s += wxT("-BIND");
		break;

	case PROXY_CMD_UDP_ASSOCIATE:
		s += wxT("-UDP");
		break;
	}

	return s;
}

bool CProxyStateMachine::Start(const amuleIPV4Address &peerAddress, CLibSocket *proxyClientSocket)
{
	m_proxyClientSocket = proxyClientSocket;
	m_peerAddress = new amuleIPV4Address(peerAddress);
	//try {
	//	const wxIPV4address &peer = dynamic_cast<const wxIPV4address &>(peerAddress);
	//	m_peerAddress = new amuleIPV4Address(peer);
	//} catch (const std::bad_cast& WXUNUSED(e)) {
	//	// Should process other types of wxIPAddres before quitting
	//	AddDebugLogLineN(logProxy, wxT("(1)bad_cast exception!"));
	//	wxFAIL;
	//	return false;
	//}

	// To run the state machine, return and just let the events start to happen.
	return true;
}

static const int MULE_SOCKET_DUMMY_VALUE = MULE_SOCKET_INPUT + MULE_SOCKET_OUTPUT + MULE_SOCKET_CONNECTION + MULE_SOCKET_LOST;

t_sm_state CProxyStateMachine::HandleEvent(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = GetState();
	switch(event)
	{
	case MULE_SOCKET_CONNECTION:
		AddDebugLogLineN(logProxy, wxT("Connection event"));
		m_isConnected = true;
		break;

	case MULE_SOCKET_INPUT:
		AddDebugLogLineN(logProxy, wxT("Input event"));
		m_canReceive = true;
		break;

	case MULE_SOCKET_OUTPUT:
		AddDebugLogLineN(logProxy, wxT("Output event"));
		m_canSend = true;
		break;

	case MULE_SOCKET_LOST:
		AddDebugLogLineN(logProxy, wxT("Lost connection event"));
		m_isLost = true;
		m_ok = false;
		break;

	case MULE_SOCKET_DUMMY_VALUE:
		AddDebugLogLineN(logProxy, wxT("Dummy event"));
		break;

	default:
		AddDebugLogLineN(logProxy, CFormat(wxT("Unknown event %d")) % event);
		break;
	}

	// Aborting conditions:
	// - MULE_SOCKET_LOST event
	// - More than 10 times on the same state
	if (	m_isLost ||
		GetClocksInCurrentState() > 10) {
		ret = PROXY_STATE_END;
	}

	return ret;
}

void CProxyStateMachine::AddDummyEvent()
{
#ifdef ASIO_SOCKETS
	CProxySocket *s = dynamic_cast<CProxySocket *>(m_proxyClientSocket);
	if (s) {	// should always be
		CoreNotify_ProxySocketEvent(s, MULE_SOCKET_DUMMY_VALUE);
	}
#else
	wxSocketEvent e(ID_PROXY_SOCKET_EVENT);
	// Make sure this is an unknown event :)
	e.m_event = (wxSocketNotify)(MULE_SOCKET_DUMMY_VALUE);
	e.SetEventObject(m_proxyClientSocket);
	g_proxyEventHandler.AddPendingEvent(e);
#endif
}

void CProxyStateMachine::ReactivateSocket()
{
	/*    If proxy is beeing used, then the TCP socket handlers
	 * (CServerSocketHandler and CClientTCPSocketHandler) will not
	 * receive a wxSOCKET_CONNECTION event, because the connection has
	 * already started with the proxy. So we must add a wxSOCKET_CONNECTION
	 * event to make things go undetected. A wxSOCKET_OUTPUT event is also
	 * necessary to start sending data to the server. */
	CProxySocket *s = dynamic_cast<CProxySocket *>(m_proxyClientSocket);
	// If that is not true, we are in serious trouble...
	wxASSERT(s);
	if (CDatagramSocketProxy *udp = s->GetUDPSocket()) {
		// The original socket was a UDP socket
		if(m_ok) {
			// From now on, the UDP socket can be used,
			// remove the protection.
			udp->SetUDPSocketOk();
		}
		// No need to call RestoreState(), that socket will no longer
		// be used after proxy negotiation.
	} else {
		// The original socket was a TCP socket
#ifdef ASIO_SOCKETS
		if (s->GetProxyState()) {	// somehow this gets called twice ?
			s->SetProxyState(false);
			CoreNotify_LibSocketConnect(s, 0);
			if (m_ok) {
				CoreNotify_LibSocketSend(s, 0);
			} else {
				CoreNotify_LibSocketLost(s);
			}
		}
#else
		s->RestoreEventHandler();
		wxSocketEvent e(s->GetEventHandlerId());
		e.m_event = wxSOCKET_CONNECTION;
		e.SetEventObject(s);
		wxEvtHandler *h(s->GetEventHandler());
		h->AddPendingEvent(e);
		e.m_event = wxSOCKET_OUTPUT;
		h->AddPendingEvent(e);
		if (!m_ok) {
			e.m_event = wxSOCKET_LOST;
			h->AddPendingEvent(e);
		}
		s->RestoreState();
#endif
	}
}

uint32 CProxyStateMachine::ProxyWrite(CLibSocket &socket, const void *buffer, wxUint32 nbytes)
{
	uint32 written = socket.Write(buffer, nbytes);
	/* Set the status of this operation */
	m_ok = true;
	if (m_proxyClientSocket->BlocksWrite()) {
		m_lastError = 0;
		m_canSend = false;
	} else if ((m_lastError = m_proxyClientSocket->LastError()) != 0) {
		m_ok = false;
	}
	AddDebugLogLineN(logProxy, CFormat(wxT("ProxyWrite %d %d ok %d cansend %d")) % nbytes % written % m_ok % m_canSend);

	return written;
}

uint32 CProxyStateMachine::ProxyRead(CLibSocket &socket, void *buffer)
{
	/* Always try to read the full buffer. That explicitly demands that
	 * the socket has the flag wxSOCKET_NONE. */
	m_lastRead = socket.Read(buffer, PROXY_BUFFER_SIZE);
	/* Set the status of this operation */
	m_ok = true;
	if (m_proxyClientSocket->BlocksRead()) {
		m_lastError = 0;
		m_canReceive = false;
	} else if ((m_lastError = m_proxyClientSocket->LastError()) != 0) {
		m_ok = false;
	}
#ifdef ASIO_SOCKETS
	// We will get a new event right away if data is left, or when new data gets available.
	// So block for now.
	m_canReceive = false;
#endif
	AddDebugLogLineN(logProxy, CFormat(wxT("ProxyRead %d ok %d canrec %d")) % m_lastRead % m_ok % m_canReceive);

	return m_lastRead;
}

bool CProxyStateMachine::CanReceive() const
{
	return m_canReceive;
}

bool CProxyStateMachine::CanSend() const
{
	return m_canSend;
}

//------------------------------------------------------------------------------
// CSocks5StateMachine
//------------------------------------------------------------------------------

/**
 * The state machine constructor must initialize the array of pointer to member
 * functions. Don't waste you time trying to statically initialize this, pointer
 * to member functions require an object to operate on, so this array must be
 * initialized at run time.
 */
CSocks5StateMachine::CSocks5StateMachine(
	const CProxyData &proxyData,
	CProxyCommand proxyCommand)
:
CProxyStateMachine(
	wxString(wxT("Socks5")), SOCKS5_MAX_STATES, proxyData, proxyCommand)
{
	m_process_state[ 0] = &CSocks5StateMachine::process_start;
	m_state_name[ 0]    = wxT("process_start");
	m_process_state[ 1] = &CSocks5StateMachine::process_end;
	m_state_name[ 1]    = wxT("process_end");
	m_process_state[ 2] = &CSocks5StateMachine::process_send_query_authentication_method;
	m_state_name[ 2]    = wxT("process_send_query_authentication_method");
	m_process_state[ 3] = &CSocks5StateMachine::process_receive_authentication_method;
	m_state_name[ 3]    = wxT("process_receive_authentication_method");
	m_process_state[ 4] = &CSocks5StateMachine::process_process_authentication_method;
	m_state_name[ 4]    = wxT("process_process_authentication_method");
	m_process_state[ 5] = &CSocks5StateMachine::process_send_authentication_gssapi;
	m_state_name[ 5]    = wxT("process_send_authentication_gssapi");
	m_process_state[ 6] = &CSocks5StateMachine::process_receive_authentication_gssapi;
	m_state_name[ 6]    = wxT("process_receive_authentication_gssapi");
	m_process_state[ 7] = &CSocks5StateMachine::process_process_authentication_gssapi;
	m_state_name[ 7]    = wxT("process_process_authentication_gssapi");
	m_process_state[ 8] = &CSocks5StateMachine::process_send_authentication_username_password;
	m_state_name[ 8]    = wxT("process_send_authentication_username_password");
	m_process_state[ 9] = &CSocks5StateMachine::process_receive_authentication_username_password;
	m_state_name[ 9]    = wxT("process_receive_authentication_username_password");
	m_process_state[10] = &CSocks5StateMachine::process_process_authentication_username_password;
	m_state_name[10]    = wxT("process_process_authentication_username_password");
	m_process_state[11] = &CSocks5StateMachine::process_send_command_request;
	m_state_name[11]    = wxT("process_send_command_request");
	m_process_state[12] = &CSocks5StateMachine::process_receive_command_reply;
	m_state_name[12]    = wxT("process_receive_command_reply");
	m_process_state[13] = &CSocks5StateMachine::process_process_command_reply;
	m_state_name[13]    = wxT("process_process_command_reply");
}

void CSocks5StateMachine::process_state(t_sm_state state, bool entry)
{
	/* Ok, the syntax is terrible, but this is correct. This is a
	 * pointer to a member function. No C equivalent for that. */
	(this->*m_process_state[state])(entry);
#ifdef __DEBUG__
	int n = 0;

	switch (state) {
	case SOCKS5_STATE_START:
	case SOCKS5_STATE_END:
	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_METHOD:
	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_GSSAPI:
	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_USERNAME_PASSWORD:
	case SOCKS5_STATE_RECEIVE_COMMAND_REPLY:
	default:
		n = 0;
		break;

	case SOCKS5_STATE_SEND_QUERY_AUTHENTICATION_METHOD:
	case SOCKS5_STATE_SEND_AUTHENTICATION_GSSAPI:
	case SOCKS5_STATE_SEND_AUTHENTICATION_USERNAME_PASSWORD:
	case SOCKS5_STATE_SEND_COMMAND_REQUEST:
		n = m_packetLenght;
		break;

	case SOCKS5_STATE_PROCESS_AUTHENTICATION_METHOD:
	case SOCKS5_STATE_PROCESS_AUTHENTICATION_GSSAPI:
	case SOCKS5_STATE_PROCESS_AUTHENTICATION_USERNAME_PASSWORD:
	case SOCKS5_STATE_PROCESS_COMMAND_REPLY:
		n = m_lastRead;
		break;
	}

	if (entry) {
		DumpMem(m_buffer, n, m_state_name[state], m_ok);
	} else {
		AddDebugLogLineN(logProxy,
			wxString(wxT("wait state -- ")) << m_state_name[state]);
	}
#endif // __DEBUG__
}

/**
 * Code this such that the next state is only entered when it is able to
 * perform the operation (read or write). State processing will assume
 * that it can read or write upon entry of the state. This is done using
 * CanSend() and CanReceive().
 */
t_sm_state CSocks5StateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = HandleEvent(event);
	switch (GetState()) {
	case SOCKS5_STATE_START:
		if (m_isConnected && !m_isLost && CanSend()) {
			ret = SOCKS5_STATE_SEND_QUERY_AUTHENTICATION_METHOD;
		}
		break;

	case SOCKS5_STATE_SEND_QUERY_AUTHENTICATION_METHOD:
		if (CanReceive()) {
			ret = SOCKS5_STATE_RECEIVE_AUTHENTICATION_METHOD;
		}
		break;

	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_METHOD:
		ret = SOCKS5_STATE_PROCESS_AUTHENTICATION_METHOD;
		break;

	case SOCKS5_STATE_PROCESS_AUTHENTICATION_METHOD:
		if (m_ok) {
			if (CanSend()) {
				switch (m_lastReply) {
				case SOCKS5_AUTH_METHOD_NO_AUTH_REQUIRED:
					ret = SOCKS5_STATE_SEND_COMMAND_REQUEST;
					break;

				case SOCKS5_AUTH_METHOD_GSSAPI:
					ret = SOCKS5_STATE_SEND_AUTHENTICATION_GSSAPI;
					break;

				case SOCKS5_AUTH_METHOD_USERNAME_PASSWORD:
					ret = SOCKS5_STATE_SEND_AUTHENTICATION_USERNAME_PASSWORD;
					break;

				case SOCKS5_AUTH_METHOD_NO_ACCEPTABLE_METHODS:
				default:
					ret = SOCKS5_STATE_END;
					break;
				}
			} else {
				AddDebugLogLineN(logProxy, wxT("Can't send"));
			}
		} else {
			ret = SOCKS5_STATE_END;
		}
		break;

	case SOCKS5_STATE_SEND_AUTHENTICATION_GSSAPI:
		if (m_ok) {
			if (CanReceive()) {
				ret = SOCKS5_STATE_RECEIVE_AUTHENTICATION_GSSAPI;
			}
		} else {
			ret = SOCKS5_STATE_END;
		}
		break;

	case SOCKS5_STATE_SEND_AUTHENTICATION_USERNAME_PASSWORD:
		if (m_ok) {
			if (CanReceive()) {
				ret = SOCKS5_STATE_RECEIVE_AUTHENTICATION_USERNAME_PASSWORD;
			}
		} else {
			ret = SOCKS5_STATE_END;
		}
		break;

	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_GSSAPI:
		ret = SOCKS5_STATE_PROCESS_AUTHENTICATION_GSSAPI;
		break;

	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_USERNAME_PASSWORD:
		ret = SOCKS5_STATE_PROCESS_AUTHENTICATION_USERNAME_PASSWORD;
		break;

	case SOCKS5_STATE_PROCESS_AUTHENTICATION_GSSAPI:
	case SOCKS5_STATE_PROCESS_AUTHENTICATION_USERNAME_PASSWORD:
		if (m_ok) {
			if (CanSend()) {
				ret = SOCKS5_STATE_SEND_COMMAND_REQUEST;
			}
		} else {
			ret = SOCKS5_STATE_END;
		}
		break;

	case SOCKS5_STATE_SEND_COMMAND_REQUEST:
		if (m_ok) {
			if (CanReceive()) {
				ret = SOCKS5_STATE_RECEIVE_COMMAND_REPLY;
			}
		} else {
			ret = SOCKS5_STATE_END;
		}
		break;

	case SOCKS5_STATE_RECEIVE_COMMAND_REPLY:
		ret = SOCKS5_STATE_PROCESS_COMMAND_REPLY;
		break;

	case SOCKS5_STATE_PROCESS_COMMAND_REPLY:
		ret = SOCKS5_STATE_END;
		break;

	case SOCKS5_STATE_END:
	default:
		break;
	}

	return ret;
}

/**
 *	So, this is how you do it: the state machine is clocked by the events
 * that happen inside the event handler. You can add a dummy event whenever
 * you see that the system will not generate an event. But don't add dummy
 * events before reads, reads should only be performed after input events.
 *
 *	Maybe it makes sense to add a dummy event before a read if there is no
 * state change (wait state).
 *
 *	The event system will generate at least 2 events, one wxSOCKET_CONNECTION,
 * one wxSOCKET_OUTPUT, so we will have 2 clocks in our state machine. Plus, each
 * time there is unread data in the receive buffer of the socket, a wxSOCKET_INPUT
 * event will be generated. If you feel you will need more clocks than these, use
 * AddDummyEvent(), but I suggest you review your state machine design first.
 */
void CSocks5StateMachine::process_start(bool)
{}

void CSocks5StateMachine::process_end(bool)
{
	ReactivateSocket();
}

void CSocks5StateMachine::process_send_query_authentication_method(bool entry)
{
	if (entry) {
		// Prepare the authentication method negotiation packet
		m_buffer[0] = SOCKS5_VERSION;
		m_buffer[1] = 2; // Number of supported methods
		//m_buffer[1] = 3; // Number of supported methods
		m_buffer[2] = SOCKS5_AUTH_METHOD_NO_AUTH_REQUIRED;
		m_buffer[3] = SOCKS5_AUTH_METHOD_USERNAME_PASSWORD;
		m_buffer[4] = SOCKS5_AUTH_METHOD_GSSAPI;
		m_packetLenght = 4;
		//m_packetLenght = 5;

		// Send the authentication method negotiation packet
		ProxyWrite(*m_proxyClientSocket, m_buffer, m_packetLenght);
	}
}

void CSocks5StateMachine::process_receive_authentication_method(bool entry)
{
	if (entry) {
		// Receive the method selection message
		m_packetLenght = 2;
		ProxyRead(*m_proxyClientSocket, m_buffer);
	}
	/* This is added because there will be no more input events. If the
	 * world was a nice place, we could think about joining the
	 * process_receive and the process_process states here, but some day
	 * we might have to deal with the fact that the i/o operation has been
	 * incomplete, and that we must finish our job the next time we enter
	 * this state. */
	AddDummyEvent();
}

void CSocks5StateMachine::process_process_authentication_method(bool entry)
{
	if (entry) {
		m_lastReply = m_buffer[1];
		m_ok = m_ok && m_buffer[0] == SOCKS5_VERSION;
	}
	/* Ok, this one is here because wxSOCKET_OUTPUT events only happen
	 * once when you connect the socket, and after that, only after a
	 * wxSOCKET_WOULDBLOCK error happens. */
	AddDummyEvent();
}

void CSocks5StateMachine::process_send_authentication_gssapi(bool)
{
	// TODO or not TODO? That is the question...
	m_ok = false;
}

void CSocks5StateMachine::process_receive_authentication_gssapi(bool)
{
	AddDummyEvent();
}

void CSocks5StateMachine::process_process_authentication_gssapi(bool)
{
	AddDummyEvent();
}

void CSocks5StateMachine::process_send_authentication_username_password(bool entry)
{
	if (entry) {
		unsigned char lenUser = m_proxyData.m_userName.Len();
		unsigned char lenPassword = m_proxyData.m_password.Len();
		m_packetLenght = 1 + 1 + lenUser + 1 + lenPassword;
		unsigned int offsetUser = 2;
		unsigned int offsetPassword = offsetUser + lenUser + 1;

		// Prepare username/password buffer
		m_buffer[0] = SOCKS5_AUTH_VERSION_USERNAME_PASSWORD;
		m_buffer[offsetUser-1] = lenUser;
		memcpy(m_buffer+offsetUser, unicode2char(m_proxyData.m_userName),
			lenUser);
		m_buffer[offsetPassword-1] = lenPassword;
		memcpy(m_buffer+offsetPassword, unicode2char(m_proxyData.m_password),
			lenPassword);

		// Send the username/password packet
		ProxyWrite(*m_proxyClientSocket, m_buffer, m_packetLenght);
	}
}

void CSocks5StateMachine::process_receive_authentication_username_password(bool entry)
{
	if (entry) {
		// Receive the server's authentication response
		m_packetLenght = 2;
		ProxyRead(*m_proxyClientSocket, m_buffer);
	}
	AddDummyEvent();
}

void CSocks5StateMachine::process_process_authentication_username_password(bool entry)
{
	if (entry) {
		// Process the server's reply
		m_lastReply = m_buffer[1];
		m_ok =	m_ok &&
			m_buffer[0] == SOCKS5_AUTH_VERSION_USERNAME_PASSWORD &&
			m_buffer[1] == SOCKS5_REPLY_SUCCEED;
	}
	AddDummyEvent();
}

void CSocks5StateMachine::process_send_command_request(bool entry)
{
	if (entry) {
		// Prepare the request command buffer
		m_buffer[0] = SOCKS5_VERSION;
		switch (m_proxyCommand) {
		case PROXY_CMD_CONNECT:
			m_buffer[1] = SOCKS5_CMD_CONNECT;
			break;

		case PROXY_CMD_BIND:
			m_buffer[1] = SOCKS5_CMD_BIND;
			break;

		case PROXY_CMD_UDP_ASSOCIATE:
			m_buffer[1] = SOCKS5_CMD_UDP_ASSOCIATE;
			break;
		}
		m_buffer[2] = SOCKS5_RSV;
		m_buffer[3] = SOCKS5_ATYP_IPV4_ADDRESS;
		PokeUInt32( m_buffer+4, StringIPtoUint32(m_peerAddress->IPAddress()) );
		RawPokeUInt16( m_buffer+8, ENDIAN_HTONS( m_peerAddress->Service() ) );

		// Send the command packet
		m_packetLenght = 10;
		ProxyWrite(*m_proxyClientSocket, m_buffer, m_packetLenght);
	}
}

void CSocks5StateMachine::process_receive_command_reply(bool entry)
{
	if (entry) {
		// The minimum number of bytes to read is 10 in the case of
		// ATYP == SOCKS5_ATYP_IPV4_ADDRESS
		m_packetLenght = 10;
		ProxyRead(*m_proxyClientSocket, m_buffer);
	}
	AddDummyEvent();
}

void CSocks5StateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		m_lastReply = m_buffer[1];
		unsigned char addressType = m_buffer[3];
		// Process the server's reply
		m_ok = m_ok &&
			m_buffer[0] == SOCKS5_VERSION &&
			m_buffer[1] == SOCKS5_REPLY_SUCCEED;
		if (m_ok) {
			// Read BND.ADDR
			unsigned int portOffset = 0;
			switch(addressType) {
			case SOCKS5_ATYP_IPV4_ADDRESS:
			{
				const unsigned int addrOffset = 4;
				portOffset = 8;
				m_proxyBoundAddressIPV4.Hostname( PeekUInt32( m_buffer+addrOffset) );
				m_proxyBoundAddress = &m_proxyBoundAddressIPV4;
				break;
			}
			case SOCKS5_ATYP_DOMAINNAME:
			{
				// Read the domain name
				const unsigned int addrOffset = 5;
				portOffset = 10 + m_buffer[4];
				char c = m_buffer[portOffset];
				m_buffer[portOffset] = 0;
				m_proxyBoundAddressIPV4.Hostname(
					char2unicode(m_buffer+addrOffset));
				m_proxyBoundAddress = &m_proxyBoundAddressIPV4;
				m_buffer[portOffset] = c;
				break;
			}
			case SOCKS5_ATYP_IPV6_ADDRESS:
			{
				portOffset = 20;
				// TODO
				// IPV6 not yet implemented in wx
				//m_proxyBoundAddress.Hostname(Uint128toStringIP(
				//	*((uint128 *)(m_buffer+addrOffset)) ));
				//m_proxyBoundAddress = &m_proxyBoundAddressIPV6;
				m_ok = false;
				break;
			}
			}
			// Set the packet length at last
			m_packetLenght = portOffset + 2;
			// Read BND.PORT
			m_proxyBoundAddress->Service( ENDIAN_NTOHS( RawPeekUInt16( m_buffer+portOffset) ) );
		}
	}
	AddDummyEvent();
}

//------------------------------------------------------------------------------
// CSocks4StateMachine
//------------------------------------------------------------------------------

CSocks4StateMachine::CSocks4StateMachine(
	const CProxyData &proxyData,
	CProxyCommand proxyCommand)
:
CProxyStateMachine(
	wxString(wxT("Socks4")), SOCKS4_MAX_STATES, proxyData, proxyCommand)
{
	m_process_state[0] = &CSocks4StateMachine::process_start;
	m_state_name[0] = wxT("process_start");
	m_process_state[1] = &CSocks4StateMachine::process_end;
	m_state_name[1] = wxT("process_end");
	m_process_state[2] = &CSocks4StateMachine::process_send_command_request;
	m_state_name[2] = wxT("process_send_command_request");
	m_process_state[3] = &CSocks4StateMachine::process_receive_command_reply;
	m_state_name[3] = wxT("process_receive_command_reply");
	m_process_state[4] = &CSocks4StateMachine::process_process_command_reply;
	m_state_name[4] = wxT("process_process_command_reply");
}

void CSocks4StateMachine::process_state(t_sm_state state, bool entry)
{
	(this->*m_process_state[state])(entry);
#ifdef __DEBUG__
	int n = 0;

	switch (state) {
	case SOCKS4_STATE_START:
	case SOCKS4_STATE_END:
	case SOCKS4_STATE_RECEIVE_COMMAND_REPLY:
	default:
		n = 0;
		break;

	case SOCKS4_STATE_SEND_COMMAND_REQUEST:
		n = m_packetLenght;
		break;

	case SOCKS4_STATE_PROCESS_COMMAND_REPLY:
		n = m_lastRead;
		break;
	}

	if (entry) {
		DumpMem(m_buffer, n, m_state_name[state], m_ok);
	} else {
		AddDebugLogLineN(logProxy,
			wxString(wxT("wait state -- ")) << m_state_name[state]);
	}
#endif // __DEBUG__
}

t_sm_state CSocks4StateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = HandleEvent(event);
	switch (GetState()) {
	case SOCKS4_STATE_START:
		if (m_isConnected && !m_isLost && CanSend()) {
			ret = SOCKS4_STATE_SEND_COMMAND_REQUEST;
		}
		break;

	case SOCKS4_STATE_SEND_COMMAND_REQUEST:
		if (m_ok) {
			if (CanReceive()) {
				ret = SOCKS4_STATE_RECEIVE_COMMAND_REPLY;
			}
		} else {
			ret = SOCKS4_STATE_END;
		}
		break;

	case SOCKS4_STATE_RECEIVE_COMMAND_REPLY:
		ret = SOCKS4_STATE_PROCESS_COMMAND_REPLY;
		break;

	case SOCKS4_STATE_PROCESS_COMMAND_REPLY:
		ret = SOCKS4_STATE_END;
		break;

	case SOCKS4_STATE_END:
	default:
		break;
	}

	return ret;
}

void CSocks4StateMachine::process_start(bool)
{}

void CSocks4StateMachine::process_end(bool)
{
	ReactivateSocket();
}

void CSocks4StateMachine::process_send_command_request(bool entry)
{
	if (entry) {
		// Prepare the request command buffer
		m_buffer[0] = SOCKS4_VERSION;
		switch (m_proxyCommand) {
		case PROXY_CMD_CONNECT:
			m_buffer[1] = SOCKS4_CMD_CONNECT;
			break;

		case PROXY_CMD_BIND:
			m_buffer[1] = SOCKS4_CMD_BIND;
			break;

		case PROXY_CMD_UDP_ASSOCIATE:
			m_ok = false;
			return;
			break;
		}
		RawPokeUInt16(m_buffer+2, ENDIAN_HTONS(m_peerAddress->Service()));
		// Special processing for SOCKS4a
		switch (m_proxyData.m_proxyType) {
		case PROXY_SOCKS4a:
			PokeUInt32(m_buffer+4, StringIPtoUint32(wxT("0.0.0.1")));
			break;
		case PROXY_SOCKS4:
		default:
			PokeUInt32(m_buffer+4, StringIPtoUint32(m_peerAddress->IPAddress()));
			break;
		}
		// Common processing for SOCKS4/SOCKS4a
		unsigned int offsetUser = 8;
		unsigned char lenUser = m_proxyData.m_userName.Len();
		memcpy(m_buffer + offsetUser,
			unicode2char(m_proxyData.m_userName), lenUser);
		m_buffer[offsetUser + lenUser] = 0;
		// Special processing for SOCKS4a
		switch (m_proxyData.m_proxyType) {
		case PROXY_SOCKS4a: {
			unsigned int offsetDomain = offsetUser + lenUser + 1;
			// The Hostname() method was used here before, but I don't see why we can't use 
			// the IP address which we actually know instead here.
			wxString hostname(m_peerAddress->IPAddress());
			unsigned char lenDomain = hostname.Len();
			memcpy(m_buffer + offsetDomain,
				unicode2char(hostname), lenDomain);
			m_buffer[offsetDomain + lenDomain] = 0;
			m_packetLenght = 1 + 1 + 2 + 4 + lenUser + 1 + lenDomain + 1;
			break;
		}
		case PROXY_SOCKS4:
		default:
			m_packetLenght = 1 + 1 + 2 + 4 + lenUser + 1;
			break;
		}
		// Send the command packet
		ProxyWrite(*m_proxyClientSocket, m_buffer, m_packetLenght);
	}
}

void CSocks4StateMachine::process_receive_command_reply(bool entry)
{
	if (entry) {
		// Receive the server's reply
		m_packetLenght = 8;
		ProxyRead(*m_proxyClientSocket, m_buffer);
	}
	AddDummyEvent();
}

void CSocks4StateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		m_lastReply = m_buffer[1];

		// Process the server's reply
		m_ok = m_ok &&
			m_buffer[0] == SOCKS4_REPLY_CODE &&
			m_buffer[1] == SOCKS4_REPLY_GRANTED;
		if (m_ok) {
			// Read BND.PORT
			const unsigned int portOffset = 2;
			m_ok = m_proxyBoundAddressIPV4.Service(ENDIAN_NTOHS(
				RawPeekUInt16( m_buffer+portOffset) ) );
			// Read BND.ADDR
			const unsigned int addrOffset = 4;
			m_ok = m_ok &&
				m_proxyBoundAddressIPV4.Hostname( PeekUInt32( m_buffer+addrOffset ) );
			m_proxyBoundAddress = &m_proxyBoundAddressIPV4;
		}
	}
	AddDummyEvent();
}

//------------------------------------------------------------------------------
// CHttpStateMachine
//------------------------------------------------------------------------------

CHttpStateMachine::CHttpStateMachine(
	const CProxyData &proxyData,
	CProxyCommand proxyCommand)
:
CProxyStateMachine(
	wxString(wxT("Http")), HTTP_MAX_STATES, proxyData, proxyCommand)
{
	m_process_state[0] = &CHttpStateMachine::process_start;
	m_state_name[0] = wxT("process_start");
	m_process_state[1] = &CHttpStateMachine::process_end;
	m_state_name[1] = wxT("process_end");
	m_process_state[2] = &CHttpStateMachine::process_send_command_request;
	m_state_name[2] = wxT("process_send_command_request");
	m_process_state[3] = &CHttpStateMachine::process_receive_command_reply;
	m_state_name[3] = wxT("process_receive_command_reply");
	m_process_state[4] = &CHttpStateMachine::process_process_command_reply;
	m_state_name[4] = wxT("process_process_command_reply");
}

void CHttpStateMachine::process_state(t_sm_state state, bool entry)
{
	(this->*m_process_state[state])(entry);
#ifdef __DEBUG__
	int n = 0;

	switch (state) {
	case HTTP_STATE_START:
	case HTTP_STATE_END:
	case HTTP_STATE_RECEIVE_COMMAND_REPLY:
	default:
		n = 0;
		break;

	case HTTP_STATE_SEND_COMMAND_REQUEST:
		n = m_packetLenght;
		break;

	case HTTP_STATE_PROCESS_COMMAND_REPLY:
		n = m_lastRead;
		break;
	}

	if (entry) {
		DumpMem(m_buffer, n, m_state_name[state], m_ok);
	} else {
		AddDebugLogLineN(logProxy,
			wxString(wxT("wait state -- ")) << m_state_name[state]);
	}
#endif // __DEBUG__
}

t_sm_state CHttpStateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = HandleEvent(event);
	switch (GetState()) {
	case HTTP_STATE_START:
		if (m_isConnected && !m_isLost && CanSend()) {
			ret = HTTP_STATE_SEND_COMMAND_REQUEST;
		}
		break;

	case HTTP_STATE_SEND_COMMAND_REQUEST:
		if (m_ok) {
			if (CanReceive()) {
				ret = HTTP_STATE_RECEIVE_COMMAND_REPLY;
			}
		} else {
			ret = HTTP_STATE_END;
		}
		break;

	case HTTP_STATE_RECEIVE_COMMAND_REPLY:
		ret = HTTP_STATE_PROCESS_COMMAND_REPLY;
		break;

	case HTTP_STATE_PROCESS_COMMAND_REPLY:
		ret = HTTP_STATE_END;
		break;

	case HTTP_STATE_END:
	default:
		break;
	}

	return ret;
}

void CHttpStateMachine::process_start(bool)
{}

void CHttpStateMachine::process_end(bool)
{
	ReactivateSocket();
}

void CHttpStateMachine::process_send_command_request(bool entry)
{
	if (entry) {
		// Prepare the request command buffer
		wxString ip = m_peerAddress->IPAddress();
		uint16 port = m_peerAddress->Service();
		wxString userPass;
		wxString userPassEncoded;
		if (m_proxyData.m_enablePassword) {
			userPass = m_proxyData.m_userName + wxT(":") + m_proxyData.m_password;
			userPassEncoded =
				EncodeBase64(unicode2char(userPass), userPass.Length());
		}
		wxString msg;

		switch (m_proxyCommand) {
		case PROXY_CMD_CONNECT:
			msg <<
			wxT("CONNECT ") << ip << wxT(":") << port << wxT(" HTTP/1.1\r\n") <<
			wxT("Host: ")   << ip << wxT(":") << port << wxT("\r\n");
			if (m_proxyData.m_enablePassword) {
				msg <<
				wxT("Authorization: Basic ")       << userPassEncoded << wxT("\r\n") <<
				wxT("Proxy-Authorization: Basic ") << userPassEncoded << wxT("\r\n");
			}
			msg << wxT("\r\n");
			break;

		case PROXY_CMD_BIND:
			m_ok = false;
			break;

		case PROXY_CMD_UDP_ASSOCIATE:
			m_ok = false;
			return;
			break;
		}
		// Send the command packet
		m_packetLenght = msg.Len();
		memcpy(m_buffer, unicode2char(msg), m_packetLenght+1);
		ProxyWrite(*m_proxyClientSocket, m_buffer, m_packetLenght);
	}
}

void CHttpStateMachine::process_receive_command_reply(bool entry)
{
	if (entry) {
		// Receive the server's reply -- Use a large number, but don't
		// Expect to get it all. HTTP protocol does not have a fixed length.
		m_packetLenght = PROXY_BUFFER_SIZE;
		ProxyRead(*m_proxyClientSocket, m_buffer);
	}
	AddDummyEvent();
}

/*
 * HTTP Proxy server response should be something like:
 * "HTTP/1.1 200 Connection established\r\n\r\n"
 * but that may vary. The important thing is the "200"
 * code, that means success.
 */
static const char HTTP_AUTH_RESPONSE[] = "HTTP/";
static const int  HTTP_AUTH_RESPONSE_LENGHT = strlen(HTTP_AUTH_RESPONSE);
void CHttpStateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		// The position of the first space in the buffer
		int i = 8;
		while (m_buffer[i] == ' ') {
			i++;
		}
		// Process the server's reply
		m_ok =	!memcmp(m_buffer + 0, HTTP_AUTH_RESPONSE, HTTP_AUTH_RESPONSE_LENGHT) &&
			!memcmp(m_buffer + i, "200", 3);
	}
	AddDummyEvent();
}

//------------------------------------------------------------------------------
// CProxySocket
//------------------------------------------------------------------------------

CProxySocket::CProxySocket(
	muleSocketFlags flags,
	const CProxyData *proxyData,
	CProxyCommand proxyCommand,
	CDatagramSocketProxy *udpSocket)
:
CLibSocket(flags),
m_proxyStateMachine(NULL),
m_udpSocket(udpSocket),
m_socketEventHandler(NULL),
m_socketEventHandlerId(0),
m_savedSocketEventHandler(NULL),
m_savedSocketEventHandlerId(0)
{
	SetProxyData(proxyData);
	if (m_useProxy) {
		switch (m_proxyData.m_proxyType) {
		case PROXY_NONE:
			break;

		case PROXY_SOCKS5:
			m_proxyStateMachine =
				new CSocks5StateMachine(*proxyData, proxyCommand);
			break;

		case PROXY_SOCKS4:
		case PROXY_SOCKS4a:
			m_proxyStateMachine =
				new CSocks4StateMachine(*proxyData, proxyCommand);
			break;

		case PROXY_HTTP:
			m_proxyStateMachine =
				new CHttpStateMachine(*proxyData, proxyCommand);
			break;

		default:
			break;
		}
	}
}

CProxySocket::~CProxySocket()
{
	delete m_proxyStateMachine;
}

void CProxySocket::SetProxyData(const CProxyData *proxyData)
{
	m_useProxy = proxyData != NULL && proxyData->m_proxyEnable;
	if (proxyData) {
		m_proxyData = *proxyData;
		m_proxyAddress.Hostname(m_proxyData.m_proxyHostName);
		m_proxyAddress.Service(m_proxyData.m_proxyPort);
	} else {
		m_proxyData.Clear();
	}
}

bool CProxySocket::Start(const amuleIPV4Address &peerAddress)
{
#ifdef ASIO_SOCKETS
	SetProxyState(true, &peerAddress);
#else
	SaveState();
	// Important note! SaveState()/RestoreState() DO NOT save/restore
	// the event handler. The method SaveEventHandler() has been created
	// for that.
	SaveEventHandler();
	SetEventHandler(g_proxyEventHandler, ID_PROXY_SOCKET_EVENT);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);
#endif
	Connect(m_proxyAddress, false);
	SetFlags(MULE_SOCKET_NONE);
	return m_proxyStateMachine->Start(peerAddress, this);
}

bool CProxySocket::ProxyIsCapableOf(CProxyCommand proxyCommand) const
{
	bool ret = false;

	switch (m_proxyData.m_proxyType) {
	case PROXY_NONE:
		ret = false;
		break;

	case PROXY_SOCKS5:
		ret =	proxyCommand == PROXY_CMD_CONNECT ||
			proxyCommand == PROXY_CMD_BIND ||
			proxyCommand == PROXY_CMD_UDP_ASSOCIATE;
		break;

	case PROXY_SOCKS4:
	case PROXY_SOCKS4a:
		ret =	proxyCommand == PROXY_CMD_CONNECT ||
			proxyCommand == PROXY_CMD_BIND;
		break;

	case PROXY_HTTP:
		ret =	proxyCommand == PROXY_CMD_CONNECT;
		break;
	}

	return ret;
}

//------------------------------------------------------------------------------
// CSocketClientProxy
//------------------------------------------------------------------------------

CSocketClientProxy::CSocketClientProxy(
	muleSocketFlags flags,
	const CProxyData *proxyData)
:
CProxySocket(flags, proxyData, PROXY_CMD_CONNECT)
{
}

bool CSocketClientProxy::Connect(amuleIPV4Address &address, bool wait)
{
	wxMutexLocker lock(m_socketLocker);
	bool ok;

	if (GetUseProxy() && ProxyIsCapableOf(PROXY_CMD_CONNECT)) {
		ok = Start(address);
	} else {
		ok = CLibSocket::Connect(address, wait);
	}

	return ok;
}

uint32 CSocketClientProxy::Read(void *buffer, wxUint32 nbytes)
{
	wxMutexLocker lock(m_socketLocker);
	return CProxySocket::Read(buffer, nbytes);
}

uint32 CSocketClientProxy::Write(const void *buffer, wxUint32 nbytes)
{
	wxMutexLocker lock(m_socketLocker);
	return CProxySocket::Write(buffer, nbytes);
}

//------------------------------------------------------------------------------
// CSocketServerProxy
//------------------------------------------------------------------------------

CSocketServerProxy::CSocketServerProxy(
	amuleIPV4Address &address,
	muleSocketFlags flags,
	const CProxyData *)
:
CLibSocketServer(address, flags)
{
	/* Maybe some day when socks6 is out... :) */
}

//------------------------------------------------------------------------------
// CDatagramSocketProxy
//------------------------------------------------------------------------------

CDatagramSocketProxy::CDatagramSocketProxy(
	amuleIPV4Address &address, muleSocketFlags flags, const CProxyData *proxyData)
:
CLibUDPSocket(address, flags),
m_proxyTCPSocket(MULE_SOCKET_NOWAIT, proxyData, PROXY_CMD_UDP_ASSOCIATE, this)
{
	m_udpSocketOk = false;
	if (	m_proxyTCPSocket.GetUseProxy() &&
		m_proxyTCPSocket.ProxyIsCapableOf(PROXY_CMD_UDP_ASSOCIATE)) {
			m_proxyTCPSocket.Start(address);
	} else {
	}
	m_lastUDPOperation = UDP_OPERATION_NONE;
}

CDatagramSocketProxy::~CDatagramSocketProxy()
{
	// From RFC-1928:
	// "A UDP association terminates when the TCP connection that the
	// UDP ASSOCIATE request arrived terminates."
}

uint32 CDatagramSocketProxy::RecvFrom(amuleIPV4Address& addr, void* buf, uint32 nBytes)
{
	uint32 read = 0;
	wxMutexLocker lock(m_socketLocker);
	m_lastUDPOperation = UDP_OPERATION_RECV_FROM;
	if (m_proxyTCPSocket.GetUseProxy()) {
		if (m_udpSocketOk) {
			char *bufUDP = NULL;
			if (nBytes + PROXY_UDP_MAXIMUM_OVERHEAD > PROXY_BUFFER_SIZE) {
				bufUDP = new char[nBytes + PROXY_UDP_MAXIMUM_OVERHEAD];
			} else {
				bufUDP = m_proxyTCPSocket.GetBuffer();
			}
			read = CLibUDPSocket::RecvFrom(
				m_proxyTCPSocket.GetProxyBoundAddress(),
				bufUDP, nBytes + PROXY_UDP_MAXIMUM_OVERHEAD);
			unsigned int offset;
			switch (m_proxyTCPSocket.GetBuffer()[3]) {
			case SOCKS5_ATYP_IPV4_ADDRESS: {
				offset = PROXY_UDP_OVERHEAD_IPV4;
				try {
					amuleIPV4Address &a = dynamic_cast<amuleIPV4Address &>(addr);
					a.Hostname( PeekUInt32( m_proxyTCPSocket.GetBuffer()+4 ) );
					a.Service( ENDIAN_NTOHS( RawPeekUInt16( m_proxyTCPSocket.GetBuffer()+8) ) );
				} catch (const std::bad_cast& WXUNUSED(e)) {
					AddDebugLogLineN(logProxy,
						wxT("(2)bad_cast exception!"));
					wxFAIL;
				}
			}
				break;

			case SOCKS5_ATYP_DOMAINNAME:
				offset = PROXY_UDP_OVERHEAD_DOMAIN_NAME;
				break;

			case SOCKS5_ATYP_IPV6_ADDRESS:
				offset = PROXY_UDP_OVERHEAD_IPV6;
				break;

			default:
				/* Error! */
				offset = 0;
				break;
			}
			memcpy(buf, bufUDP + offset, nBytes);
			// Uncomment here to see the buffer contents on console
			// DumpMem(bufUDP, wxDatagramSocket::LastCount(), wxT("RecvFrom"), 3);

			/* Only delete buffer if it was dynamically created */
			if (bufUDP != m_proxyTCPSocket.GetBuffer()) {
				/* We should use a fixed buffer to avoid
				 * new/delete it all the time.
				 * I need an upper bound */
				delete [] bufUDP;
			}
			/* There is still one problem pending, fragmentation.
			 * Either we support it or we have to drop fragmented
			 * messages. I vote for drop :)
			 */
		}
	} else {
		read = CLibUDPSocket::RecvFrom(addr, buf, nBytes);
	}

	return read;
}

uint32 CDatagramSocketProxy::SendTo(const amuleIPV4Address& addr, const void* buf, uint32 nBytes)
{
	uint32 sent = 0;
	wxMutexLocker lock(m_socketLocker);
	m_lastUDPOperation = UDP_OPERATION_SEND_TO;
	m_lastUDPOverhead = PROXY_UDP_OVERHEAD_IPV4;
	if (m_proxyTCPSocket.GetUseProxy()) {
		if (m_udpSocketOk) {
			m_proxyTCPSocket.GetBuffer()[0] = SOCKS5_RSV;	// Reserved
			m_proxyTCPSocket.GetBuffer()[1] = SOCKS5_RSV;	// Reserved
			m_proxyTCPSocket.GetBuffer()[2] = 0;		// FRAG
			m_proxyTCPSocket.GetBuffer()[3] = SOCKS5_ATYP_IPV4_ADDRESS;
			PokeUInt32( m_proxyTCPSocket.GetBuffer()+4, StringIPtoUint32(addr.IPAddress()));
			RawPokeUInt16( m_proxyTCPSocket.GetBuffer()+8, ENDIAN_HTONS( addr.Service() ) );
			memcpy(m_proxyTCPSocket.GetBuffer() + PROXY_UDP_OVERHEAD_IPV4, buf, nBytes);
			nBytes += PROXY_UDP_OVERHEAD_IPV4;
			sent = CLibUDPSocket::SendTo(
				m_proxyTCPSocket.GetProxyBoundAddress(),
				m_proxyTCPSocket.GetBuffer(), nBytes);
			// Uncomment here to see the buffer contents on console
			// DumpMem(m_proxyTCPSocket.GetBuffer(), nBytes, wxT("SendTo"), 3);
		}
	} else {
		sent = CLibUDPSocket::SendTo(addr, buf, nBytes);
	}

	return sent;
}

#endif // CLIENT_GUI

/******************************************************************************/
// File_checked_for_headers

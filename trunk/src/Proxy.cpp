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

/******************************************************************************/

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "Proxy.h"
#endif

#include "Proxy.h"		/* for Interface		*/


#include <typeinfo>		/* For bad_cast			*/


#include <netinet/in.h>		/* for htons()			*/

#include "amule.h"		/* Needed for wxGetApp		*/
#include "opcodes.h"		/* for PROXY_SOCKET_HANDLER	*/
#include "NetworkFunctions.h"	/* for CStringIPtoUint32()	*/
#include "otherfunctions.h"	/* for EncodeBase64()		*/


/**
 * Dumps a buffer to stdout
 */
static void dump(char *msg, bool ok, const void *buff, int n)
{
	register const unsigned char *p = (const unsigned char *)buff;
	register int lines = (n + 15)/ 16;
	register int chars = 0;
	if (msg) {
		printf("%s - ok=%d, %d bytes\n", msg, ok, n);
	}
	for( int i = 0; i < lines; ++i) {
		int chars_save = chars;
		int j;
		// Prints the hexadecimal codes
		for( j = 0; j < 16 && chars < n; ++j) {
			printf("%02X ", p[chars++]);
		}
		// Completes the missing spaces
		for( int k = j; k < 16; ++k)
		{
			printf("   ");
		}
		// Rewind and print the ASCII codes
		chars = chars_save;
		printf("|");
		for( j = 0; j < 16 && chars < n; ++j) {
			char l = p[chars++];
			if (isspace(l)) {
				l = ' ';
			} else if (!isgraph(l)) {
				l = '.';
			}
			printf("%c", l);
		}
		// Completes the missing spaces
		for( int k = j; k < 16; ++k)
		{
			printf(" ");
		}
		printf("|\n");
	}
	printf("\n");
}

//------------------------------------------------------------------------------
// wxProxyData
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// ProxyEventHandler
//------------------------------------------------------------------------------

ProxyEventHandler::ProxyEventHandler()
{
}

#ifndef AMULE_DAEMON
BEGIN_EVENT_TABLE(ProxyEventHandler, wxEvtHandler)
	EVT_SOCKET(PROXY_SOCKET_HANDLER, ProxyEventHandler::ProxySocketHandler)
END_EVENT_TABLE()

//
// THE one and only Event Handler
//
static ProxyEventHandler TheProxyEventHandler;

void ProxyEventHandler::ProxySocketHandler(wxSocketEvent& event)
{
	amuleProxyClientSocket *sock = dynamic_cast<amuleProxyClientSocket *>(event.GetSocket());
	if (sock) {
		sock->m_ProxyStateMachine->Schedule(event.GetSocketEvent());
	} else {
		// we're doomed :)
	}
	sock->m_ProxyStateMachine->Clock();
}

#else
// TODO -- amuled
ProxyEventHandler::~ProxyEventHandler()
{
}

void *ProxyEventHandler::Entry()
{
	return NULL;
}
#endif

//------------------------------------------------------------------------------
// ProxyStateMachine
//------------------------------------------------------------------------------

ProxyStateMachine::ProxyStateMachine(
		wxString name,
		const unsigned int max_states,
		const wxProxyData &ProxyData,
		wxProxyCommand ProxyCommand)
:
StateMachine(NewName(name, ProxyCommand), max_states, PROXY_STATE_START),
m_ProxyData(ProxyData),
m_ProxyCommand(ProxyCommand)
{
	m_IsLost = false;
	m_IsConnected = false;
	m_CanReceive = false;
	m_CanSend = false;
	m_ok = true;
	m_LastRead = 0;
	m_LastWritten = 0;
	// Will be initialized at Start()
	m_PeerAddress = NULL;
	m_ProxyClientSocket = NULL;
	m_ProxyBoundAddress = NULL;
	// Temporary variables
	m_LastReply = 0;
	m_PacketLenght = 0;
}

ProxyStateMachine::~ProxyStateMachine()
{
	delete m_PeerAddress;
}

wxString &ProxyStateMachine::NewName(wxString &s, wxProxyCommand ProxyCommand)
{
	switch (ProxyCommand) {
		case wxPROXY_CMD_CONNECT:
			s += wxT("-CONNECT");
			break;
			
		case wxPROXY_CMD_BIND:
			s += wxT("-BIND");
			break;
			
		case wxPROXY_CMD_UDP_ASSOCIATE:
			s += wxT("-UDP");
			break;
	}
	
	return s;
}

bool ProxyStateMachine::Start(const wxIPaddress &PeerAddress, wxSocketClient *ProxyClientSocket)
{
	m_ProxyClientSocket = ProxyClientSocket;
	try {
		const wxIPV4address &peer = dynamic_cast<const wxIPV4address &>(PeerAddress);
		m_PeerAddress = new amuleIPV4Address(peer);
	} catch (std::bad_cast e) {
		// Should process other types of wxIPAddres before quitting
		printf("(1)bad_cast exception!\n");
		wxASSERT(false);
		return false;
	}
	
#if 0
	// Debug message
	printf("amuleProxySocket::Start\nHostname Orig:%s, IPAddr:%s, Port:%d\n",
		unicode2char(m_PeerAddress->Hostname()),
		unicode2char(m_PeerAddress->IPAddress()),
		m_PeerAddress->Service());
#endif
	
	// To run the state machine, return and just let the events start to happen.
	return true;
}

t_sm_state ProxyStateMachine::HandleEvent(t_sm_event event)
{
	// Default is stay in current state	
	t_sm_state ret = GetState();
	switch(event)
	{
	case wxSOCKET_CONNECTION:
		printf("Connection event\n");
		m_IsConnected = true;
		break;
		
	case wxSOCKET_INPUT:
		printf("Input event\n");
		m_CanReceive = true;
		break;
		
	case wxSOCKET_OUTPUT:
		printf("Output event\n");
		m_CanSend = true;
		break;
		
	case wxSOCKET_LOST:
		printf("Lost connection event\n");
		m_IsLost = true;
		m_ok = false;
		break;
		
	default:
		printf("Unknown event\n");
		break;
	}
	
	// Aborting conditions:
	// - wxSOCKET_LOST event
	// - More than 10 times on the same state
	if (	m_IsLost ||
		GetClocksInCurrentState() > 10) {
		ret = PROXY_STATE_END;
	}
	
	return ret;
}

void ProxyStateMachine::AddDummyEvent()
{
#ifndef AMULE_DAEMON
	wxSocketEvent e(PROXY_SOCKET_HANDLER);
	// Make sure this is an unknown event :)
	e.m_event = (wxSocketNotify)(
		wxSOCKET_INPUT + wxSOCKET_OUTPUT +
		wxSOCKET_CONNECTION + wxSOCKET_LOST);
	e.SetEventObject(m_ProxyClientSocket);
	TheProxyEventHandler.AddPendingEvent(e);
#endif
}

/*
 * Notice! These includes are here as long as it is impossible to retrieve 
 * the event handler from the socket. They should be removed. For now,
 * please leave it here.
 */
#include "ListenSocket.h"	// For CClientReqSocketHandler
#include "ServerSocket.h"	// For CServerSocketHandler

void ProxyStateMachine::ReactivateSocket()
{
#if 0
	// Debug message
	if (m_ok) {
		if (m_ProxyBoundAddress) {
			printf("Proxy Bound Address: IP:%s, Port:%u, ok:%d\n",
				unicode2char(GetProxyBoundAddress().IPAddress()),
				GetProxyBoundAddress().Service(), m_ok);
		} else {
			printf("Failed to bind proxy address (ok for http proxy), ok=%d\n", m_ok);
		}
	} else {
		printf("Proxy request failed, ok=%d\n", m_ok);
	}
#endif
	
#ifndef AMULE_DAEMON
	/*    If proxy is beeing used, then the TCP socket handlers 
	 * (CServerSocketHandler and CClientReqSocketHandler) will not
	 * receive a wxSOCKET_CONNECTION event, because the connection has
	 * already started with the proxy. So we must add a wxSOCKET_CONNECTION
	 * event to make things go undetected. A wxSOCKET_OUTPUT event is also
	 * necessary to start sending data to the server. */
	 
	/*    If the wxSocket had a GetEventHandler() method, this could be 
	 * much more cleaner. All this fuzz with dynamic_cast<>() is because
	 * the GetEventHandler() method had to be implemented in CClientReqSocket
	 * and CServerSocket, and both are aMule classes. Maybe we can add this
	 * method to a common ancestor?
	 *    Another possibility (for wxWidgets) would be if 
	 * SaveState()/RestoreState() also saved/restored the event handler, 
	 * which is currently not the case, and will probably never be, because
	 * most certainly will break wxWidgets code compatibility. */
	wxSocketEvent e(SERVERSOCKET_HANDLER);
	e.m_event = wxSOCKET_CONNECTION;
	e.SetEventObject(m_ProxyClientSocket);
	if (CClientReqSocket *s1 =
		dynamic_cast<CClientReqSocket *>(m_ProxyClientSocket)) {
		CClientReqSocketHandler *h = s1->GetEventHandler();
		m_ProxyClientSocket->SetEventHandler(*h, CLIENTREQSOCKET_HANDLER);
		h->AddPendingEvent(e);
		e.m_event = wxSOCKET_OUTPUT;
		h->AddPendingEvent(e);
		if (!m_ok) {
			e.m_event = wxSOCKET_LOST;
			h->AddPendingEvent(e);
		}
		s1->RestoreState();
	} else if (CServerSocket *s2 =
		dynamic_cast<CServerSocket *>(m_ProxyClientSocket)) {
		CServerSocketHandler *h = s2->GetEventHandler();
		m_ProxyClientSocket->SetEventHandler(*h, SERVERSOCKET_HANDLER);
		h->AddPendingEvent(e);
		e.m_event = wxSOCKET_OUTPUT;
		h->AddPendingEvent(e);
		if (!m_ok) {
			e.m_event = wxSOCKET_LOST;
			h->AddPendingEvent(e);
		}
		s2->RestoreState();
	} else if (amuleProxyClientSocket *s3 = 
		dynamic_cast<amuleProxyClientSocket *>(m_ProxyClientSocket)) {
		// If the socket was not of the types above, then we assume
		// we are in the UDP socket case.
		wxASSERT(m_ProxyCommand == wxPROXY_CMD_UDP_ASSOCIATE);
		wxDatagramSocketProxy *udp = s3->GetUDPSocket();
		if (udp) {
			if(m_ok) {
				// From now on, the UDP socket can be used,
				// remove the protection.
				udp->SetUDPSocketOk();
			}
		}
		// No need to call RestoreState(), that socket will no longer
		// be used after proxy negotiation.
	} else {
		wxASSERT(false);
	}
#endif
}

wxSocketBase &ProxyStateMachine::ProxyWrite(wxSocketBase &socket, const void *buffer, wxUint32 nbytes)
{
	wxSocketBase &ret = socket.Write(buffer, nbytes);
	m_LastWritten = m_ProxyClientSocket->LastCount();
	/* Set the status of this operation */
	m_LastError = wxSOCKET_NOERROR;
	m_ok = !m_ProxyClientSocket->Error();
	if (!m_ok) {
		m_LastError = m_ProxyClientSocket->LastError();
		m_ok = m_LastError == wxSOCKET_WOULDBLOCK;
		if (m_ok) {
			m_CanSend = false;
		}
	}
	
	return ret;
}

wxSocketBase &ProxyStateMachine::ProxyRead(wxSocketBase &socket, void *buffer)
{
	/* Always try to read the full buffer. That explicitly demands that
	 * the socket has the flag wxSOCKET_NONE. */
	wxSocketBase &ret = socket.Read(buffer, wxPROXY_BUFFER_SIZE);
	m_LastRead = m_ProxyClientSocket->LastCount();
	/* Set the status of this operation */
	m_LastError = wxSOCKET_NOERROR;
	m_ok = !m_ProxyClientSocket->Error();
	if (!m_ok) {
		m_LastError = m_ProxyClientSocket->LastError();
		m_ok = m_LastError == wxSOCKET_WOULDBLOCK;
		if (m_ok) {
			m_CanReceive = false;
		}
	}
	
	return ret;
}

//------------------------------------------------------------------------------
// Socks5StateMachine
//------------------------------------------------------------------------------

/**
 * The state machine constructor must initialize the array of pointer to member
 * functions. Don't waste you time trying to statically initialize this, pointer
 * to member functions require an object to operate on, so this array must be
 * initialized at run time.
 */
Socks5StateMachine::Socks5StateMachine(
	const wxProxyData &ProxyData,
	wxProxyCommand ProxyCommand)
:
ProxyStateMachine(
	wxString(wxT("Socks5")), SOCKS5_MAX_STATES, ProxyData, ProxyCommand)
{
	m_process_state[ 0] = &Socks5StateMachine::process_start;
	m_process_state[ 1] = &Socks5StateMachine::process_end;
	m_process_state[ 2] = &Socks5StateMachine::process_send_query_authentication_method;
	m_process_state[ 3] = &Socks5StateMachine::process_receive_authentication_method;
	m_process_state[ 4] = &Socks5StateMachine::process_process_authentication_method;
	m_process_state[ 5] = &Socks5StateMachine::process_send_authentication_gssapi;
	m_process_state[ 6] = &Socks5StateMachine::process_receive_authentication_gssapi;
	m_process_state[ 7] = &Socks5StateMachine::process_process_authentication_gssapi;
	m_process_state[ 8] = &Socks5StateMachine::process_send_authentication_username_password;
	m_process_state[ 9] = &Socks5StateMachine::process_receive_authentication_username_password;
	m_process_state[10] = &Socks5StateMachine::process_process_authentication_username_password;
	m_process_state[11] = &Socks5StateMachine::process_send_command_request;
	m_process_state[12] = &Socks5StateMachine::process_receive_command_reply;
	m_process_state[13] = &Socks5StateMachine::process_process_command_reply;
}

void Socks5StateMachine::process_state(t_sm_state state, bool entry)
{
	/* Ok, the syntax is terrible, but this is correct. This is a
	 * pointer to a member function. No C equivalent for that. */
	(this->*m_process_state[state])(entry);
}

/**
 * Code this such that the next state is only entered when it is able to
 * perform the operation (read or write). State processing will assume
 * that it can read or write upon entry of the state. This is done using
 * CanSend() and CanReceive(). On daemon, these functions always return
 * true, because sockets are blocking.
 */
t_sm_state Socks5StateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = HandleEvent(event);
	switch (GetState()) {
	case SOCKS5_STATE_START:
		if (m_IsConnected && !m_IsLost && CanSend()) {
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
				switch (m_LastReply) {
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
				printf("Cant send\n");
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
 * 	So, this is how you do it: the state machine is clocked by the events
 * that happen inside the event handler. You can add a dummy event whenever
 * you see that the system will not generate an event. But don't add dummy
 * events before reads, reads should only be performed after input events.
 *
 * 	Maybe it makes sense to add a dummy event before a read if there is no
 * state change (wait state).
 *
 * 	The event system will generate at least 2 events, one wxSOCKET_CONNECTION,
 * one wxSOCKET_OUTPUT, so we will have 2 clocks in our state machine. Plus, each
 * time there is unread data in the receive buffer of the socket, a wxSOCKET_INPUT
 * event will be generated. If you feel you will need more clocks than these, use
 * AddDummyEvent(), but I suggest you review your state machine design first.
 */
void Socks5StateMachine::process_start(bool entry)
{
	if (entry) {
dump("process_start", m_ok, NULL, 0);
	} else {
printf("wait state -- process_start\n");
	}
}

void Socks5StateMachine::process_end(bool)
{
	ReactivateSocket();
dump("process_end", m_ok, NULL, 0);
}

void Socks5StateMachine::process_send_query_authentication_method(bool entry)
{
	if (entry) {
		// Prepare the authentication method negotiation packet
		m_buffer[0] = SOCKS5_VERSION;
		m_buffer[1] = 2; // Number of supported methods
		//m_buffer[1] = 3; // Number of supported methods
		m_buffer[2] = SOCKS5_AUTH_METHOD_NO_AUTH_REQUIRED;
		m_buffer[3] = SOCKS5_AUTH_METHOD_USERNAME_PASSWORD;
		m_buffer[4] = SOCKS5_AUTH_METHOD_GSSAPI;
		m_PacketLenght = 4;
		//m_PacketLenght = 5;
		
		// Send the authentication method negotiation packet
		ProxyWrite(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_send_query_authentication_method", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_send_query_authentication_method\n");
	}
}

void Socks5StateMachine::process_receive_authentication_method(bool entry)
{
	if (entry) {
		// Receive the method selection message
		m_PacketLenght = 2;
		ProxyRead(*m_ProxyClientSocket, m_buffer);
dump("process_receive_authentication_method", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_authentication_method\n");
	}
	/* This is added because there will be no more input events. If the 
	 * world was a nice place, we could think about joining the
	 * process_receive and the process_process states here, but some day
	 * we might have to deal with the fact that the i/o operation has been
	 * incomplete, and that we must finish our job the next time we enter
	 * this state. */
	AddDummyEvent();
}

void Socks5StateMachine::process_process_authentication_method(bool entry)
{
	if (entry) {
		m_LastReply = m_buffer[1];
		m_ok = m_ok && m_buffer[0] == SOCKS5_VERSION;
dump("process_process_authentication_method", m_ok, m_buffer, m_LastRead);
	} else {
printf("wait state -- process_receive_authentication_method\n");
	}
	/* Ok, this one is here because wxSOCKET_OUTPUT events only happen
	 * once when you connect the socket, and after that, only after a
	 * wxSOCKET_WOULDBLOCK error happens. */
	AddDummyEvent();
}

void Socks5StateMachine::process_send_authentication_gssapi(bool)
{
	// TODO or not TODO? That is the question...
	m_ok = false;
}

void Socks5StateMachine::process_receive_authentication_gssapi(bool)
{
	AddDummyEvent();
}

void Socks5StateMachine::process_process_authentication_gssapi(bool)
{
	AddDummyEvent();
}

void Socks5StateMachine::process_send_authentication_username_password(bool entry)
{
	if (entry) {
		unsigned char LenUser = m_ProxyData.m_UserName.Len();
		unsigned char LenPassword = m_ProxyData.m_Password.Len();
		m_PacketLenght = 1 + 1 + LenUser + 1 + LenPassword;
		unsigned int OffsetUser = 2;
		unsigned int OffsetPassword = OffsetUser + LenUser + 1;
		
		// Prepare username/password buffer
		m_buffer[0] = SOCKS5_AUTH_VERSION_USERNAME_PASSWORD;
		m_buffer[OffsetUser-1] = LenUser;
		memcpy(m_buffer+OffsetUser, unicode2char(m_ProxyData.m_UserName),
			LenUser);
		m_buffer[OffsetPassword-1] = LenPassword;
		memcpy(m_buffer+OffsetPassword, unicode2char(m_ProxyData.m_Password),
			LenPassword);
		
		// Send the username/password packet
		ProxyWrite(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_send_authentication_username_password", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_send_authentication_username_password\n");
	}
}

void Socks5StateMachine::process_receive_authentication_username_password(bool entry)
{
	if (entry) {
		// Receive the server's authentication response
		m_PacketLenght = 2;
		ProxyRead(*m_ProxyClientSocket, m_buffer);
dump("process_receive_authentication_username_password", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_authentication_username_password\n");
	}
	AddDummyEvent();
}

void Socks5StateMachine::process_process_authentication_username_password(bool entry)
{
	if (entry) {
		// Process the server's reply
		m_LastReply = m_buffer[1];
		m_ok =	m_ok &&
			m_buffer[0] == SOCKS5_AUTH_VERSION_USERNAME_PASSWORD &&
			m_buffer[1] == SOCKS5_REPLY_SUCCEED;
dump("process_process_authentication_username_password", m_ok, m_buffer, m_LastRead);
	} else {
printf("wait state -- process_receive_authentication_username_password\n");
	}
	AddDummyEvent();
}

void Socks5StateMachine::process_send_command_request(bool entry)
{
	if (entry) {
		// Prepare the request command buffer
		m_buffer[0] = SOCKS5_VERSION;
		switch (m_ProxyCommand) {
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
		*((uint32 *)(m_buffer+4)) = StringIPtoUint32(m_PeerAddress->IPAddress());
		*((uint16 *)(m_buffer+8)) = htons(m_PeerAddress->Service());
		
		// Send the command packet
		m_PacketLenght = 10;
		ProxyWrite(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_send_command_request", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_send_command_request\n");
	}
}

void Socks5StateMachine::process_receive_command_reply(bool entry)
{
	if (entry) {
		// The minimum number of bytes to read is 10 in the case of
		// ATYP == SOCKS5_ATYP_IPV4_ADDRESS
		m_PacketLenght = 10;
		ProxyRead(*m_ProxyClientSocket, m_buffer);
dump("process_receive_command_reply", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
	AddDummyEvent();
}

void Socks5StateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		m_LastReply = m_buffer[1];
		unsigned char AddressType = m_buffer[3];
		// Process the server's reply
		m_ok = m_ok &&
			m_buffer[0] == SOCKS5_VERSION &&
			m_buffer[1] == SOCKS5_REPLY_SUCCEED;
		if (m_ok) {
			// Read BND.ADDR
			unsigned int Port_offset = 0;
			switch(AddressType) {
			case SOCKS5_ATYP_IPV4_ADDRESS:
			{
				const unsigned int Addr_offset = 4;
				Port_offset = 8;
				wxString strAddr = Uint32toStringIP(
					*((uint32 *)(m_buffer+Addr_offset)) );
				m_ProxyBoundAddressIPV4.Hostname(strAddr);
				m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
				break;
			}
			case SOCKS5_ATYP_DOMAINNAME:
			{
				// Read the domain name
				const unsigned int Addr_offset = 5;
				Port_offset = 10 + m_buffer[4];
				char c = m_buffer[Port_offset];
				m_buffer[Port_offset] = 0;
				m_ProxyBoundAddressIPV4.Hostname(
					char2unicode(m_buffer+Addr_offset));
				m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
				m_buffer[Port_offset] = c;
				break;
			}
			case SOCKS5_ATYP_IPV6_ADDRESS:
			{
				Port_offset = 20;
				// TODO
				// IPV6 not yet implemented in wx
				//m_ProxyBoundAddress.Hostname(Uint128toStringIP(
				//	*((uint128 *)(m_buffer+Addr_offset)) ));
				//m_ProxyBoundAddress = &m_ProxyBoundAddressIPV6;
				m_ok = false;
				break;
			}
			}
			// Set the packet length at last
			m_PacketLenght = Port_offset + 2;
			// Read BND.PORT
			m_ProxyBoundAddress->Service(ntohs(
				*((uint16 *)(m_buffer+Port_offset)) ));
		}
dump("process_process_command_reply", m_ok, m_buffer, m_LastRead);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
	AddDummyEvent();
}

//------------------------------------------------------------------------------
// Socks4StateMachine
//------------------------------------------------------------------------------

Socks4StateMachine::Socks4StateMachine(
	const wxProxyData &ProxyData,
	wxProxyCommand ProxyCommand)
:
ProxyStateMachine(
	wxString(wxT("Socks4")), SOCKS4_MAX_STATES, ProxyData, ProxyCommand)
{
	m_process_state[0] = &Socks4StateMachine::process_start;
	m_process_state[1] = &Socks4StateMachine::process_end;
	m_process_state[2] = &Socks4StateMachine::process_send_command_request;
	m_process_state[3] = &Socks4StateMachine::process_receive_command_reply;
	m_process_state[4] = &Socks4StateMachine::process_process_command_reply;
}

void Socks4StateMachine::process_state(t_sm_state state, bool entry)
{
	(this->*m_process_state[state])(entry);
}

t_sm_state Socks4StateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = HandleEvent(event);
	switch (GetState()) {
	case SOCKS4_STATE_START:
		if (m_IsConnected && !m_IsLost && CanSend()) {
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

void Socks4StateMachine::process_start(bool entry)
{
	if (entry) {
dump("process_start", m_ok, NULL, 0);
	} else {
printf("wait state -- process_start\n");
	}
}

void Socks4StateMachine::process_end(bool)
{
	ReactivateSocket();
dump("process_end", m_ok, NULL, 0);
}

void Socks4StateMachine::process_send_command_request(bool entry)
{
	if (entry) {
		// Prepare the request command buffer
		m_buffer[0] = SOCKS4_VERSION;
		switch (m_ProxyCommand) {
		case wxPROXY_CMD_CONNECT:
			m_buffer[1] = SOCKS4_CMD_CONNECT;
			break;
			
		case wxPROXY_CMD_BIND:
			m_buffer[1] = SOCKS4_CMD_BIND;
			break;
			
		case wxPROXY_CMD_UDP_ASSOCIATE:
			m_ok = false;
			return;
			break;
		}
		*((uint16 *)(m_buffer+2)) = htons(m_PeerAddress->Service());
		*((uint32 *)(m_buffer+4)) = StringIPtoUint32(m_PeerAddress->IPAddress());
		unsigned int OffsetUser = 8;
		unsigned char LenUser = m_ProxyData.m_UserName.Len();
		memcpy(m_buffer + OffsetUser, 
			unicode2char(m_ProxyData.m_UserName), LenUser);
		m_buffer[OffsetUser + LenUser] = 0;
		// Send the command packet
		m_PacketLenght = 1 + 1 + 2 + 4 + LenUser + 1 ;
		ProxyWrite(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_send_command_request", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_send_command_request\n");
	}
}

void Socks4StateMachine::process_receive_command_reply(bool entry)
{
	if (entry) {
		// Receive the server's reply
		m_PacketLenght = 8;
		ProxyRead(*m_ProxyClientSocket, m_buffer);
dump("process_receive_command_reply", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
	AddDummyEvent();
}

void Socks4StateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		m_LastReply = m_buffer[1];
		
		// Process the server's reply
		m_ok = m_ok &&
			m_buffer[0] == SOCKS4_REPLY_CODE &&
			m_buffer[1] == SOCKS4_REPLY_GRANTED;
		if (m_ok) {
			// Read BND.PORT
			const unsigned int Port_offset = 2;
			m_ok = m_ProxyBoundAddressIPV4.Service(ntohs(
				*((uint16 *)(m_buffer+Port_offset)) ));
			// Read BND.ADDR
			const unsigned int Addr_offset = 4;
			m_ok = m_ok &&
				m_ProxyBoundAddressIPV4.Hostname(Uint32toStringIP(
					*((uint32 *)(m_buffer+Addr_offset)) ));
			m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
		}
dump("process_process_command_reply", m_ok, m_buffer, m_LastRead);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
	AddDummyEvent();
}

//------------------------------------------------------------------------------
// HttpStateMachine
//------------------------------------------------------------------------------

HttpStateMachine::HttpStateMachine(
	const wxProxyData &ProxyData,
	wxProxyCommand ProxyCommand)
:
ProxyStateMachine(
	wxString(wxT("Http")), HTTP_MAX_STATES, ProxyData, ProxyCommand)
{
	m_process_state[0] = &HttpStateMachine::process_start;
	m_process_state[1] = &HttpStateMachine::process_end;
	m_process_state[2] = &HttpStateMachine::process_send_command_request;
	m_process_state[3] = &HttpStateMachine::process_receive_command_reply;
	m_process_state[4] = &HttpStateMachine::process_process_command_reply;
}

void HttpStateMachine::process_state(t_sm_state state, bool entry)
{
	(this->*m_process_state[state])(entry);
}

t_sm_state HttpStateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state
	t_sm_state ret = HandleEvent(event);
	switch (GetState()) {
	case HTTP_STATE_START:
		if (m_IsConnected && !m_IsLost && CanSend()) {
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

void HttpStateMachine::process_start(bool entry)
{
	if (entry) {
dump("process_start", m_ok, NULL, 0);
	} else {
printf("wait state -- process_start\n");
	}
}

void HttpStateMachine::process_end(bool)
{
	ReactivateSocket();
dump("process_end", m_ok, NULL, 0);
}

void HttpStateMachine::process_send_command_request(bool entry)
{
	if (entry) {
		// Prepare the request command buffer
		wxCharBuffer buf(unicode2charbuf(m_PeerAddress->IPAddress()));
		const char *host = (const char *)buf;
		uint16 port = m_PeerAddress->Service();
		wxString UserPass;
		wxString UserPassEncoded;
		if (m_ProxyData.m_EnablePassword) {
			UserPass = m_ProxyData.m_UserName + wxT(":") + m_ProxyData.m_Password;
			UserPassEncoded =
				otherfunctions::EncodeBase64(m_buffer, wxPROXY_BUFFER_SIZE);
		}
		wxString msg;
		
		switch (m_ProxyCommand) {
		case wxPROXY_CMD_CONNECT:
			if (m_ProxyData.m_EnablePassword) {
				msg = wxString::Format(
					wxT(
					"CONNECT %s:%d HTTP/1.1\r\n"
					"Host: %s:%d\r\n"
					"Authorization: Basic %s"
					"Proxy-Authorization: Basic %s\r\n"),
					host, port, host, port, unicode2char(UserPassEncoded),
					unicode2char(UserPassEncoded));
			} else {
				msg = wxString::Format(
					wxT(
					"CONNECT %s:%d HTTP/1.1\r\n"
					"Host: %s:%d\r\n\r\n"),
					host, port, host, port);
			}
			break;
			
		case wxPROXY_CMD_BIND:
			m_ok = false;	
			break;
			
		case wxPROXY_CMD_UDP_ASSOCIATE:
			m_ok = false;
			return;
			break;
		}
		// Send the command packet
		m_PacketLenght = msg.Len();
		memcpy(m_buffer, unicode2char(msg), m_PacketLenght+1);
		ProxyWrite(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_send_command_request", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_send_command_request\n");
	}
}

void HttpStateMachine::process_receive_command_reply(bool entry)
{
	if (entry) {
		// Receive the server's reply -- Use a large number, but don't
		// Expect to get it all. HTTP protocol does not have a fixed length.
		m_PacketLenght = wxPROXY_BUFFER_SIZE;
		ProxyRead(*m_ProxyClientSocket, m_buffer);
dump("process_receive_command_reply", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_command_reply\n");
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
void HttpStateMachine::process_process_command_reply(bool entry)
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
dump("process_process_command_reply", m_ok, m_buffer, m_LastRead);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
	AddDummyEvent();
}

//------------------------------------------------------------------------------
// amuleProxyClientSocket
//------------------------------------------------------------------------------

amuleProxyClientSocket::amuleProxyClientSocket(
	wxSocketFlags flags,
	const wxProxyData *ProxyData,
	wxProxyCommand ProxyCommand,
	wxDatagramSocketProxy *UDPSocket)
:
wxSocketClient(flags)
{
	SetProxyData(ProxyData);
	m_ProxyStateMachine = NULL;
	m_UDPSocket = UDPSocket;
	if (m_UseProxy) {
		switch (m_ProxyData.m_ProxyType) {
		case wxPROXY_NONE:
			break;
	
		case wxPROXY_SOCKS5:
			m_ProxyStateMachine =
				new Socks5StateMachine(*ProxyData, ProxyCommand);
			break;
		
		case wxPROXY_SOCKS4:
			m_ProxyStateMachine =
				new Socks4StateMachine(*ProxyData, ProxyCommand);
			break;
		
		case wxPROXY_HTTP:
			m_ProxyStateMachine =
				new HttpStateMachine(*ProxyData, ProxyCommand);
			break;
		
		default:
			break;
		}
	}
}

amuleProxyClientSocket::~amuleProxyClientSocket()
{
	delete m_ProxyStateMachine;
}

void amuleProxyClientSocket::SetProxyData(const wxProxyData *ProxyData)
{
	m_UseProxy = ProxyData != NULL && ProxyData->m_ProxyEnable;
	if (ProxyData) {
		m_ProxyData = *ProxyData;
		m_ProxyAddress.Hostname(m_ProxyData.m_ProxyHostName);
		m_ProxyAddress.Service(m_ProxyData.m_ProxyPort);
	} else {
		m_ProxyData.Empty();
	}
}

bool amuleProxyClientSocket::Start(const wxIPaddress &PeerAddress)
{
	//
	// Important note! SaveState()/RestoreState() DO NOT save/restore
	// the event handler.
	//
	SaveState();
#ifndef AMULE_DAEMON
	SetEventHandler(TheProxyEventHandler, PROXY_SOCKET_HANDLER);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);
#else
	Notify(false);
#endif
	Connect(m_ProxyAddress, false);
//	SetFlags(wxSOCKET_WAITALL);
	SetFlags(wxSOCKET_NONE);
//	SetFlags(wxSOCKET_NOWAIT);
	bool ok = m_ProxyStateMachine->Start(PeerAddress, this);
	
	return ok;
}
	
bool amuleProxyClientSocket::ProxyIsCapableOf(wxProxyCommand ProxyCommand) const
{
	bool ret = false;
	
	switch (m_ProxyData.m_ProxyType) {
	case wxPROXY_NONE:
		ret = false;
		break;
		
	case wxPROXY_SOCKS5:
		ret =	ProxyCommand == wxPROXY_CMD_CONNECT ||
			ProxyCommand == wxPROXY_CMD_BIND ||
			ProxyCommand == wxPROXY_CMD_UDP_ASSOCIATE;
		break;
		
	case wxPROXY_SOCKS4:
		ret =	ProxyCommand == wxPROXY_CMD_CONNECT ||
			ProxyCommand == wxPROXY_CMD_BIND;
		break;
		
	case wxPROXY_HTTP:
		ret =	ProxyCommand == wxPROXY_CMD_CONNECT;
		break;
	}
	
	return ret;
}

//------------------------------------------------------------------------------
// wxSocketClientProxy
//------------------------------------------------------------------------------

wxSocketClientProxy::wxSocketClientProxy(
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
amuleProxyClientSocket(flags, ProxyData, wxPROXY_CMD_CONNECT)
{
}

bool wxSocketClientProxy::Connect(wxIPaddress &address, bool wait)
{
	bool ok;
	
	if (GetUseProxy() && ProxyIsCapableOf(wxPROXY_CMD_CONNECT)) {
		ok = Start(address);
	} else {
		ok = wxSocketClient::Connect(address, wait);
	}
	
	return ok;
}

//------------------------------------------------------------------------------
// wxSocketServerProxy
//------------------------------------------------------------------------------

wxSocketServerProxy::wxSocketServerProxy(
	wxIPaddress &address,
	wxSocketFlags flags,
	const wxProxyData *)
:
wxSocketServer(address, flags)
{
	/* Maybe some day when socks6 is out... :) */
}

//------------------------------------------------------------------------------
// wxDatagramSocketProxy
//------------------------------------------------------------------------------

#if !wxCHECK_VERSION(2,5,3)
IMPLEMENT_ABSTRACT_CLASS(wxDatagramSocketProxy,wxDatagramSocket)
#endif

wxDatagramSocketProxy::wxDatagramSocketProxy(
	wxIPaddress &address, wxSocketFlags flags, const wxProxyData *ProxyData)
:
wxDatagramSocket(address, flags),
m_ProxyTCPSocket(wxSOCKET_NOWAIT, ProxyData, wxPROXY_CMD_UDP_ASSOCIATE, this)
{
	m_UDPSocketOk = false;
	if (	m_ProxyTCPSocket.GetUseProxy() &&
		m_ProxyTCPSocket.ProxyIsCapableOf(wxPROXY_CMD_UDP_ASSOCIATE)) {
			m_ProxyTCPSocket.Start(address);
	} else {
	}
	m_LastUDPOperation = wxUDP_OPERATION_NONE;
}

wxDatagramSocketProxy::~wxDatagramSocketProxy()
{
	// From RFC-1928:
	// "A UDP association terminates when the TCP connection that the
	// UDP ASSOCIATE request arrived terminates."
}

wxDatagramSocket &wxDatagramSocketProxy::RecvFrom(
	wxSockAddress &addr, void* buf, wxUint32 nBytes )
{
	m_LastUDPOperation = wxUDP_OPERATION_RECV_FROM;
	if (m_ProxyTCPSocket.GetUseProxy()) {
		if (m_UDPSocketOk) {
			char *bufUDP = NULL;
			if (nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD > wxPROXY_BUFFER_SIZE) {
				bufUDP = new char[nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD];
			} else {
				bufUDP = m_ProxyTCPSocket.GetBuffer();
			}
			wxDatagramSocket::RecvFrom(
				m_ProxyTCPSocket.GetProxyBoundAddress(),
				bufUDP, nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD);
			unsigned int offset;
			switch (m_ProxyTCPSocket.GetBuffer()[3]) {
			case SOCKS5_ATYP_IPV4_ADDRESS: {
				offset = wxPROXY_UDP_OVERHEAD_IPV4;
				try {
					amuleIPV4Address &a = dynamic_cast<amuleIPV4Address &>(addr);
					a.Hostname(Uint32toStringIP(
						*((uint32 *)(m_ProxyTCPSocket.GetBuffer()+4)) ));
					a.Service(ntohs(
						*((uint16 *)(m_ProxyTCPSocket.GetBuffer()+8)) ));
				} catch (std::bad_cast e) {
					printf("(2)bad_cast exception!\n");
					wxASSERT(false);
				}
			}
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
dump("RecvFrom", 3, bufUDP, wxDatagramSocket::LastCount());
			/* Only delete buffer if it was dynamically created */
			if (bufUDP != m_ProxyTCPSocket.GetBuffer()) {
				/* We should use a fixed buffer to avoid
				 * new/delete it all the time. 
				 * I need an upper bound */
				delete bufUDP;
			}
			/* There is still one problem pending, fragmentation.
			 * Either we support it or we have to drop fragmented
			 * messages. I vote for drop :)
			 */
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
	if (m_ProxyTCPSocket.GetUseProxy()) {
		if (m_UDPSocketOk) {
			m_ProxyTCPSocket.GetBuffer()[0] = SOCKS5_RSV;	// Reserved
			m_ProxyTCPSocket.GetBuffer()[1] = SOCKS5_RSV;	// Reserved
			m_ProxyTCPSocket.GetBuffer()[2] = 0;		// FRAG
			m_ProxyTCPSocket.GetBuffer()[3] = SOCKS5_ATYP_IPV4_ADDRESS;
			*((uint32 *)(m_ProxyTCPSocket.GetBuffer()+4)) = StringIPtoUint32(addr.IPAddress());
			*((uint16 *)(m_ProxyTCPSocket.GetBuffer()+8)) = htons(addr.Service());
			memcpy(m_ProxyTCPSocket.GetBuffer() + wxPROXY_UDP_OVERHEAD_IPV4, buf, nBytes);
			nBytes += wxPROXY_UDP_OVERHEAD_IPV4;
			wxDatagramSocket::SendTo(
				m_ProxyTCPSocket.GetProxyBoundAddress(),
				m_ProxyTCPSocket.GetBuffer(), nBytes);
dump("SendTo", 3, m_ProxyTCPSocket.GetBuffer(), nBytes);
		}
	} else {
		wxDatagramSocket::SendTo(addr, buf, nBytes);
	}
	
	return *this;
}

wxUint32 wxDatagramSocketProxy::LastCount(void) const
{
	wxUint32 ret;

	if (m_ProxyTCPSocket.GetUseProxy()) {
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

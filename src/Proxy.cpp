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


#include <typeinfo>		/* For bad_cast			*/


#include <netinet/in.h>		/* for htons()			*/


#include "amule.h"		/* Needed for wxGetApp		*/
#include "opcodes.h"		/* for PROXY_SOCKET_HANDLER	*/
#include "NetworkFunctions.h"	/* for CStringIPtoUint32()	*/
#include "otherfunctions.h"	/* for EncodeBase64()		*/


static void dump(char *s, bool ok, const void *v, int n)
{
	register int lines = n / 16 + 1;
	register int c = 0;
	register const unsigned char *p = (const unsigned char *)v;
	if (s) {
		printf("%s - ok=%d\n", s, ok);
	}
	for( int i = 0; i < lines; ++i) {
		for( int j = 0; j < 16 && c < n; ++j) {
			printf("0x%02X ", p[c++]);
		}
		printf("\n");
	}
	printf("\n");
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

ProxyEventHandler::ProxyEventHandler()
{
}

#ifndef AMULE_DAEMON
BEGIN_EVENT_TABLE(ProxyEventHandler, wxEvtHandler)
	EVT_SOCKET(PROXY_SOCKET_HANDLER, ProxyEventHandler::ProxySocketHandler)
END_EVENT_TABLE()
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

void ProxyEventHandler::ProxySocketHandler(wxSocketEvent& event)
{
	amuleProxyClientSocket *sock = dynamic_cast<amuleProxyClientSocket *>(event.GetSocket());
	if (sock) {
		sock->m_ProxyStateMachine->Schedule(event.GetSocketEvent());
	} else {
		// we're doomed :)
	}
}

// THE one and only Event Handler
static ProxyEventHandler TheProxyEventHandler;

/******************************************************************************/

ProxyStateMachine::ProxyStateMachine(
		const wxString &name,
		const unsigned int max_states,
		const t_sm_state initial_state,
		const wxProxyData &ProxyData,
		wxProxyCommand ProxyCommand)
:
StateMachine(name, max_states, initial_state),
m_ProxyData(ProxyData)
{
	m_IsLost = false;
	m_IsConnected = false;
	m_CanReceive = false;
	m_CanSend = false;
	m_ok = true;
	m_ProxyCommand = ProxyCommand;
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

bool ProxyStateMachine::Start(const wxIPaddress &PeerAddress, wxSocketClient *ProxyClientSocket)
{
	m_ProxyClientSocket = ProxyClientSocket;
	try {
		const wxIPV4address &peer = dynamic_cast<const wxIPV4address &>(PeerAddress);
		m_PeerAddress = new wxIPV4address(peer);
	} catch (std::bad_cast e) {
		// Should process other types of wxIPAddres before quitting
		printf("bad_cast\n");
		return false;
	}
	
	// Debug message
	printf("amuleProxySocket::Start\nHostname Orig:%s, IPAddr:%s, Port:%d\n",
		unicode2char(m_PeerAddress->Hostname()),
		unicode2char(m_PeerAddress->IPAddress()),
		m_PeerAddress->Service());
	
	// Run the state machine.
	while (!IsEndState() ) {
		wxGetApp().Yield();
		Clock();
	}
	
	// Debug message
	if (m_ProxyBoundAddress) {
		printf("Proxy Bound Address: IP:%s, Port:%u, ok:%d\n",
			unicode2char(GetProxyBoundAddress().IPAddress()),
			GetProxyBoundAddress().Service(), m_ok);
	} else {
		printf("Failed to bind proxy address, ok=%d\n", m_ok);
	}
	
	return m_ok;
}

wxSocketBase &ProxyStateMachine::ProxyWrite(wxSocketBase &socket, const void *buffer, wxUint32 nbytes)
{
	wxSocketBase &ret = socket.Write(buffer, nbytes);
	m_ok = !m_ProxyClientSocket->Error();
	wxSocketError LastErr = wxSOCKET_NOERROR;
	if (!m_ok) {
		LastErr = m_ProxyClientSocket->LastError();
		m_ok = LastErr == wxSOCKET_WOULDBLOCK;
		if (m_ok) {
			m_CanSend = false;
		}
	}
	
	return ret;
}

wxSocketBase &ProxyStateMachine::ProxyRead(wxSocketBase &socket, void *buffer, wxUint32 nbytes)
{
	wxSocketBase &ret = socket.Read(buffer, nbytes);
	m_ok = !m_ProxyClientSocket->Error();
	wxSocketError LastErr = wxSOCKET_NOERROR;
	if (!m_ok) {
		LastErr = m_ProxyClientSocket->LastError();
		m_ok = LastErr == wxSOCKET_WOULDBLOCK;
		if (m_ok) {
			m_CanReceive = false;
		}
	}
	
	return ret;
}

//------------------------------------------------------------------------------
// Socks5StateMachine
//------------------------------------------------------------------------------

Socks5StateMachine::Socks5StateMachine(
	const wxProxyData &ProxyData,
	wxProxyCommand ProxyCommand)
:
ProxyStateMachine(
	wxString(wxT("Socks5")), SOCKS5_MAX_STATES, SOCKS5_STATE_START,
	ProxyData, ProxyCommand)
{
	m_process_state[ 0] = &Socks5StateMachine::process_start;
	m_process_state[ 1] = &Socks5StateMachine::process_send_query_authentication_method;
	m_process_state[ 2] = &Socks5StateMachine::process_receive_authentication_method;
	m_process_state[ 3] = &Socks5StateMachine::process_process_authentication_method;
	m_process_state[ 4] = &Socks5StateMachine::process_send_authentication_gssapi;
	m_process_state[ 5] = &Socks5StateMachine::process_receive_authentication_gssapi;
	m_process_state[ 6] = &Socks5StateMachine::process_process_authentication_gssapi;
	m_process_state[ 7] = &Socks5StateMachine::process_send_authentication_username_password;
	m_process_state[ 8] = &Socks5StateMachine::process_receive_authentication_username_password;
	m_process_state[ 9] = &Socks5StateMachine::process_process_authentication_username_password;
	m_process_state[10] = &Socks5StateMachine::process_send_command_request;
	m_process_state[11] = &Socks5StateMachine::process_receive_command_reply;
	m_process_state[12] = &Socks5StateMachine::process_process_command_reply;
	m_process_state[13] = &Socks5StateMachine::process_end;
}

void Socks5StateMachine::process_state(t_sm_state state, bool entry)
{
	(this->*m_process_state[state])(entry);
}

t_sm_state Socks5StateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state	
	t_sm_state ret = m_state;
	
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
		printf("Lost connection vent\n");
		m_IsLost = true;
		break;
		
	default:
		printf("No event\n");
		break;
	}
	
	if (m_IsLost) {
		ret = SOCKS5_STATE_END;
		
		return ret;
	}
	
	switch (m_state) {
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
		if (CanReceive()) {
			ret = SOCKS5_STATE_PROCESS_AUTHENTICATION_METHOD;
		}
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
		if (CanReceive()) {
			ret = SOCKS5_STATE_PROCESS_AUTHENTICATION_GSSAPI;
		}
		break;
		
	case SOCKS5_STATE_RECEIVE_AUTHENTICATION_USERNAME_PASSWORD:
		if (CanReceive()) {
			ret = SOCKS5_STATE_PROCESS_AUTHENTICATION_USERNAME_PASSWORD;
		}
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
		if (CanReceive()) {
			ret = SOCKS5_STATE_PROCESS_COMMAND_REPLY;
		}
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

void Socks5StateMachine::process_start(bool entry)
{
	if (entry) {
dump("process_start", m_ok, NULL, 0);
	} else {
printf("wait state -- process_start\n");
	}
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
		ProxyRead(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_receive_authentication_method", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_authentication_method\n");
//		if (GetClocksInCurrentState() > 5) m_CanSend = true;
	}
}

void Socks5StateMachine::process_process_authentication_method(bool entry)
{
	if (entry) {
		m_LastReply = m_buffer[1];
		m_ok = m_ok && m_buffer[0] == SOCKS5_VERSION;
dump("process_process_authentication_method", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_receive_authentication_method\n");
//		if (GetClocksInCurrentState() > 5) m_CanSend = true;
	}
}

void Socks5StateMachine::process_send_authentication_gssapi(bool)
{
	// TODO or not TODO? That is the question...
	m_ok = false;
}

void Socks5StateMachine::process_receive_authentication_gssapi(bool)
{
}

void Socks5StateMachine::process_process_authentication_gssapi(bool)
{
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
		ProxyRead(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_receive_authentication_username_password", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_authentication_username_password\n");
	}
}

void Socks5StateMachine::process_process_authentication_username_password(bool entry)
{
	if (entry) {
		// Process the server's reply
		m_LastReply = m_buffer[1];
		m_ok =	m_ok &&
			m_buffer[0] == SOCKS5_AUTH_VERSION_USERNAME_PASSWORD &&
			m_buffer[1] == SOCKS5_REPLY_SUCCEED;
dump("process_process_authentication_username_password", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_receive_authentication_username_password\n");
	}
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
		// Try to minimize the number of Read operations
		// The minimum number of bytes to read is 10 in the case of
		// ATYP == SOCKS5_ATYP_IPV4_ADDRESS
		m_PacketLenght = 10;
		ProxyRead(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_receive_command_reply", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
}

void Socks5StateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		// Already read 10 bytes
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
				m_ok = m_ProxyBoundAddressIPV4.Hostname(strAddr);
				m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
				break;
			}
			case SOCKS5_ATYP_DOMAINNAME:
			{
				// Read the size
				unsigned int LenPacket = m_buffer[4] + 2 - 10;
				const unsigned int Addr_offset = 5;
				Port_offset = 10 + LenPacket;
				// Read the rest of the address and port
				m_PacketLenght += LenPacket + 2;
				ProxyRead(*m_ProxyClientSocket, m_buffer + 10, LenPacket);
				if (m_ok) {
					m_buffer[Port_offset] = 0;
					m_ProxyBoundAddressIPV4.Hostname(
						char2unicode(m_buffer+Addr_offset));
					m_ProxyBoundAddress = &m_ProxyBoundAddressIPV4;
				}
				break;
			}
			case SOCKS5_ATYP_IPV6_ADDRESS:
			{
				Port_offset = 20;
				unsigned int LenPacket = 16 - 10 + 2;
				m_PacketLenght += LenPacket;
				ProxyRead(*m_ProxyClientSocket, m_buffer + 10, LenPacket);
				// TODO
				// IPV6 not yet implemented in wx
				//m_ProxyBoundAddress.Hostname(Uint128toStringIP(
				//	*((uint128 *)(m_buffer+Addr_offset)) ));
				//m_ProxyBoundAddress = &m_ProxyBoundAddressIPV6;
				m_ok = false;
				break;
			}
			}
			if (m_ok) {
				// Read BND.PORT
				m_ok = m_ProxyBoundAddress->Service(ntohs(
					*((uint16 *)(m_buffer+Port_offset)) ));
			}
		}
dump("process_process_command_reply", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
}

void Socks5StateMachine::process_end(bool)
{
dump("process_end", m_ok, NULL, 0);
}

//------------------------------------------------------------------------------
// Socks4StateMachine
//------------------------------------------------------------------------------

Socks4StateMachine::Socks4StateMachine(
	const wxProxyData &ProxyData,
	wxProxyCommand ProxyCommand)
:
ProxyStateMachine(
	wxString(wxT("Socks4")), SOCKS4_MAX_STATES, SOCKS4_STATE_START,
	ProxyData, ProxyCommand)
{
	m_process_state[0] = &Socks4StateMachine::process_start;
	m_process_state[1] = &Socks4StateMachine::process_send_command_request;
	m_process_state[2] = &Socks4StateMachine::process_receive_command_reply;
	m_process_state[3] = &Socks4StateMachine::process_process_command_reply;
	m_process_state[4] = &Socks4StateMachine::process_end;
}

void Socks4StateMachine::process_state(t_sm_state state, bool entry)
{
	(this->*m_process_state[state])(entry);
}

t_sm_state Socks4StateMachine::next_state(t_sm_event event)
{
	// Default is stay in current state	
	t_sm_state ret = m_state;
	
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
		break;
		
	default:
		printf("No event\n");
		break;
	}
	
	if (m_IsLost) {
		ret = SOCKS5_STATE_END;
		
		return ret;
	}
	
	switch (m_state) {
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
		if (CanReceive()) {
			ret = SOCKS4_STATE_PROCESS_COMMAND_REPLY;
		}
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
		ProxyRead(*m_ProxyClientSocket, m_buffer, m_PacketLenght);
dump("process_receive_command_reply", m_ok, m_buffer, 0);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
}

void Socks4StateMachine::process_process_command_reply(bool entry)
{
	if (entry) {
		m_LastReply = m_buffer[1];
		
		// Process the server's reply
		m_ok = m_ok &&
			m_buffer[0] == SOCKS4_VERSION &&
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
dump("process_process_command_reply", m_ok, m_buffer, m_PacketLenght);
	} else {
printf("wait state -- process_receive_command_reply\n");
	}
}

void Socks4StateMachine::process_end(bool)
{
dump("process_end", m_ok, NULL, 0);
}

//------------------------------------------------------------------------------
// amuleProxy
//------------------------------------------------------------------------------

amuleProxy::amuleProxy(
	const wxProxyData *ProxyData,
	wxProxyCommand )
{
	SetProxyData(ProxyData);
}

amuleProxy::~amuleProxy()
{
}

void amuleProxy::SetProxyData(const wxProxyData *ProxyData)
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

bool amuleProxy::Start(wxIPaddress &address, wxProxyCommand cmd, wxSocketClient *socket)
{
	bool ok = false;

	m_ProxyBoundAddress = NULL;
printf("amuleProxy::Start\nHostname Orig:%s, IPAddr:%s, Port:%d\n",
unicode2char(address.Hostname()),
unicode2char(address.IPAddress()),
address.Service());
	m_ProxyClientSocket = socket;
	m_ProxyClientSocket->SaveState();
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
				//ok = DoSocks4(address, cmd);
				ok = false;
				break;
				
			case wxPROXY_SOCKS5:
				//ok = DoSocks5(address, cmd);
				ok = false;
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

bool amuleProxy::DoHttp(wxIPaddress &address, wxProxyCommand cmd)
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

bool amuleProxy::DoHttpRequest(wxIPaddress &address, wxProxyCommand cmd)
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

bool amuleProxy::DoHttpReply(void)
{
	// TODO
	
	return false;
}

bool amuleProxy::DoHttpCmdConnect(void)
{
	// Nothing to do here.
	
	return true;
}

/******************************************************************************/

amuleProxyClientSocket::amuleProxyClientSocket(
	wxSocketFlags flags,
	const wxProxyData *ProxyData,
	wxProxyCommand ProxyCommand)
:
wxSocketClient(flags)
{
	SetProxyData(ProxyData);
	m_ProxyStateMachine = NULL;
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
	SaveState();
#ifndef AMULE_DAEMON
	SetEventHandler(TheProxyEventHandler, PROXY_SOCKET_HANDLER);
#endif
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);
	Connect(m_ProxyAddress, false);
	// Without this flag, we would end up with more states, 
	// because we can't test the read buffer until we get 
	// the next input event.
//	SetFlags(wxSOCKET_WAITALL);
//	SetFlags(wxSOCKET_NOWAIT);
	// wxSOCKET_WAITALL tends to assert inside wx, while wxSOCKET_NONE behaves better.
	SetFlags(wxSOCKET_NONE);
	bool ok = m_ProxyStateMachine->Start(PeerAddress, this);
	RestoreState();
	
	return ok;
}

/******************************************************************************/

wxSocketClientProxy::wxSocketClientProxy(
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
amuleProxyClientSocket(flags, ProxyData, wxPROXY_CMD_CONNECT)
{
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
	
	if (GetUseProxy()) {
		ok = Start(address);
#ifndef AMULE_DAEMON
		/* If proxy is beeing used, CServerSocketHandler will not receive a 
		 * wxSOCKET_CONNECTION event, because the connection has already 
		 * started with the proxy. So we must add a wxSOCKET_CONNECTION
		 * event to make things go undetected. A wxSOCKET_OUTPUT event is
		 * also necessary to start sending data to the server. */
		if (ok) {
			/* If the wxSocket had a GetEventHandler method, this could be 
			 * much more cleaner. All this fuzz is because GetEventHandler()
			 * is implemented in CClientReqSocket and CServerSocket, both
		 	* are aMule classes. Maybe we can add this method to a common
		 	* ancestor. */
			wxSocketEvent e(SERVERSOCKET_HANDLER);
			e.m_event = wxSOCKET_CONNECTION;
			e.SetEventObject(this);
			CClientReqSocket *s1 = dynamic_cast<CClientReqSocket *>(this);
			if (s1) {
				CClientReqSocketHandler *h = s1->GetEventHandler();
				SetEventHandler(*h, CLIENTREQSOCKET_HANDLER);
				h->AddPendingEvent(e);
				e.m_event = wxSOCKET_OUTPUT;
				h->AddPendingEvent(e);
				goto end;
			}
			CServerSocket *s2 = dynamic_cast<CServerSocket *>(this);
			if (s2) {
				CServerSocketHandler *h = s2->GetEventHandler();
				SetEventHandler(*h, SERVERSOCKET_HANDLER);
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

/******************************************************************************/

wxSocketServerProxy::wxSocketServerProxy(
	wxIPaddress &address,
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
wxSocketServer(address, flags),
m_SocketProxy(ProxyData, wxPROXY_CMD_BIND)
{
	if (m_SocketProxy.GetUseProxy()) {
		/* Maybe some day when socks6 is out... :) */
	}
}

/******************************************************************************/

#if !wxCHECK_VERSION(2,5,3)
IMPLEMENT_ABSTRACT_CLASS(wxDatagramSocketProxy,wxDatagramSocket)
#endif

wxDatagramSocketProxy::wxDatagramSocketProxy(
	wxIPaddress &address, wxSocketFlags flags, const wxProxyData *ProxyData)
:
wxDatagramSocket(address, flags),
m_ProxyTCPSocket(wxSOCKET_NOWAIT, ProxyData, wxPROXY_CMD_UDP_ASSOCIATE)
{
	bool ok = false;
	
	if (m_ProxyTCPSocket.GetUseProxy()) {
		m_ProxyTCPSocket.Start(address);
	} else {
	}
	m_UDPSocketOk = ok;
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
			char *bufUDP = new char[nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD];
			wxDatagramSocket::RecvFrom(
				m_ProxyTCPSocket.GetProxyBoundAddress(),
				bufUDP, nBytes + wxPROXY_UDP_MAXIMUM_OVERHEAD);
			unsigned int offset;
			switch (m_ProxyTCPSocket.GetBuffer()[3]) {
			case SOCKS5_ATYP_IPV4_ADDRESS: {
				offset = wxPROXY_UDP_OVERHEAD_IPV4;
				wxIPV4address &a = dynamic_cast<wxIPV4address &>(addr);
				a.Hostname(Uint32toStringIP( *((uint32 *)(m_ProxyTCPSocket.GetBuffer()+4)) ));
				a.Service(ntohs(             *((uint16 *)(m_ProxyTCPSocket.GetBuffer()+8)) ));
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
printf("RecvFrom\n");
printf("LastCount:%d\n", wxDatagramSocket::LastCount());
dump("nbufUDP:", 3, bufUDP, wxDatagramSocket::LastCount());
			/* We should use a fixed buffer to avoid new/delete it all the time. I need an upper bound */
			delete bufUDP;
			/* There is still one problem pending, fragmentation.
			 * Either we support it or we have to drop fragmented
			 * messages.
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
printf("SendTo\n");
printf("nBytes:%d\n", nBytes);
dump("buf:", 3, buf, nBytes);
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

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

/******************************************************************************/


#include "Proxy.h"		/* for Interface		*/


#include <netinet/in.h>		/* for htons()			*/


#include "opcodes.h"		/* for PROXY_SOCKET_HANDLER	*/
#include "NetworkFunctions.h"	/* for CStringIPtoUint32()	*/


/******************************************************************************/

BEGIN_EVENT_TABLE(wxProxyEventHandler, wxEvtHandler)
	EVT_SOCKET(PROXY_SOCKET_HANDLER, wxProxyEventHandler::m_ProxySocketHandler)
END_EVENT_TABLE()

wxProxyEventHandler::wxProxyEventHandler(wxSocketProxy *parent)
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

wxSocketProxy::wxSocketProxy(const wxProxyData *ProxyData)
{
	if (ProxyData) {
		m_ProxyData = *ProxyData;
		m_ProxyClientSocket = new wxSocketClient();
/*
		m_ProxyClientSocket->SetEventHandler(
			wxProxyEventHandler, PROXY_SOCKET_HANDLER);
*/
		m_ProxyClientSocket->SetNotify(
			wxSOCKET_CONNECTION_FLAG |
			wxSOCKET_INPUT_FLAG |
			wxSOCKET_LOST_FLAG);
		m_ProxyClientSocket->Notify(true);
	
		m_ProxyAddress.Hostname(m_ProxyData.ProxyHostName);
		m_ProxyAddress.Service(m_ProxyData.ProxyPort);
	}
}

bool wxSocketProxy::Connect(wxIPaddress& address)
{
	bool ok = false;

	m_ProxyClientSocket->Connect(m_ProxyAddress, false);
	if (m_ProxyClientSocket->WaitOnConnect(10,0) )
	{
		if (m_ProxyClientSocket->IsConnected()) {
			// Prepare for transmition
			m_ProxyClientSocket->SetFlags(wxSOCKET_WAITALL);

			/* Call the proxy stuff routine */
			switch(m_ProxyData.ProxyType)
			{
			case wxPROXY_SOCKS4:
				break;
				
			case wxPROXY_SOCKS5:
				ok = DoSocks5(address, wxPROXY_CMD_CONNECT);
				break;
			}
			m_ProxyClientSocket->Destroy();
		}
	}
	
	return ok;
}

bool wxSocketProxy::DoSocks5(wxIPaddress& address, wxProxyCommand cmd)
{
	// Use the short circuit evaluation
	int ok = 
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
	unsigned char LenUser = m_ProxyData.Username.Len();
	unsigned char LenPassword = m_ProxyData.Password.Len();
	unsigned int LenPacket = 1 + 1 + LenUser + 1 + LenPassword;
	unsigned int OffsetUser = 2;
	unsigned int OffsetPassword = OffsetUser + LenUser + 1;
	
	// Prepare username/password buffer
	m_buffer[0] = SOCKS5_VERSION;
	m_buffer[OffsetUser-1] = LenUser;
	memcpy(m_buffer+OffsetUser, NULL, LenUser);
	m_buffer[OffsetPassword-1] = LenPassword;
	memcpy(m_buffer+OffsetPassword, NULL, LenPassword);

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

bool wxSocketProxy::DoSocks5Request(wxIPaddress& address, unsigned char cmd)
{
	// Prepare the request command buffer
	m_buffer[0] = SOCKS5_VERSION;
	m_buffer[1] = cmd;
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
		unsigned int Port_offset;
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
				m_TargetAddressIPV4.Hostname(Uint32toStringIP(
					*((uint32 *)(m_buffer+Addr_offset)) ));
				m_TargetAddress = &m_TargetAddressIPV4;
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
					m_TargetAddressIPV4.Hostname(
						char2unicode(m_buffer+Addr_offset));
					m_TargetAddress = &m_TargetAddressIPV4;
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
			// IPV6 not yet implemented
			//m_TargetAddress.Hostname(Uint128toStringIP(
			//	*((uint128 *)(m_buffer+Addr_offset)) ));
			//m_TargetAddress = &m_TargetAddressIPV6;
			ok = false;
			break;
		}
		}
		if (ok) {
			// Read BND.PORT
			LenPacket = 2;
			m_ProxyClientSocket->Read(m_buffer+Port_offset, LenPacket);
			m_TargetAddress->Service(ntohs(
				*((uint16 *)(m_buffer+Port_offset)) ));
			ok =	!m_ProxyClientSocket->Error() &&
				m_ProxyClientSocket->LastCount() == LenPacket;
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

/******************************************************************************/

wxSocketClientProxy::wxSocketClientProxy(
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
wxSocketClient(flags),
m_SocketProxy(ProxyData)
{
}

bool wxSocketClientProxy::Connect(wxIPaddress& address, bool wait)
{
	bool ok;
	
	ok = m_SocketProxy.Connect(address);
	if (ok) {
		ok = wxSocketClient::Connect(m_SocketProxy.GetTargetAddress(), wait);
	}

	return ok;
}

/******************************************************************************/

wxSocketServerProxy::wxSocketServerProxy(
	wxIPaddress& address,
	wxSocketFlags flags,
	const wxProxyData *ProxyData)
:
wxSocketServer(address, flags),
m_SocketProxy(ProxyData)
{
}

/******************************************************************************/


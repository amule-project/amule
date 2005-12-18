//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#include "RemoteConnect.h"

#include <memory>			// Needed for auto_ptr
using std::auto_ptr;

#include <wx/intl.h>			// Needed for i18n

#include "ECVersion.h"		// Needed for EC_VERSION_ID
#include "ECPacket.h"		// Needed for CECPacket

DEFINE_LOCAL_EVENT_TYPE(wxEVT_EC_CONNECTION);

CECLoginPacket::CECLoginPacket(const wxString &pass,
						const wxString& client, const wxString& version) : CECPacket(EC_OP_AUTH_REQ)
{
	AddTag(CECTag(EC_TAG_CLIENT_NAME, client));
	AddTag(CECTag(EC_TAG_CLIENT_VERSION, version));
	AddTag(CECTag(EC_TAG_PROTOCOL_VERSION, (uint16)EC_CURRENT_PROTOCOL_VERSION));
	AddTag(CECTag(EC_TAG_PASSWD_HASH, CMD4Hash(pass)));

	#ifdef EC_VERSION_ID
	AddTag(CECTag(EC_TAG_VERSION_ID, CMD4Hash(wxT(EC_VERSION_ID))));
	#endif

}

/*!
 * Connection to remote core
 * 
 */

CRemoteConnect::CRemoteConnect(wxEvtHandler* evt_handler) : CECSocket(evt_handler != 0)
{
	notifier = evt_handler;
	m_ec_state = EC_INIT;
	
	//
	// Give application some indication about how fast requests are served
	// When request fifo contain more that certain number of entries, it may
	// indicate that either core or network is slowing us down
	m_req_count = 0;
	// This is not mean to be absolute limit, because we can't drop requests
	// out of calling context; it is just signal to application to slow down
	m_req_fifo_thr = 20;
}

bool CRemoteConnect::ConnectToCore(const wxString &host, int port,
	const wxString &WXUNUSED(login), const wxString &pass, 
	const wxString& client, const wxString& version)
{
	
	ConnectionPassword = pass;
	
	m_client = client;
	m_version = version;
	
	// don't even try to connect without password
	if (ConnectionPassword.IsEmpty() || ConnectionPassword == wxT("d41d8cd98f00b204e9800998ecf8427e") || CMD4Hash(ConnectionPassword).IsEmpty()) {
		server_reply = _("You must specify a non-empty password.");
		return false;
	}

	wxIPV4address addr;

	addr.Hostname(host);
	addr.Service(port);

	if ( !Connect(addr) ) {
		return false;
	}
	
	// if we're using blocking calls - enter login sequence now. Else, 
	// we will wait untill OnConnect gets called
	if ( !notifier ) {
		CECLoginPacket login_req(ConnectionPassword, m_client, m_version);
		const CECPacket *reply = SendRecvPacket(&login_req);
		return ConnectionEstablished(reply);
	} else {
		m_ec_state = EC_CONNECT_SENT;
	}
	
	return true;
}

void CRemoteConnect::OnConnect() {
	if (notifier) {
		wxASSERT(m_ec_state == EC_CONNECT_SENT);
		CECLoginPacket login_req(ConnectionPassword, m_client, m_version);
		CECSocket::SendPacket(&login_req);
		
		m_ec_state = EC_REQ_SENT;
	} else {
		// do nothing, calling code will take from here
	}
}

void CRemoteConnect::OnClose() {
	if (notifier) {
		// Notify app of failure
		wxECSocketEvent event(wxEVT_EC_CONNECTION,false,_("Connection failure"));
		notifier->AddPendingEvent(event);
	}
	// FIXME: wtf is that ?
	//CECSocket::OnClose();
}

const CECPacket *CRemoteConnect::OnPacketReceived(const CECPacket *packet)
{
	CECPacket *next_packet = 0;
	m_req_count--;
	switch(m_ec_state) {
		case EC_REQ_SENT:
			if ( ConnectionEstablished(packet) ) {
				m_ec_state = EC_OK;
			} else {
				m_ec_state = EC_FAIL;
			}
			break;
		case EC_OK: 
			if ( !m_req_fifo.empty() ) {
				CECPacketHandlerBase *handler = m_req_fifo.front();
				m_req_fifo.pop_front();
				if ( handler ) {
					handler->HandlePacket(packet);
				}
			} else {
				printf("EC error - packet received, but request fifo is empty\n");
			}
			break;
		default:
			break;
	}
	
	// no reply by default
	return next_packet;
}

/*
 * Our requests are served by core in FCFS order. And core always replies. So, even
 * if we're not interested in reply, we preserve place in request fifo.
 */
void CRemoteConnect::SendRequest(CECPacketHandlerBase *handler, CECPacket *request)
{
	m_req_count++;
	m_req_fifo.push_back(handler);
	CECSocket::SendPacket(request);
}

void CRemoteConnect::SendPacket(CECPacket *request)
{
	SendRequest(0, request);
}

bool CRemoteConnect::ConnectionEstablished(const CECPacket *reply) {
	bool result = false;
	
	if (!reply) {
		server_reply = _("EC Connection Failed. Empty reply.");
		Close();
	} else {
		if (reply->GetOpCode() == EC_OP_AUTH_FAIL) {
			const CECTag *reason = reply->GetTagByName(EC_TAG_STRING);
			if (reason != NULL) {
				server_reply = wxString(_("ExternalConn: Access denied because: ")) +
					wxGetTranslation(reason->GetStringData());
			} else {
				server_reply = _("ExternalConn: Access denied");
			}
			Close();
		} else if (reply->GetOpCode() != EC_OP_AUTH_OK) {
			server_reply = _("ExternalConn: Bad reply from server. Connection closed.");
			Close();
		} else {
			if (reply->GetTagByName(EC_TAG_SERVER_VERSION)) {
				server_reply = _("Succeeded! Connection established to aMule ") +
					reply->GetTagByName(EC_TAG_SERVER_VERSION)->GetStringData();
			} else {
				server_reply = _("Succeeded! Connection established.");
			}
			result = true;
		}
	}
	if ( notifier ) {
		wxECSocketEvent event(wxEVT_EC_CONNECTION, result, server_reply);
		notifier->AddPendingEvent(event);
	}
	return result;
}

/******************** EC API ***********************/

void CRemoteConnect::StartKad() {
	CECPacket req(EC_OP_KAD_START);
	SendPacket(&req);	
}

void CRemoteConnect::StopKad() {
	CECPacket req(EC_OP_KAD_STOP);
	SendPacket(&req);	
}

void CRemoteConnect::ConnectED2K(uint32 ip, uint16 port) {
	CECPacket req(EC_OP_SERVER_CONNECT);
	if (ip && port) {
		req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
	}
	SendPacket(&req);
}

void CRemoteConnect::DisconnectED2K() {
	CECPacket req(EC_OP_SERVER_DISCONNECT);
	SendPacket(&req);	
}

void CRemoteConnect::RemoveServer(uint32 ip, uint16 port) {
	CECPacket req(EC_OP_SERVER_REMOVE);
	if (ip && port) {
		req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
	}
	SendPacket(&req);
}

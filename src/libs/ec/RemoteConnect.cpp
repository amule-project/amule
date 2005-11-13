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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include "RemoteConnect.h"

#include <memory>			// Needed for auto_ptr
using std::auto_ptr;

#include <wx/intl.h>			// Needed for i18n

#include "ECVersion.h"		// Needed for EC_VERSION_ID
#include "ECPacket.h"		// Needed for CECPacket

DEFINE_LOCAL_EVENT_TYPE(wxEVT_EC_CONNECTION);

/*!
 * Connection to remote core
 * 
 */

CRemoteConnect::CRemoteConnect(wxEvtHandler* evt_handler) : CECSocket()
{
	notifier = evt_handler;
	m_busy = false;
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

	Connect(addr, false);
	
	return true;
}

void CRemoteConnect::OnConnect() {
	bool auth = ConnectionEstablished();
	if (notifier) {
		// Notify app of success / failure
		wxECSocketEvent event(wxEVT_EC_CONNECTION,auth,server_reply);
		notifier->AddPendingEvent(event);
	}
}

void CRemoteConnect::OnClose() {
	if (notifier) {
		// Notify app of failure
		wxECSocketEvent event(wxEVT_EC_CONNECTION,false,_("Connection failure"));
		notifier->AddPendingEvent(event);
	}
	CECSocket::OnClose();
}

bool CRemoteConnect::ConnectionEstablished() {
	
	SetFlags(wxSOCKET_BLOCK);
	
	// Authenticate ourselves
	CECPacket packet(EC_OP_AUTH_REQ);
	packet.AddTag(CECTag(EC_TAG_CLIENT_NAME, m_client));
	packet.AddTag(CECTag(EC_TAG_CLIENT_VERSION, m_version));
	packet.AddTag(CECTag(EC_TAG_PROTOCOL_VERSION, (uint16)EC_CURRENT_PROTOCOL_VERSION));
	packet.AddTag(CECTag(EC_TAG_PASSWD_HASH, CMD4Hash(ConnectionPassword)));

#ifdef EC_VERSION_ID
	packet.AddTag(CECTag(EC_TAG_VERSION_ID, CMD4Hash(wxT(EC_VERSION_ID))));
#endif

	if (! WritePacket(&packet) ) {
		server_reply = _("EC Connection Failed. Unable to write data to the socket.");
		Close();
		return false;
	}
    
	auto_ptr<CECPacket> reply(ReadPacket());
	
	if (!reply.get()) {
		server_reply = _("EC Connection Failed. Empty reply.");
		Close();
		return false;
	}
	
	if (reply->GetOpCode() == EC_OP_AUTH_FAIL) {
		const CECTag *reason = reply->GetTagByName(EC_TAG_STRING);
		if (reason != NULL) {
			server_reply = wxString(_("ExternalConn: Access denied because: ")) +
				wxGetTranslation(reason->GetStringData());
		} else {
		    server_reply = _("ExternalConn: Access denied");
		}
		Close();
		return false;
    } else if (reply->GetOpCode() != EC_OP_AUTH_OK) {
        server_reply = _("ExternalConn: Bad reply from server. Connection closed.");
		Close();
		return false;
    } else {
        if (reply->GetTagByName(EC_TAG_SERVER_VERSION)) {
                server_reply = _("Succeeded! Connection established to aMule ") +
                	reply->GetTagByName(EC_TAG_SERVER_VERSION)->GetStringData();
        } else {
                server_reply = _("Succeeded! Connection established.");
        }
    }
    
	return true;	
}


CECPacket *CRemoteConnect::SendRecv(CECPacket *packet)
{
	m_busy = true;
    if (! WritePacket(packet) ) {
		m_busy = false;
    	return 0;
    }
    CECPacket *reply = ReadPacket();

	m_busy = false;
	return reply;
}

void CRemoteConnect::Send(CECPacket *packet)
{
	// Just send and ignore reply
    auto_ptr<CECPacket> reply(SendRecv(packet));
}

/******************** EC API ***********************/

void CRemoteConnect::StartKad() {
	CECPacket req(EC_OP_KAD_START);
	Send(&req);	
}

void CRemoteConnect::StopKad() {
	CECPacket req(EC_OP_KAD_STOP);
	Send(&req);	
}

void CRemoteConnect::ConnectED2K(uint32 ip, uint16 port) {
	CECPacket req(EC_OP_SERVER_CONNECT);
	if (ip && port) {
		req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
	}
	Send(&req);
}

void CRemoteConnect::DisconnectED2K() {
	CECPacket req(EC_OP_SERVER_DISCONNECT);
	Send(&req);	
}

void CRemoteConnect::RemoveServer(uint32 ip, uint16 port) {
	CECPacket req(EC_OP_SERVER_REMOVE);
	if (ip && port) {
		req.AddTag(CECTag(EC_TAG_SERVER, EC_IPv4_t(ip, port)));
	}
	Send(&req);
}

//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
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

#include <common/MD5Sum.h>
#include <common/Format.h>

#include <wx/intl.h>

using std::auto_ptr;

DEFINE_LOCAL_EVENT_TYPE(wxEVT_EC_CONNECTION)

CECLoginPacket::CECLoginPacket(const wxString& client, const wxString& version,
							   bool canZLIB, bool canUTF8numbers, bool canNotify)
:
CECPacket(EC_OP_AUTH_REQ)
{
	AddTag(CECTag(EC_TAG_CLIENT_NAME, client));
	AddTag(CECTag(EC_TAG_CLIENT_VERSION, version));
	AddTag(CECTag(EC_TAG_PROTOCOL_VERSION, (uint64)EC_CURRENT_PROTOCOL_VERSION));

	#ifdef EC_VERSION_ID
	CMD4Hash versionhash;
	wxCHECK2(versionhash.Decode(wxT(EC_VERSION_ID)), /* Do nothing. */);
	AddTag(CECTag(EC_TAG_VERSION_ID, versionhash));
	#endif

	// Send capabilities:
	// support ZLIB compression
	if (canZLIB)		AddTag(CECEmptyTag(EC_TAG_CAN_ZLIB));
	// support encoding of integers as UTF-8
	if (canUTF8numbers) AddTag(CECEmptyTag(EC_TAG_CAN_UTF8_NUMBERS));
	// client accepts push messages
	if (canNotify)		AddTag(CECEmptyTag(EC_TAG_CAN_NOTIFY));
}

CECAuthPacket::CECAuthPacket(const wxString& pass)
:
CECPacket(EC_OP_AUTH_PASSWD)
{
	CMD4Hash passhash;
	wxCHECK2(passhash.Decode(pass), /* Do nothing. */);
	AddTag(CECTag(EC_TAG_PASSWD_HASH, passhash));
}

/*!
 * Connection to remote core
 * 
 */

CRemoteConnect::CRemoteConnect(wxEvtHandler* evt_handler)
:
CECMuleSocket(evt_handler != 0),
m_ec_state(EC_INIT),
m_req_fifo(),
// Give application some indication about how fast requests are served
// When request fifo contain more that certain number of entries, it may
// indicate that either core or network is slowing us down
m_req_count(0),
// This is not mean to be absolute limit, because we can't drop requests
// out of calling context; it is just signal to application to slow down
m_req_fifo_thr(20),
m_notifier(evt_handler),
m_canZLIB(false),
m_canUTF8numbers(false),
m_canNotify(false)
{
}

void CRemoteConnect::SetCapabilities(bool canZLIB, bool canUTF8numbers, bool canNotify)
{ 
	m_canZLIB = canZLIB;
	if (canZLIB) {
		m_my_flags |= EC_FLAG_ZLIB;
	}
	m_canUTF8numbers = canUTF8numbers;
	if (canUTF8numbers) {
		m_my_flags |= EC_FLAG_UTF8_NUMBERS;
	}
	m_canNotify = canNotify;
}

bool CRemoteConnect::ConnectToCore(const wxString &host, int port,
	const wxString &WXUNUSED(login), const wxString &pass, 
	const wxString& client, const wxString& version)
{
	m_connectionPassword = pass;
	
	m_client = client;
	m_version = version;
	
	// don't even try to connect without a valid password
	if (m_connectionPassword.IsEmpty() || m_connectionPassword == wxT("d41d8cd98f00b204e9800998ecf8427e")) {
		m_server_reply = _("You must specify a non-empty password.");
		return false;
	} else {
		CMD4Hash hash;
		if (!hash.Decode(m_connectionPassword)) {
			m_server_reply = _("Invalid password, not a MD5 hash!");
			return false;
		} else if (hash.IsEmpty()) {
			m_server_reply = _("You must specify a non-empty password.");
			return false;
		}
	}

	wxIPV4address addr;

	addr.Hostname(host);
	addr.Service(port);

	if (ConnectSocket(addr)) {
		CECLoginPacket login_req(m_client, m_version, m_canZLIB, m_canUTF8numbers, m_canNotify);

		std::auto_ptr<const CECPacket> getSalt(SendRecvPacket(&login_req));
		m_ec_state = EC_REQ_SENT;

		ProcessAuthPacket(getSalt.get());

		CECAuthPacket passwdPacket(m_connectionPassword);

		std::auto_ptr<const CECPacket> reply(SendRecvPacket(&passwdPacket));
		m_ec_state = EC_PASSWD_SENT;

		return ProcessAuthPacket(reply.get());
	} else if (m_notifier) {
		m_ec_state = EC_CONNECT_SENT;
	} else {
		return false;
	}

	return true;
}

bool CRemoteConnect::IsConnectedToLocalHost()
{
	wxIPV4address addr;
	return GetPeer(addr) ? addr.IsLocalHost() : false;
}

void CRemoteConnect::WriteDoneAndQueueEmpty()
{
}

void CRemoteConnect::OnConnect() {
	if (m_notifier) {
		wxASSERT(m_ec_state == EC_CONNECT_SENT);
		CECLoginPacket login_req(m_client, m_version, m_canZLIB, m_canUTF8numbers, m_canNotify);
		CECSocket::SendPacket(&login_req);
		
		m_ec_state = EC_REQ_SENT;
	} else {
		// do nothing, calling code will take from here
	}
}

void CRemoteConnect::OnLost() {
	if (m_notifier) {
		// Notify app of failure
		wxECSocketEvent event(wxEVT_EC_CONNECTION,false,_("Connection failure"));
		m_notifier->AddPendingEvent(event);
	}
}

const CECPacket *CRemoteConnect::OnPacketReceived(const CECPacket *packet, uint32 trueSize)
{
	CECPacket *next_packet = 0;
	m_req_count--;
	packet->DebugPrint(true, trueSize);
	switch(m_ec_state) {
		case EC_REQ_SENT:
			if (ProcessAuthPacket(packet)) {
				CECAuthPacket passwdPacket(m_connectionPassword);
				CECSocket::SendPacket(&passwdPacket);
				m_ec_state = EC_PASSWD_SENT;
			}
			break;
		case EC_PASSWD_SENT:
			ProcessAuthPacket(packet);
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
void CRemoteConnect::SendRequest(CECPacketHandlerBase *handler, const CECPacket *request)
{
	m_req_count++;
	m_req_fifo.push_back(handler);
	CECSocket::SendPacket(request);
}

void CRemoteConnect::SendPacket(const CECPacket *request)
{
	SendRequest(0, request);
}

bool CRemoteConnect::ProcessAuthPacket(const CECPacket *reply) {
	bool result = false;
	
	if (!reply) {
		m_server_reply = _("EC connection failed. Empty reply.");
		CloseSocket();
	} else {
		if ((m_ec_state == EC_REQ_SENT) && (reply->GetOpCode() == EC_OP_AUTH_SALT)) {
				const CECTag *passwordSalt = reply->GetTagByName(EC_TAG_PASSWD_SALT);
				if ( NULL != passwordSalt) {
					wxString saltHash = MD5Sum(CFormat(wxT("%lX")) % passwordSalt->GetInt()).GetHash();
					m_connectionPassword = MD5Sum(m_connectionPassword.Lower() + saltHash).GetHash();
					m_ec_state = EC_SALT_RECEIVED;
					return true;
				} else {
					m_server_reply = _("External Connection: Bad reply, handshake failed. Connection closed.");
					m_ec_state = EC_FAIL;
					CloseSocket();
				}
		} else if ((m_ec_state == EC_PASSWD_SENT) && (reply->GetOpCode() == EC_OP_AUTH_OK)) {
			m_ec_state = EC_OK;
			result = true;
			if (reply->GetTagByName(EC_TAG_SERVER_VERSION)) {
				m_server_reply = _("Succeeded! Connection established to aMule ") +
					reply->GetTagByName(EC_TAG_SERVER_VERSION)->GetStringData();
			} else {
				m_server_reply = _("Succeeded! Connection established.");
			}
		}else {
			m_ec_state = EC_FAIL;
			const CECTag *reason = reply->GetTagByName(EC_TAG_STRING);
			if (reason != NULL) {
				m_server_reply = wxString(_("External Connection: Access denied because: ")) +
					wxGetTranslation(reason->GetStringData());
			} else {
				m_server_reply = _("External Connection: Handshake failed.");
			}
			CloseSocket();	
		}
	}
	if ( m_notifier ) {
		wxECSocketEvent event(wxEVT_EC_CONNECTION, result, m_server_reply);
		m_notifier->AddPendingEvent(event);
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
// File_checked_for_headers

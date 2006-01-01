//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2006 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#ifndef REMOTECONNECT_H
#define REMOTECONNECT_H

#include <wx/string.h>
#include <list>

#include "ECSocket.h"
#include "ECPacket.h"		// Needed for CECPacket

class CECPacketHandlerBase {
	public:
		virtual ~CECPacketHandlerBase() { }
		virtual void HandlePacket(const CECPacket *) = 0;
};

class CECLoginPacket : public CECPacket {
	public:
		CECLoginPacket(const wxString &pass,
						const wxString& client, const wxString& version);
};

class CRemoteConnect : public CECSocket {
	// State enums for connection SM ( client side ) in case of async processing
	enum { 
		EC_INIT,         // initial state
		EC_CONNECT_SENT, // socket connect request sent
		EC_REQ_SENT,     // sent auth request to core, waiting for reply
		EC_OK,           // core replyed "ok"
		EC_FAIL          // core replyed "bad"
	} m_ec_state;
	
	virtual const CECPacket *OnPacketReceived(const CECPacket *packet);
	
	bool ConnectionEstablished(const CECPacket *reply);

	// fifo of handlers for no-the-air requests. all EC consept is working in fcfs
	// order, so it is ok to assume that order of replies is same as order of requests
	std::list<CECPacketHandlerBase *> m_req_fifo;
	int m_req_count, m_req_fifo_thr;
public:
	// The event handler is used for notifying connect/close 
	CRemoteConnect(wxEvtHandler* evt_handler);

	bool ConnectToCore(const wxString &host, int port,
						const wxString& login, const wxString &pass,
						const wxString& client, const wxString& version);

	const wxString& GetServerReply() const { return server_reply; }

	bool RequestFifoFull()
	{
		return m_req_count > m_req_fifo_thr;
	}
	
	virtual void OnConnect(); // To override connection events
	virtual void OnClose(); // To override close events

	void SendRequest(CECPacketHandlerBase *handler, CECPacket *request);
	void SendPacket(CECPacket *request);
	
	/********************* EC API ********************/
	
	/* Kad */
	
	// Connects Kad network
	void StartKad();
	
	// Disconnects Kad network
	void StopKad();
	
	
	/* ED2K */
	
	// Connects to ED2K. If ip and port are not 0, connect 
	// to the specific port. Otherwise, connect to any.
	void ConnectED2K(uint32 ip, uint16 port);
	
	// Disconnects from ED2K
	void DisconnectED2K();


	/* Servers */
	
	// Remove specific server
	// Returns: Error message or empty string for no error
	void RemoveServer(uint32 ip, uint16 port);
	
private:

	wxEvtHandler* notifier;
	wxString ConnectionPassword;
	wxString server_reply;
	wxString m_client;
	wxString m_version;
};

DECLARE_LOCAL_EVENT_TYPE(wxEVT_EC_CONNECTION, wxEVT_USER_FIRST + 1000)

class wxECSocketEvent : public wxEvent {
public:
	wxECSocketEvent(int id, int event_id) : wxEvent(event_id, id)
	{
	}
	wxECSocketEvent(int id) : wxEvent(-1, id)
	{
	}
	wxECSocketEvent(int id, bool result, const wxString& reply) : wxEvent(-1, id)
	{
		m_value = result;
		server_reply = reply;
	}
	wxEvent *Clone(void) const
	{
		return new wxECSocketEvent(*this);
	}
	long GetResult() const
	{
		return m_value;
	}
	const wxString& GetServerReply() const
	{
		return server_reply;
	}
private:
	bool m_value;
	wxString server_reply;
};

#endif // REMOTECONNECT_H

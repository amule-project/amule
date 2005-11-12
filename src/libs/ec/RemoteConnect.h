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

#ifndef REMOTECONNECT_H
#define REMOTECONNECT_H

#include <wx/string.h>

#include "ECSocket.h"

class CECPacket;

class CRemoteConnect : public CECSocket {

public:
	// The event handler is used for notifying connect/close 
	CRemoteConnect(wxEvtHandler* evt_handler);
		
	bool ConnectToCore(const wxString &host, int port, 
						const wxString& login, const wxString &pass,
						const wxString& client, const wxString& version);

	bool ConnectionEstablished();

	CECPacket *SendRecv(CECPacket *);
	void Send(CECPacket *);
		
	bool Busy() { return m_busy; }
		
	const wxString& GetServerReply() const { return server_reply; }

	virtual void OnConnect(); // To override connection events
	virtual void OnClose(); // To override close events
	
private:

	wxEvtHandler* notifier;
	bool m_busy;
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

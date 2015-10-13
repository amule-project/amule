//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "ListenSocket.h"	// Interface declarations

#include <common/EventIDs.h>

#include "ClientTCPSocket.h"	// Needed for CClientRequestSocket
#include "Logger.h"			// Needed for AddLogLineM
#include "Statistics.h"		// Needed for theStats
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"		// Needed for theApp
#include "ServerConnect.h"	// Needed for CServerConnect

//-----------------------------------------------------------------------------
// CListenSocket
//-----------------------------------------------------------------------------
//
// This is the socket that listens to incoming connections in aMule's TCP port
// As soon as a connection is detected, it creates a new socket of type
// CClientTCPSocket to handle (accept) the connection.
//

CListenSocket::CListenSocket(amuleIPV4Address &addr, const CProxyData *ProxyData)
:
// wxSOCKET_NOWAIT    - means non-blocking i/o
// wxSOCKET_REUSEADDR - means we can reuse the socket immediately (wx-2.5.3)
CSocketServerProxy(addr, MULE_SOCKET_NOWAIT|MULE_SOCKET_REUSEADDR, ProxyData)
{
	// 0.42e - vars not used by us
	m_pending = false;
	shutdown = false;
	m_OpenSocketsInterval = 0;
	totalconnectionchecks = 0;
	averageconnections = 0.0;
	memset(m_ConnectionStates, 0, 3 * sizeof(m_ConnectionStates[0]));
	// Set the listen socket event handler -- The handler is written in amule.cpp
	if (IsOk()) {
		SetEventHandler(*theApp, ID_LISTENSOCKET_EVENT);
		SetNotify(wxSOCKET_CONNECTION_FLAG);
		Notify(true);

		AddLogLineNS(_("ListenSocket: Ok."));
	} else {
		AddLogLineCS(_("ERROR: Could not listen to TCP port.") );
	}
}


CListenSocket::~CListenSocket()
{
	shutdown = true;
	Discard();
	Close();

#ifdef __DEBUG__
	// No new sockets should have been opened by now
	for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); ++it) {
		wxASSERT((*it)->IsDestroying());
	}
#endif

	KillAllSockets();
}


void CListenSocket::OnAccept()
{
	m_pending = theApp->IsRunning();	// just do nothing if we are shutting down
	// If the client is still at maxconnections,
	// this will allow it to go above it ...
	// But if you don't, you will get a lowID on all servers.
	while (m_pending && (theApp->serverconnect->IsConnecting() || !TooManySockets())) {
		if (!SocketAvailable()) {
			m_pending = false;
		} else {
			// Create a new socket to deal with the connection
			CClientTCPSocket* newclient = new CClientTCPSocket();
			// Accept the connection and give it to the newly created socket
			if (!AcceptWith(*newclient, false)) {
				newclient->Safe_Delete();
				m_pending = false;
			} else {
				if (!newclient->InitNetworkData()) {
					// IP or port were not returned correctly
					// from the accepted address, or filtered.
					newclient->Safe_Delete();
				}
			}
		}
	}
	if (m_pending) {
		theStats::AddMaxConnectionLimitReached();
	}
}

void CListenSocket::AddConnection()
{
	m_OpenSocketsInterval++;
}

void CListenSocket::Process()
{
	// 042e + Kry changes for Destroy
	m_OpenSocketsInterval = 0;
	SocketSet::iterator it = socket_list.begin();
	while ( it != socket_list.end() ) {
		CClientTCPSocket* cur_socket = *it++;
		if (!cur_socket->IsDestroying()) {
			cur_socket->CheckTimeOut();
		}
	}

	if (m_pending) {
		OnAccept();
	}
}

void CListenSocket::RecalculateStats()
{
	// 0.42e
	memset(m_ConnectionStates, 0, 3 * sizeof(m_ConnectionStates[0]));
	for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); ) {
		CClientTCPSocket* cur_socket = *it++;
		switch (cur_socket->GetConState()) {
			case ES_DISCONNECTED:
				m_ConnectionStates[0]++;
				break;
			case ES_NOTCONNECTED:
				m_ConnectionStates[1]++;
				break;
			case ES_CONNECTED:
				m_ConnectionStates[2]++;
				break;
		}
	}
}

void CListenSocket::AddSocket(CClientTCPSocket* toadd)
{
	wxASSERT(toadd);
	socket_list.insert(toadd);
	theStats::AddActiveConnection();
}

void CListenSocket::RemoveSocket(CClientTCPSocket* todel)
{
	wxASSERT(todel);
	socket_list.erase(todel);
	theStats::RemoveActiveConnection();
}

void CListenSocket::KillAllSockets()
{
	// 0.42e reviewed - they use delete, but our safer is Destroy...
	// But I bet it would be better to call Safe_Delete on the socket.
	// Update: no... Safe_Delete MARKS for deletion. We need to delete it.
	for (SocketSet::iterator it = socket_list.begin(); it != socket_list.end(); ) {
		CClientTCPSocket* cur_socket = *it++;
		if (cur_socket->GetClient()) {
			cur_socket->Safe_Delete_Client();
		} else {
			cur_socket->Safe_Delete();
			cur_socket->Destroy();
		}
	}
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval)
{
	if (GetOpenSockets() > thePrefs::GetMaxConnections()
		|| (!bIgnoreInterval && m_OpenSocketsInterval > (thePrefs::GetMaxConperFive() * GetMaxConperFiveModifier()))) {
		return true;
	} else {
		return false;
	}
}

bool CListenSocket::IsValidSocket(CClientTCPSocket* totest)
{
	// 0.42e
	return socket_list.find(totest) != socket_list.end();
}


void CListenSocket::UpdateConnectionsStatus()
{
	// 0.42e xcept for the khaos stats
	if( theApp->IsConnected() ) {
		totalconnectionchecks++;
		float percent;
		percent = (float)(totalconnectionchecks-1)/(float)totalconnectionchecks;
		if( percent > .99f ) {
			percent = .99f;
		}
		averageconnections = (averageconnections*percent) + (float)GetOpenSockets()*(1.0f-percent);
	}
}


float CListenSocket::GetMaxConperFiveModifier()
{
	float SpikeSize = GetOpenSockets() - averageconnections;
	if ( SpikeSize < 1 ) {
		return 1;
	}

	float SpikeTolerance = 2.5f * thePrefs::GetMaxConperFive();
	if ( SpikeSize > SpikeTolerance ) {
		return 0;
	}

	return 1.0f - (SpikeSize/SpikeTolerance);
}
// File_checked_for_headers

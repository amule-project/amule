//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "ServerConnect.h"	// Interface declarations.

#include <include/protocol/Protocols.h>
#include <include/protocol/ed2k/ClientSoftware.h> // Sometimes we reply with TCP packets.
#include <include/tags/ClientTags.h>
#include <include/common/ClientVersion.h>
#include <include/common/EventIDs.h>

#include "SearchList.h"		// Needed for CSearchList
#include "ServerUDPSocket.h"	// Needed for CServerUDPSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "Packet.h"		// Needed for CTag
#include "MemFile.h"		// Needed for CMemFile
#include "ServerSocket.h"	// Needed for CServerSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "Server.h"		// Needed for CServer
#include "amule.h"		// Needed for theApp
#include "ServerList.h"		// Needed for CServerList
#include "Preferences.h"	// Needed for CPreferences
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include "GuiEvents.h"		// Needed for Notify_*
#include <common/Format.h>


//#define DEBUG_CLIENT_PROTOCOL


void CServerConnect::TryAnotherConnectionrequest()
{
	if ( connectionattemps.size() < (( thePrefs::IsSafeServerConnectEnabled()) ? 1 : 2) ) {
	
		CServer*  next_server = used_list->GetNextServer(m_bTryObfuscated);

		if ( thePrefs::AutoConnectStaticOnly() ) {
			while (next_server && !next_server->IsStaticMember()) {
				next_server = used_list->GetNextServer(m_bTryObfuscated);
			}
		}

		if (!next_server) {
			if ( connectionattemps.empty() ) {
				if (m_bTryObfuscated && !thePrefs::IsClientCryptLayerRequired()){
					AddLogLineM(true, _("Failed to connect to all obfuscated servers listed. Making another pass without obfuscation."));					
					// try all servers on the non-obfuscated port next
					m_bTryObfuscated = false;
					ConnectToAnyServer( false, true);
				} else {					
					AddLogLineM(true, _("Failed to connect to all servers listed. Making another pass."));
					ConnectToAnyServer( false );
				}
			}
			return;
		}

		ConnectToServer(next_server, true, !m_bTryObfuscated);
	}
}

void CServerConnect::ConnectToAnyServer(bool prioSort, bool bNoCrypt)
{
	if (!thePrefs::GetNetworkED2K()){
		AddLogLineM(true,_("ED2K network disabled on preferences, not connecting."));
		return;
	}

	StopConnectionTry();
	Disconnect();
	connecting = true;
	singleconnecting = false;
	m_bTryObfuscated = thePrefs::IsServerCryptLayerTCPRequested() && !bNoCrypt;
		
	// Barry - Only auto-connect to static server option
	if (thePrefs::AutoConnectStaticOnly()) {
		bool anystatic = false;
		CServer *next_server; 
		used_list->ResetServerPos();
		while ((next_server = used_list->GetNextServer(false)) != NULL) {
			if (next_server->IsStaticMember()) {
				anystatic = true;
				break;
			}
		}
		if (!anystatic) {
			connecting = false;
			AddLogLineM(true,_("No valid servers to connect in serverlist found"));
			return;
		}
	}

	if ( thePrefs::Score() && prioSort ) {
		used_list->Sort();
	}
	
	used_list->ResetServerPos();

	if (used_list->GetServerCount()==0 ) {
		connecting = false;
		AddLogLineM(true,_("No valid servers to connect in serverlist found"));
		return;
	}
	
	theApp->listensocket->Process();

	TryAnotherConnectionrequest();
}


void CServerConnect::ConnectToServer(CServer* server, bool multiconnect, bool bNoCrypt)
{	
	if (!thePrefs::GetNetworkED2K()){
		AddLogLineM(true,_("ED2K network disabled on preferences, not connecting."));
		return;
	}
	
	if (!multiconnect) {
		StopConnectionTry();
		Disconnect();
	}
	connecting = true;
	singleconnecting = !multiconnect;

	CServerSocket* newsocket = new CServerSocket(this, thePrefs::GetProxyData());
	m_lstOpenSockets.push_back(newsocket);
	newsocket->ConnectToServer(server, bNoCrypt);

	connectionattemps[GetTickCount()] = newsocket;
}


void CServerConnect::StopConnectionTry()
{
	connectionattemps.clear();
	connecting = false;
	singleconnecting = false;

	if (m_idRetryTimer.IsRunning()) 
	{ 
	  m_idRetryTimer.Stop();
	} 

	// close all currenty opened sockets except the one which is connected to our current server
	for(SocketsList::iterator it = m_lstOpenSockets.begin(); it != m_lstOpenSockets.end(); ) {
		CServerSocket *pSck = *it++;
		if (pSck == connectedsocket)		// don't destroy socket which is connected to server
			continue;
		if (pSck->m_bIsDeleting == false)	// don't destroy socket if it is going to destroy itself later on
			DestroySocket(pSck);
	}
}

#define CAPABLE_ZLIB				0x01
#define CAPABLE_IP_IN_LOGIN_FRAME	0x02
#define CAPABLE_AUXPORT				0x04
#define CAPABLE_NEWTAGS				0x08
#define CAPABLE_UNICODE				0x10
#define CAPABLE_LARGEFILES			0x100
#define SRVCAP_SUPPORTCRYPT     0x0200
#define SRVCAP_REQUESTCRYPT     0x0400
#define SRVCAP_REQUIRECRYPT     0x0800

void CServerConnect::ConnectionEstablished(CServerSocket* sender)
{
	if (connecting == false)
	{
		// we are already connected to another server
		DestroySocket(sender);
		return;
	}	
	
	if (sender->GetConnectionState() == CS_WAITFORLOGIN) {
		AddLogLineM(false, CFormat( _("Connected to %s (%s:%i)") )
			% sender->cur_server->GetListName()
			% sender->cur_server->GetFullIP()
			% sender->cur_server->GetPort() );

		//send loginpacket
		CServer* update = theApp->serverlist->GetServerByAddress( sender->cur_server->GetAddress(), sender->cur_server->GetPort() );
		if (update){
			update->ResetFailedCount();
			Notify_ServerRefresh( update );
		}
		
		CMemFile data(256);
		data.WriteHash(thePrefs::GetUserHash());
		// Why pass an ID, if we are loggin in?
		data.WriteUInt32(GetClientID());
		data.WriteUInt16(thePrefs::GetPort());
		data.WriteUInt32(4); // tagcount

		// Kry - Server doesn't support VBT tags afaik.
		// Not to mention we don't know its flags yet
		
		CTagString tagname(CT_NAME,thePrefs::GetUserNick());
		tagname.WriteTagToFile(&data);

		CTagInt32 tagversion(CT_VERSION,EDONKEYVERSION);
		tagversion.WriteTagToFile(&data);

		uint32 dwCryptFlags = 0;
		
		if (thePrefs::IsClientCryptLayerSupported()) {
			dwCryptFlags |= SRVCAP_SUPPORTCRYPT;
		}
		
		if (thePrefs::IsClientCryptLayerRequested()) {
			dwCryptFlags |= SRVCAP_REQUESTCRYPT;
		}
		
		if (thePrefs::IsClientCryptLayerRequired()) {
			dwCryptFlags |= SRVCAP_REQUIRECRYPT;
		}
		
		// FLAGS for server connection
		CTagInt32 tagflags(CT_SERVER_FLAGS, CAPABLE_ZLIB 
								| CAPABLE_AUXPORT 
								| CAPABLE_NEWTAGS 
								| CAPABLE_UNICODE
								| CAPABLE_LARGEFILES
								| dwCryptFlags
											); 
		
		tagflags.WriteTagToFile(&data);

		// eMule Version (14-Mar-2004: requested by lugdunummaster (need for LowID clients which have no chance 
		// to send an Hello packet to the server during the callback test))
		CTagInt32 tagMuleVersion(CT_EMULE_VERSION, 
			(SO_AMULE	<< 24) |
			make_full_ed2k_version(VERSION_MJR, VERSION_MIN, VERSION_UPDATE)
			 );
		tagMuleVersion.WriteTagToFile(&data);

		CPacket* packet = new CPacket(data, OP_EDONKEYPROT, OP_LOGINREQUEST);
		#ifdef DEBUG_CLIENT_PROTOCOL
		AddLogLineM(true,wxT("Client: OP_LOGINREQUEST"));
		AddLogLineM(true,wxString(wxT("        Hash     : ")) << thePrefs::GetUserHash().Encode());
		AddLogLineM(true,wxString(wxT("        ClientID : ")) << GetClientID());
		AddLogLineM(true,wxString(wxT("        Port     : ")) << thePrefs::GetPort());
		AddLogLineM(true,wxString(wxT("        User Nick: ")) << thePrefs::GetUserNick());
		AddLogLineM(true,wxString(wxT("        Edonkey  : ")) << EDONKEYVERSION);
		#endif
		theStats::AddUpOverheadServer(packet->GetPacketSize());
		SendPacket(packet, true, sender);
	} else if (sender->GetConnectionState() == CS_CONNECTED){
		theStats::AddReconnect();
		theStats::GetServerConnectTimer()->ResetTimer();
		connected = true;
		AddLogLineM(true, CFormat( _("Connection established on: %s") ) % sender->cur_server->GetListName());
		connectedsocket = sender;
		
		StopConnectionTry();
		
		CServer* update = theApp->serverlist->GetServerByAddress(connectedsocket->cur_server->GetAddress(),sender->cur_server->GetPort());
		if ( update ) {
			Notify_ServerHighlight(update, true);
		}
		
		theApp->sharedfiles->ClearED2KPublishInfo();

		Notify_ServerRemoveDead();
		
		// tecxx 1609 2002 - serverlist update
		if (thePrefs::AddServersFromServer()) {
			CPacket* packet = new CPacket(OP_GETSERVERLIST, 0, OP_EDONKEYPROT);
			theStats::AddUpOverheadServer(packet->GetPacketSize());
			SendPacket(packet, true);
			#ifdef DEBUG_CLIENT_PROTOCOL
			AddLogLineM(true,wxT("Client: OP_GETSERVERLIST"));
			#endif
		}
	}
	
	theApp->ShowConnectionState();
}


bool CServerConnect::SendPacket(CPacket* packet,bool delpacket, CServerSocket* to)
{
	if (!to) {
		if (connected) {
			connectedsocket->SendPacket(packet, delpacket, true);
			return true;
		} else {
			if ( delpacket ) {
				delete packet;
			}
			
			return false;
		}
	} else {
		to->SendPacket(packet, delpacket, true);
		return true;
	}
}


bool CServerConnect::SendUDPPacket(CPacket* packet, CServer* host, bool delpacket, bool rawpacket, uint16 port_offset)
{
	if (connected) {
		serverudpsocket->SendPacket(packet, host, delpacket, rawpacket, port_offset);
	} else if (delpacket) {
		delete packet;
	}

	return true;
}


void CServerConnect::ConnectionFailed(CServerSocket* sender)
{
	if (connecting == false && sender != connectedsocket)
	{
		// just return, cleanup is done by the socket itself
		return;
	}
	//messages
	CServer* pServer = theApp->serverlist->GetServerByAddress(sender->cur_server->GetAddress(), sender->cur_server->GetPort());
	switch (sender->GetConnectionState()){
		case CS_FATALERROR:
			AddLogLineM(true, _("Fatal Error while trying to connect. Internet connection might be down"));
			break;
		case CS_DISCONNECTED:
			theApp->sharedfiles->ClearED2KPublishInfo();
			AddLogLineM(false,CFormat( _("Lost connection to %s (%s:%i)") )
				% sender->cur_server->GetListName()
				% sender->cur_server->GetFullIP()
				% sender->cur_server->GetPort() );

			if (pServer){
				Notify_ServerHighlight(pServer, false);
			}
			break;
		case CS_SERVERDEAD:
			AddLogLineM(false, CFormat( _("%s (%s:%i) appears to be dead.") )
				% sender->cur_server->GetListName()
				% sender->cur_server->GetFullIP()
				% sender->cur_server->GetPort() );

			if (pServer) {
				pServer->AddFailedCount();
				Notify_ServerRefresh( pServer );
			}
			break;
		case CS_ERROR:
			break;
		case CS_SERVERFULL:
			AddLogLineM(false, CFormat( _("%s (%s:%i) appears to be full.") )
				% sender->cur_server->GetListName()
				% sender->cur_server->GetFullIP()
				% sender->cur_server->GetPort() );
			
			break;
		case CS_NOTCONNECTED:; 
			break; 
	}

	// IMPORTANT: mark this socket not to be deleted in StopConnectionTry(), 
	// because it will delete itself after this function!
	sender->m_bIsDeleting = true;

	switch (sender->GetConnectionState()) {
		case CS_FATALERROR:{
			bool autoretry= !singleconnecting;
			StopConnectionTry();
			if ((thePrefs::Reconnect()) && (autoretry) && (!m_idRetryTimer.IsRunning())){ 
				AddLogLineM(false, wxString::Format(_("Automatic connection to server will retry in %d seconds"), CS_RETRYCONNECTTIME)); 
				m_idRetryTimer.Start(1000*CS_RETRYCONNECTTIME);
			}
			break;
		}
		case CS_DISCONNECTED:{
			theApp->sharedfiles->ClearED2KPublishInfo();		
			connected = false;
			Notify_ServerHighlight(sender->cur_server,false);
			if (connectedsocket)  {
				connectedsocket->Close();
			}
			connectedsocket = NULL;
			theApp->searchlist->StopGlobalSearch();			
			Notify_SearchCancel();
			theStats::GetServerConnectTimer()->StopTimer();
			if (thePrefs::Reconnect() && !connecting){
				ConnectToAnyServer();		
			}
			
			AddLogLineM( true, _("Connection lost") );
			break;
		}
		case CS_ERROR:
		case CS_NOTCONNECTED:{
			if (!connecting)
				break;
			AddLogLineM(false, CFormat( _("Connecting to %s (%s:%i) failed.") )
				% sender->info
				% sender->cur_server->GetFullIP()
				% sender->cur_server->GetPort() );
		}
		case CS_SERVERDEAD:
		case CS_SERVERFULL:{
			if (!connecting) {
				break;
			}
			if (singleconnecting) {
				if (pServer && sender->IsServerCryptEnabledConnection() && !thePrefs::IsClientCryptLayerRequired()){
					// try reconnecting without obfuscation
					ConnectToServer(pServer, false, true);
					break;
				}
				
				StopConnectionTry();
				break;
			}

			ServerSocketMap::iterator it = connectionattemps.begin();
			while ( it != connectionattemps.end() ){
				if ( it->second == sender ) {
					connectionattemps.erase( it );
					break;
				} 
				++it;
			}			
			TryAnotherConnectionrequest();
		}
	}
	theApp->ShowConnectionState();
}

void CServerConnect::CheckForTimeout()
{
	uint32 dwCurTick = GetTickCount();

	ServerSocketMap::iterator it = connectionattemps.begin();
	while ( it != connectionattemps.end() ){
		if ( !it->second ) {
			AddLogLineM(false, _("Error: Socket invalid at timeoutcheck"));
			connectionattemps.erase( it );
			return;
		}

		if ( dwCurTick - it->first > CONSERVTIMEOUT) {
			uint32 key = it->first;
			CServerSocket* value = it->second;
			++it;
			if (!value->IsSolving()) {
				AddLogLineM(false, CFormat( _("Connection attempt to %s (%s:%i) timed out.") )
					% value->info
					% value->cur_server->GetFullIP()
					% value->cur_server->GetPort() );
			
				connectionattemps.erase( key );
	
				TryAnotherConnectionrequest();
				DestroySocket( value );
			}				
		} else {
			++it;
		}
	}
}


bool CServerConnect::Disconnect()
{
	if (connected && connectedsocket) {
		theApp->sharedfiles->ClearED2KPublishInfo();

		connected = false;

		CServer* update = theApp->serverlist->GetServerByAddress(
			connectedsocket->cur_server->GetAddress(),
			connectedsocket->cur_server->GetPort());
		Notify_ServerHighlight(update, false);
		theApp->SetPublicIP(0);
		DestroySocket(connectedsocket);
		connectedsocket = NULL;
		theApp->ShowConnectionState();
		theStats::GetServerConnectTimer()->StopTimer();
		return true;
	} else {
		return false;
	}
}


CServerConnect::CServerConnect(CServerList* in_serverlist, amuleIPV4Address &address)
: m_idRetryTimer(theApp,ID_SERVER_RETRY_TIMER_EVENT)
{
	connectedsocket = NULL;
	used_list = in_serverlist;
	max_simcons = (thePrefs::IsSafeServerConnectEnabled()) ? 1 : 2;
	connecting = false;
	connected = false;
	clientid = 0;
	singleconnecting = false;

	// initalize socket for udp packets
	serverudpsocket = new CServerUDPSocket(address, thePrefs::GetProxyData());
}


CServerConnect::~CServerConnect()
{
	m_idRetryTimer.Stop();
	// stop all connections
	StopConnectionTry();
	// close connected socket, if any
	DestroySocket(connectedsocket);
	connectedsocket = NULL;
	// close udp socket
	delete serverudpsocket;
}


CServer* CServerConnect::GetCurrentServer()
{
	if (IsConnected() && connectedsocket) {
		return connectedsocket->cur_server;
	}
	return NULL;
}


void CServerConnect::SetClientID(uint32 newid)
{
	clientid = newid;
	
	if (!::IsLowID(newid)) {
		theApp->SetPublicIP(newid);
	}
}


void CServerConnect::DestroySocket(CServerSocket* pSck)
{
	if (pSck == NULL) {
		return;
	}
	m_lstOpenSockets.remove(pSck);
	pSck->Destroy();
}


bool CServerConnect::IsLocalServer(uint32 dwIP, uint16 nPort)
{
	if (IsConnected()){
		if (connectedsocket->cur_server->GetIP() == dwIP && connectedsocket->cur_server->GetPort() == nPort)
			return true;
	}
	return false;
}


void CServerConnect::KeepConnectionAlive()
{
	uint32 dwServerKeepAliveTimeout = thePrefs::GetServerKeepAliveTimeout();
	if (dwServerKeepAliveTimeout && connected && connectedsocket &&
	connectedsocket->connectionstate == CS_CONNECTED &&
	GetTickCount() - connectedsocket->GetLastTransmission() >= dwServerKeepAliveTimeout) {
		// "Ping" the server if the TCP connection was not used for the specified interval with
		// an empty publish files packet -> recommended by lugdunummaster himself!
		
		CMemFile files(4);
		files.WriteUInt32(0); //nFiles
	
		CPacket* packet = new CPacket(files, OP_EDONKEYPROT, OP_OFFERFILES);
		#ifdef DEBUG_CLIENT_PROTOCOL
		AddLogLineM(true,wxT("Client: OP_OFFERFILES"));
		#endif
		// compress packet
		//   - this kind of data is highly compressable (N * (1 MD4 and at least 3 string meta data tags and 1 integer meta data tag))
		//   - the min. amount of data needed for one published file is ~100 bytes
		//   - this function is called once when connecting to a server and when a file becomes shareable - so, it's called rarely.
		//   - if the compressed size is still >= the original size, we send the uncompressed packet
		// therefor we always try to compress the packet
		theStats::AddUpOverheadServer(packet->GetPacketSize());
		connectedsocket->SendPacket(packet,true);
		
		AddDebugLogLineM(false, logServer, wxT("Refreshing server connection"));
 	}
}

// true if the IP is one of a server which we currently try to connect to
bool CServerConnect::AwaitingTestFromIP(uint32 dwIP)
{
	ServerSocketMap::iterator it = connectionattemps.begin();

	while (it != connectionattemps.end()) {
		const CServerSocket* serversocket = it->second;
		if (serversocket && (serversocket->GetConnectionState() == CS_WAITFORLOGIN) &&
			(serversocket->GetServerIP() == dwIP)) {
			return true;
		}
		++it;
	}
	
	return false;
}

bool CServerConnect::IsConnectedObfuscated() const {
	return connectedsocket != NULL && connectedsocket->IsObfusicating();
}
// File_checked_for_headers

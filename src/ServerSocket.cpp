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

#include "ServerSocket.h"	// Interface declarations

#include <protocol/Protocols.h>
#include <common/EventIDs.h>
#include <tags/ServerTags.h>

#include <wx/tokenzr.h>

#include "Packet.h"		// Needed for CPacket
#include "updownclient.h"	// Needed for CUpDownClient
#include "ClientList.h"		// Needed for CClientList
#include "MemFile.h"		// Needed for CMemFile
#include "PartFile.h"		// Needed for CPartFile
#include "SearchList.h"		// Needed for CSearchList
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ServerList.h"		// Needed for CServerList
#include "Server.h"		// Needed for CServer
#include "amule.h"		// Needed for theApp
#include "Statistics.h"		// Needed for theStats
#include "AsyncDNS.h" // Needed for CAsyncDNS
#include "Logger.h"
#include <common/Format.h>
#include "IPFilter.h"
#include "GuiEvents.h"		// Needed for Notify_*
#ifdef ASIO_SOCKETS
#	include <boost/system/error_code.hpp>
#endif


#ifndef ASIO_SOCKETS

//------------------------------------------------------------------------------
// CServerSocketHandler
//------------------------------------------------------------------------------

class CServerSocketHandler: public wxEvtHandler
{
public:
	CServerSocketHandler() {};

public:
private:
	void ServerSocketHandler(wxSocketEvent& event);
	DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(CServerSocketHandler, wxEvtHandler)
	EVT_SOCKET(ID_SERVERSOCKET_EVENT, CServerSocketHandler::ServerSocketHandler)
END_EVENT_TABLE()

void CServerSocketHandler::ServerSocketHandler(wxSocketEvent& event)
{
	CServerSocket *socket = dynamic_cast<CServerSocket *>(event.GetSocket());
	wxASSERT(socket);
	if (!socket) {
		return;
	}

	if (socket->IsDestroying()) {
		return;
	}

	switch(event.GetSocketEvent()) {
		case wxSOCKET_CONNECTION:
			socket->OnConnect(wxSOCKET_NOERROR);
			break;
		case wxSOCKET_LOST:
			socket->OnError(socket->LastError());
			break;
		case wxSOCKET_INPUT:
			socket->OnReceive(wxSOCKET_NOERROR);
			break;
		case wxSOCKET_OUTPUT:
			socket->OnSend(wxSOCKET_NOERROR);
			break;
		default:
			wxFAIL;
			break;
	}


}

//
// There can be only one. :)
//
static CServerSocketHandler g_serverSocketHandler;

#endif /* !ASIO_SOCKETS */


//------------------------------------------------------------------------------
// CServerSocket
//------------------------------------------------------------------------------

CServerSocket::CServerSocket(CServerConnect* in_serverconnect, const CProxyData *ProxyData)
:
CEMSocket(ProxyData)
{
	serverconnect = in_serverconnect;
	connectionstate = 0;
	cur_server = 0;
	info.Clear();
	m_bIsDeleting = false;

#ifndef ASIO_SOCKETS
	SetEventHandler(g_serverSocketHandler, ID_SERVERSOCKET_EVENT);

	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
#endif
	Notify(true);

	m_dwLastTransmission = 0;
	m_IsSolving = false;
	m_bNoCrypt = false;
}

CServerSocket::~CServerSocket()
{
#ifndef ASIO_SOCKETS
	// remove event handler...
	SetNotify(0);
	Notify(FALSE);
#endif

	if (cur_server) {
		delete cur_server;
	}
	cur_server = NULL;
}


void CServerSocket::OnConnect(int nErrorCode)
{
	switch (nErrorCode) {
#ifdef ASIO_SOCKETS
		case boost::system::errc::success:
#else
		case wxSOCKET_NOERROR:
#endif
			if (cur_server->HasDynIP()) {
				uint32 server_ip = GetPeerInt();
				cur_server->SetID(server_ip);
				// GetServerByAddress may return NULL, so we must test!
				// This was the reason why amule would crash when trying to
				// connect in wxWidgets 2.5.2
				CServer *pServer = theApp->serverlist->GetServerByAddress(
					cur_server->GetAddress(), cur_server->GetPort());
				if (pServer) {
					pServer->SetID(server_ip);
				} else {
					AddDebugLogLineN(logServer, wxT("theApp->serverlist->GetServerByAddress() returned NULL"));
					return;
				}
			}
			SetConnectionState(CS_WAITFORLOGIN);
			break;

#ifdef ASIO_SOCKETS
		case boost::system::errc::address_in_use:
		case boost::system::errc::address_not_available:
		case boost::system::errc::bad_address:
		case boost::system::errc::connection_refused:
		case boost::system::errc::host_unreachable:
		case boost::system::errc::invalid_argument:
		case boost::system::errc::timed_out:
#else
		case wxSOCKET_INVADDR:
		case wxSOCKET_NOHOST:
		case wxSOCKET_INVPORT:
		case wxSOCKET_TIMEDOUT:
#endif
			m_bIsDeleting = true;
			SetConnectionState(CS_SERVERDEAD);
			serverconnect->DestroySocket(this);
			return;

		default:
			m_bIsDeleting = true;
			SetConnectionState(CS_FATALERROR);
			serverconnect->DestroySocket(this);
			return;

	}

}

void CServerSocket::OnReceive(int nErrorCode)
{
	if (connectionstate != CS_CONNECTED && !serverconnect->IsConnecting()) {
		serverconnect->DestroySocket(this);
		return;
	}
	CEMSocket::OnReceive(nErrorCode);
	m_dwLastTransmission = GetTickCount();
}

bool CServerSocket::ProcessPacket(const byte* packet, uint32 size, int8 opcode)
{
	try {
		AddDebugLogLineN( logServer, wxT("Processing Server Packet: ") );

		switch(opcode) {
			case OP_SERVERMESSAGE: {
				/* Kry import of lugdunum 16.40 new features */
				AddDebugLogLineN( logServer, wxT("Server: OP_SERVERMESSAGE") );

				theStats::AddDownOverheadServer(size);
				char* buffer = new char[size-1];
				memcpy(buffer,&packet[2],size-2);
				buffer[size-2] = 0;

				wxString strMessages(char2unicode(buffer));

				delete[] buffer;

				// 16.40 servers do not send separate OP_SERVERMESSAGE packets for each line;
				// instead of this they are sending all text lines with one OP_SERVERMESSAGE packet.

				wxStringTokenizer token(strMessages,wxT("\r\n"),wxTOKEN_DEFAULT );

				while (token.HasMoreTokens()) {
					wxString message = token.GetNextToken();

					bool bOutputMessage = true;
					if (message.StartsWith(wxT("server version"))) {
						wxString strVer = message.Mid(15,64); // truncate string to avoid misuse by servers in showing ads
						strVer.Trim();
						CServer* eserver = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						if (eserver) {
							eserver->SetVersion(strVer);
							Notify_ServerRefresh(eserver);
						}
					} else if (message.StartsWith(wxT("ERROR"))) {
						CServer* pServer = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();
						} else {
							servername = _("Server");
						}
						AddLogLineN(CFormat( _("ERROR: %s (%s) - %s") )
							% servername
							% Uint32_16toStringIP_Port(cur_server->GetIP(), cur_server->GetPort())
							% message.Mid(5,message.Len()).Trim(wxT(" :")));
						bOutputMessage = false;

					} else if (message.StartsWith(wxT("WARNING"))) {

						CServer* pServer = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();
						} else {
							servername = _("Server");
						}
						AddLogLineN(CFormat( _("WARNING: %s (%s) - %s") )
							% servername
							% Uint32_16toStringIP_Port(cur_server->GetIP(), cur_server->GetPort())
							% message.Mid(5,message.Len()).Trim(wxT(" :")));

						bOutputMessage = false;
					}

					if (message.Find(wxT("[emDynIP: ")) != (-1) && message.Find(wxT("]")) != (-1) && message.Find(wxT("[emDynIP: ")) < message.Find(wxT("]"))){
						wxString dynip = message.Mid(message.Find(wxT("[emDynIP: "))+10,message.Find(wxT("]")) - (message.Find(wxT("[emDynIP: "))+10));
						dynip.Trim(wxT(" "));
						if ( dynip.Length() && dynip.Length() < 51){
							CServer* eserver = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
							if (eserver){
								eserver->SetDynIP(dynip);
								cur_server->SetDynIP(dynip);
								Notify_ServerRefresh(eserver);
							}
						}
					}

					if (bOutputMessage) {
						theApp->AddServerMessageLine(message);
					}
				}
				break;
			}
			case OP_IDCHANGE: {
				AddDebugLogLineN(logServer, wxT("Server: OP_IDCHANGE"));

				theStats::AddDownOverheadServer(size);

				if (size < 4 /* uint32 (ID)*/) {
					throw wxString(wxT("Corrupt or invalid loginanswer from server received"));
				}

				CMemFile data(packet, size);

				uint32 new_id = data.ReadUInt32();

				// save TCP flags in 'cur_server'
				wxASSERT(cur_server);
				CServer* pServer = NULL;
				if (cur_server) {
					uint32 ConnPort = 0;
					uint32 rport = cur_server->GetConnPort();
					pServer = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(), rport);
					if (size >= 4+4 /* uint32 (ID) + uint32 (TCP flags)*/) {
						cur_server->SetTCPFlags(data.ReadUInt32());
						if (size >= 4+4+4 /* uint32 (ID) + uint32 (TCP flags) + uint32 (aux port) */) {
							// aux port login : we should use the 'standard' port of this server to advertize to other clients
							ConnPort = data.ReadUInt32();
							cur_server->SetPort(ConnPort);
							if (cur_server->GetAuxPortsList().IsEmpty()) {
								cur_server->SetAuxPortsList(CFormat(wxT("%u")) % rport);
							}
						}
					} else {
						cur_server->SetTCPFlags(0);
					}
					// copy TCP flags into the server in the server list
					if (pServer) {
						pServer->SetTCPFlags(cur_server->GetTCPFlags());
						if (ConnPort) {
							pServer->SetPort(ConnPort);
							if (pServer->GetAuxPortsList().IsEmpty()) {
								pServer->SetAuxPortsList(CFormat(wxT("%u")) % pServer->GetConnPort());
							}
							Notify_ServerRefresh(pServer);
							Notify_ServerUpdateED2KInfo();
						}
					}
				}

				uint32 dwServerReportedIP = 0;
				if (size >= 4 + 4 + 4 + 4 + 4 /* All of the above + reported ip + obfuscation port */) {
					dwServerReportedIP = data.ReadUInt32();
					if (::IsLowID(dwServerReportedIP)){
						wxFAIL;
						dwServerReportedIP = 0;
					}
					wxASSERT( dwServerReportedIP == new_id || ::IsLowID(new_id) );
					uint32 dwObfuscationTCPPort = data.ReadUInt32();
					if (dwObfuscationTCPPort != 0) {
						if (cur_server != NULL) {
							cur_server->SetObfuscationPortTCP(dwObfuscationTCPPort);
						}
						if (pServer != NULL) {
							pServer->SetObfuscationPortTCP(dwObfuscationTCPPort);
						}
					}
				}

				if (new_id == 0) {
					uint8 state = thePrefs::GetSmartIdState();
					if ( state > 0 ) {
						state++;
						if(state > 3) {
							thePrefs::SetSmartIdState(0);
						} else {
							thePrefs::SetSmartIdState(state);
						}
					}
					break;
				}
				if(thePrefs::GetSmartIdCheck()) {
					if (!IsLowID(new_id)) {
						thePrefs::SetSmartIdState(1);
					} else {
						uint8 state = thePrefs::GetSmartIdState();
						if ( state > 0 ) {
							state++;
							if(state > 3) {
								thePrefs::SetSmartIdState(0);
							} else {
								thePrefs::SetSmartIdState(state);
							}
							break;
						}
					}
				}

				// we need to know our client when sending our shared files (done indirectly on SetConnectionState)

				serverconnect->SetClientID(new_id);

				if (::IsLowID(new_id) && dwServerReportedIP != 0) {
					theApp->SetPublicIP(dwServerReportedIP);
				}

				if (connectionstate != CS_CONNECTED) {
					AddDebugLogLineN(logServer, wxT("Connected"));

					SetConnectionState(CS_CONNECTED);
					theApp->OnlineSig();       // Added By Bouc7
				}

				theApp->ShowConnectionState();

				AddLogLineN(CFormat(_("New clientid is %u")) % new_id);
				if (::IsLowID(new_id)) {
					AddLogLineC(_("WARNING: You have received Low-ID!"));
					AddLogLineN(_("\tMost likely this is because you're behind a firewall or router."));
					AddLogLineN(_("\tFor more information, please refer to http://wiki.amule.org"));
				}

				theApp->downloadqueue->ResetLocalServerRequests();
				break;
			}
			case OP_SEARCHRESULT: {
				AddDebugLogLineN(logServer, wxT("Server: OP_SEARCHRESULT"));

				theStats::AddDownOverheadServer(size);
				CServer* cur_srv = (serverconnect) ?
					serverconnect->GetCurrentServer() : NULL;
				theApp->searchlist->ProcessSearchAnswer(
					packet,
					size,
					true /*(cur_srv && cur_srv->GetUnicodeSupport())*/,
					cur_srv ? cur_srv->GetIP() : 0,
					cur_srv ? cur_srv->GetPort() : 0);
				theApp->searchlist->LocalSearchEnd();
				break;
			}
			case OP_FOUNDSOURCES_OBFU:
			case OP_FOUNDSOURCES: {
				AddDebugLogLineN(logServer, CFormat(wxT("ServerMsg - OP_FoundSources; sources = %u")) % packet[16]);
				theStats::AddDownOverheadServer(size);
				CMemFile sources(packet,size);
				CMD4Hash fileid = sources.ReadHash();
				if (CPartFile* file = theApp->downloadqueue->GetFileByID(fileid)) {
					file->AddSources(sources, cur_server->GetIP(), cur_server->GetPort(), SF_LOCAL_SERVER, (opcode == OP_FOUNDSOURCES_OBFU));
				} else {
					AddDebugLogLineN(logServer, wxT("Sources received for unknown file: ") + fileid.Encode());
				}
				break;
			}
			case OP_SERVERSTATUS: {
				AddDebugLogLineN(logServer, wxT("Server: OP_SERVERSTATUS"));
				// FIXME some statuspackets have a different size -> why? structur?
				if (size < 8) {
					throw wxString(wxT("Invalid server status packet"));
				}
				CServer* update = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort());
				if (update) {
					CMemFile data(packet, size);
					update->SetUserCount(data.ReadUInt32());
					update->SetFileCount(data.ReadUInt32());
					Notify_ServerRefresh( update );
					theApp->ShowUserCount();
				}
				break;
			}
			// Cleaned.
			case OP_SERVERIDENT: {
				AddDebugLogLineN(logServer, wxT("Server: OP_SERVERIDENT"));

				theStats::AddDownOverheadServer(size);
				if (size<38) {
					AddLogLineN(_("Unknown server info received! - too short"));
					// throw wxString(wxT("Unknown server info received!"));
					break;
				}
				CServer* update = theApp->serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
				if (update) {
					CMemFile data(packet,size);
					CMD4Hash hash = data.ReadHash();
					if (RawPeekUInt32(hash.GetHash()) == 0x2A2A2A2A){ // No endian problem here
						const wxString& rstrVersion = update->GetVersion();
						if (!rstrVersion.IsEmpty()) {
							update->SetVersion(wxT("eFarm ") + rstrVersion);
						} else {
							update->SetVersion(wxT("eFarm"));
						}
					}
					// Unused
					/*uint32 nServerIP = */data.ReadUInt32();
					/*uint16 nServerPort = */data.ReadUInt16();

					uint32 nTags = data.ReadUInt32();
					for (uint32 i = 0; i < nTags; i++){
						CTag tag(data, update->GetUnicodeSupport());
						if (tag.GetNameID() == ST_SERVERNAME){
							update->SetListName(tag.GetStr());
						} else if (tag.GetNameID() == ST_DESCRIPTION){
							update->SetDescription(tag.GetStr());
						} // No more known tags from server
					}

					theApp->ShowConnectionState();
					Notify_ServerRefresh(update);
				}
				break;
			}
			// tecxx 1609 2002 - add server's serverlist to own serverlist
			case OP_SERVERLIST: {
				AddDebugLogLineN(logServer, wxT("Server: OP_SERVERLIST"));

				CMemFile* servers = new CMemFile(packet,size);
				uint8 count = servers->ReadUInt8();
				if (((int32)(count*6 + 1) > size)) {
					count = 0;
				}
				int addcount = 0;
				while(count) {
					uint32 ip	= servers->ReadUInt32();
					uint16 port = servers->ReadUInt16();
					CServer* srv = new CServer(
								port ,				// Port
								Uint32toStringIP(ip));	// Ip
					srv->SetListName(srv->GetFullIP());
					if (!theApp->AddServer(srv)) {
						delete srv;
					} else {
						addcount++;
					}
					count--;
				}
				delete servers;
				if (addcount) {
					AddLogLineN(CFormat(wxPLURAL("Received %d new server", "Received %d new servers", addcount)) % addcount);
				}
				theApp->serverlist->SaveServerMet();
				AddLogLineN(_("Saving of server-list completed."));
				break;
			}
			case OP_CALLBACKREQUESTED: {
				AddDebugLogLineN(logServer, wxT("Server: OP_CALLBACKREQUESTED"));

				theStats::AddDownOverheadServer(size);
				if (size >= 6) {
					CMemFile data(packet,size);
					uint32 dwIP = data.ReadUInt32();
					uint16 nPort = data.ReadUInt16();

					uint8 byCryptOptions = 0;
					CMD4Hash achUserHash;
					if (size >= 23){
						byCryptOptions = data.ReadUInt8();;
						achUserHash = data.ReadHash();
					}

					CUpDownClient* client = theApp->clientlist->FindClientByIP(dwIP,nPort);

					if (!client) {
						client = new CUpDownClient(nPort,dwIP,0,0,0, true, true);
						theApp->clientlist->AddClient(client);
					}
					if (size >= 23 && client->HasValidHash()){
						if (client->GetUserHash() != achUserHash){
							AddDebugLogLineN(logServer, wxT("Reported Userhash from OP_CALLBACKREQUESTED differs with our stored hash"));
							// disable crypt support since we dont know which hash is true
							client->SetCryptLayerRequest(false);
							client->SetCryptLayerSupport(false);
							client->SetCryptLayerRequires(false);
						} else {
							client->SetConnectOptions(byCryptOptions, true, false);
						}
					} else if (size >= 23) {
						client->SetUserHash(achUserHash);
						client->SetConnectOptions(byCryptOptions, true, false);
					}

					client->TryToConnect();
				}
				break;
			}
			case OP_CALLBACK_FAIL: {
				AddDebugLogLineN(logServer, wxT("Server: OP_CALLBACK_FAIL"));
				break;
			}
			case OP_REJECT: {
				AddDebugLogLineN(logServer, wxT("Server: OP_REJECT"));
				AddLogLineN(_("Server rejected last command"));
				break;
			}
			default:
				AddDebugLogLineN(logPacketErrors, CFormat(wxT("Unknown server packet with OPcode %x")) % opcode);
		}
		return true;
	} catch (const CInvalidPacket& e) {
		AddLogLineN(CFormat( _("Bogus packet received from server: %s") ) % e.what());
	} catch (const CEOFException& e) {
		AddLogLineN(CFormat( _("Bogus packet received from server: %s") ) % e.what());
	} catch (const wxString& error) {
		AddLogLineN(CFormat( _("Unhandled error while processing packet from server: %s") ) % error);
	}

	// Don't disconnect because of wrong sources.
	if (opcode==OP_SEARCHRESULT || opcode==OP_FOUNDSOURCES) {
		return true;
	}

	SetConnectionState(CS_DISCONNECTED);
	return false;
}

void CServerSocket::ConnectToServer(CServer* server, bool bNoCrypt)
{
	AddDebugLogLineN(logServer, wxT("Trying to connect"));

	if (cur_server){
		wxFAIL;
		delete cur_server;
		cur_server = NULL;
	}

	cur_server = new CServer(server);

	m_bNoCrypt = bNoCrypt;

	SetConnectionState(CS_CONNECTING);

	info = cur_server->GetListName();

	// This must be used if we want to reverse-check the addr of the server
	if (cur_server->HasDynIP() || !cur_server->GetIP()) {
		m_IsSolving = true;
		// Send it to solving thread.
		CAsyncDNS* dns = new CAsyncDNS(server->GetAddress(), DNS_SERVER_CONNECT, theApp, this);

		if ( dns->Create() == wxTHREAD_NO_ERROR ) {
			if ( dns->Run() != wxTHREAD_NO_ERROR ) {
				dns->Delete();
				AddLogLineN(CFormat( _("Cannot create DNS solving thread for connecting to %s") ) % cur_server->GetAddress());
			}
		} else {
			dns->Delete();
			AddLogLineN(CFormat( _("Cannot create DNS solving thread for connecting to %s") ) % cur_server->GetAddress());
		}
	} else {
		// Nothing to solve, we already have the IP
		OnHostnameResolved(cur_server->GetIP());
	}

}

void CServerSocket::OnError(int DEBUG_ONLY(nErrorCode))
{
	AddDebugLogLineN(logServer, CFormat(wxT("Error in serversocket: %s(%s:%i): %u"))
		% cur_server->GetListName() % cur_server->GetFullIP() % cur_server->GetPort() % nErrorCode);
	SetConnectionState(CS_DISCONNECTED);
}


bool CServerSocket::PacketReceived(CPacket* packet)
{
	AddDebugLogLineN(logServer, CFormat(wxT("Server: Packet Received: Prot %x, Opcode %x, Length %u")) % packet->GetProtocol() % packet->GetOpCode() % packet->GetPacketSize());

	if (packet->GetProtocol() == OP_PACKEDPROT) {
		if (!packet->UnPackPacket(250000)){
			AddDebugLogLineN(logZLib, CFormat(wxT("Failed to decompress server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"))
				% packet->GetProtocol() % packet->GetOpCode() % packet->GetPacketSize());
			theStats::AddDownOverheadServer(packet->GetPacketSize());
			return true;
		}

		packet->SetProtocol(OP_EDONKEYPROT);
	}

	if (packet->GetProtocol() == OP_EDONKEYPROT) {
		ProcessPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
	} else {
		AddDebugLogLineN(logServer, CFormat(wxT("Received server TCP packet with unknown protocol: protocol=0x%02x  opcode=0x%02x  size=%u"))
			% packet->GetProtocol() % packet->GetOpCode() % packet->GetPacketSize());
		theStats::AddDownOverheadServer(packet->GetPacketSize());
	}

	return true;
}


void CServerSocket::OnClose(int WXUNUSED(nErrorCode))
{
	CEMSocket::OnClose(0);

	switch (connectionstate) {
		case CS_WAITFORLOGIN:	SetConnectionState(CS_SERVERFULL);		break;
		case CS_CONNECTED:	SetConnectionState(CS_DISCONNECTED);	break;
		default:				SetConnectionState(CS_NOTCONNECTED);
	}

	serverconnect->DestroySocket(this);
}

void CServerSocket::SetConnectionState(sint8 newstate)
{
	connectionstate = newstate;
	if (newstate < CS_CONNECTING) {
		serverconnect->ConnectionFailed(this);
	} else if (newstate == CS_CONNECTED || newstate == CS_WAITFORLOGIN) {
		if (serverconnect) {
			serverconnect->ConnectionEstablished(this);
		}
	}
}


void CServerSocket::SendPacket(CPacket* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize)
{
	m_dwLastTransmission = GetTickCount();
	CEMSocket::SendPacket(packet, delpacket, controlpacket, actualPayloadSize);
}


void CServerSocket::OnHostnameResolved(uint32 ip) {

	m_IsSolving = false;
	if (ip) {
		if (theApp->ipfilter->IsFiltered(ip, true)) {
			AddLogLineC(CFormat( _("Server IP %s (%s) is filtered.  Not connecting.") )
				% Uint32toStringIP(ip) % cur_server->GetAddress() );
#ifdef ASIO_SOCKETS
			OnConnect(boost::system::errc::invalid_argument);
#else
			OnConnect(wxSOCKET_INVADDR);
#endif
		} else {
			amuleIPV4Address addr;
			addr.Hostname(ip);
			uint16 nPort = 0;
			wxString useObfuscation;
			if ( !m_bNoCrypt && thePrefs::IsServerCryptLayerTCPRequested() && cur_server->GetObfuscationPortTCP() != 0 && cur_server->SupportsObfuscationTCP()){
				nPort = cur_server->GetObfuscationPortTCP();
				useObfuscation = _("using protocol obfuscation.");
				SetConnectionEncryption(true, NULL, true);
			} else {
				nPort = cur_server->GetConnPort();
				SetConnectionEncryption(false, NULL, true);
			}

			addr.Service(nPort);

			AddLogLineN(CFormat( _("Connecting to %s (%s - %s:%i) %s") )
				% cur_server->GetListName()
				% cur_server->GetAddress()
				% cur_server->GetFullIP()
				% nPort
				% useObfuscation
			);

			AddDebugLogLineN(logServer, CFormat(wxT("Server %s(%s) Port %i"))
				% cur_server->GetAddress() % Uint32toStringIP(ip) % cur_server->GetConnPort());
			Connect(addr, false);
		}
	} else {
		AddLogLineC(CFormat( _("Could not solve dns for server %s: Unable to connect!") )
			% cur_server->GetAddress() );
#ifdef ASIO_SOCKETS
		OnConnect(boost::system::errc::host_unreachable);
#else
		OnConnect(wxSOCKET_NOHOST);
#endif
	}

}
uint32 CServerSocket::GetServerIP() const
{
	return cur_server ? cur_server->GetIP() : 0;
}
// File_checked_for_headers

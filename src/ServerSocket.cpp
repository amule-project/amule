// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <ctime>
#include <cerrno>
#include "types.h"
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
	#define EINPROGRESS WSAEINPROGRESS
	#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
	#define ECONNREFUSED WSAECONNREFUSED
	#define ENETUNREACH WSAENETUNREACH
	#define ETIMEDOUT WSAETIMEDOUT
	#define EADDRINUSE WSAEADDRINUSE
#else
#ifdef __BSD__
       #include <sys/types.h>
#endif /* __BSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <wx/tokenzr.h>

#include "ServerSocket.h"	// Interface declarations
#include "packets.h"		// Needed for Packet
#include "updownclient.h"	// Needed for CUpDownClient
#include "ClientList.h"		// Needed for CClientList
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "PartFile.h"		// Needed for CPartFile
#include "CMemFile.h"		// Needed for CMemFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "SearchList.h"		// Needed for CSearchList
#include "otherstructs.h"	// Needed for LoginAnswer_Struct
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "opcodes.h"		// Needed for OP_SERVERMESSAGE
#include "otherfunctions.h"	// Needed for GetTickCount
#include "sockets.h"		// Needed for CS_WAITFORLOGIN
#include "ServerList.h"		// Needed for CServerList
#include "server.h"		// Needed for CServer
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"		// Needed for theApp
#include "amuleIPV4Address.h" // Needed for amuleIPV4Address

//#define DEBUG_SERVER_PROTOCOL

#ifndef AMULE_DAEMON
BEGIN_EVENT_TABLE(CServerSocketHandler, wxEvtHandler)
	EVT_SOCKET(SERVERSOCKET_HANDLER, CServerSocketHandler::ServerSocketHandler)
END_EVENT_TABLE()
#endif

IMPLEMENT_DYNAMIC_CLASS(CServerSocket,CEMSocket)

CServerSocket::CServerSocket(CServerConnect* in_serverconnect)
{
	// AddLogLine(true,"Serversocket: size %d\n",sizeof(CServerSocket));

	serverconnect = in_serverconnect;
	connectionstate = 0;
	cur_server = 0;
	info= wxEmptyString;
	m_bIsDeleting = false;
	my_handler = new CServerSocketHandler(this);
#ifndef AMULE_DAEMON	
	SetEventHandler(*my_handler,SERVERSOCKET_HANDLER);
	SetNotify(wxSOCKET_CONNECTION_FLAG|wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG|wxSOCKET_LOST_FLAG);
	Notify(TRUE);
#endif
	m_dwLastTransmission = 0;	
}

CServerSocket::~CServerSocket()
{
	// remove event handler...
	SetNotify(0);
	Notify(FALSE);
	
	if (cur_server) {
		delete cur_server;
	}
	cur_server = NULL;
#ifdef AMULE_DAEMON
	printf("CServerSocket: destroying socket %p\n", this);
	my_handler->Delete();
#else
	delete my_handler;
#endif
}


void CServerSocket::OnConnect(wxSocketError nErrorCode)
{
	CALL_APP_DATA_LOCK;
	//CAsyncSocket::OnConnect(nErrorCode);
	switch (nErrorCode) {
		case wxSOCKET_NOERROR:
			if (cur_server->HasDynIP()) {
				amuleIPV4Address tmpaddr;
				GetPeer(tmpaddr);
				uint32 server_ip = StringIPtoUint32(tmpaddr.IPAddress());
				cur_server->SetID(server_ip);
				// GetServerByAddress may return NULL, so we must test!
				// This was the reason why amule would crash when trying to
				// connect in wxWidgets 2.5.2
				CServer *pServer = theApp.serverlist->GetServerByAddress(
					cur_server->GetAddress(), cur_server->GetPort());
				if (pServer) {
					pServer->SetID(server_ip);
				} else {
					AddLogLineM(false,_("theApp.serverlist->GetServerByAddress() returned NULL"));
					return;
				}
			}
			SetConnectionState(CS_WAITFORLOGIN);
			break;

		case wxSOCKET_INVADDR:
		case wxSOCKET_NOHOST:
		case wxSOCKET_INVPORT:
		case wxSOCKET_TIMEDOUT:
			m_bIsDeleting = true;
			SetConnectionState(CS_SERVERDEAD);
			serverconnect->DestroySocket(this);
			return;
		
		case wxSOCKET_IOERR:
		case wxSOCKET_MEMERR:
		case wxSOCKET_INVOP:
		default:
			m_bIsDeleting = true;
			SetConnectionState(CS_FATALERROR);
			serverconnect->DestroySocket(this);
			return;
		
	}
	
}

void CServerSocket::OnReceive(wxSocketError nErrorCode)
{
	if (connectionstate != CS_CONNECTED && !this->serverconnect->IsConnecting()) {
		serverconnect->DestroySocket(this);
		return;
	}
	CEMSocket::OnReceive((int)nErrorCode);
	m_dwLastTransmission = GetTickCount();
}

bool CServerSocket::ProcessPacket(const char* packet, uint32 size, int8 opcode)
{
	try {
		#ifdef DEBUG_SERVER_PROTOCOL
		AddLogLineM(true,wxT("Processing Server Packet: "));
		#endif
		CServer* update;
		switch(opcode) {
			case OP_SERVERMESSAGE: {
				/* Kry import of lugdunum 16.40 new features */
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_SERVERMESSAGE\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				char* buffer = new char[size-1];
				memcpy(buffer,&packet[2],size-2);
				buffer[size-2] = 0;

				wxString strMessages(char2unicode(buffer));

				delete[] buffer;
				
				// 16.40 servers do not send separate OP_SERVERMESSAGE packets for each line;
				// instead of this they are sending all text lines with one OP_SERVERMESSAGE packet.
				//wxString message = strMessages.Tokenize("\r\n", iPos);

				wxStringTokenizer token(strMessages,wxT("\r\n"),wxTOKEN_DEFAULT );

				while (token.HasMoreTokens()) {
					wxString message = token.GetNextToken();

					bool bOutputMessage = true;
					if (message.StartsWith(wxT("server version"))) {
						wxString strVer = message.Mid(15,64); // truncate string to avoid misuse by servers in showing ads
						strVer.Trim();
						CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						if (eserver) {
							eserver->SetVersion(strVer);
							Notify_ServerRefresh(eserver);
						}
					} else if (message.StartsWith(wxT("ERROR"))) {
						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();	
						} else {	
							servername = _("Server");
						}
						AddLogLineM(false, wxT("Error: ") + servername +
														wxT(" (") + Uint32_16toStringIP_Port(cur_server->GetIP(), cur_server->GetPort()) + wxT(") - ") +
														message.Mid(5,message.Len()).Trim(_T(" :")));
						bOutputMessage = false;

					} else if (message.StartsWith(wxT("WARNING"))) {

						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();	
						} else {	
							servername = _("Server");
						}
						AddLogLineM(false, wxT("Warning: ") + servername +
										wxT(" (") + Uint32_16toStringIP_Port(cur_server->GetIP(), cur_server->GetPort()) + wxT(") - ") +
										message.Mid(5,message.Len()).Trim(_T(" :")));

						bOutputMessage = false;
					}
					/* Give it a try ... (Creteil) END */

					if (message.Find(wxT("[emDynIP: ")) != (-1) && message.Find(wxT("]")) != (-1) && message.Find(wxT("[emDynIP: ")) < message.Find(wxT("]"))){
						wxString dynip = message.Mid(message.Find(wxT("[emDynIP: "))+10,message.Find(wxT("]")) - (message.Find(wxT("[emDynIP: "))+10));
						dynip.Trim(wxT(" "));
						if ( dynip.Length() && dynip.Length() < 51){
							CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
							if (eserver){
								eserver->SetDynIP(dynip);
								cur_server->SetDynIP(dynip);
								Notify_ServerRefresh(eserver);	
							}
						}
					}

					if (bOutputMessage) {
						theApp.AddServerMessageLine(message);
					}
				}
				break;
			}
			case OP_IDCHANGE:{
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_IDCHANGE\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				if (size < sizeof(LoginAnswer_Struct)) {
					throw wxString(_("Corrupt or invalid loginanswer from server received"));
				}
				
				LoginAnswer_Struct* la = (LoginAnswer_Struct*) packet;
				
				// Kry - This is because we have to check if there is only a uint32 
				// on the struct.If this fails, we had extended the login struct.
				wxASSERT(sizeof(LoginAnswer_Struct) == 4);
				
				ENDIAN_SWAP_I_32(la->clientid);

				/* Add more from 0.30c (Creteil) BEGIN */
				// save TCP flags in 'cur_server'
				wxASSERT(cur_server);
				if (cur_server) {
					if (size >= sizeof(LoginAnswer_Struct)+4) {						
						cur_server->SetTCPFlags(ENDIAN_SWAP_32(*((uint32*)(packet + sizeof(LoginAnswer_Struct)))));
					} else {
						cur_server->SetTCPFlags(0);
					}
					// copy TCP flags into the server in the server list
					CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort());
					if (pServer) {
						pServer->SetTCPFlags(cur_server->GetTCPFlags());
					}
				}
				/* Add more from 0.30c (Creteil) END */
				if (la->clientid == 0) {
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
					if (la->clientid >= 16777216) {
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
				serverconnect->clientid = la->clientid;

				if (connectionstate != CS_CONNECTED) {
					#ifdef DEBUG_SERVER_PROTOCOL
					AddLogLineM(true,wxT("Connected\n"));
					#endif
					SetConnectionState(CS_CONNECTED);
					theApp.OnlineSig();       // Added By Bouc7
				}
				serverconnect->SetClientID(la->clientid);
				AddLogLineM(false, wxString::Format(_("New clientid is %u"),la->clientid));
								
				theApp.downloadqueue->ResetLocalServerRequests();
				break;
			}
			case OP_SEARCHRESULT: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_SEARCHRESULT\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				CServer* cur_srv = (serverconnect) ? serverconnect->GetCurrentServer() : NULL;
				theApp.searchlist->ProcessSearchanswer(packet,size,(cur_srv)?cur_srv->GetIP():0,(cur_srv)?cur_srv->GetPort():0);
				theApp.searchlist->LocalSearchEnd();
				break;
			}
			case OP_FOUNDSOURCES: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxString::Format(wxT("ServerMsg - OP_FoundSources; sources = %u\n"), (UINT)(uchar)packet[16]));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				CSafeMemFile* sources = new CSafeMemFile((BYTE*)packet,size);
				uint8 fileid[16];
				sources->ReadHash16(fileid);
				if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid)) {
					file->AddSources(sources,cur_server->GetIP(), cur_server->GetPort());
				}
				delete sources;
				break;
			}
			case OP_SERVERSTATUS: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_SERVERSTATUS\n"));
				#endif
				// FIXME some statuspackets have a different size -> why? structur?
				if (size < 8) {
					throw wxString(_("Invalid server status packet"));
					break;
				}
				uint32 cur_user;
				memcpy(&cur_user,packet,4);
				uint32 cur_files;
				memcpy(&cur_files,packet+4,4);
				ENDIAN_SWAP_I_32(cur_user);
				ENDIAN_SWAP_I_32(cur_files);
				update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort());
				if (update) {
					update->SetUserCount(cur_user);
					update->SetFileCount(cur_files);
					Notify_ServerRefresh( update );
					Notify_ShowUserCount(update);
				}
				break;
			}
			//<<--Working, but needs to be cleaned up..
			case OP_SERVERIDENT: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_SERVERIDENT\n"));
				#endif
				//DumpMem(packet,size);

				theApp.downloadqueue->AddDownDataOverheadServer(size);
				if (size<38) {
					AddLogLineM(false, _("Unknown server info received! - too short"));
					// throw wxString(wxT("Unknown server info received!"));
					break;
				}
				char* buffer = new char[size-29];
				CServer* update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
				uint16 num,num2;
				memcpy(buffer,&packet[30],size-30);// 1st 30 char contain only server address & fillers
				memcpy(&num,&buffer[0],2); // length of server_name
				ENDIAN_SWAP_I_16(num);
				if ((uint32)(num + 2) > (size - 30)) {
	                                printf("%u + 2 > %u - 30\n",num,size);
					delete[] buffer;
					throw wxString(_("Unknown server info received!- wrong server name"));
				}
				char* temp=new char[size-38+1];
				memcpy(temp,&buffer[2],num);
				temp[num]=0;//close the string
				update->SetListName(char2unicode(temp));
				memcpy(&num2,&buffer[num+6],2);
                                ENDIAN_SWAP_I_16(num2);
				if ((uint32)(num2 + num + 8) > (size - 30)) {
	                                printf("%u + %u + 8 > %u - 30\n",num2,num,size);
					delete[] temp;
					delete[] buffer;
					throw wxString(_("Unknown server info received! - wrong description"));
				}
				memcpy (temp,&buffer[num+8],num2);
				temp[num2]=0; //close the string
				update->SetDescription(char2unicode(temp));
				Notify_ShowConnState(true,update->GetListName());
				Notify_ServerRefresh(update);
				delete[] temp;
				delete[] buffer;
				break;
			}
			// tecxx 1609 2002 - add server's serverlist to own serverlist
			case OP_SERVERLIST: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_SERVERLIST\n"));
				#endif
				CSafeMemFile* servers = new CSafeMemFile((BYTE*)packet,size);
				uint8 count = servers->ReadUInt8();
				if (((int32)(count*6 + 1) > size)) {
					count = 0;
				}
				int addcount = 0;
				while(count) {
					CServer* srv = new CServer(
														servers->ReadUInt16() ,  						// Port
														Uint32toStringIP(servers->ReadUInt32())); 	// Ip
					srv->SetListName(srv->GetFullIP());
					if (!theApp.AddServer(srv)) {
						delete srv;
					} else {
						addcount++;
					}
					count--;
				}
				delete servers;
				if (addcount) {
					AddLogLineM(false, wxString::Format(_("Received %d new servers"), addcount));
				}
				theApp.serverlist->SaveServermetToFile();
				AddLogLineM(false, _("Saving of server-list completed.\n"));
				break;
			}
			case OP_CALLBACKREQUESTED: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_CALLBACKREQUESTED\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				if (size == 6) {
					uint32 dwIP;
					memcpy(&dwIP,packet,4);
					ENDIAN_SWAP_I_32(dwIP);
					uint16 nPort;
					memcpy(&nPort,packet+4,2);
					ENDIAN_SWAP_I_16(nPort);
					CUpDownClient* client = theApp.clientlist->FindClientByIP(dwIP,nPort);
					if (client) {
						client->TryToConnect();
					} else {
						client = new CUpDownClient(nPort,dwIP,0,0,0);
						theApp.clientlist->AddClient(client);
						client->TryToConnect();
					}
				}
				break;
			}
			case OP_CALLBACK_FAIL: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_CALLBACK_FAIL\n"));
				#endif
				break;
			}
			case OP_REJECT: {
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: OP_REJECT\n"));
				#endif
				AddLogLineM(false, _("Server rejected last command"));
				break;
			}
			default: {
				printf("Unknown server packet with OPcode %x\n",opcode);
				#ifdef DEBUG_SERVER_PROTOCOL
				AddLogLineM(true,wxT("Server: Unrecognized packet -- protocol error\n\n"));
				#endif
			}
				;
		}
		return true;
	} catch (wxString error) {
		AddLogLineM(false,_("Unhandled error while processing packet from server - ") + error);
		SetConnectionState(CS_DISCONNECTED);
		return false;
	} catch (...) {
		AddLogLineM(false, _("Unknown exception while processing packet from server!"));
		SetConnectionState(CS_DISCONNECTED);
		return false;
	}
}

void CServerSocket::ConnectToServer(CServer* server)
{
	#ifdef DEBUG_SERVER_PROTOCOL
	AddLogLineM(true,wxT("Trying to connect\n"));
	#endif
	cur_server = new CServer(server);
	AddLogLineM(false, _("Connecting to ") + cur_server->GetListName() + wxT(" (") + server->GetAddress() + wxT(" - ") + cur_server->GetFullIP() + wxString::Format(wxT(":%i)"),cur_server->GetPort()));
	SetConnectionState(CS_CONNECTING);
	// This must be used if we want to reverse-check the addr of the server
	#define GET_ADDR true
	#ifdef GET_ADDR
	wxIPV4address addr;
	#else
	amuleIPV4Address addr;
	#endif
	addr.Hostname(server->GetAddress());
	addr.Service(server->GetPort());
	AddDebugLogLineM(true, wxT("Server ") + server->GetAddress() + wxString::Format(wxT(" Port %i"),server->GetPort()));
	#ifdef GET_ADDR	
	AddDebugLogLineM(true, wxT("Addr ") + addr.Hostname() + wxString::Format(wxT(" Port %i"),server->GetPort()));
	#endif
	this->Connect(addr,FALSE);

	info = server->GetListName();
	SetConnectionState(CS_CONNECTING);
}

void CServerSocket::OnError(wxSocketError nErrorCode)
{
	if (thePrefs::GetVerbose()) {
		AddLogLineM(false,_("Error in serversocket: ") + cur_server->GetListName() + wxT("(") + cur_server->GetFullIP() + wxString::Format(wxT(":%i): %u"),cur_server->GetPort(), (int)nErrorCode));
	}
	SetConnectionState(CS_DISCONNECTED);
}

bool CServerSocket::PacketReceived(Packet* packet)
{
	CALL_APP_DATA_LOCK;
	#ifdef DEBUG_SERVER_PROTOCOL
	AddLogLineM(true,wxT("Server: Packet Received: "));
	#endif
	try {
		if (packet->GetProtocol() == OP_PACKEDPROT) {
			if (!packet->UnPackPacket(250000)){
				AddDebugLogLineM(false, wxString::Format(wxT("Failed to decompress server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0));
				theApp.downloadqueue->AddDownDataOverheadServer(packet->GetPacketSize());
				return true;
			}
			packet->SetProtocol(OP_EDONKEYPROT);
		}
		if (packet->GetProtocol() == OP_EDONKEYPROT) {
			ProcessPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
		} else {
			AddDebugLogLineM(false, wxString::Format(wxT("Received server TCP packet with unknown protocol: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0));
			theApp.downloadqueue->AddDownDataOverheadServer(packet->GetPacketSize());
		}
	} catch(...) {
		AddDebugLogLineM(false, wxString::Format(wxT("Error: Unhandled exception while processing server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0));
		wxASSERT(0);
		return false;		
	}
	return true;
}

void CServerSocket::OnClose(wxSocketError WXUNUSED(nErrorCode))
{
	CEMSocket::OnClose(0);
	if (connectionstate == CS_WAITFORLOGIN) {
		SetConnectionState(CS_SERVERFULL);
	} else if (connectionstate == CS_CONNECTED) {
		SetConnectionState(CS_DISCONNECTED);
	} else {
		SetConnectionState(CS_NOTCONNECTED);
	}
	serverconnect->DestroySocket(this);
}

void CServerSocket::SetConnectionState(sint8 newstate)
{
	connectionstate = newstate;
	if (newstate < 1) {
		serverconnect->ConnectionFailed(this);
	} else if (newstate == CS_CONNECTED || newstate == CS_WAITFORLOGIN) {
		if (serverconnect) {
			serverconnect->ConnectionEstablished(this);
		}
	}
}

bool CServerSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket)
{
	m_dwLastTransmission = GetTickCount();
	return CEMSocket::SendPacket(packet, delpacket, controlpacket);
}

CServerSocketHandler::CServerSocketHandler(CServerSocket* parent)
#ifdef AMULE_DAEMON
 : wxThread(wxTHREAD_JOINABLE)
#endif
{
	socket = parent;
#ifdef AMULE_DAEMON
	if ( Create() != wxTHREAD_NO_ERROR ) {
		AddLogLineM(true,_("CServerSocketHandler: can not create my thread"));
	}
#endif
}

#ifdef AMULE_DAEMON
bool CServerSocket::Connect(wxIPV4address &addr, bool wait)
{
	bool res = CEMSocket::Connect(addr, wait);
	my_handler->Run();
	return res;
}
#endif

void CServerSocketHandler::ServerSocketHandler(wxSocketEvent& event) {
	//printf("Got a server event\n");
	//wxMessageBox(wxString::Format("Got Server Event %u",event.GetSocketEvent()));
	
	if (!socket) {
		return;
	}

	if (socket->OnDestroy()) {
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
			wxASSERT(0);
			break;
	}
	
	
}

#ifdef AMULE_DAEMON
void *CServerSocketHandler::Entry()
{
	while ( !TestDestroy() ) {
		if ( socket->WaitOnConnect(1,0) ) {
			break;
		}
	}
	if ( !socket->wxSocketClient::IsConnected() ) {
		printf("CServerSocket: connection refused or timed out\n");
	}
	if ( TestDestroy() ) {
		return 0;
	}
	socket->OnConnect(wxSOCKET_NOERROR);
	socket->OnSend(wxSOCKET_NOERROR);
	while ( !TestDestroy() ) {
		if ( socket->WaitForLost(0, 0) ) {
			socket->OnError(socket->LastError());

			printf("CServerSocket: connection closed\n");
			return 0;
		}
		// lfroen: setting timeout to give app a chance gracefully destroy
		// thread before deleting object
		if ( socket->WaitForRead(1, 0) ) {
			socket->OnReceive(wxSOCKET_NOERROR);
		}
	}
	printf("CServerSocket: terminated\n");
	return 0;

}
#endif

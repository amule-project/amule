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
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "opcodes.h"		// Needed for OP_SERVERMESSAGE
#include "otherfunctions.h"	// Needed for GetTickCount
#include "sockets.h"		// Needed for CS_WAITFORLOGIN
#include "ServerList.h"		// Needed for CServerList
#include "server.h"		// Needed for CServer
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"		// Needed for theApp

//#define SERVER_NET_TEST 

BEGIN_EVENT_TABLE(CServerSocketHandler, wxEvtHandler)
	EVT_SOCKET(SERVERSOCKET_HANDLER, CServerSocketHandler::ServerSocketHandler)
END_EVENT_TABLE()

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
	SetEventHandler(*my_handler,SERVERSOCKET_HANDLER);
	SetNotify(wxSOCKET_CONNECTION_FLAG|wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG|wxSOCKET_LOST_FLAG);
	Notify(TRUE);
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
	
	delete my_handler;
}


void CServerSocket::OnConnect(wxSocketError nErrorCode)
{
	//CAsyncSocket::OnConnect(nErrorCode);
	switch (nErrorCode) {

		case wxSOCKET_NOERROR: {
			if (cur_server->HasDynIP()) {
				struct sockaddr_in sockAddr;
				memset(&sockAddr, 0, sizeof(sockAddr));
				wxIPV4address tmpaddr;
				GetPeer(tmpaddr);
				printf("Connection Event from %s : %u\n",unicode2char(tmpaddr.IPAddress()),cur_server->GetPort());
				sockAddr.sin_addr.s_addr = inet_addr(unicode2char(tmpaddr.IPAddress()));
				cur_server->SetID(sockAddr.sin_addr.s_addr);
				theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort())->SetID(sockAddr.sin_addr.s_addr);
			}
			SetConnectionState(CS_WAITFORLOGIN);
			break;
		}

		case wxSOCKET_INVADDR :
		case wxSOCKET_NOHOST:
		case wxSOCKET_INVPORT:
		case wxSOCKET_TIMEDOUT :
			
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
	if (nErrorCode==wxSOCKET_NOERROR) {
		CEMSocket::OnReceive(0);
	} else {
		CEMSocket::OnReceive((int)nErrorCode);
	}
	m_dwLastTransmission = GetTickCount();
}

bool CServerSocket::ProcessPacket(const char* packet, uint32 size, int8 opcode)
{
	try{
		#ifdef SERVER_NET_TEST
		AddLogLineM(true,_("Processing Server Packet: "));
		#endif
		CServer* update;
		switch(opcode) {
			case OP_SERVERMESSAGE: {
				/* Kry import of lugdunum 16.40 new features */
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("Server message\n"));
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
					wxString wxMessage = token.GetNextToken();
					wxString message = wxMessage.GetData();

					bool bOutputMessage = true;
					if (message.StartsWith(wxT("server version")) == 0) {
						wxString strVer = message.Mid(15,64); // truncate string to avoid misuse by servers in showing ads
						strVer.Trim();
						CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						if (eserver) {
							eserver->SetVersion(strVer);
							theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(eserver);
						}

					/* Give it a try ... (Creteil) BEGIN */
					} else if (message.StartsWith(wxT("ERROR"))) {
						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();	
						} else {	
							servername = _("Server");
						}
						AddDebugLogLineM(false, _("Error ") + servername +
														wxString::Format(wxT(" (%s:%u) - "),unicode2char(cur_server->GetAddress()), cur_server->GetPort()) 
														+ message.Mid(5,message.Len()).Trim(_T(" :")))
						bOutputMessage = false;

					} else if (message.StartsWith(wxT("WARNING"))) {

						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						wxString servername;
						if (pServer) {
							servername	= pServer->GetListName();	
						} else {	
							servername = _("Server");
						}
						AddDebugLogLineM(false, _("Warning ") + servername +
														wxString::Format(wxT(" (%s:%u) - "),unicode2char(cur_server->GetAddress()), cur_server->GetPort()) 
														+ message.Mid(5,message.Len()).Trim(_T(" :")))

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
								theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(eserver);
							}
						}
					}

					if (bOutputMessage) {
						theApp.amuledlg->AddServerMessageLine(message);
					}
				}
				break;
			}
			case OP_IDCHANGE:{
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_IDChange\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				if (size < sizeof(LoginAnswer_Struct)) {
					throw wxString(_("Corrupt or invalid loginanswer from server received"));
				}
				LoginAnswer_Struct* la = (LoginAnswer_Struct*) packet;

				/* Add more from 0.30c (Creteil) BEGIN */
				// save TCP flags in 'cur_server'
				wxASSERT(cur_server);
				if (cur_server) {
					if (size >= sizeof(LoginAnswer_Struct)+4) {
						cur_server->SetTCPFlags(*((uint32*)(packet + sizeof(LoginAnswer_Struct))));
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
					uint8 state = theApp.glob_prefs->GetSmartIdState();
					if ( state > 0 ) {
						state++;
						if(state > 3) {
							theApp.glob_prefs->SetSmartIdState(0);
						} else {
							theApp.glob_prefs->SetSmartIdState(state);
						}
					}
					break;
				}
				if(theApp.glob_prefs->GetSmartIdCheck()) {
					if (la->clientid >= 16777216) {
						theApp.glob_prefs->SetSmartIdState(1);
					} else {
						uint8 state = theApp.glob_prefs->GetSmartIdState();
						if ( state > 0 ) {
							state++;
							if(state > 3) {
								theApp.glob_prefs->SetSmartIdState(0);
							} else {
								theApp.glob_prefs->SetSmartIdState(state);
							}
							break;
						}
					}
				}
				// we need to know our client when sending our shared files (done indirectly on SetConnectionState)
				serverconnect->clientid = la->clientid;

				if (connectionstate != CS_CONNECTED) {
					#ifdef SERVER_NET_TEST
					AddLogLineM(true,_("Connected\n"));
					#endif
					SetConnectionState(CS_CONNECTED);
					theApp.OnlineSig();       // Added By Bouc7
				}
				serverconnect->SetClientID(la->clientid);
				AddLogLineF(false,_("New clientid is %u"),la->clientid);
				
				
				// Kry - No need for this. eMule doesn't do it either.
				// I migth be wrong, anyway, so I'll leave it around.
				/*								
				for(POSITION pos = theApp.downloadqueue->filelist.GetHeadPosition(); pos != NULL;theApp.downloadqueue->filelist.GetNext(pos)) {
					CPartFile* cur_file = theApp.downloadqueue->filelist.GetAt(pos);
					if(cur_file->GetStatus() == PS_READY) {
						cur_file->ResumeFile();
					}
				}
				*/
				
				theApp.downloadqueue->ResetLocalServerRequests();
				break;
			}
			case OP_SEARCHRESULT: {
				#ifdef SERVER_NET_TEST
				AddLogLine(true,_("ServerMsg - OP_SearchResult\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				CServer* cur_srv = (serverconnect) ? serverconnect->GetCurrentServer() : NULL;
				theApp.amuledlg->searchwnd->LocalSearchEnd(theApp.searchlist->ProcessSearchanswer(packet,size,(cur_srv)?cur_srv->GetIP():0,(cur_srv)?cur_srv->GetPort():0));
				break;
			}
			case OP_FOUNDSOURCES: {
				#ifdef SERVER_NET_TEST
				AddLogLineF(true,_("ServerMsg - OP_FoundSources; Sources=%u  %s\n"), (UINT)(uchar)packet[16], DbgGetFileInfo((uchar*)packet)); // Creteil
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				CMemFile* sources = new CMemFile((BYTE*)packet,size);
				uint8 fileid[16];
				sources->Read(fileid);
				if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid)) {
					file->AddSources(sources,cur_server->GetIP(), cur_server->GetPort());
				}
				delete sources;
				break;
			}
			case OP_SERVERSTATUS: {
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_ServerStatus\n"));
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
				update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort());
				if (update) {
					update->SetUserCount(cur_user);
					update->SetFileCount(cur_files);
					theApp.amuledlg->ShowUserCount(cur_user, cur_files);
					theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(update);
				}
				break;
			}
			//<<--Working, but needs to be cleaned up..
			case OP_SERVERIDENT: {
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_ServerIdent\n"));
				#endif

				theApp.downloadqueue->AddDownDataOverheadServer(size);
				if (size<38) {
					AddLogLineM(false, _("Unknown server info received !"));
					// throw wxString(wxT("Unknown server info received!"));
					break;
				}
				char* buffer = new char[size-29];
				CServer* update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
				uint16 num,num2;
				memcpy(buffer,&packet[30],size-30);// 1st 30 char contain only server address & fillers
				memcpy(&num,&buffer[0],2); // length of server_name
				if ((uint32)(num + 2) > (size - 30)) {
					delete[] buffer;
					throw wxString(_("Unknown server info received!"));
				}
				char* temp=new char[size-38+1];
				memcpy(temp,&buffer[2],num);
				temp[num]=0;//close the string
				update->SetListName(char2unicode(temp));
				memcpy(&num2,&buffer[num+6],2);
				if ((uint32)(num2 + num + 8) > (size - 30)) {
					delete[] temp;
					delete[] buffer;
					throw wxString(_("Unknown server info received!"));
				}
				memcpy (temp,&buffer[num+8],num2);
				temp[num2]=0; //close the string
				update->SetDescription(char2unicode(temp));
				theApp.amuledlg->ShowConnectionState(true,update->GetListName());
				theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(update);
				delete[] temp;
				delete[] buffer;
				break;
			}
			// tecxx 1609 2002 - add server's serverlist to own serverlist
			case OP_SERVERLIST: {
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_ServerList\n"));
				#endif
				CSafeMemFile* servers = new CSafeMemFile((BYTE*)packet,size);
				unsigned char count;
				if ((1 != servers->Read(count)) || ((int32)(count*6 + 1) > size)) {
					count = 0;
				}
				int addcount = 0;
				while(count) {
					uint32 ip;
					uint16 port;
					if (4 != servers->Read(ip)) {
						break;
					}
					if (2 != servers->Read(port)) {
						break;
					}
					in_addr host;
					host.s_addr=ip;
					CServer* srv = new CServer(port, char2unicode(inet_ntoa(host)));
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
					AddLogLineF(false,_("Received %d new servers"), addcount);
				}
				theApp.serverlist->SaveServermetToFile();
				AddLogLineM(true,_("Saving of server.met file Done !!!\n"));
				break;
			}
			case OP_CALLBACKREQUESTED: {
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_CallbackRequested\n"));
				#endif
				theApp.downloadqueue->AddDownDataOverheadServer(size);
				if (size == 6) {
					uint32 dwIP;
					memcpy(&dwIP,packet,4);
					uint16 nPort;
					memcpy(&nPort,packet+4,2);
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
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_Callback_Fail\n"));
				#endif
				break;
			}
			case OP_REJECT: {
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("ServerMsg - OP_Reject\n"));
				#endif
				AddLogLineM(false, _("Server rejected last command"));
				break;
			}
			default:
				;
		}
		return true;
	}
	catch(wxString error) {
		AddLogLineM(false,_("Unhandled error while processing packet from server - ") + error);
		SetConnectionState(CS_DISCONNECTED);
		return false;
	}
}

void CServerSocket::ConnectToServer(CServer* server)
{
	#ifdef SERVER_NET_TEST
	AddLogLine(true,_("Trying to connect\n"));
	#endif
	cur_server = new CServer(server);
	AddLogLineM(false, _("Connecting to ") + cur_server->GetListName() + wxT(" (") + server->GetAddress() + wxT(" - ") + cur_server->GetFullIP() + wxString::Format(wxT(":%i)"),cur_server->GetPort()));
	SetConnectionState(CS_CONNECTING);
	wxIPV4address addr;
	addr.Hostname(server->GetAddress());
	addr.Service(server->GetPort());
	AddDebugLogLineM(true, _("Server ") + server->GetAddress() + wxString::Format(_(" Port %i"),server->GetPort()));
	AddDebugLogLineM(true, _("Addr ") + addr.Hostname() + wxString::Format(_(" Port %i"),server->GetPort()));
	this->Connect(addr,FALSE);

	info = server->GetListName();
	SetConnectionState(CS_CONNECTING);
}

void CServerSocket::OnError(wxSocketError nErrorCode)
{
	if (theApp.glob_prefs->GetVerbose()) {
		AddLogLineM(false,_("Error in serversocket: ") + cur_server->GetListName() + wxT("(") + cur_server->GetFullIP() + wxString::Format(wxT(":%i): %u"),cur_server->GetPort(), (int)nErrorCode));
	}
	SetConnectionState(CS_DISCONNECTED);
}

bool CServerSocket::PacketReceived(Packet* packet)
{
	#ifdef SERVER_NET_TEST
	AddLogLineM(true,_("Server Packet Received:  "));
	#endif
	try {
		if (packet->GetProtocol() == OP_PACKEDPROT) {
			#ifdef SERVER_NET_TEST
			AddLogLineM(true,_("Compressed packet, uncompressing... "));
			#endif
			if (!packet->UnPackPacket(250000)){
				#ifdef SERVER_NET_TEST
				AddLogLineM(true,_("FAILED\n"));
				#endif
				AddDebugLogLineF(false,_("Failed to decompress server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0);
				theApp.downloadqueue->AddDownDataOverheadServer(packet->GetPacketSize());
				return true;
			}
			#ifdef SERVER_NET_TEST
			AddLogLineM(true,_("SUCCESS\n"));
			#endif
			packet->SetProtocol(OP_EDONKEYPROT);
		}
		if (packet->GetProtocol() == OP_EDONKEYPROT) {
			#ifdef SERVER_NET_TEST
			AddLogLineM(true,_("Uncompressed packet\n"));
			#endif			
			ProcessPacket(packet->GetDataBuffer(), packet->GetPacketSize(), packet->GetOpCode());
		} else {
			AddDebugLogLineF(false,_("Received server TCP packet with unknown protocol: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0);
			theApp.downloadqueue->AddDownDataOverheadServer(packet->GetPacketSize());
		}
	} catch(...) {
		AddDebugLogLineF(false,_("Error: Unhandled exception while processing server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->GetProtocol() : 0, packet ? packet->GetOpCode() : 0, packet ? packet->GetPacketSize() : 0);
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

//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/defs.h>
	#include <wx/msw/winundef.h>
#else
	#include <netdb.h>
#endif
#include <unistd.h>


#include "sockets.h"		// Interface declarations.
#include "GetTickCount.h"	// Needed for GetTickCount
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "SysTray.h"		// Needed for TBN_IMPORTANTEVENT
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "UDPSocket.h"		// Needed for CUDPSocket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "packets.h"		// Needed for CTag
#include "opcodes.h"		// Needed for CT_NAME
#include "CMemFile.h"		// Needed for CMemFile
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "otherfunctions.h"	// Needed for GetTickCount
#include "ServerSocket.h"	// Needed for CServerSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "server.h"		// Needed for CServer
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"		// Needed for theApp
#include "ServerList.h"		// Needed for CServerList
#include "Preferences.h"	// Needed for CPreferences

// CServerConnect

void CServerConnect::TryAnotherConnectionrequest(){
	if ( connectionattemps.GetCount()<((app_prefs->IsSafeServerConnectEnabled()) ? 1 : 2) ) {

		CServer*  next_server = used_list->GetNextServer();

		if (!next_server)
		{
			if (connectionattemps.GetCount()==0){
				theApp.amuledlg->AddLogLine(true,CString(_("Failed to connect to all servers listed. Making another pass.")));
				ConnectToAnyServer(lastStartAt);
			}
			return;
		}

		// Barry - Only auto-connect to static server option
		if (theApp.glob_prefs->AutoConnectStaticOnly())
		{
			if (next_server->IsStaticMember())
                ConnectToServer(next_server,true);
		}
		else
			ConnectToServer(next_server,true);
	}
}

void CServerConnect::ConnectToAnyServer(uint32 startAt,bool prioSort,bool isAuto){
	lastStartAt=startAt;
	StopConnectionTry();
	Disconnect();
	theApp.amuledlg->ShowConnectionState(false,"",true);
	connecting = true;
	singleconnecting = false;

	// Barry - Only auto-connect to static server option
	if (theApp.glob_prefs->AutoConnectStaticOnly() && isAuto)
	{
		bool anystatic = false;
		CServer *next_server; 
		used_list->SetServerPosition( startAt );
		while ((next_server = used_list->GetNextServer()) != NULL)
		{
			if (next_server->IsStaticMember())
			{
				anystatic = true;
				break;
			}
		}
		if (!anystatic)
		{
			connecting = false;
			theApp.amuledlg->AddLogLine(true,CString(_("No valid servers to connect in serverlist found")));
			return;
		}
	}

	used_list->SetServerPosition( startAt );
	if( theApp.glob_prefs->Score() && prioSort ) used_list->Sort();

	if (used_list->GetServerCount()==0 ){
		connecting = false;
		theApp.amuledlg->AddLogLine(true,CString(_("No valid servers to connect in serverlist found")));
		return;
	}
	theApp.listensocket->Process();

	TryAnotherConnectionrequest();
}

void CServerConnect::ConnectToServer(CServer* server, bool multiconnect){
	if (!multiconnect) {
		StopConnectionTry();
		Disconnect();
	}
	connecting = true;
	singleconnecting = !multiconnect;

	CServerSocket* newsocket = new CServerSocket(this);
	m_lstOpenSockets.AddTail((void*&)newsocket);
	//newsocket->Create(0,SOCK_STREAM,FD_READ|FD_WRITE|FD_CLOSE|FD_CONNECT,NULL);
	newsocket->ConnectToServer(server);

	DWORD x=GetTickCount();
	connectionattemps.SetAt(x,newsocket);
}

void CServerConnect::StopConnectionTry(){
	connectionattemps.RemoveAll();
	connecting = false;
	singleconnecting = false;

	if (m_idRetryTimer.IsRunning()) 
	{ 
	  //KillTimer(NULL, m_idRetryTimer); 
	  m_idRetryTimer.Stop();
	} 

	// close all currenty opened sockets except the one which is connected to our current server
	for( POSITION pos = m_lstOpenSockets.GetHeadPosition(); pos != NULL; )
	{
		CServerSocket* pSck = (CServerSocket*)m_lstOpenSockets.GetNext(pos);
		if (pSck == connectedsocket)		// don't destroy socket which is connected to server
			continue;
		if (pSck->m_bIsDeleting == false)	// don't destroy socket if it is going to destroy itself later on
			DestroySocket(pSck);
	}
}

void CServerConnect::ConnectionEstablished(CServerSocket* sender){
	if (connecting == false)
	{
		// we are already connected to another server
		DestroySocket(sender);
		return;
	}	
	InitLocalIP();
	
	if (sender->GetConnectionState() == CS_WAITFORLOGIN){
		theApp.amuledlg->AddLogLine(false,CString(_("Connected to %s (%s:%i)")),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort());
		//send loginpacket
		CServer* update = theApp.serverlist->GetServerByAddress( sender->cur_server->GetAddress(), sender->cur_server->GetPort() );
		if (update){
			update->ResetFailedCount();
			theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer( update );
		}
		CMemFile* data = new CMemFile();
		data->Write((const uint8*)theApp.glob_prefs->GetUserHash());
		data->Write(GetClientID());
		data->Write(app_prefs->GetPort());
		data->Write((uint32)4); // tagcount

		CTag* tag = new CTag(CT_NAME,app_prefs->GetUserNick());
		tag->WriteTagToFile(data);
		delete tag;
		tag = new CTag(CT_VERSION,EDONKEYVERSION);
		tag->WriteTagToFile(data);
		delete tag;
		tag = new CTag(CT_PORT,app_prefs->GetPort());
		tag->WriteTagToFile(data);
		delete tag;
		tag = new CTag(0x20,0x00000001); // FLAGS for server connection : zlib awareness
		tag->WriteTagToFile(data);
		delete tag;

		Packet* packet = new Packet(data);
		packet->opcode = OP_LOGINREQUEST;
		this->SendPacket(packet,true,sender);
		delete data;
	}
	else if (sender->GetConnectionState() == CS_CONNECTED){
		theApp.stat_reconnects++;
		theApp.stat_serverConnectTime=GetTickCount64();
		connected = true;
		theApp.amuledlg->AddLogLine(true,CString(_("Connection established on: %s")),sender->cur_server->GetListName());
		theApp.amuledlg->ShowConnectionState(true,sender->cur_server->GetListName());
		CServer* update = theApp.serverlist->GetServerByAddress(sender->cur_server->GetAddress(),sender->cur_server->GetPort());
		theApp.amuledlg->serverwnd->serverlistctrl->HighlightServer(update);
		connectedsocket = sender;
		StopConnectionTry();
		theApp.sharedfiles->SendListToServer();
		theApp.amuledlg->serverwnd->serverlistctrl->RemoveDeadServer();
		// tecxx 1609 2002 - serverlist update
		if (theApp.glob_prefs->AddServersFromServer())
		{
			Packet* packet = new Packet(OP_GETSERVERLIST,0);
			SendPacket(packet,true);
		}
	}
}
bool CServerConnect::SendPacket(Packet* packet,bool delpacket, CServerSocket* to){
	if (!to){
		if (connected){
			connectedsocket->SendPacket(packet,delpacket,true);
		}
		else
			return false;
	}
	else{
		to->SendPacket(packet,delpacket,true);
	}
	return true;
}

bool CServerConnect::SendUDPPacket(Packet* packet,CServer* host,bool delpacket){
		if (connected){
			udpsocket->SendPacket(packet,host);
		}
		if (delpacket)
			delete packet;
	return true;
}

void CServerConnect::ConnectionFailed(CServerSocket* sender){
	if (connecting == false && sender != connectedsocket)
	{
		// just return, cleanup is done by the socket itself
		return;
	}
	//messages
	CServer* update;
	switch (sender->GetConnectionState()){
		case CS_FATALERROR:
			theApp.amuledlg->AddLogLine(true,CString(_("Fatal Error while trying to connect. Internet connection might be down")));
			break;
		case CS_DISCONNECTED:
			theApp.sharedfiles->ClearED2KPublishInfo();
			theApp.amuledlg->AddLogLine(true,CString(_("Lost connection to %s (%s:%i)")),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort());
			break;
		case CS_SERVERDEAD:
			theApp.amuledlg->AddLogLine(false,CString(_("%s (%s:%i) appears to be dead.")),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort()); //<<--
			update = theApp.serverlist->GetServerByAddress( sender->cur_server->GetAddress(), sender->cur_server->GetPort() );
			if(update){
				update->AddFailedCount();
				theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer( update );
			}
			break;
		case CS_ERROR:
			break;
		case CS_SERVERFULL:
			theApp.amuledlg->AddLogLine(false,CString(_("%s (%s:%i) appears to be full")),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort());
			break;
		case CS_NOTCONNECTED:; 
			break; 
	}

	// IMPORTANT: mark this socket not to be deleted in StopConnectionTry(), 
	// because it will delete itself after this function!
	sender->m_bIsDeleting = true;

	switch (sender->GetConnectionState()){
		case CS_FATALERROR:{
			bool autoretry= !singleconnecting;
			StopConnectionTry();
			if ((app_prefs->Reconnect()) && (autoretry) && (!m_idRetryTimer.IsRunning())){ 
				theApp.amuledlg->AddLogLine(false,CString(_("Automatic connection to server will retry in %d seconds")), CS_RETRYCONNECTTIME); 
				//m_idRetryTimer= SetTimer(NULL, 0, 1000*CS_RETRYCONNECTTIME, (TIMERPROC)RetryConnectCallback);
				m_idRetryTimer.SetOwner(theApp.amuledlg,4333);
				m_idRetryTimer.Start(1000*CS_RETRYCONNECTTIME);
			}
			theApp.amuledlg->ShowConnectionState(false,"",true);
			break;
		}
		case CS_DISCONNECTED:{
			connected = false;
			if (connectedsocket) 
				connectedsocket->Close();
			connectedsocket = NULL;
			wxCommandEvent evt;
			theApp.amuledlg->searchwnd->OnBnClickedCancels(evt);
//			printf("Reconn %d conn %d\n",app_prefs->Reconnect(),connecting);
			if (app_prefs->Reconnect() && !connecting){
				ConnectToAnyServer();		
			}
			if (theApp.glob_prefs->GetNotifierPopOnImportantError()) {
				theApp.amuledlg->ShowNotifier(CString(_("Connection lost")), TBN_IMPORTANTEVENT, false);
			}
			theApp.amuledlg->ShowConnectionState(false,"",true);
			break;
		}
		case CS_ERROR:
		case CS_NOTCONNECTED:{
			if (!connecting)
				break;
			theApp.amuledlg->AddLogLine(false,CString(_("Connecting to %s (%s:%i ) failed.")),sender->info.GetData(), sender->cur_server->GetFullIP(), sender->cur_server->GetPort() );
		}
		case CS_SERVERDEAD:
		case CS_SERVERFULL:{
			if (!connecting)
				break;
			if (singleconnecting){
				StopConnectionTry();
				break;
			}

			DWORD tmpkey;
			CServerSocket* tmpsock;
			POSITION pos = connectionattemps.GetStartPosition();
			while (pos){
				connectionattemps.GetNextAssoc(pos,tmpkey,tmpsock);
				if (tmpsock==sender) {
					connectionattemps.RemoveKey(tmpkey);
					break;
				}
			}			
			TryAnotherConnectionrequest();
		}
	}
	theApp.amuledlg->ShowConnectionState(false,"",true);
}

#if 0
// 09/28/02, by zegzav
VOID CALLBACK CServerConnect::RetryConnectCallback(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime) 
{ 
	CServerConnect *_this= theApp.serverconnect; 
	ASSERT(_this); 
	_this->StopConnectionTry();
	if (_this->IsConnected()) return; 

	_this->ConnectToAnyServer(); 
}
#endif

void CServerConnect::CheckForTimeout()
{ 
	DWORD maxage=GetTickCount() - CONSERVTIMEOUT;
	DWORD tmpkey;
	CServerSocket* tmpsock;
	POSITION pos = connectionattemps.GetStartPosition();
	while (pos){
		connectionattemps.GetNextAssoc(pos,tmpkey,tmpsock);
		if (!tmpsock) {
			theApp.amuledlg->AddLogLine(false, CString(_("Error: Socket invalid at timeoutcheck")));
			connectionattemps.RemoveKey(tmpkey);
			return;
		}

		if (tmpkey<=maxage) {
			theApp.amuledlg->AddLogLine(false,CString(_("Connection attempt to %s (%s:%i ) timed out")),tmpsock->info.GetData(), tmpsock->cur_server->GetFullIP(), tmpsock->cur_server->GetPort() );
			connectionattemps.RemoveKey(tmpkey);
			TryAnotherConnectionrequest();
			DestroySocket(tmpsock);
		}
	}
}

bool CServerConnect::Disconnect(){
	if (connected && connectedsocket){
		DestroySocket(connectedsocket);
		connectedsocket = NULL;
		connected = false;
		theApp.amuledlg->ShowConnectionState(false,"");
		//theApp.amuledlg->serverwnd->servermsgbox.AppendText(CString("\n\n\n\n"));
		theApp.stat_serverConnectTime=0;
		return true;
	}
	else
		return false;
}

CServerConnect::CServerConnect(CServerList* in_serverlist, CPreferences* in_prefs){
	connectedsocket = NULL;
	app_prefs = in_prefs;
	used_list = in_serverlist;
	max_simcons = (app_prefs->IsSafeServerConnectEnabled()) ? 1 : 2;
	connecting = false;
	connected = false;
	clientid = 0;
	singleconnecting = false;
	wxIPV4address tmp;
	tmp.AnyAddress();
	tmp.Service(theApp.glob_prefs->GetPort()+3);
	udpsocket = new CUDPSocket(this,tmp); // initalize socket for udp packets
	//udpsocket->Create();
	m_idRetryTimer.SetOwner(theApp.amuledlg,4333);
	lastStartAt=0;	
	InitLocalIP();	
}

CServerConnect::~CServerConnect(){
  m_idRetryTimer.Stop();
	// stop all connections
	StopConnectionTry();
	// close connected socket, if any
	DestroySocket(connectedsocket);
	connectedsocket = NULL;
	// close udp socket
	udpsocket->Close();
	delete udpsocket;
}

CServer* CServerConnect::GetCurrentServer(){
	if (IsConnected() && connectedsocket)
		return connectedsocket->cur_server;
	return NULL;
}

void CServerConnect::SetClientID(uint32 newid){
	clientid = newid;
	theApp.amuledlg->ShowConnectionState(IsConnected(),GetCurrentServer()->GetListName() );
}

void CServerConnect::DestroySocket(CServerSocket* pSck){
	if (pSck == NULL)
		return;
	// remove socket from list of opened sockets
	for( POSITION pos = m_lstOpenSockets.GetHeadPosition(); pos != NULL; )
	{
		POSITION posDel = pos;
		CServerSocket* pTestSck = (CServerSocket*)m_lstOpenSockets.GetNext(pos);
		if (pTestSck == pSck)
		{
			m_lstOpenSockets.RemoveAt(posDel);
			break;
		}
	}
	//if (pSck->m_hSocket != INVALID_SOCKET){
	  //pSck->AsyncSelect(0);
// Kry - From wxWindows documentation - "Destroy calls Close automatically."
	  //pSck->Close();
	  //}

	//delete pSck;
	pSck->Destroy();
}

bool CServerConnect::IsLocalServer(uint32 dwIP, uint16 nPort){
	if (IsConnected()){
		if (connectedsocket->cur_server->GetIP() == dwIP && connectedsocket->cur_server->GetPort() == nPort)
			return true;
	}
	return false;
}

void CServerConnect::KeepConnectionAlive()
{
	DWORD dwServerKeepAliveTimeout = theApp.glob_prefs->GetServerKeepAliveTimeout();
	if (dwServerKeepAliveTimeout && connected && connectedsocket &&
	connectedsocket->connectionstate == CS_CONNECTED &&
	GetTickCount() - connectedsocket->GetLastTransmission() >= dwServerKeepAliveTimeout) {
		// "Ping" the server if the TCP connection was not used for the specified interval with
		// an empty publish files packet -> recommended by lugdunummaster himself!
		
		CMemFile* files = new CMemFile(4);
		files->Write((uint32)0); //nFiles
	
		Packet* packet = new Packet(files);
		packet->opcode = OP_OFFERFILES;
		connectedsocket->SendPacket(packet,true);
		
		theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
		theApp.amuledlg->AddDebugLogLine(false, _("Refreshing server connection"));
		delete files;
 	}
}

void CServerConnect::InitLocalIP(){
	m_nLocalIP = 0;
	// Don't use 'gethostbyname(NULL)'. The winsock DLL may be replaced by a DLL from a third party
	// which is not fully compatible to the original winsock DLL. ppl reported crash with SCORSOCK.DLL
	// when using 'gethostbyname(NULL)'.
	try{
		char szHost[256];
		if (gethostname(szHost, sizeof szHost) == 0){
			hostent* pHostEnt = gethostbyname(szHost);
			if (pHostEnt != NULL && pHostEnt->h_length == 4 && pHostEnt->h_addr_list[0] != NULL)
				m_nLocalIP = *((uint32*)pHostEnt->h_addr_list[0]);
		}
	}
	catch(...){
		// at least two ppl reported crashs when using 'gethostbyname' with third party winsock DLLs
		theApp.amuledlg->AddDebugLogLine(false, _T("Unknown exception in CServerConnect::InitLocalIP"));
	}
}
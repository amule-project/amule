// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Team ( http://www.amule-project.net )
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


#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#include <netdb.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

//#include <pthread.h> aquatroll - think somebody forgot to delete the line, if it breaks: uncomment it
#include <wx/intl.h>		// Needed for _

#ifndef __linux__
#include "otherfunctions.h"
#endif

#include "UDPSocket.h"		// Interface declarations.
#include "packets.h"		// Needed for Packet
#include "PartFile.h"		// Needed for CPartFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "SearchList.h"		// Needed for CSearchList
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "ServerList.h"		// Needed for CServerList
#include "opcodes.h"		// Needed for OP_EDONKEYPROT
#include "server.h"		// Needed for CServer
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp
#include "amuleIPV4Address.h"	// Needed for wxIPV4address

#define TM_DNSDONE 17851

class AsyncDNS : public wxThread
{
public:
  AsyncDNS();
  virtual ExitCode Entry();

  wxString ipName;
  CUDPSocket* socket;
};

AsyncDNS::AsyncDNS() : wxThread(wxTHREAD_DETACHED)
{
}

wxThread::ExitCode AsyncDNS::Entry()
{
  struct hostent ret,*result=NULL;
  int errorno=0;
  char dataBuf[512]={0};

#if defined(__linux__)
  gethostbyname_r(ipName.GetData(),&ret,dataBuf,sizeof(dataBuf),&result,&errorno);
#elif defined(__WXMSW__)
  result = gethostbyname(ipName.GetData());
#else
  result = gethostbyname_r(ipName.GetData(),&ret,dataBuf,sizeof(dataBuf),&errorno); 
#endif

  if(result) {
    unsigned long addr=*(unsigned long*)ret.h_addr;
    struct sockaddr_in* newsi=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));//new struct sockaddr_in;
    newsi->sin_addr.s_addr=addr;

    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_DNSDONE);
    evt.SetClientData(socket);
    evt.SetExtraLong((long)newsi);
    wxPostEvent(theApp.amuledlg,evt);    
  }

  return result;
}

#if 0
CUDPSocketWnd::CUDPSocketWnd(){
}

BEGIN_MESSAGE_MAP(CUDPSocketWnd, CWnd)
	ON_MESSAGE(WM_DNSLOOKUPDONE, OnDNSLookupDone)
END_MESSAGE_MAP()

LRESULT CUDPSocketWnd::OnDNSLookupDone(WPARAM wParam,LPARAM lParam){
	m_pOwner->DnsLookupDone(wParam,lParam);
	return true;
};
#endif

IMPLEMENT_DYNAMIC_CLASS(CUDPSocket,wxDatagramSocket)

static wxIPV4address tmpaddress;
#define ID_SOKETTI 7772

CUDPSocket::CUDPSocket(CServerConnect* in_serverconnect,wxIPV4address& address) 
: wxDatagramSocket(address,wxSOCKET_NOWAIT){
  //m_hWndResolveMessage = NULL;
  sendbuffer = 0;
  cur_server = 0;
  serverconnect = in_serverconnect;

  printf("*** UDP socket at %d  (chat)\n",address.Service());
  SetEventHandler(*theApp.amuledlg,ID_SOKETTI);
  SetNotify(wxSOCKET_INPUT_FLAG);
  Notify(TRUE);

  //DnsTaskHandle = 0;
}

CUDPSocket::~CUDPSocket(){
  SetNotify(0);
  Notify(FALSE);
	if (cur_server)
		delete cur_server;
	if (sendbuffer)
		delete[] sendbuffer;
	//m_udpwnd.DestroyWindow();
}

#if 0
bool  CUDPSocket::Create(){
  //VERIFY( m_udpwnd.CreateEx(0, AfxRegisterWndClass(0),_T("Emule Socket Wnd"),WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL));
  //m_hWndResolveMessage = m_udpwnd.m_hWnd;
  //m_udpwnd.m_pOwner = this;
  return 1; //CAsyncSocket::Create(0,SOCK_DGRAM,FD_READ);
}
#endif

void CUDPSocket::OnReceive(int nErrorCode){
	char buffer[5000];
	wxString serverbuffer;
	//uint32 port;
	wxIPV4address addr;
	//wxUint32 length = ReceiveFrom(buffer,5000,serverbuffer,port);
	RecvFrom(addr,buffer,5000);
	wxUint32 length = LastCount();
	// strip IP address from wxSockAddress (do not call Hostname(). we do not want DNS)
	struct in_addr addr_in;
	#ifdef __WXMSW__
	addr_in.s_addr = inet_addr(addr.IPAddress().c_str());
	#else
	addr_in.s_addr=GAddress_INET_GetHostAddress(addr.GetAddress());
	#endif
	char* fromIP=inet_ntoa(addr_in);

	if (buffer[0] == (char)OP_EDONKEYPROT && length != static_cast<wxUint32>(-1))
	  //ProcessPacket(buffer+2,length-2,buffer[1],(char*)addr.Hostname().GetData(),addr.Service()); //serverbuffer.GetBuffer(),port);	  
	  ProcessPacket(buffer+2,length-2,buffer[1],fromIP,addr.Service()); //serverbuffer.GetBuffer(),port);
	else if ((buffer[0] == (char)OP_EMULEPROT) && length != static_cast<wxUint32>(-1))
	  //ProcessExtPacket(buffer+2,length-2,buffer[1],(char*)addr.Hostname().GetData(),addr.Service()); //serverbuffer.GetBuffer(),port);
	  ProcessExtPacket(buffer+2,length-2,buffer[1],fromIP,addr.Service()); //serverbuffer.GetBuffer(),port);
}

bool CUDPSocket::ProcessPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port){
	try{
		CServer* update;
		update = theApp.serverlist->GetServerByAddress( host, port-4 );
		if( update ){
			update->ResetFailedCount();
			theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer( update );
		}
		// Imported: OP_GLOBSEARCHRES, OP_GLOBFOUNDSORUCES (yes, soruces)  & OP_GLOBSERVSTATRES
		// This makes Server UDP Flags to be set correctly so we use less bandwith on asking servers for sources
		// Also we process Search results and Found sources correctly now on 16.40 behaviour.
		switch(opcode){
			case OP_GLOBSEARCHRES:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
				// process all search result packets
				int iLeft;
				do{
					uint16 uResultCount = theApp.searchlist->ProcessUDPSearchanswer(data, inet_addr(host), port-4);
					theApp.amuledlg->searchwnd->AddUDPResult(uResultCount);

					// check if there is another source packet
					iLeft = (int)(data->GetLength() - data->GetPosition());
					if (iLeft >= 2){
						uint8 protocol;
						data->Read(protocol);
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data->Seek(-1, wxFromCurrent);
							iLeft += 1;
							break;
						}

						uint8 opcode;
						data->Read(opcode);
						iLeft--;
						if (opcode != OP_GLOBSEARCHRES){
							data->Seek(-2,wxFromCurrent);
							iLeft += 2;
							break;
						}
					}
				}
				while (iLeft > 0);
				delete data;
/*
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				theApp.amuledlg->searchwnd->AddUDPResult(theApp.searchlist->ProcessUDPSearchanswer(packet,size));
*/
				break;
			}				
			case OP_GLOBFOUNDSORUCES:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
				// process all source packets
				int iLeft;
				do{
					uint8 fileid[16];
					data->Read(fileid);
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
						file->AddSources(data, inet_addr(host), port-4);
					else{
						// skip sources for that file
						uint8 count;
						data->Read(count);
						data->Seek(count*(4+2), wxFromStart);
					}

					// check if there is another source packet
					iLeft = (int)(data->GetLength() - data->GetPosition());
					if (iLeft >= 2){
						uint8 protocol;
						data->Read(protocol);
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data->Seek(-1, wxFromCurrent);
							iLeft += 1;
							break;
						}

						uint8 opcode;
						data->Read(opcode);
						iLeft--;
						if (opcode != OP_GLOBFOUNDSORUCES){
							data->Seek(-2, wxFromCurrent);
							iLeft += 2;
							break;
						}
					}
				}
				while (iLeft > 0);
				delete data;
				break;
			}				

 			case OP_GLOBSERVSTATRES:{
				// Imported from 0.30
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if( size < 12 || update == NULL )
					return true;
#define get_uint32(p)	*((uint32*)(p))
				uint32 challenge = get_uint32(packet);
				if( challenge != update->GetChallenge() )
					return true; 
				uint32 cur_user = get_uint32(packet+4);
				uint32 cur_files = get_uint32(packet+8);
				uint32 cur_maxusers = 0;
				uint32 cur_softfiles = 0;
				uint32 cur_hardfiles = 0;
				uint32 uUDPFlags = 0;
				if( size >= 16 ){
					cur_maxusers = get_uint32(packet+12);
				}
				if( size >= 24 ){
					cur_softfiles = get_uint32(packet+16);
					cur_hardfiles = get_uint32(packet+20);
				}
				if( size >= 28 ){
					uUDPFlags = get_uint32(packet+24);
				}
				if( update ){
					update->SetPing( ::GetTickCount() - update->GetLastPinged() );
					update->SetUserCount( cur_user );
					update->SetFileCount( cur_files );
					update->SetMaxUsers( cur_maxusers );
					update->SetSoftFiles( cur_softfiles );
					update->SetHardFiles( cur_hardfiles );
					//printf("->> reading Stats from server, flags are %i\n",uUDPFlags);
					update->SetUDPFlags( uUDPFlags );
					theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer( update );
				}				
				break;
			}

			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				return false;
		}
		return true;
	}
	catch(...){
	  //OUTPUT_DEBUG_TRACE();
		theApp.amuledlg->AddDebugLogLine(false,CString(_("Error while processing incoming UDP Packet (Most likely a misconfigured server)")));
		return false;
	}
}

bool CUDPSocket::ProcessExtPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port){
	try{
		switch(opcode){
			/*
			case OP_UDPVERIFYUPREQ:{
				ASSERT (size == 6);
				if (size == 6){
					uint32 checkclientip = 0;
					uint16 checkclientport = 0;
					memcpy(&checkclientip,packet,4);
					memcpy(&checkclientport,packet,2);
					if (theApp.clientlist->VerifyUpload(checkclientip,checkclientport)){
						Packet answer(OP_UDPVERIFYUPA,0,OP_EMULEPROT);
						SendTo(answer.GetUDPHeader(),2,port,host);
					}
				}
				break;
			}*/
			case OP_UDPVERIFYUPA:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				break;
			}
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				return false;
		}
		return true;
	}
	catch(...){
	  //OUTPUT_DEBUG_TRACE();
		theApp.amuledlg->AddDebugLogLine(false,CString(_("Error while processing incoming extended protocol UDP Packet")));
		return false;
	}
}

#if 0
void CUDPSocket::AsyncResolveDNS(LPCTSTR lpszHostAddress, UINT nHostPort){
	m_lpszHostAddress = lpszHostAddress;
	m_nHostPort = nHostPort;
	//if (DnsTaskHandle)
	//	WSACancelAsyncRequest(DnsTaskHandle);
	//DnsTaskHandle = NULL;
	// see if we have a ip already
	USES_CONVERSION;
	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	LPSTR lpszAscii = T2A((LPTSTR)m_lpszHostAddress);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
	sockAddr.sin_port = m_nHostPort; //htons((u_short)m_nHostPort);

	// backup for send socket
	m_SaveAddr = sockAddr;

	if (sockAddr.sin_addr.s_addr == INADDR_NONE){
		/* Resolve hostname "hostname" asynchronously */ 
		memset(DnsHostBuffer, 0, sizeof(DnsHostBuffer));

		DnsTaskHandle = WSAAsyncGetHostByName(
			m_hWndResolveMessage,
			WM_DNSLOOKUPDONE,
			lpszHostAddress,
			DnsHostBuffer,
			MAXGETHOSTSTRUCT);

		if (DnsTaskHandle == 0){
			delete[] sendbuffer;
			sendbuffer = 0;
			delete cur_server;
			cur_server = 0;
#ifdef __DEBUG__
			AfxMessageBox("LOOKUPERROR DNSTASKHANDLE = 0");
#endif
		}
	}
	else{
		SendBuffer();
	}
}
#endif

void CUDPSocket::DnsLookupDone(struct sockaddr_in* inaddr) {
  /* An asynchronous database routine completed. */
  if (inaddr==NULL) { /*WSAGETASYNCERROR(lp) != 0){*/
    if (sendbuffer)
      delete[] sendbuffer;
    sendbuffer = 0;
    if (cur_server)
      delete cur_server;
    cur_server = 0;
    return;
  }
  
  if (m_SaveAddr.sin_addr.s_addr == INADDR_NONE){
    m_SaveAddr.sin_addr.s_addr = inaddr->sin_addr.s_addr;
  }
  if (cur_server){
    CServer* update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
    if (update)
      update->SetID(m_SaveAddr.sin_addr.s_addr);
    SendBuffer();
  }
  if(inaddr) {
    // must free it
    //delete inaddr;
    free(inaddr);
  }
}

void CUDPSocket::SendBuffer(){
	if(cur_server && sendbuffer){
	  //SendTo(sendbuffer,sendblen,(SOCKADDR*)&m_SaveAddr, sizeof(m_SaveAddr));
	  //wxIPV4address addr;
	  amuleIPV4Address addr;
	  struct in_addr tmpa;
	  tmpa.s_addr=m_SaveAddr.sin_addr.s_addr;
	  addr.Hostname(m_SaveAddr.sin_addr.s_addr);
	  addr.Service(m_SaveAddr.sin_port);
	  // don't send if socket isn't there
	  if(Ok()) SendTo(addr,sendbuffer,sendblen);
	  delete[] sendbuffer;
	  sendbuffer = 0;
	  delete cur_server;
	  cur_server = 0;
	}
}

void CUDPSocket::SendPacket(Packet* packet,CServer* host){
        int ret = 0;
	cur_server = new CServer(host);
	sendbuffer = new char[packet->size+2];
	memcpy(sendbuffer,packet->GetUDPHeader(),2);
	memcpy(sendbuffer+2,packet->pBuffer,packet->size);
	sendblen = packet->size+2;

	// see if we need to dns()
	struct sockaddr_in sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	sockAddr.sin_family=AF_INET;
        // inet_addr is obsolete
#ifndef __WXMSW__
        if (!(ret = inet_aton(cur_server->GetAddress(), &sockAddr.sin_addr))) {
		sockAddr.sin_addr.s_addr = INADDR_NONE;
	  }
#endif
	sockAddr.sin_port=cur_server->GetPort()+4; //htons(cur_server->GetPort()+4);
	m_SaveAddr=sockAddr;

	//AsyncResolveDNS(cur_server->GetAddress(),cur_server->GetPort()+4);
	// Done using threads

        // don't resolve IPs
	if (!ret) {
	  AsyncDNS* dns=new AsyncDNS();
	  if(dns->Create()!=wxTHREAD_NO_ERROR) {
	    // uh?
	    return;
	  }
	  dns->ipName=cur_server->GetAddress();
	  dns->socket=this;
	  dns->Run();
	} else {
	  SendBuffer();
	}
}

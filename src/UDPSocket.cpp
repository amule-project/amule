//
// Copyright (c) 2003-2004 aMule Team ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


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
/*
 * INADDR_BROADCAST is identical to INADDR_NONE which is not defined
 * on all systems. INADDR_BROADCAST should be fine to indicate an error.
 */
#ifndef INADDR_NONE
#define INADDR_NONE INADDR_BROADCAST
#endif

//#include <pthread.h> aquatroll - think somebody forgot to delete the line, if it breaks: uncomment it
#include <wx/intl.h>		// Needed for _

#ifndef __linux__
#include "otherfunctions.h"
#endif

#include "UDPSocket.h"		// Interface declarations.
#include "packets.h"		// Needed for Packet
#include "PartFile.h"		// Needed for CPartFile
#include "SearchList.h"		// Needed for CSearchList
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ServerList.h"		// Needed for CServerList
#include "opcodes.h"		// Needed for OP_EDONKEYPROT
#include "server.h"		// Needed for CServer
#include "amule.h"			// Needed for theApp
#include "amuleIPV4Address.h"	// Needed for wxIPV4address
#include "GetTickCount.h"

#define get_uint32(p)	ENDIAN_SWAP_32(*((uint32*)(p)))
#define get_uint16(p)	ENDIAN_SWAP_16(*((uint16*)(p)))

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

  struct hostent *result=NULL;
#ifndef __WXMSW__
  struct hostent ret;
  int errorno=0;
  char dataBuf[512]={0};
#endif

#if defined(__linux__)
  gethostbyname_r(unicode2char(ipName),&ret,dataBuf,sizeof(dataBuf),&result,&errorno);
#elif defined(__WXMSW__)
  result = gethostbyname(unicode2char(ipName));
#else
  result = gethostbyname_r(unicode2char(ipName),&ret,dataBuf,sizeof(dataBuf),&errorno);
#endif

  if(result) {
    #if defined(__WXMSW__)
    unsigned long addr=*(unsigned long*)result->h_addr;
    #else
    unsigned long addr=*(unsigned long*)ret.h_addr;
    #endif
    struct sockaddr_in* newsi=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));//new struct sockaddr_in;
    // addr is in network byte order
    newsi->sin_addr.s_addr = addr;

    wxMuleInternalEvent evt(wxEVT_CORE_DNS_DONE);
    evt.SetClientData(socket);
    evt.SetExtraLong((long)newsi);
    wxPostEvent(&theApp,evt);
  }



  return result;
}

IMPLEMENT_DYNAMIC_CLASS(CUDPSocket,wxDatagramSocket)

//static wxIPV4address tmpaddress;

CUDPSocket::CUDPSocket(CServerConnect* in_serverconnect,wxIPV4address& address)
: wxDatagramSocket(address,wxSOCKET_NOWAIT){
  //m_hWndResolveMessage = NULL;
  sendbuffer = 0;
  cur_server = 0;
  serverconnect = in_serverconnect;

  printf("*** UDP socket at %d\n",address.Service());
#ifdef AMULE_DAEMON
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("ERROR: CUDPSocket failed create\n");
		wxASSERT(0);
	}
	Run();
#else
  SetEventHandler(theApp,UDPSOCKET_HANDLER);
  SetNotify(wxSOCKET_INPUT_FLAG);
  Notify(TRUE);
#endif
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

#define SERVER_UDP_BUFFER_SIZE 5000

void CUDPSocket::OnReceive(int WXUNUSED(nErrorCode)) {
	char buffer[SERVER_UDP_BUFFER_SIZE];

	wxIPV4address addr;
	RecvFrom(addr,buffer,SERVER_UDP_BUFFER_SIZE);
	wxUint32 length = LastCount();
	// strip IP address from wxSockAddress (do not call Hostname(). we do not want DNS)

	if (buffer[0] == (char)OP_EDONKEYPROT && length != static_cast<wxUint32>(-1))
	  ProcessPacket(buffer+2,length-2,buffer[1],addr.IPAddress(),addr.Service()); 
	else if ((buffer[0] == (char)OP_EMULEPROT) && length != static_cast<wxUint32>(-1))
		theApp.downloadqueue->AddDownDataOverheadOther(length-2);
	  //ProcessExtPacket(buffer+2,length-2,buffer[1],addr.IPAddress(),addr.Service()); 
}


void CUDPSocket::ReceiveAndDiscard() {
	char buffer[SERVER_UDP_BUFFER_SIZE];
	
	wxIPV4address addr;
	RecvFrom(addr,buffer,SERVER_UDP_BUFFER_SIZE);
	// And just discard it :)	
};

bool CUDPSocket::ProcessPacket(char* packet, int16 size, int8 opcode, const wxString& host, uint16 port){
	try{
		CServer* update;
		update = theApp.serverlist->GetServerByAddress( host, port-4 );
		if( update ){
			update->ResetFailedCount();
			Notify_ServerRefresh( update );
		}
		// Imported: OP_GLOBSEARCHRES, OP_GLOBFOUNDSORUCES (yes, soruces)  & OP_GLOBSERVSTATRES
		// This makes Server UDP Flags to be set correctly so we use less bandwith on asking servers for sources
		// Also we process Search results and Found sources correctly now on 16.40 behaviour.
//		printf("UDP packet processing\n");
		switch(opcode){
			case OP_GLOBSEARCHRES: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
				// process all search result packets
				int iLeft;
				do{
					/*uint16 uResultCount =*/ theApp.searchlist->ProcessUDPSearchanswer(data, inet_addr(unicode2char(host)), port-4);
					// There is no need because we don't limit the global results
					// theApp.amuledlg->searchwnd->AddUDPResult(uResultCount);
					// check if there is another source packet
					iLeft = (int)(data->GetLength() - data->GetPosition());
					if (iLeft >= 2){
						uint8 protocol = data->ReadUInt8();
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data->Seek(-1, wxFromCurrent);
							iLeft += 1;
							break;
						}

						uint8 opcode = data->ReadUInt8();
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

				theApp.downloadqueue->AddDownDataOverheadOther(size);

				break;
			}
			case OP_GLOBFOUNDSORUCES:{
//				printf("Sources coming...\n");
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
				// process all source packets
				int iLeft;
				do{
					uint8 fileid[16];
					data->ReadHash16(fileid);
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
						file->AddSources(data, inet_addr(unicode2char(host)), port-4);
					else{
						// skip sources for that file
						uint8 count = data->ReadUInt8();
						data->Seek(count*(4+2), wxFromStart);
					}

					// check if there is another source packet
					iLeft = (int)(data->GetLength() - data->GetPosition());
					if (iLeft >= 2){
						uint8 protocol = data->ReadUInt8();
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data->Seek(-1, wxFromCurrent);
							iLeft += 1;
							break;
						}

						uint8 opcode = data->ReadUInt8();
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
				// Imported from 0.43b
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if( size < 12 || update == NULL )
					return true;
				uint32 challenge = get_uint32(packet);
				if( challenge != update->GetChallenge() )
					return true;
				uint32 cur_user = get_uint32(packet+4);
				uint32 cur_files = get_uint32(packet+8);
				uint32 cur_maxusers = 0;
				uint32 cur_softfiles = 0;
				uint32 cur_hardfiles = 0;
				uint32 uUDPFlags = 0;
				uint32 uLowIDUsers = 0;				
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
				if( size >= 32 ){
					uLowIDUsers = get_uint32(packet+28);
					/*
					if (thePrefs.GetDebugServerUDPLevel() > 0)
						Debug(_T(" LowID users=%u\n"), uLowIDUsers);
					*/
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
					update->SetLowIDUsers( uLowIDUsers );
					Notify_ServerRefresh( update );
				}
				break;
			}
 			case OP_SERVER_DESC_RES:{
				// 0.43b
				/*
				if (thePrefs.GetDebugServerUDPLevel() > 0)
					Debug(_T("ServerUDPMessage from %s:%u - OP_ServerDescRes\n"), host, nUDPPort-4);
				*/
				if (!update)
					return true;

				// old packet: <name_len 2><name name_len><desc_len 2 desc_en>
				// new packet: <challenge 4><taglist>
				//
				// NOTE: To properly distinguish between the two packets which are both useing the same opcode...
				// the first two bytes of <challenge> (in network byte order) have to be an invalid <name_len> at least.

				CSafeMemFile srvinfo((BYTE*)packet, size);
				if (size >= 8 && get_uint16(packet) == INV_SERV_DESC_LEN)
				{
					if (update->GetDescReqChallenge() != 0 && get_uint32(packet) == update->GetDescReqChallenge())
					{
						update->SetDescReqChallenge(0);
						// skip challenge
						/* uint32 challenge = */ srvinfo.ReadUInt32();
						
						uint32 uTags = srvinfo.ReadUInt32();
						for (uint32 i = 0; i < uTags; ++i)
						{
							CTag tag(srvinfo);
							if (tag.tag.specialtag == ST_SERVERNAME && tag.tag.type == 2)
								update->SetListName(char2unicode(tag.tag.stringvalue));
							else if (tag.tag.specialtag == ST_DESCRIPTION && tag.tag.type == 2)
								update->SetDescription(char2unicode(tag.tag.stringvalue));
							else if (tag.tag.specialtag == ST_DYNIP && tag.tag.type == 2)
								update->SetDynIP(char2unicode(tag.tag.stringvalue));
							else if (tag.tag.specialtag == ST_VERSION && tag.tag.type == 2)
								update->SetVersion(char2unicode(tag.tag.stringvalue));
							else if (tag.tag.specialtag == ST_VERSION && tag.tag.type == 3){
								wxString strVersion;
								strVersion.Printf(wxT("%u.%u"), tag.tag.intvalue >> 16,
									tag.tag.intvalue & 0xFFFF);
								update->SetVersion(strVersion);
							}
							else if (tag.tag.specialtag == ST_AUXPORTSLIST && tag.tag.type == 2)
								// currently not implemented.
								; // <string> = <port> [, <port>...]
							else{
								/*
								if (thePrefs.GetDebugServerUDPLevel() > 0)
									Debug(_T("***NOTE: Unknown tag in OP_ServerDescRes: %s\n"), tag.GetFullInfo());
								*/
							}
						}
					}
					else
					{
						// A server sent us a new server description packet (including a challenge) although we did not
						// ask for it. This may happen, if there are multiple servers running on the same machine with
						// multiple IPs. If such a server is asked for a description, the server will answer 2 times,
						// but with the same IP.
						/*
						if (thePrefs.GetDebugServerUDPLevel() > 0)
							Debug(_T("***NOTE: Received unexpected new format OP_ServerDescRes from %s:%u with challenge %08x (waiting on packet with challenge %08x)\n"), host, nUDPPort-4, PeekUInt32(packet), update->GetDescReqChallenge());
						*/
						; // ignore this packet
						
					}
				}
				else
				{
					wxString strName = srvinfo.ReadString();
					wxString strDesc = srvinfo.ReadString();
					update->SetDescription(strDesc);
					update->SetListName(strName);
				}
				/*
				if (thePrefs.GetDebugServerUDPLevel() > 0){
					UINT uAddData = srvinfo.GetLength() - srvinfo.GetPosition();
					if (uAddData)
						Debug(_T("***NOTE: ServerUDPMessage from %s:%u - OP_ServerDescRes:  ***AddData: %s\n"), host, nUDPPort-4, GetHexDump(packet + srvinfo.GetPosition(), uAddData));
				}
				*/
				Notify_ServerRefresh( update );
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
		AddDebugLogLineM(false,_("Error while processing incoming UDP Packet (Most likely a misconfigured server)"));
		return false;
	}
}

#if 0
bool CUDPSocket::ProcessExtPacket(char* packet, int16 size, int8 opcode, const wxString& host, uint16 port){
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
				*/
				theApp.downloadqueue->AddDownDataOverheadOther(size);
			}
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
		AddDebugLogLineM(false,_("Error while processing incoming extended protocol UDP Packet"));
		return false;
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
		tmpa.s_addr = m_SaveAddr.sin_addr.s_addr;
#if wxCHECK_VERSION(2,5,2)
		addr.Hostname(ntohl(m_SaveAddr.sin_addr.s_addr));
#else
		addr.Hostname(m_SaveAddr.sin_addr.s_addr);
#endif
		addr.Service(m_SaveAddr.sin_port);
		// don't send if socket isn't there
		if ( Ok() ) 
			SendTo(addr,sendbuffer,sendblen);
		delete[] sendbuffer;
		sendbuffer = 0;
		delete cur_server;
		cur_server = 0;
	}
}

void CUDPSocket::SendPacket(Packet* packet,CServer* host){
        int ret = 0;
	cur_server = new CServer(host);
	sendbuffer = new char[packet->GetPacketSize()+2];
	memcpy(sendbuffer,packet->GetUDPHeader(),2);
	memcpy(sendbuffer+2,packet->GetDataBuffer(),packet->GetPacketSize());
	sendblen = packet->GetPacketSize()+2;

	// see if we need to dns()
	struct sockaddr_in sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	sockAddr.sin_family=AF_INET;
        // inet_addr is obsolete
#ifndef __WXMSW__
        if (!(ret = inet_aton(unicode2char(cur_server->GetAddress()), &sockAddr.sin_addr))) {
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

#ifdef AMULE_DAEMON

void *CUDPSocket::Entry()
{
	while ( !TestDestroy() ) {
		if ( WaitForRead(0, 1000) ) {
			OnReceive(0);
		}
	}
	return 0;
}

#endif

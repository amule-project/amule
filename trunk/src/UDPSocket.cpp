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
#include <wx/intl.h>		// Needed for _

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
#include "NetworkFunctions.h" // Needed for CAsyncDNS
#include "GetTickCount.h"

#define get_uint32(p)	ENDIAN_SWAP_32(*((uint32*)(p)))
#define get_uint16(p)	ENDIAN_SWAP_16(*((uint16*)(p)))


IMPLEMENT_DYNAMIC_CLASS(CUDPSocket,wxDatagramSocket)

CUDPSocket::CUDPSocket(CServerConnect* in_serverconnect, amuleIPV4Address& address)
: wxDatagramSocket(address,wxSOCKET_NOWAIT)
#ifdef AMULE_DAEMON
	, wxThread(wxTHREAD_JOINABLE)
#endif
{
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

	amuleIPV4Address addr;
	RecvFrom(addr,buffer,SERVER_UDP_BUFFER_SIZE);
	wxUint32 length = LastCount();

	if (buffer[0] == (char)OP_EDONKEYPROT && length != static_cast<wxUint32>(-1)) {
	  ProcessPacket(buffer+2,length-2,buffer[1],addr.IPAddress(),addr.Service()); 
	} else if ((buffer[0] == (char)OP_EMULEPROT) && length != static_cast<wxUint32>(-1)) {
		theApp.downloadqueue->AddDownDataOverheadOther(length-2);
	}
}


void CUDPSocket::ReceiveAndDiscard() {
	char buffer[SERVER_UDP_BUFFER_SIZE];
	
	amuleIPV4Address addr;
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
		switch(opcode){
			case OP_GLOBSEARCHRES: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
				// process all search result packets
				int iLeft;
				do{
					/*uint16 uResultCount =*/ theApp.searchlist->ProcessUDPSearchanswer(data, StringIPtoUint32(host), port-4);
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
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile* data = new CSafeMemFile((BYTE*)packet,size);
				// process all source packets
				int iLeft;
				do{
					uint8 fileid[16];
					data->ReadHash16(fileid);
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
						file->AddSources(data, StringIPtoUint32(host), port-4);
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
				}
				if( update ){
					update->SetPing( ::GetTickCount() - update->GetLastPinged() );
					update->SetUserCount( cur_user );
					update->SetFileCount( cur_files );
					update->SetMaxUsers( cur_maxusers );
					update->SetSoftFiles( cur_softfiles );
					update->SetHardFiles( cur_hardfiles );
					update->SetUDPFlags( uUDPFlags );
					update->SetLowIDUsers( uLowIDUsers );
					Notify_ServerRefresh( update );
					Notify_ShowUserCount(update);
				}
				break;
			}
 			case OP_SERVER_DESC_RES:{
				// 0.43b
				if (!update) {
					return true;
				}

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
						// ignore this packet
						
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
		AddDebugLogLineM(false,wxT("Error while processing incoming UDP Packet (Most likely a misconfigured server)"));
		return false;
	}
}

void CUDPSocket::DnsLookupDone(uint32 ip) {
  /* An asynchronous database routine completed. */
  if (!ip) { 
    if (sendbuffer) {
      delete[] sendbuffer;
    }
    sendbuffer = 0;
    if (cur_server) {
      delete cur_server;
    }
    cur_server = NULL;
    return;
  }
 
    m_SaveAddr.Hostname(ip);

  if (cur_server){
    CServer* update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
    if (update) {
      update->SetID(ip);
    }
    SendBuffer();
  }
  
}

void CUDPSocket::SendBuffer(){
	if(cur_server && sendbuffer){
		// don't send if socket isn't there
		if ( Ok() ) 
			SendTo(m_SaveAddr,sendbuffer,sendblen);
		delete[] sendbuffer;
		sendbuffer = 0;
		delete cur_server;
		cur_server = NULL;
	}
}

void CUDPSocket::SendPacket(Packet* packet,CServer* host){
	wxASSERT(!cur_server);
	cur_server = new CServer(host);
	sendbuffer = new char[packet->GetPacketSize()+2];
	memcpy(sendbuffer,packet->GetUDPHeader(),2);
	memcpy(sendbuffer+2,packet->GetDataBuffer(),packet->GetPacketSize());
	sendblen = packet->GetPacketSize()+2;

	m_SaveAddr.Service(cur_server->GetPort()+4); 

	// see if we need to dns()
	if (cur_server->HasDynIP()) {
		// This not an ip but a hostname. Resolve it.
		CAsyncDNS* dns=new CAsyncDNS();
		if(dns->Create()!=wxTHREAD_NO_ERROR) {
			// uh?
			return;
		}
		dns->ipName=cur_server->GetDynIP();
		dns->socket=this;
		dns->Run();
	} else {
		m_SaveAddr.Hostname(cur_server->GetIP());
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

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
#include "ServerSocket.h"
#include <sys/types.h>

IMPLEMENT_DYNAMIC_CLASS(CUDPSocket,wxDatagramSocket)

CUDPSocket::CUDPSocket(CServerConnect* in_serverconnect, amuleIPV4Address& address)
: wxDatagramSocket(address,wxSOCKET_NOWAIT)
#ifdef AMULE_DAEMON
	, wxThread(wxTHREAD_JOINABLE)
#endif
{

	sendbuffer = NULL;
	cur_server = NULL;
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
  
}

CUDPSocket::~CUDPSocket(){
	SetNotify(0);
	Notify(FALSE);
	if (cur_server) {
		delete cur_server;
	}
	if (sendbuffer) {
		delete[] sendbuffer;
	}
	server_packet_queue.RemoveAll();
}

#define SERVER_UDP_BUFFER_SIZE 5000

void CUDPSocket::OnReceive(int WXUNUSED(nErrorCode)) {
	uint8 buffer[SERVER_UDP_BUFFER_SIZE];

	amuleIPV4Address addr;
	RecvFrom(addr,buffer,SERVER_UDP_BUFFER_SIZE);
	int length = LastCount();
	
	if (length > 2) { 
		// 2 bytes (protocol and opcode) must be there. This 'if' also takes care
		// of len == -1 (read error on udp socket)
			
		switch (/*Protocol*/buffer[0]) {
			case OP_EDONKEYPROT: {
				length = length - 2; // skip protocol and opcode
				// Create the safe mem file.	
				CSafeMemFile  data(buffer+2,length);
				wxASSERT(length == data.GetLength());
				ProcessPacket(data,length,/*opcode*/buffer[1],addr.IPAddress(),addr.Service()); 
				break;
			}
			case OP_EMULEPROT:
				// Silently drop it.
				theApp.downloadqueue->AddDownDataOverheadOther(length);
				break;
			default:
				printf("Received UDP server packet with unknown protocol 0x%x!\n",buffer[0]);
		}		
	}
}


void CUDPSocket::ReceiveAndDiscard() {
	uint32  buffer[SERVER_UDP_BUFFER_SIZE];
	
	amuleIPV4Address addr;
	RecvFrom(addr,buffer,SERVER_UDP_BUFFER_SIZE);
	// And just discard it :)	
};

void CUDPSocket::ProcessPacket(CSafeMemFile& packet, int16 size, int8 opcode, const wxString& host, uint16 port){

	CServer* update = theApp.serverlist->GetServerByAddress( host, port-4 );

	try{
		// Imported: OP_GLOBSEARCHRES, OP_GLOBFOUNDSORUCES (yes, soruces)  & OP_GLOBSERVSTATRES
		// This makes Server UDP Flags to be set correctly so we use less bandwith on asking servers for sources
		// Also we process Search results and Found sources correctly now on 16.40 behaviour.
		switch(opcode){
			case OP_GLOBSEARCHRES: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// process all search result packets
				int iLeft;
				do{
					/*uint16 uResultCount =*/ theApp.searchlist->ProcessUDPSearchanswer(packet, StringIPtoUint32(host), port-4);
					// There is no need because we don't limit the global results
					// theApp.amuledlg->searchwnd->AddUDPResult(uResultCount);
					// check if there is another source packet
					iLeft = (int)(size - packet.GetPosition());
					if (iLeft >= 2){
						uint8 protocol = packet.ReadUInt8();
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							packet.Seek(-1, wxFromCurrent);
							iLeft += 1;
							break;
						}

						uint8 opcode = packet.ReadUInt8();
						iLeft--;
						if (opcode != OP_GLOBSEARCHRES){
							packet.Seek(-2,wxFromCurrent);
							iLeft += 2;
							break;
						}
					}
				} while (iLeft > 0);
				
				theApp.downloadqueue->AddDownDataOverheadOther(size);

				break;
			}
			case OP_GLOBFOUNDSORUCES:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				// process all source packets
				int iLeft;
				do{
					uint8 fileid[16];
					packet.ReadHash16(fileid);
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
						file->AddSources(packet, StringIPtoUint32(host), port-4);
					else{
						// skip sources for that file
						uint8 count = packet.ReadUInt8();
						packet.Seek(count*(4+2), wxFromStart);
					}

					// check if there is another source packet
					iLeft = (int)(size - packet.GetPosition());
					if (iLeft >= 2){
						uint8 protocol = packet.ReadUInt8();
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							packet.Seek(-1, wxFromCurrent);
							iLeft += 1;
							break;
						}

						uint8 opcode = packet.ReadUInt8();
						iLeft--;
						if (opcode != OP_GLOBFOUNDSORUCES){
							packet.Seek(-2, wxFromCurrent);
							iLeft += 2;
							break;
						}
					}
				} while (iLeft > 0);
				break;
			}

 			case OP_GLOBSERVSTATRES:{
				// Imported from 0.43b
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if( size < 12 || !update) {
					throw(wxString(wxT("Invalid OP_GLOBSERVSTATRES packet or unknown server")));
				}
				uint32 challenge = packet.ReadUInt32();
				if (challenge != update->GetChallenge()) {
					throw(wxString(wxT("Invalid challenge on OP_GLOBSERVSTATRES packet")));
				}
				uint32 cur_user = packet.ReadUInt32();
				uint32 cur_files = packet.ReadUInt32();
				uint32 cur_maxusers = 0;
				uint32 cur_softfiles = 0;
				uint32 cur_hardfiles = 0;
				uint32 uUDPFlags = 0;
				uint32 uLowIDUsers = 0;				
				if( size >= 16 ){
					cur_maxusers = packet.ReadUInt32();
					if( size >= 24 ){
						cur_softfiles = packet.ReadUInt32();
						cur_hardfiles = packet.ReadUInt32();
						if( size >= 28 ){
							uUDPFlags = packet.ReadUInt32();
							if( size >= 32 ){
								uLowIDUsers = packet.ReadUInt32();
							}
						}
					}
				}

				update->SetPing( ::GetTickCount() - update->GetLastPinged() );
				update->SetUserCount( cur_user );
				update->SetFileCount( cur_files );
				update->SetMaxUsers( cur_maxusers );
				update->SetSoftFiles( cur_softfiles );
				update->SetHardFiles( cur_hardfiles );
				update->SetUDPFlags( uUDPFlags );
				update->SetLowIDUsers( uLowIDUsers );
				if (update == theApp.serverconnect->GetCurrentServer()) {
					Notify_ShowUserCount(update);
				}
				break;
			}
 			case OP_SERVER_DESC_RES:{
				// 0.43b
				if (!update) {
					throw(wxString(wxT("Received OP_SERVER_DESC_RES from an unknown server")));
				}

				// old packet: <name_len 2><name name_len><desc_len 2 desc_en>
				// new packet: <challenge 4><taglist>
				//
				// NOTE: To properly distinguish between the two packets which are both useing the same opcode...
				// the first two bytes of <challenge> (in network byte order) have to be an invalid <name_len> at least.
				
				uint16 Len = packet.ReadUInt16();
				
				packet.Seek(-2, wxFromCurrent); // Step back
				
				if (size >= 8 && Len == INV_SERV_DESC_LEN) {
					
					if (update->GetDescReqChallenge() != 0 && packet.ReadUInt32() == update->GetDescReqChallenge()) {
						
						update->SetDescReqChallenge(0);
						
						uint32 uTags = packet.ReadUInt32();
						for (uint32 i = 0; i < uTags; ++i) {
							CTag tag(packet);
							if (tag.tag.specialtag == ST_SERVERNAME && tag.tag.type == 2) {
								update->SetListName(char2unicode(tag.tag.stringvalue));
							} else if (tag.tag.specialtag == ST_DESCRIPTION && tag.tag.type == 2) {
								update->SetDescription(char2unicode(tag.tag.stringvalue));
							} else if (tag.tag.specialtag == ST_DYNIP && tag.tag.type == 2) {
								update->SetDynIP(char2unicode(tag.tag.stringvalue));
							} else if (tag.tag.specialtag == ST_VERSION && tag.tag.type == 2) {
								update->SetVersion(char2unicode(tag.tag.stringvalue));
							} else if (tag.tag.specialtag == ST_VERSION && tag.tag.type == 3) {
								wxString strVersion;
								strVersion.Printf(wxT("%u.%u"), tag.tag.intvalue >> 16,
									tag.tag.intvalue & 0xFFFF);
								update->SetVersion(strVersion);
							} else if (tag.tag.specialtag == ST_AUXPORTSLIST && tag.tag.type == 2) {
								update->SetAuxPortsList(char2unicode(tag.tag.stringvalue));
							} else {
								// Unknown tag
								;
							}
						}
					} else {
						// A server sent us a new server description packet (including a challenge) although we did not
						// ask for it. This may happen, if there are multiple servers running on the same machine with
						// multiple IPs. If such a server is asked for a description, the server will answer 2 times,
						// but with the same IP.
						// ignore this packet
						
					}
				} else {
					update->SetDescription(packet.ReadString());
					update->SetListName(packet.ReadString());
				}
				break;
			}
			default:
				theApp.downloadqueue->AddDownDataOverheadOther(size);
		}
	} catch(wxString error) {
		AddDebugLogLineM(false,wxT("Error while processing incoming UDP Packet: ")+error);
	} catch(...) {
		AddDebugLogLineM(false,wxT("Error while processing incoming UDP Packet (Most likely a misconfigured server)"));
	}
	
	if (update) {
		update->ResetFailedCount();
		Notify_ServerRefresh( update );
	}
	
}

void CUDPSocket::DnsLookupDone(uint32 ip) {
  /* An asynchronous database routine completed. */
	//printf("Server UDP packet dns lookup done\n");
	if (!ip) { 
		if (sendbuffer) {
			delete[] sendbuffer;
			sendbuffer = NULL;
		}
    
		if (cur_server) {
			delete cur_server;
			cur_server = NULL;
		}
		
		if (!server_packet_queue.IsEmpty()) {
			//printf("Sending a queued Server UDP packet after dns lookup\n");
			ServerUDPPacket* queued_packet = server_packet_queue.RemoveHead();
			SendPacket(queued_packet->packet, queued_packet->server);
			delete queued_packet->packet;
			delete queued_packet;
		}
    
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
		if ( Ok() ) {
			SendTo(m_SaveAddr,sendbuffer,sendblen);
		}
		delete[] sendbuffer;
		sendbuffer = NULL;
		delete cur_server;
		cur_server = NULL;
	} else if (cur_server) {	
		delete cur_server;
		cur_server = NULL;		
	} else  if (sendbuffer) {
		delete[] sendbuffer;
		sendbuffer = NULL;		
	}
	
	//printf("Server UDP packet sent.\n");
	
	if (!server_packet_queue.IsEmpty()) {
		//printf("Sending a queued Server UDP packet\n");
		ServerUDPPacket* queued_packet = server_packet_queue.RemoveHead();
		SendPacket(queued_packet->packet, queued_packet->server);
		delete queued_packet->packet;
		delete queued_packet;
	}
}

void CUDPSocket::SendPacket(Packet* packet,CServer* host){

	if (cur_server) {
		// There's a packet being processed, queue this one.
		//printf("Trying to send a Server UDP packet while there's one active, queueing\n");
		ServerUDPPacket* queued_packet = new ServerUDPPacket;
		queued_packet->packet = new Packet(*packet); // Because packet might be deleted
		queued_packet->server = host;
		server_packet_queue.AddTail(queued_packet);
		return;
	}

	//printf("Sending a Server UDP packet ... ");
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
		//printf("Have to solve hostname\n");
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
		//printf("Sending\n");
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

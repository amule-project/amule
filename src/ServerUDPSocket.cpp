//
// This file is part of the aMule Project
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


#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "UDPSocket.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "ServerUDPSocket.h"	// Interface declarations.
#include "Packet.h"		// Needed for CPacket
#include "PartFile.h"		// Needed for CPartFile
#include "SearchList.h"		// Needed for CSearchList
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ServerList.h"		// Needed for CServerList
#include "OPCodes.h"		// Needed for OP_EDONKEYPROT
#include "Server.h"		// Needed for CServer
#include "amule.h"			// Needed for theApp
#include "NetworkFunctions.h" // Needed for CAsyncDNS
#include "GetTickCount.h"
#include "ServerSocket.h"
#include "Statistics.h"		// Needed for CStatistics
#include "StringFunctions.h" // Needed for unicode2char 
#include <sys/types.h>

//
// (TCP+3) UDP socket
//

CServerUDPSocket::CServerUDPSocket(
	CServerConnect* in_serverconnect,
	amuleIPV4Address &address,
	const CProxyData *ProxyData)
:
CDatagramSocketProxy(address, wxSOCKET_NOWAIT, ProxyData)
#ifdef AMULE_DAEMON
, wxThread(wxTHREAD_JOINABLE)
#endif
{
	sendbuffer = NULL;
	cur_server = NULL;
	serverconnect = in_serverconnect;

#ifdef AMULE_DAEMON
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("ERROR: CServerUDPSocket failed create\n");
		wxASSERT(0);
	}
	Run();
#else
	SetEventHandler(theApp, UDPSOCKET_HANDLER);
	SetNotify(wxSOCKET_INPUT_FLAG);
	Notify(true);
#endif
  
}

CServerUDPSocket::~CServerUDPSocket(){
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

void CServerUDPSocket::OnReceive(int WXUNUSED(nErrorCode)) {
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
				theApp.statistics->AddDownDataOverheadOther(length);
				break;
			default:
				printf("Received UDP server packet with unknown protocol 0x%x!\n",buffer[0]);
		}		
	}
}


void CServerUDPSocket::ReceiveAndDiscard() {
	uint32  buffer[SERVER_UDP_BUFFER_SIZE];
	
	amuleIPV4Address addr;
	RecvFrom(addr,buffer,SERVER_UDP_BUFFER_SIZE);
	// And just discard it :)	
};

void CServerUDPSocket::ProcessPacket(CSafeMemFile& packet, int16 size, int8 opcode, const wxString& host, uint16 port){

	CServer* update = theApp.serverlist->GetServerByAddress( host, port-4 );
	
	theApp.statistics->AddDownDataOverheadOther(size);
	
	try{
		// Imported: OP_GLOBSEARCHRES, OP_GLOBFOUNDSOURCES & OP_GLOBSERVSTATRES
		// This makes Server UDP Flags to be set correctly so we use less bandwith on asking servers for sources
		// Also we process Search results and Found sources correctly now on 16.40 behaviour.
		switch(opcode){
			case OP_GLOBSEARCHRES: {

				// process all search result packets

				do{
					/*uint16 uResultCount =*/ theApp.searchlist->ProcessUDPSearchanswer(packet, true /* (update && update->GetUnicodeSupport())*/, StringIPtoUint32(host), port-4);
					// There is no need because we don't limit the global results
					// theApp.amuledlg->searchwnd->AddUDPResult(uResultCount);
					// check if there is another source packet
					
					if (packet.GetPosition()+2 < size) {
						// An additional packet?
						uint8 protocol = packet.ReadUInt8();
						uint8 new_opcode = packet.ReadUInt8();
					
						if (protocol != OP_EDONKEYPROT || new_opcode != OP_GLOBSEARCHRES) {
							printf("Server search reply got additional bogus bytes\n");
							break;
						} else {
							printf("Got server search reply with additional packet\n");
						}
					}					
					
				} while (packet.GetPosition()+2 < size);
				
				break;
			}
			case OP_GLOBFOUNDSOURCES:{
				// process all source packets
				do{
					uint8 fileid[16];
					packet.ReadHash16(fileid);
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid)) {
						file->AddSources(packet, StringIPtoUint32(host), port-4);
					} else {
						printf("\tSources received for unknown file\n");
						// skip sources for that file
						uint8 count = packet.ReadUInt8();
						packet.Seek(count*(4+2), wxFromCurrent);
					}

					if (packet.GetPosition()+2 < size) {
						// An additional packet?
						uint8 protocol = packet.ReadUInt8();
						uint8 new_opcode = packet.ReadUInt8();
					
						if (protocol != OP_EDONKEYPROT || new_opcode != OP_GLOBFOUNDSOURCES) {
							printf("\tServer sources reply got additional bogus bytes\n");
							break;
						} 
					}
				} while ((packet.GetPosition() + 2) < size);
				break;
			}

 			case OP_GLOBSERVSTATRES:{
				// Imported from 0.43b
				
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
							CTag tag(packet, update->GetUnicodeSupport());
							switch (tag.GetNameID()) {
								case ST_SERVERNAME:
									if (tag.IsStr()) {
										update->SetListName(tag.GetStr());
									}
									break;
								case ST_DESCRIPTION:
									if (tag.IsStr()) {
										update->SetDescription(tag.GetStr());
									}
									break;
								case ST_DYNIP:
									if (tag.IsStr()) {
										update->SetDynIP(tag.GetStr());
									}
									break;
								case ST_VERSION:
									if (tag.IsStr()) {
										update->SetVersion(tag.GetStr());
									} else if (tag.IsInt()) {
										wxString strVersion = wxString::Format(wxT("%u.%u"), tag.GetInt() >> 16, tag.GetInt() & 0xFFFF);
										update->SetVersion(strVersion);
									}
									break;
								case ST_AUXPORTSLIST:
									if (tag.IsStr()) {
										update->SetAuxPortsList(tag.GetStr());
									}
									break;
								default:
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
					update->SetDescription(packet.ReadString(update->GetUnicodeSupport()));
					update->SetListName(packet.ReadString(update->GetUnicodeSupport()));
				}
				break;
			}
			default:
				printf("Unknown Server UDP opcode %x\n",opcode);
		}
	} catch(wxString error) {
		printf(	"Brrrrrr wrong UDP packet from server! (%s)\n",
			(const char *)unicode2char(error));
		AddDebugLogLineM(false, wxT("Error while processing incoming UDP Packet: ") + error);
	} catch(...) {
		printf("Brrrrrr wrong UDP packet from server!\n");
		AddDebugLogLineM(false,
			wxT("Error while processing incoming UDP Packet (Most likely a misconfigured server)"));
	}
	
	if (update) {
		update->ResetFailedCount();
		Notify_ServerRefresh( update );
	}
	
}

void CServerUDPSocket::OnHostnameResolved(uint32 ip) {
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

void CServerUDPSocket::SendBuffer(){
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

void CServerUDPSocket::SendPacket(CPacket* packet,CServer* host){

	if (cur_server) {
		// There's a packet being processed, queue this one.
		//printf("Trying to send a Server UDP packet while there's one active, queueing\n");
		ServerUDPPacket* queued_packet = new ServerUDPPacket;
		queued_packet->packet = new CPacket(*packet); // Because packet might be deleted
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
		CAsyncDNS* dns=new CAsyncDNS(cur_server->GetDynIP(), DNS_UDP, this);
		if(dns->Create()!=wxTHREAD_NO_ERROR) {
			// uh?
			return;
		}
		dns->Run();
	} else {
		m_SaveAddr.Hostname(cur_server->GetIP());
		//printf("Sending\n");
		SendBuffer();
	}
}

#ifdef AMULE_DAEMON

void *CServerUDPSocket::Entry()
{
	while ( !TestDestroy() ) {
		if ( WaitForRead(0, 1000) ) {
			OnReceive(0);
		}
	}
	return 0;
}

#endif

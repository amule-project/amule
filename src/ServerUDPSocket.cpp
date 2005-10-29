//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "ServerUDPSocket.h"	// Interface declarations.
#include "Packet.h"		// Needed for CPacket
#include "PartFile.h"		// Needed for CPartFile
#include "SearchList.h"		// Needed for CSearchList
#include "MemFile.h"		// Needed for CMemFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "ServerList.h"		// Needed for CServerList
#include "OPCodes.h"		// Needed for OP_EDONKEYPROT
#include "Server.h"		// Needed for CServer
#include "amule.h"			// Needed for theApp
#include "NetworkFunctions.h" // Needed for CAsyncDNS
#include "GetTickCount.h"
#include "ServerSocket.h"
#include "Statistics.h"		// Needed for theStats
#include "StringFunctions.h" // Needed for unicode2char 
#include "Logger.h"
#include "Format.h"
#include "updownclient.h"	// Needed for SF_SERVER

#include <sys/types.h>



//
// (TCP+3) UDP socket
//

CServerUDPSocket::CServerUDPSocket(amuleIPV4Address &address, const CProxyData *ProxyData)
	: CDatagramSocketProxy(address, wxSOCKET_NOWAIT, ProxyData)
{
	SetEventHandler(theApp, SERVERUDPSOCKET_HANDLER);
	SetNotify(wxSOCKET_INPUT_FLAG);
	Notify(true);
}


CServerUDPSocket::~CServerUDPSocket()
{
	SetNotify(0);
	Notify(FALSE);
}


const unsigned SERVER_UDP_BUFFER_SIZE = 5000;

void CServerUDPSocket::OnReceive(int WXUNUSED(nErrorCode))
{
	uint8 buffer[SERVER_UDP_BUFFER_SIZE];

	amuleIPV4Address addr;
	int length = DoReceive(addr,(char*)buffer,SERVER_UDP_BUFFER_SIZE);
	if (length > 2) { 
		// 2 bytes (protocol and opcode) must be there. This 'if' also takes care
		// of len == -1 (read error on udp socket)
			
		switch (/*Protocol*/buffer[0]) {
			case OP_EDONKEYPROT: {
				length = length - 2; // skip protocol and opcode
				// Create the safe mem file.	
				CMemFile  data(buffer+2,length);
				wxASSERT(length == data.GetLength());
				ProcessPacket(data,length,/*opcode*/buffer[1],addr.IPAddress(),addr.Service()); 
				break;
			}
			case OP_EMULEPROT:
				// Silently drop it.
				AddDebugLogLineM( true, logServerUDP,
					wxString::Format( wxT("Received UDP server packet with eDonkey protocol (0x%x) and opcode (0x%x)!"), buffer[0], buffer[0] ) );
				theStats::AddDownOverheadOther(length);
				break;
			default:
				AddDebugLogLineM( true, logServerUDP,
					wxString::Format( wxT("Received UDP server packet with unknown protocol 0x%x!"), buffer[0] ) );
		}		
	}
}


int CServerUDPSocket::DoReceive(amuleIPV4Address& addr, char* buffer, uint32 max_size)
{
	RecvFrom(addr,buffer,max_size);
	return LastCount();
}


void CServerUDPSocket::ProcessPacket(CMemFile& packet, int16 size, int8 opcode, const wxString& host, uint16 port)
{
	CServer* update = theApp.serverlist->GetServerByIP(StringIPtoUint32(host), port - 4);
	
	theStats::AddDownOverheadOther(size);
	AddDebugLogLineM( false, logServerUDP,
					CFormat( wxT("Received UDP server packet from %s:%u, opcode (0x%x)!")) % host % port % opcode );
	
	try {
		// Imported: OP_GLOBSEARCHRES, OP_GLOBFOUNDSOURCES & OP_GLOBSERVSTATRES
		// This makes Server UDP Flags to be set correctly so we use less bandwith on asking servers for sources
		// Also we process Search results and Found sources correctly now on 16.40 behaviour.
		switch(opcode){
			case OP_GLOBSEARCHRES: {

				// process all search result packets

				do{
					theApp.searchlist->ProcessUDPSearchanswer(packet, true, StringIPtoUint32(host), port - 4);
					
					if (packet.GetPosition()+2 < size) {
						// An additional packet?
						uint8 protocol = packet.ReadUInt8();
						uint8 new_opcode = packet.ReadUInt8();
					
						if (protocol != OP_EDONKEYPROT || new_opcode != OP_GLOBSEARCHRES) {
							AddDebugLogLineM( true, logServerUDP,
								wxT("Server search reply got additional bogus bytes.") );
							break;
						} else {
							AddDebugLogLineM( true, logServerUDP, 
								wxT("Got server search reply with additional packet.") );
						}
					}					
					
				} while (packet.GetPosition()+2 < size);
				
				break;
			}
			case OP_GLOBFOUNDSOURCES:{
				// process all source packets
				do {
					CMD4Hash fileid = packet.ReadHash();
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid)) {
						file->AddSources(packet, StringIPtoUint32(host), port-4, SF_SERVER);
					} else {
						AddDebugLogLineM( true, logServerUDP, wxT("Sources received for unknown file") );
						// skip sources for that file
						uint8 count = packet.ReadUInt8();
						packet.Seek(count*(4+2), wxFromCurrent);
					}

					if (packet.GetPosition()+2 < size) {
						// An additional packet?
						uint8 protocol = packet.ReadUInt8();
						uint8 new_opcode = packet.ReadUInt8();
					
						if (protocol != OP_EDONKEYPROT || new_opcode != OP_GLOBFOUNDSOURCES) {
							AddDebugLogLineM( true, logServerUDP,
								wxT("Server sources reply got additional bogus bytes.") );
							break;
						} 
					}
				} while ((packet.GetPosition() + 2) < size);
				break;
			}

 			case OP_GLOBSERVSTATRES:{
				// Imported from 0.43b
				if (!update) {
					throw wxString(wxT("Unknown server on a OP_GLOBSERVSTATRES packet (") + host + wxString::Format(wxT(":%d)"), port-4));
				}
				if( size < 12) {
					throw(wxString(wxString::Format(wxT("Invalid OP_GLOBSERVSTATRES packet (size=%d)"),size)));
				}
				uint32 challenge = packet.ReadUInt32();
				if (challenge != update->GetChallenge()) {
					throw(wxString(wxString::Format(wxT("Invalid challenge on OP_GLOBSERVSTATRES packet (0x%x != 0x%x)"),challenge,update->GetChallenge())));
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
				Notify_ServerRefresh( update );
				theApp.ShowUserCount();
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
									update->SetListName(tag.GetStr());
									break;
								case ST_DESCRIPTION:
									update->SetDescription(tag.GetStr());
									break;
								case ST_DYNIP:
									update->SetDynIP(tag.GetStr());
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
									update->SetAuxPortsList(tag.GetStr());
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
				AddDebugLogLineM( true, logServerUDP,
					wxString::Format( wxT("Unknown Server UDP opcode %x"), opcode ) );
		}
	} catch (const wxString& error) {
		AddDebugLogLineM(false, logServer, wxT("Error while processing incoming UDP Packet: ") + error);
	} catch (const CInvalidPacket& error) {
		AddDebugLogLineM(false, logServer, wxT("Invalid UDP packet encountered: ") + error.what());
	} catch (const CEOFException& e) {
		AddDebugLogLineM(false, logServer, wxT("IO error while processing incoming UDP Packet: ") + e.what());
	}
	
	if (update) {
		update->ResetFailedCount();
		Notify_ServerRefresh( update );
	}
	
}


void CServerUDPSocket::SendPacket(CPacket* packet, CServer* host, bool delPacket)
{
	ServerUDPPacket item;
	
	if (delPacket) {
		item.packet = packet;
	} else {
		item.packet = new CPacket(*packet);
	}
	
	item.port = host->GetPort();
	
	if (host->HasDynIP()) {
		item.ip = 0;
		item.addr = host->GetDynIP();
	} else {
		item.ip = host->GetIP();
	}
	
	m_queue.push_back(item);

	// If there is more than one item in the queue,
	// then we are already waiting for a dns query.
	if (m_queue.size() == 1) {
		SendBuffer();
	}
}


void CServerUDPSocket::SendBuffer()
{
	while (m_queue.size()) {
		if (Ok()) {
			ServerUDPPacket item = m_queue.front();
			CPacket* packet = item.packet;
		
			// Do we need to do a DNS lookup before sending?
			wxASSERT(item.ip xor !item.addr.IsEmpty());
			if (!item.addr.IsEmpty()) {
				// This not an ip but a hostname. Resolve it.
				CAsyncDNS* dns = new CAsyncDNS(item.addr, DNS_UDP, this);
				if ((dns->Create() != wxTHREAD_NO_ERROR) or (dns->Run() != wxTHREAD_NO_ERROR)) {
					// Not much we can do here, just drop the packet.
					m_queue.pop_front();
				
					continue;
				}

				// Wait for the DNS query to be resolved
				return;
			}
		
			char buffer[packet->GetPacketSize() + 2];
			memcpy(buffer, packet->GetUDPHeader(), 2);
			memcpy(buffer + 2, packet->GetDataBuffer(), packet->GetPacketSize());
		
			amuleIPV4Address addr;
			addr.Hostname(item.ip);
			addr.Service(item.port + 4);
	
			SendTo(addr, buffer, packet->GetPacketSize() + 2);

			if (Error()) {
				printf("Server UDP port returned an error: %i\n", LastError());
			}
		
			delete packet;
		}
		
		m_queue.pop_front();
	}
}


void CServerUDPSocket::OnHostnameResolved(uint32 ip)
{
	/* An asynchronous database routine completed. */
	if (ip == 0) { 
		m_queue.pop_front();	
	} else {
		ServerUDPPacket item = m_queue.front();
		wxASSERT(!item.ip and !item.addr.IsEmpty());

		CServer* update = theApp.serverlist->GetServerByAddress(item.addr, item.port);
		if (update) {
			update->SetID(ip);
		}
		
		item.addr.Clear();
		item.ip = ip;
	}
	
	SendBuffer();
}


//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "Types.h"

#include "ClientUDPSocket.h"	// Interface declarations
#include "Preferences.h"		// Needed for CPreferences
#include "PartFile.h"			// Needed for CPartFile
#include "updownclient.h"		// Needed for CUpDownClient
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "Packet.h"				// Needed for CPacket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "KnownFile.h"			// Needed for CKnownFile
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "OPCodes.h"			// Needed for OP_EMULEPROT
#include "Statistics.h"			// Needed for theStats
#include "amule.h"				// Needed for theApp
#include "ClientList.h"			// Needed for clientlist (buddy support)
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "OtherFunctions.h"
#include "MemFile.h"			// Needed for CMemFile
#include "Logger.h"
#include "UploadBandwidthThrottler.h"
#include "kademlia/kademlia/Kademlia.h"
#include "zlib.h"

//
// CClientUDPSocket -- Extended eMule UDP socket
//

CClientUDPSocket::CClientUDPSocket(const amuleIPV4Address& address, const CProxyData* ProxyData)
	: CMuleUDPSocket(wxT("Client UDP-Socket"), CLIENTUDPSOCKET_HANDLER, address, ProxyData)
{
	if (!thePrefs::IsUDPDisabled()) {
		Open();
	}
}


void CClientUDPSocket::OnReceive(int errorCode)
{
	CMuleUDPSocket::OnReceive(errorCode);

	// TODO: A better solution is needed.
	if (thePrefs::IsUDPDisabled()) {
		Close();
	}
}


void CClientUDPSocket::OnPacketReceived(amuleIPV4Address& addr, byte* buffer, size_t length)
{
	wxCHECK_RET(length >= 2, wxT("Invalid packet."));
	
	uint8 protocol	= buffer[0];
	uint8 opcode	= buffer[1];
	uint32 ip		= StringIPtoUint32(addr.IPAddress());
	uint16 port		= addr.Service();
	
	try {
		switch (protocol) {
			case OP_EMULEPROT:
				ProcessPacket(buffer + 2,length - 2, opcode, ip, port);
				break;
				
			case OP_KADEMLIAHEADER:
				theStats::AddDownOverheadKad(length);
				Kademlia::CKademlia::ProcessPacket(buffer, length, wxUINT32_SWAP_ALWAYS(ip), port);
				break;
				
			case OP_KADEMLIAPACKEDPROT: {
				theStats::AddDownOverheadKad(length);
				uint32 nNewSize = length*10+300; // Should be enough...
				byte unpack[nNewSize];
				uLongf unpackedsize = nNewSize-2;
				uint16 result = uncompress(unpack + 2, &unpackedsize, buffer + 2, length-2);
				if (result == Z_OK) {
					unpack[0] = OP_KADEMLIAHEADER;
					unpack[1] = opcode;
					Kademlia::CKademlia::ProcessPacket(unpack, unpackedsize + 2, wxUINT32_SWAP_ALWAYS(ip), port);
				} else {
					AddDebugLogLineM(false, logClientKadUDP, wxT("Failed to uncompress Kademlia packet"));
				}
				break;
			}
			default:
				AddDebugLogLineM(false, logClientUDP, wxString::Format(wxT("Unknown opcode on received packet: 0x%x"), protocol));
		}
	} catch (const wxString& e) {
		AddDebugLogLineM(false, logClientUDP, wxT("Error while parsing UDP packet: ") + e);
	} catch (const CInvalidPacket& e) {
		AddDebugLogLineM(false, logClientUDP, wxT("Invalid UDP packet encountered: ") + e.what());
	} catch (const CEOFException& e) {
		AddDebugLogLineM(false, logClientUDP, wxT("Malformed packet encountered while parsing UDP packet: ") + e.what());
	}
}


void CClientUDPSocket::ProcessPacket(byte* packet, int16 size, int8 opcode, uint32 host, uint16 port)
{
	switch (opcode) {
		case OP_REASKCALLBACKUDP: {
			AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket; OP_REASKCALLBACKUDP") );
			theStats::AddDownOverheadOther(size);
			CUpDownClient* buddy = theApp.clientlist->GetBuddy();
			if( buddy ) {
				if( size < 17 || buddy->GetSocket() == NULL ) {
					break;
				}
				if (!md4cmp(packet, buddy->GetBuddyID())) {
					/*
						The packet has an initial 16 bytes key for the buddy.
						This is currently unused, so to make the transformation 
						we discard the first 10 bytes below and then overwrite 
						the other 6 with ip/port.
					*/
					CMemFile mem_packet(packet+10,size-10);
					// Change the ip and port while leaving the rest untouched
					mem_packet.Seek(0,wxFromStart);
					mem_packet.WriteUInt32(host);
					mem_packet.WriteUInt16(port);
					CPacket* response = new CPacket(&mem_packet, OP_EMULEPROT, OP_REASKCALLBACKTCP);
					AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: send OP_REASKCALLBACKTCP") );
					theStats::AddUpOverheadFileRequest(response->GetPacketSize());
					buddy->GetSocket()->SendPacket(response);
				}
			}
			break;
		}
		case OP_REASKFILEPING: {
			AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: OP_REASKFILEPING") );
			theStats::AddDownOverheadFileRequest(size);
			
			CMemFile data_in(packet, size);
			CMD4Hash reqfilehash = data_in.ReadHash();
			CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
			if (!reqfile) {
				CPacket* response = new CPacket(OP_FILENOTFOUND,0,OP_EMULEPROT);
				theStats::AddUpOverheadFileRequest(response->GetPacketSize());
				SendPacket(response,host,port);
				break;
			}
			CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(host, port);
			if (sender){
				sender->CheckForAggressive();
				
				//Make sure we are still thinking about the same file
				if (reqfilehash == sender->GetUploadFileID()) {
					sender->AddAskedCount();
					sender->SetUDPPort(port);
					sender->SetLastUpRequest();

					if (sender->GetUDPVersion() > 3) {
						sender->ProcessExtendedInfo(&data_in, reqfile);
					} else  if (sender->GetUDPVersion() > 2) {
						uint16 nCompleteCountLast = sender->GetUpCompleteSourcesCount();
						uint16 nCompleteCountNew = data_in.ReadUInt16();
						sender->SetUpCompleteSourcesCount(nCompleteCountNew);							
						if (nCompleteCountLast != nCompleteCountNew) {
							reqfile->UpdatePartsInfo();
						}
					}
					
					CMemFile data_out(128);
					if(sender->GetUDPVersion() > 3) {
						if (reqfile->IsPartFile()) {
							((CPartFile*)reqfile)->WritePartStatus(&data_out);
						} else {
							data_out.WriteUInt16(0);
						}
					}
					
					data_out.WriteUInt16(theApp.uploadqueue->GetWaitingPosition(sender));
					CPacket* response = new CPacket(&data_out, OP_EMULEPROT, OP_REASKACK);
					theStats::AddUpOverheadFileRequest(response->GetPacketSize());
					AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: OP_REASKACK to ") + sender->GetFullIP());
					SendPacket(response, host, port);
				} else {
					AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket; ReaskFilePing; reqfile does not match") );
				}						
			} else {
				if ((theStats::GetWaitingUserCount() + 50) > thePrefs::GetQueueSize()) {
					CPacket* response = new CPacket(OP_QUEUEFULL,0,OP_EMULEPROT);
					theStats::AddUpOverheadFileRequest(response->GetPacketSize());
					SendPacket(response,host,port);
				}
			}
			break;
		}
		case OP_QUEUEFULL: {
			AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: OP_QUEUEFULL") );
			theStats::AddDownOverheadOther(size);
			CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(host,port);
			if (sender) {
				sender->SetRemoteQueueFull(true);
				sender->UDPReaskACK(0);
			}
			break;
		}
		case OP_REASKACK: {				
			theStats::AddDownOverheadFileRequest(size);
			CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(host,port);
			if (sender) {
				CMemFile data_in(packet,size);
				if ( sender->GetUDPVersion() > 3 ) {
					sender->ProcessFileStatus(true, &data_in, sender->GetRequestFile());
				}
				uint16 nRank = data_in.ReadUInt16();
				sender->SetRemoteQueueFull(false);
				sender->UDPReaskACK(nRank);
			}
			break;
		}
		case OP_FILENOTFOUND: {
			AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: OP_FILENOTFOUND") );
			theStats::AddDownOverheadFileRequest(size);
			CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(host,port);
			if (sender){
				sender->UDPReaskFNF(); // may delete 'sender'!
				sender = NULL;
			}
			break;
		}
		
		default:
			theStats::AddDownOverheadOther(size);				
	}
}

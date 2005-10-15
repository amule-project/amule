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

#include "Types.h"

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/socket.h>

#include "ClientUDPSocket.h"	// Interface declarations
#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include "Preferences.h"	// Needed for CPreferences
#include "PartFile.h"		// Needed for CPartFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "Packet.h"		// Needed for CPacket
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "KnownFile.h"		// Needed for CKnownFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "OPCodes.h"		// Needed for OP_EMULEPROT
#include "Statistics.h"		// Needed for theStats
#include "amule.h"		// Needed for theApp
#include "ClientList.h"		// Needed for clientlist (buddy support)
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "OtherFunctions.h"
#include "MemFile.h"		// Needed for CMemFile
#include "Logger.h"
#include "UploadBandwidthThrottler.h"

#ifdef __COMPILE_KAD__
	#include "kademlia/kademlia/Kademlia.h"
	#include "kademlia/io/IOException.h"
	#include "zlib.h"
#endif

//
// CClientUDPSocket -- Extended eMule UDP socket
//

IMPLEMENT_DYNAMIC_CLASS(CClientUDPSocket, CDatagramSocketProxy)

CClientUDPSocket::CClientUDPSocket(amuleIPV4Address &address, const CProxyData *ProxyData)
	: CDatagramSocketProxy(address, wxSOCKET_NOWAIT, ProxyData)
{
	m_bWouldBlock = false;

	SetEventHandler(theApp, CLIENTUDPSOCKET_HANDLER);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
	Notify(true);
}

CClientUDPSocket::~CClientUDPSocket()
{
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this);

	SetNotify(0);
	Notify(FALSE);
}

#define CLIENT_UDP_BUFFER_SIZE 5000

void CClientUDPSocket::OnReceive(int WXUNUSED(nErrorCode))
{
	amuleIPV4Address	addr;
	byte				buffer[CLIENT_UDP_BUFFER_SIZE];
	uint32				length;

	{
		wxMutexLocker lock(m_sendLocker);
	
		length = RecvFrom(addr, buffer, CLIENT_UDP_BUFFER_SIZE).LastCount();
		
		if (Error()) {
			AddDebugLogLineM(false, logClientUDP, wxString::Format(wxT("Error while reading from CClientUDPSocket: %i"), LastError()));
			return;
		} else if (length < 2) {
			AddDebugLogLineM(false, logClientUDP, wxT("Packet too short on CClientUDPSocket::OnReceive"));
			return;
		}
		
		if (thePrefs::IsUDPDisabled()) {
			Close();
			return;
		}
	}

	if (!StringIPtoUint32(addr.IPAddress())) {
		printf("Unknown ip receiving an UDP packet! Ignoring\n");
		wxASSERT(0);
		return;
	}
	
	if (!addr.Service()) {
		printf("Unknown port receiving an UDP packet! Ignoring\n");
		wxASSERT(0);
		return;
	}
	
	try {
		switch (buffer[0]) {
			case OP_EMULEPROT:
				ProcessPacket(buffer+2,length-2,buffer[1],StringIPtoUint32(addr.IPAddress()),addr.Service());
				break;
			#ifdef __COMPILE_KAD__
			case OP_KADEMLIAHEADER:
				theStats::AddDownOverheadKad(length);
				Kademlia::CKademlia::processPacket(buffer, length, wxUINT32_SWAP_ALWAYS(StringIPtoUint32(addr.IPAddress())),addr.Service());
				break;
			case OP_KADEMLIAPACKEDPROT: {
				theStats::AddDownOverheadKad(length);
				uint32 nNewSize = length*10+300; // Should be enough...
				byte unpack[nNewSize];
				uLongf unpackedsize = nNewSize-2;
				uint16 result = uncompress(unpack +2, &unpackedsize, buffer+2, length-2);
				if (result == Z_OK) {
					unpack[0] = OP_KADEMLIAHEADER;
					unpack[1] = buffer[1];
					Kademlia::CKademlia::processPacket(unpack, unpackedsize+2, wxUINT32_SWAP_ALWAYS(StringIPtoUint32(addr.IPAddress())),addr.Service());
				} else {
					AddDebugLogLineM(false, logClientKadUDP, wxT("Failed to uncompress Kademlia packet"));
				}
				break;
			}
			#endif
			default:
				AddDebugLogLineM(false, logClientUDP, wxString::Format(wxT("Unknown opcode on received packet: 0x%x"),buffer[0]));
		}
	} catch (const wxString& e) {
		AddDebugLogLineM(false, logClientUDP, wxT("Error while parsing UDP packet: ") + e);
	} catch (const CEOFException& e) {
		AddDebugLogLineM(false, logClientUDP, wxT("Malformed packet encountered while parsing UDP packet: ") + e.what());
#ifdef __COMPILE_KAD__
	} catch (const Kademlia::CIOException&) {
		AddDebugLogLineM(false, logClientUDP, wxT("Malformed packet encountered while parsing UDP packet"));
#endif
	}
}


bool CClientUDPSocket::ProcessPacket(byte* packet, int16 size, int8 opcode, uint32 host, uint16 port)
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
					CMemFile mem_packet((byte*)packet+10,size-10);
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
			
			CMemFile data_in((byte*)packet, size);
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
					#ifdef __USE_DEBUG__						
					if (thePrefs.GetDebugClientUDPLevel() > 0) {
						DebugSend("OP__ReaskAck", sender);
					}
					#endif
					CPacket* response = new CPacket(&data_out, OP_EMULEPROT);
					response->SetOpCode(OP_REASKACK);
					theStats::AddUpOverheadFileRequest(response->GetPacketSize());
					theApp.clientudp->SendPacket(response, host, port);
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
				CMemFile data_in((byte*)packet,size);
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
			return false;
	}

	return true;
}


void CClientUDPSocket::OnSend(int nErrorCode)
{
	if (nErrorCode) {
		return;
	}

	wxMutexLocker lock(m_sendLocker);
	m_bWouldBlock = false;

    if(!controlpacket_queue.IsEmpty()) {
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }
}


#define UDPMAXQUEUETIME                       SEC2MS(30)      //30 Seconds

SocketSentBytes CClientUDPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 WXUNUSED(minFragSize))
{
    wxMutexLocker lock(m_sendLocker);
    uint32 sentBytes = 0;

	while (!controlpacket_queue.IsEmpty() && !IsBusy() && sentBytes < maxNumberOfBytesToSend){ // ZZ:UploadBandWithThrottler (UDP)
		UDPPack cur_packet = controlpacket_queue.GetHead();
		if( GetTickCount() - cur_packet.dwTime < UDPMAXQUEUETIME )
		{
			char* sendbuffer = new char[cur_packet.packet->GetPacketSize()+2];
			memcpy(sendbuffer,cur_packet.packet->GetUDPHeader(),2);
			memcpy(sendbuffer+2,cur_packet.packet->GetDataBuffer(),cur_packet.packet->GetPacketSize());

            if (SendTo(sendbuffer, cur_packet.packet->GetPacketSize()+2, cur_packet.dwIP, cur_packet.nPort)){
                sentBytes += cur_packet.packet->GetPacketSize()+2; // ZZ:UploadBandWithThrottler (UDP)

				controlpacket_queue.RemoveHead();
				delete cur_packet.packet;
				delete[] sendbuffer;
            } else {
				// TODO: Needs better error handling, see SentTo
				delete[] sendbuffer;
				break;
			}
		} else {
			controlpacket_queue.RemoveHead();
			delete cur_packet.packet;
		}
	}

    if(!IsBusy() && !controlpacket_queue.IsEmpty()) {
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }

    SocketSentBytes returnVal = { true, 0, sentBytes };
    return returnVal;
}


bool CClientUDPSocket::SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort)
{
	amuleIPV4Address addr;
	addr.Hostname(dwIP);
	addr.Service(nPort);

	// We better clear this flag here, status might have been changed
	// between the U.B.T. adition and the real sending happening later
	m_bWouldBlock = false; 

	bool sent = false;
	
	if (Ok()) {
		CDatagramSocketProxy::SendTo(addr,lpBuf,nBufLen);

		if (Error()) {
			wxSocketError error = LastError();
			printf("Client UDP port returned an error: %i\n", error);
			
			switch (error) {
				case wxSOCKET_WOULDBLOCK:
					// Socket is busy and can't send this data right now,
					// so we just return not sent and set the wouldblock 
					// flag so it gets resent when socket is ready.
					m_bWouldBlock = true;
					sent = false;
					break;
					
				default:
					// An error which we can't handle happended, so we drop 
					// the packet rather than risk entering an infinite loop.
					printf("WARNING! Discarded packet due to errors while sending.\n");
					sent = true;
					break;
			}
		} else {
			sent = true;
		}
	} else {
		// If the socket is not ok, we can do nothing... just run for your life
		// (and return true or this packet will be sent over and over again)
		sent = true;
	}

	return sent;
}


bool CClientUDPSocket::SendPacket(CPacket* packet, uint32 dwIP, uint16 nPort)
{
	wxASSERT(nPort);
	
	UDPPack newpending;
	newpending.dwIP = dwIP;
	newpending.nPort = nPort;
	newpending.packet = packet;
	newpending.dwTime = GetTickCount();
    
	{
		wxMutexLocker lock(m_sendLocker);
		controlpacket_queue.AddTail(newpending);
	}
	
	theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
	return true;
}

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

//
// ClientUDPSocket.cpp : implementation file
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ClientUDPSocket.h"
#endif

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
#include "Statistics.h"		// Needed for CStatistics
#include "amule.h"			// Needed for theApp
#include "ClientList.h"		// Needed for clientlist (buddy support)
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "OtherFunctions.h"
#include "MemFile.h"		// Needed for CMemFile
#include "Logger.h"
#include "UploadBandwidthThrottler.h"

#ifdef __COMPILE_KAD__
	#include "kademlia/kademlia/Kademlia.h"
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

	try {
		switch (buffer[0]) {
			case OP_EMULEPROT:
				ProcessPacket(buffer+2,length-2,buffer[1],StringIPtoUint32(addr.IPAddress()),addr.Service());
				break;
			#ifdef __COMPILE_KAD__
			case OP_KADEMLIAHEADER:
				//theStats.AddDownDataOverheadKad(length);
				Kademlia::CKademlia::processPacket(buffer, length, ENDIAN_NTOHL(StringIPtoUint32(addr.IPAddress())),addr.Service());
				break;
			case OP_KADEMLIAPACKEDPROT: {
				//theStats.AddDownDataOverheadKad(length);
				uint32 nNewSize = length*10+300; // Should be enough...
				byte unpack[nNewSize];
				try {
					uLongf unpackedsize = nNewSize-2;
					uint16 result = uncompress(unpack +2, &unpackedsize, buffer+2, length-2);
					if (result == Z_OK) {
						unpack[0] = OP_KADEMLIAHEADER;
						unpack[1] = buffer[1];
						Kademlia::CKademlia::processPacket(unpack, unpackedsize+2, ENDIAN_NTOHL(StringIPtoUint32(addr.IPAddress())),addr.Service());
					} else {
						AddDebugLogLineM(false, logClientKadUDP, wxT("Failed to uncompress Kademlia packet"));
					}
				} catch(...) {
					AddDebugLogLineM(false, logClientKadUDP, wxT("Something wrong happened on a compressed Kad packet\n"));
				}
				break;
			}
			#endif
			default:
				AddDebugLogLineM(false, logClientUDP, wxString::Format(wxT("Unknown opcode on received packet: 0x%x"),buffer[0]));
		}
	} catch (...) {
		AddDebugLogLineM(false, logClientUDP, wxT("Unhanled exception on UDP socket receive!"));
	}
}


bool CClientUDPSocket::ProcessPacket(byte* packet, int16 size, int8 opcode, uint32 host, uint16 port)
{
	try {
		switch(opcode) {
			case OP_REASKCALLBACKUDP: {
				AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket; OP_REASKCALLBACKUDP") );
				theApp.statistics->AddDownDataOverheadOther(size);
				CUpDownClient* buddy = theApp.clientlist->GetBuddy();
				if( buddy ) {
					if( size < 17 || buddy->GetSocket() == NULL ) {
						break;
					}
					if (!md4cmp(packet, buddy->GetBuddyID())) {
						CMemFile mem_packet((byte*)packet,size-10);
						// Change the ip and port while leaving the rest untouched
						mem_packet.Seek(0,wxFromStart);
						mem_packet.WriteUInt32(host);
						mem_packet.WriteUInt16(port);
						CPacket* response = new CPacket(&mem_packet, OP_EMULEPROT, OP_REASKCALLBACKTCP);
						AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: send OP_REASKCALLBACKTCP") );
						theApp.statistics->AddUpDataOverheadFileRequest(response->GetPacketSize());
						buddy->GetSocket()->SendPacket(response);
					}
				}
				break;
			}
			case OP_REASKFILEPING: {
				AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: OP_REASKFILEPING") );
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				
				CMemFile data_in((byte*)packet, size);
				byte reqfilehash[16];
				data_in.ReadHash16(reqfilehash);
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
				if (!reqfile) {
					CPacket* response = new CPacket(OP_FILENOTFOUND,0,OP_EMULEPROT);
					theApp.statistics->AddUpDataOverheadFileRequest(response->GetPacketSize());
					SendPacket(response,host,port);
					break;
				}
				CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(host, port);
				if (sender){
					sender->CheckForAggressive();
					
					//Make sure we are still thinking about the same file
					if (md4cmp(reqfilehash, sender->GetUploadFileID()) == 0) {
					
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
						theApp.statistics->AddUpDataOverheadFileRequest(response->GetPacketSize());
						theApp.clientudp->SendPacket(response, host, port);
					} else {
						AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket; ReaskFilePing; reqfile does not match") );
					}						
				} else {
					if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs::GetQueueSize()) {
						CPacket* response = new CPacket(OP_QUEUEFULL,0,OP_EMULEPROT);
						theApp.statistics->AddUpDataOverheadFileRequest(response->GetPacketSize());
						SendPacket(response,host,port);
					}
				}
				break;
			}
			case OP_QUEUEFULL: {
				AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: OP_QUEUEFULL") );
				theApp.statistics->AddDownDataOverheadOther(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(host,port);
				if (sender) {
					sender->SetRemoteQueueFull(true);
					sender->UDPReaskACK(0);
				}
				break;
			}
			case OP_REASKACK: {				
				theApp.statistics->AddDownDataOverheadFileRequest(size);
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
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(host,port);
				if (sender){
					sender->UDPReaskFNF(); // may delete 'sender'!
					sender = NULL;
				}
				break;
			}
			default:
				theApp.statistics->AddDownDataOverheadOther(size);				
				return false;
		}
		return true;
	} catch(...) {
		AddDebugLogLineM( false, logClientUDP, wxT("Error while processing incoming UDP Packet (Most likely a misconfigured server)") );
	}
	return false;
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

		sent = !Error();
		
		if(!sent) {
			uint32 error = LastError();
			printf("UDP port returned a error: %i\n", error);
			switch (error) {
				case wxSOCKET_IOERR:
					// Seems like wxDatagramSocket raises this error
					// on some special events like invalid addresses or
					// unreachable hosts. We'll just discard this packet 
					// by pretending it was sent ok.
					printf("WARNING! Discarded packet because I/O error on UDP socket\n");
					sent = true;
					break;				
				case wxSOCKET_WOULDBLOCK:
					// Socket is busy and can't send this data right now,
					// so we just return not sent and set the wouldblock 
					// flag so it gets resent when socket is ready.
					m_bWouldBlock = true;
					break;
				default:
					// An unknown error happened on send. This packet
					// must better be deleted so we don't get into some
					// infinite loop...
					printf("WARNING! Discarded packet because unknown error on UDP socket\n");
					sent = true;
					break;
			}
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

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "Logger.h"

//
// CClientUDPSocket -- Extended eMule UDP socket
//

IMPLEMENT_DYNAMIC_CLASS(CClientUDPSocket, CDatagramSocketProxy)

CClientUDPSocket::CClientUDPSocket(amuleIPV4Address &address, const CProxyData *ProxyData)
:
CDatagramSocketProxy(address, wxSOCKET_NOWAIT, ProxyData)
#ifdef AMULE_DAEMON
 , wxThread(wxTHREAD_JOINABLE)
#endif
{
	m_bWouldBlock = false;

#ifdef AMULE_DAEMON
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("ERROR: CClientUDPSocket failed create\n");
		wxASSERT(0);
	}
	Run();
#else
	SetEventHandler(theApp, CLIENTUDPSOCKET_HANDLER);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
	Notify(true);
#endif
}

CClientUDPSocket::~CClientUDPSocket()
{
	SetNotify(0);
	Notify(FALSE);
}

#define CLIENT_UDP_BUFFER_SIZE 5000

void CClientUDPSocket::OnReceive(int WXUNUSED(nErrorCode))
{
	char buffer[CLIENT_UDP_BUFFER_SIZE];
	amuleIPV4Address addr;
	uint32 length = DoReceive(addr,buffer,CLIENT_UDP_BUFFER_SIZE);
	
	if (!thePrefs::IsUDPDisabled()) {
		if (buffer[0] == (char)OP_EMULEPROT && length != static_cast<uint32>(-1)) {
			ProcessPacket(buffer+2,length-2,buffer[1],StringIPtoUint32(addr.IPAddress()),addr.Service());
		}
	} else {
		Close();
	}
}

int CClientUDPSocket::DoReceive(amuleIPV4Address& addr, char* buffer, uint32 max_size) {
	RecvFrom(addr,buffer,max_size);
	int length = LastCount();
	#ifndef AMULE_DAEMON
	// Daemon doesn't need this because it's a thread, checking every X time.
	if (length <= 0 && (LastError() == wxSOCKET_WOULDBLOCK)) {
		// Evil trick to retry later.
		wxSocketEvent input_event(CLIENTUDPSOCKET_HANDLER);
		input_event.m_event = (wxSocketNotify)(wxSOCKET_INPUT);
		input_event.SetEventObject(this);
		theApp.AddPendingEvent(input_event);
	}
	#endif
	return length;
}

bool CClientUDPSocket::ProcessPacket(char* packet, int16 size, int8 opcode, uint32 host, uint16 port)
{
	try {
		switch(opcode) {
			case OP_REASKFILEPING: {
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				
				CSafeMemFile data_in((byte*)packet, size);
				byte reqfilehash[16];
				data_in.ReadHash16(reqfilehash);
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
				if (!reqfile) {
					CPacket* response = new CPacket(OP_FILENOTFOUND,0,OP_EMULEPROT);
					theApp.statistics->AddUpDataOverheadFileRequest(response->GetPacketSize());
					SendPacket(response,host,port);
					break;
				}
				CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP(host);
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
						
						CSafeMemFile data_out(128);
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
					CSafeMemFile data_in((byte*)packet,size);
					if ( sender->GetUDPVersion() > 3 ) {
						sender->ProcessFileStatus(true, &data_in, sender->GetRequestFile());
					}
					uint16 nRank = data_in.ReadUInt16();
					sender->SetRemoteQueueFull(false);
					sender->UDPReaskACK(nRank);
				}
				break;
			}
			case OP_FILENOTFOUND:
			{
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
	m_bWouldBlock = false;
	while (controlpacket_queue.GetHeadPosition() != 0 && !IsBusy()) {
		UDPPack* cur_packet = controlpacket_queue.GetHead();
		char* sendbuffer = new char[cur_packet->packet->GetPacketSize()+2];
		memcpy(sendbuffer, cur_packet->packet->GetUDPHeader(), 2);
		memcpy(sendbuffer+2, cur_packet->packet->GetDataBuffer(), cur_packet->packet->GetPacketSize());
		bool is_sent = SendTo(sendbuffer, cur_packet->packet->GetPacketSize()+2, cur_packet->dwIP, cur_packet->nPort);
		if ((is_sent) || (!is_sent && !IsBusy())) {
			// Either we sent the packet, or faced an error different from WOULDBLOCK,
			// like the other guy is not there anymore, so drop it.
			controlpacket_queue.RemoveHead();
			delete cur_packet->packet;
			delete cur_packet;
		}
		delete [] sendbuffer;
	}

}

bool CClientUDPSocket::SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort)
{

	amuleIPV4Address addr;
	addr.Hostname(dwIP);
	addr.Service(nPort);

	if(Ok()) {
		CDatagramSocketProxy::SendTo(addr,lpBuf,nBufLen);
	} else {
		// hmm. if there is no socket then what?
		return false;
	}

	if(Error()) {
		uint32 error = LastError();
		if (error == wxSOCKET_WOULDBLOCK) {
			m_bWouldBlock = true;
		}
		return false;
	}
	return true;
}


bool CClientUDPSocket::SendPacket(CPacket* packet, uint32 dwIP, uint16 nPort)
{
	// Send any previously queued packet before this one.
	OnSend(0);
	
	UDPPack* newpending = new UDPPack;
	newpending->dwIP = dwIP;
	newpending->nPort = nPort;
	newpending->packet = packet;
	newpending->trial = 0;
	if ( IsBusy() ) {
		controlpacket_queue.AddTail(newpending);
		return true;
	}
	char* sendbuffer = new char[packet->GetPacketSize()+2];
	memcpy(sendbuffer,packet->GetUDPHeader(),2);
	memcpy(sendbuffer+2,packet->GetDataBuffer(),packet->GetPacketSize());
	if (!SendTo(sendbuffer, packet->GetPacketSize()+2, dwIP, nPort)) {
		controlpacket_queue.AddTail(newpending);
	} else {
		delete newpending->packet;
		delete newpending;
	}
	delete[] sendbuffer;
	return true;
}


#ifdef AMULE_DAEMON

void *CClientUDPSocket::Entry()
{
	while ( !TestDestroy() ) {
		if ( WaitForRead(1, 0) ) {
			OnReceive(0);
		}
	}
	return 0;
}

#endif

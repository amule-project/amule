//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

// ClientUDPSocket.cpp : implementation file
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ClientUDPSocket.h"
#endif

#include "types.h"

#include <cerrno>
#ifdef __WXMSW__
	#include <winsock.h>
#else
#ifdef __BSD__
       #include <sys/types.h>
#endif /* __BSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/socket.h>

#include "ClientUDPSocket.h"	// Interface declarations
#include "amuleIPV4Address.h"	// Needed for amuleIPV4Address
#include "Preferences.h"	// Needed for CPreferences
#include "PartFile.h"		// Needed for CPartFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "KnownFile.h"		// Needed for CKnownFile
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "opcodes.h"		// Needed for OP_EMULEPROT
#include "Statistics.h"		// Needed for CStatistics
#include "amule.h"			// Needed for theApp
#include "otherfunctions.h"
#include "SafeFile.h"

//
// CClientUDPSocket -- Extended eMule UDP socket
//

IMPLEMENT_DYNAMIC_CLASS(CClientUDPSocket,wxDatagramSocketProxy)

CClientUDPSocket::CClientUDPSocket(amuleIPV4Address &address, const wxProxyData *ProxyData)
:
wxDatagramSocketProxy(address, wxSOCKET_NOWAIT, ProxyData)
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
	if (thePrefs::IsUDPDisabled()) {
		ReceiveAndDiscard();
	}
	char buffer[CLIENT_UDP_BUFFER_SIZE];
	amuleIPV4Address addr;
	RecvFrom(addr,buffer,CLIENT_UDP_BUFFER_SIZE);
	uint32 length = LastCount();

	if (buffer[0] == (char)OP_EMULEPROT && length != static_cast<uint32>(-1)) {
		ProcessPacket(buffer+2,length-2,buffer[1],StringIPtoUint32(addr.IPAddress()),addr.Service());
	}
}

void CClientUDPSocket::ReceiveAndDiscard() {
	
	char buffer[CLIENT_UDP_BUFFER_SIZE];
	amuleIPV4Address addr;
	RecvFrom(addr,buffer,CLIENT_UDP_BUFFER_SIZE);	
	// And then discard.

	if (thePrefs::IsUDPDisabled()) {
		Close();
	}
}

bool CClientUDPSocket::ProcessPacket(char* packet, int16 size, int8 opcode, uint32 host, uint16 port)
{
	try {
		switch(opcode) {
			case OP_REASKFILEPING: {
				theApp.statistics->AddDownDataOverheadFileRequest(size);
				if (size != 16) {
					break;
				}
				
				CSafeMemFile data_in((BYTE*)packet, size);
				uchar reqfilehash[16];
				data_in.ReadHash16(reqfilehash);
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
				if (!reqfile) {
					Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
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
						Packet* response = new Packet(&data_out, OP_EMULEPROT);
						response->SetOpCode(OP_REASKACK);
						theApp.statistics->AddUpDataOverheadFileRequest(response->GetPacketSize());
						theApp.clientudp->SendPacket(response, host, port);
					} else {					
						AddDebugLogLineM(false, wxT("Client UDP socket; ReaskFilePing; reqfile does not match"));
					}						
				} else {
					if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs::GetQueueSize()) {
						Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
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
					CSafeMemFile data_in((BYTE*)packet,size);
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
	}
	catch(...) {
		AddDebugLogLineM(false, wxT("Error while processing incoming UDP Packet (Most likely a misconfigured server)"));
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
		wxDatagramSocketProxy::SendTo(addr,lpBuf,nBufLen);
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


bool CClientUDPSocket::SendPacket(Packet* packet, uint32 dwIP, uint16 nPort)
{
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

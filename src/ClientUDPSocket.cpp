// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

// ClientUDPSocket.cpp : implementation file
//
#include "types.h"

#include <cerrno>
#ifdef __WXMSW__
	#include <winsock.h>
#else
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
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp

// CClientUDPSocket
#define ID_SOKETTI 7772

IMPLEMENT_DYNAMIC_CLASS(CClientUDPSocket,wxDatagramSocket)

CClientUDPSocket::CClientUDPSocket(wxIPV4address address)
: wxDatagramSocket(address,wxSOCKET_NOWAIT)
{
	m_bWouldBlock = false;

	printf("*** UDP socket at %d\n",address.Service());
	SetEventHandler(*theApp.amuledlg,ID_SOKETTI);
	SetNotify(wxSOCKET_INPUT_FLAG|wxSOCKET_OUTPUT_FLAG);
	Notify(TRUE);
}

CClientUDPSocket::~CClientUDPSocket()
{
	SetNotify(0);
	Notify(FALSE);
}

void CClientUDPSocket::OnReceive(int nErrorCode)
{
	char buffer[5000];
	wxString serverbuffer;
	wxIPV4address addr;
	RecvFrom(addr,buffer,5000);
	wxUint32 length = LastCount();

	// strip IP address from wxSockAddress (do not call Hostname(). we do not want DNS)
	struct in_addr addr_in;
	#ifdef __WXMSW__
	addr_in.s_addr = inet_addr(addr.IPAddress().c_str());
	#else
	addr_in.s_addr=GAddress_INET_GetHostAddress(addr.GetAddress());
	#endif
	char* fromIP=inet_ntoa(addr_in);

	//wxUint32 length = ReceiveFrom(buffer,5000,serverbuffer,port);
	if (buffer[0] == (char)OP_EMULEPROT && length != static_cast<wxUint32>(-1)) {
		ProcessPacket(buffer+2,length-2,buffer[1],/*serverbuffer.GetBuffer()*/fromIP,addr.Service());
	}
}


bool CClientUDPSocket::ProcessPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port)
{
	try {
		switch(opcode) {
			case OP_REASKFILEPING: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size != 16) {
					break;
				}
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
				if (!reqfile) {
					Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
					theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size);
					SendPacket(response,inet_addr(host),port);
					break;
				}
				CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP(inet_addr(host));
				if (sender){					
					sender->AddAskedCount();
					sender->SetUDPPort(port);
					sender->UDPFileReasked();

					/*
					printf("sender->GetUDPVersion = %i\n",sender->GetUDPVersion());
					printf("size = %i\n",size);
					*/

					if ((sender->GetUDPVersion() > 2) && (size > 17)) {
						uint16 nCompleteCountLast= sender->GetUpCompleteSourcesCount();
						uint16 nCompleteCountNew= *(uint16*)(packet+16);
						sender->SetUpCompleteSourcesCount(nCompleteCountNew);
						if (nCompleteCountLast != nCompleteCountNew) {
							if(reqfile->IsPartFile()) {
								((CPartFile*)reqfile)->NewSrcPartsInfo();
							} else {
								reqfile->NewAvailPartsInfo();
							}
						}
					}
					break;
				} else {
					if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > theApp.glob_prefs->GetQueueSize()) {
						Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size);
						SendPacket(response,inet_addr(host),port);
					}
				}
				break;
			}
			case OP_QUEUEFULL: {
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP(inet_addr(host));
				if (sender) {
					sender->SetRemoteQueueFull(true);
					sender->UDPReaskACK(0);
				}
				break;
			}
			case OP_REASKACK: {
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP(inet_addr(host));
				if (sender) {
					if (size != 2) {
						break;
					}
					uint16 nRank;
					memcpy(&nRank,packet,2);
					sender->SetRemoteQueueFull(false);
					sender->UDPReaskACK(nRank);
					sender->AddAskedCountDown();
				}
				break;
			}
			case OP_FILENOTFOUND:
			{
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP(inet_addr(host));
				if (sender){
					sender->UDPReaskFNF();
				}
				/* else {
					ASSERT (false); //Need to double check these asserts to make sure it's running well.
				}*/
				break;
			}
			default:
				return false;
		}
		return true;
	}
	catch(...) {
		theApp.amuledlg->AddDebugLogLine(false,CString(_("Error while processing incoming UDP Packet (Most likely a misconfigured server)")));
	}
	return false;
}

/*
void CClientUDPSocket::OnSend(int nErrorCode)
{
	if (nErrorCode) {
		return;
	}

	m_bWouldBlock = false;
	CTypedPtrList<CPtrList, UDPPack*> failed_packets;

	while (controlpacket_queue.GetHeadPosition() != 0 && !IsBusy()) {
		UDPPack* cur_packet = controlpacket_queue.RemoveHead();
		char* sendbuffer = new char[cur_packet->packet->size+2];
		memcpy(sendbuffer,cur_packet->packet->GetUDPHeader(),2);
		memcpy(sendbuffer+2,cur_packet->packet->pBuffer,cur_packet->packet->size);
		if (SendTo(sendbuffer, cur_packet->packet->size+2, cur_packet->dwIP, cur_packet->nPort) ) {
			delete cur_packet->packet;
			delete cur_packet;
		} else if ( ++cur_packet->trial < 3 ) {
			failed_packets.AddTail(cur_packet);
		} else {
			delete cur_packet->packet;
			delete cur_packet;
		}
		delete[] sendbuffer;
	}

	for (POSITION pos = failed_packets.GetHeadPosition(); pos; ) {
		UDPPack* packet = failed_packets.GetNext(pos);
		controlpacket_queue.AddTail(packet);
	}
	failed_packets.RemoveAll();
}
*/

void CClientUDPSocket::OnSend(int nErrorCode)
{
	if (nErrorCode) {
		return;
	}
	m_bWouldBlock = false;
	while (controlpacket_queue.GetHeadPosition() != 0 && !IsBusy()) {
		UDPPack* cur_packet = controlpacket_queue.GetHead();
		char* sendbuffer = new char[cur_packet->packet->size+2];
		memcpy(sendbuffer,cur_packet->packet->GetUDPHeader(),2);
		memcpy(sendbuffer+2,cur_packet->packet->pBuffer,cur_packet->packet->size);
		if (!SendTo(sendbuffer, cur_packet->packet->size+2, cur_packet->dwIP, cur_packet->nPort)) {
			controlpacket_queue.RemoveHead();
			delete cur_packet->packet;
			delete cur_packet;
		}
		delete[] sendbuffer;
	}

}

bool CClientUDPSocket::SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort)
{
	in_addr host;
	//host.S_un.S_addr = dwIP;
	host.s_addr=dwIP;

	amuleIPV4Address addr;
	struct in_addr tmpa;
	tmpa.s_addr=dwIP;//m_SaveAddr.sin_addr.s_addr;
	addr.Hostname(tmpa.s_addr);
	addr.Service(nPort);

	//uint32 result = CAsyncSocket::SendTo(lpBuf,nBufLen,nPort,inet_ntoa(host));
	if(Ok()) {
		wxDatagramSocket::SendTo(addr,lpBuf,nBufLen);
	} else {
		// hmm. if there is no socket then what?
		return false;
	}

	//if (result == (uint32)SOCKET_ERROR){
	if(Error()) {
		uint32 error = LastError();//GetLastError();
		if (error == wxSOCKET_WOULDBLOCK) {
			m_bWouldBlock = true;
			return false;
		} else {
			return false;
		}
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
	char* sendbuffer = new char[packet->size+2];
	memcpy(sendbuffer,packet->GetUDPHeader(),2);
	memcpy(sendbuffer+2,packet->pBuffer,packet->size);
	if (!SendTo(sendbuffer, packet->size+2, dwIP, nPort)) {
		controlpacket_queue.AddTail(newpending);
	} else {
		delete newpending->packet;
		delete newpending;
	}
	delete[] sendbuffer;
	return true;
}

bool  CClientUDPSocket::Create()
{
	// constructor does dirty work
	return true;

	/*
	if (theApp.glob_prefs->GetUDPPort()) {
		return CAsyncSocket::Create(theApp.glob_prefs->GetUDPPort(),SOCK_DGRAM,FD_READ);
	} else {
		return true;
	}*/
}

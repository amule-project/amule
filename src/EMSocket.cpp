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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "EMSocket.h"
#endif

#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>

#include "EMSocket.h"		// Interface declarations.
#include "packets.h"		// Needed for Packet
#include "GetTickCount.h"

#define MAX_SIZE 2000000

//#define __PACKET_SEND_DUMP__

IMPLEMENT_DYNAMIC_CLASS(CEMSocket,wxSocketClient)

CEMSocket::CEMSocket(const wxProxyData *ProxyData)
  : wxSocketClientProxy(wxSOCKET_NOWAIT/*wxSOCKET_BLOCK*/, ProxyData)
{
	#ifdef __DEBUG__
	from_destroy =  false;
	created = GetTickCount();
	#endif		
	
	byConnected = ES_NOTCONNECTED;
	
	limitenabled = false;
	downloadlimit = 0;
	pendingOnReceive = false;

	// Download partial header
	pendingHeaderSize = 0;

	// Download partial packet
	pendingPacket = NULL;
	pendingPacketSize = 0;	
	
	sendbuffer = 0;
	sendblen = 0;
	sent = 0;

	m_bLinkedPackets = false;
	DoingDestroy = false;
	
}

CEMSocket::~CEMSocket(){
	#ifdef __DEBUG__
	wxASSERT(from_destroy);
	#endif	    
	byConnected = ES_DISCONNECTED;

	SetNotify(0);
	Notify(FALSE);
	ClearQueues();
	OnClose(0);
	
}

void CEMSocket::Destroy() {
	#ifdef __DEBUG__
	from_destroy =  true;
	#endif
	if (!DoingDestroy) {		
		DoingDestroy = true;
#ifdef AMULE_DAEMON
		delete this;
#else
		wxSocketClient::Destroy();
#endif
	}	
}

void CEMSocket::ClearQueues(){
	for (POSITION pos = controlpacket_queue.GetHeadPosition();pos != 0;) {
		delete controlpacket_queue.GetNext(pos);
	}
	controlpacket_queue.RemoveAll();
	for (POSITION pos = standartpacket_queue.GetHeadPosition();pos != 0;) {
		delete standartpacket_queue.GetNext(pos);
	}
	standartpacket_queue.RemoveAll();
	
	limitenabled = false;	
	downloadlimit=0;
	pendingOnReceive = false;

	// Download partial header
	pendingHeaderSize = 0;

	// Download partial packet
	if(pendingPacket != NULL){
		delete pendingPacket;
		pendingPacket = NULL;
		pendingPacketSize = 0;
	}

	if (sendbuffer!=NULL) {
		delete[] sendbuffer;
		sendbuffer = NULL;
	}
	
	sendblen = 0;
	sent = 0;
	m_bLinkedPackets = false;
}

void CEMSocket::OnClose(int WXUNUSED(nErrorCode)){
	
	byConnected = ES_DISCONNECTED;
	
	ClearQueues();
};

void CEMSocket::OnReceive(int nErrorCode){
	// the 2 meg size was taken from another place
	static char GlobalReadBuffer[MAX_SIZE];

	if(nErrorCode) {
		uint32 error = LastError(); 
		if (error != wxSOCKET_WOULDBLOCK) {
			OnError(nErrorCode);
			return;
		}
	}
	
	// Check current connection state
	if(byConnected == ES_DISCONNECTED){
		return;
	}
	else {	
		byConnected = ES_CONNECTED; // ES_DISCONNECTED, ES_NOTCONNECTED, ES_CONNECTED
	}

	// CPU load improvement
	if(limitenabled == true && downloadlimit == 0){
		pendingOnReceive = true;
		return;
	}

	// Remark: an overflow can not occur here
	uint32 readMax = sizeof(GlobalReadBuffer) - pendingHeaderSize; 
	if((limitenabled == true) && (readMax > downloadlimit)) {
		readMax = downloadlimit;
	}

	// We attempt to read up to 2 megs at a time (minus whatever is in our internal read buffer)
	Read(GlobalReadBuffer + pendingHeaderSize, readMax);
	uint32 ret=LastCount();
	if (Error() || ret == 0) {
		return;
	}
	
	
	// Bandwidth control
	if(limitenabled == true){
		// Update limit
		downloadlimit -= ret;
	}

	// CPU load improvement
	// Detect if the socket's buffer is empty (or the size did match...)
	pendingOnReceive = (ret == readMax) ? true : false;

	// Copy back the partial header into the global read buffer for processing
	if(pendingHeaderSize > 0) {
  		memcpy(GlobalReadBuffer, pendingHeader, pendingHeaderSize);
		ret += pendingHeaderSize;
		pendingHeaderSize = 0;
	}

	char *rptr = GlobalReadBuffer; // floating index initialized with begin of buffer
	const char *rend = GlobalReadBuffer + ret; // end of buffer

	// Loop, processing packets until we run out of them
	while((rend - rptr >= PACKET_HEADER_SIZE) ||
	      ((pendingPacket != NULL) && (rend - rptr > 0 ))){ 

		// Two possibilities here: 
		//
		// 1. There is no pending incoming packet
		// 2. There is already a partial pending incoming packet
		//
		// It's important to remember that emule exchange two kinds of packet
		// - The control packet
		// - The data packet for the transport of the block
		// 
		// The biggest part of the traffic is done with the data packets. 
		// The default size of one block is 10240 bytes (or less if compressed), but the
		// maximal size for one packet on the network is 1300 bytes. It's the reason
		// why most of the Blocks are splitted before to be sent. 
		//
		// Conclusion: When the download limit is disabled, this method can be at least 
		// called 8 times (10240/1300) by the lower layer before a splitted packet is 
		// rebuild and transfered to the above layer for processing.
		//
		// The purpose of this algorithm is to limit the amount of data exchanged between buffers

		if(pendingPacket == NULL){
			pendingPacket = new Packet(rptr); // Create new packet container. 
			rptr += 6;                        // Only the header is initialized so far

			// Bugfix We still need to check for a valid protocol
			// Remark: the default eMule v0.26b had removed this test......
			switch (pendingPacket->GetProtocol()){
				case OP_EDONKEYPROT:
				case OP_PACKEDPROT:
				case OP_EMULEPROT:
					break;
				default:
					delete pendingPacket;
					pendingPacket = NULL;
					OnError(ERR_WRONGHEADER);
					return;
			}

			// Security: Check for buffer overflow (2MB)
			if(pendingPacket->GetPacketSize() > sizeof(GlobalReadBuffer)) {
				delete pendingPacket;
				pendingPacket = NULL;
				OnError(ERR_TOOBIG);
				return;
			}

			// Init data buffer
			pendingPacket->AllocDataBuffer();
			pendingPacketSize = 0;
		}

		// Bytes ready to be copied into packet's internal buffer
		wxASSERT(rptr <= rend);
		uint32 toCopy = ((pendingPacket->GetPacketSize() - pendingPacketSize) < (uint32)(rend - rptr)) ? 
					(pendingPacket->GetPacketSize() - pendingPacketSize) : (uint32)(rend - rptr);

		// Copy Bytes from Global buffer to packet's internal buffer
		pendingPacket->CopyToDataBuffer(pendingPacketSize, rptr, toCopy);
		pendingPacketSize += toCopy;
		rptr += toCopy;
		
		// Check if packet is complet
		wxASSERT(pendingPacket->GetPacketSize() >= pendingPacketSize);
		if(pendingPacket->GetPacketSize() == pendingPacketSize) {
			// Process packet
			bool bPacketResult = PacketReceived(pendingPacket);
			delete pendingPacket;	
			pendingPacket = NULL;
			pendingPacketSize = 0;
			
			if (!bPacketResult)
				return;			
		}
	}

	// Finally, if there is any data left over, save it for next time
	wxASSERT(rptr <= rend);
	wxASSERT(rend - rptr < PACKET_HEADER_SIZE);
	if(rptr != rend) {
		// Keep the partial head
		pendingHeaderSize = rend - rptr;
		memcpy(pendingHeader, rptr, pendingHeaderSize);
	}	
}

void CEMSocket::SetDownloadLimit(uint32 limit){
	limitenabled = true;
	downloadlimit = limit;
	// Avoiding CPU load
	if(limit > 0 && pendingOnReceive == true){
		OnReceive(0);
	}
}

void CEMSocket::DisableDownloadLimit(){
	limitenabled = false;
	if(pendingOnReceive == true){
		OnReceive(0);
	}
}

bool CEMSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket) {
	if (!delpacket){
		Packet* copy = new Packet(*packet);
		packet = copy;
	}			
	if ( ( (!IsConnected()) || IsBusy() ) || ( m_bLinkedPackets && controlpacket ) ){
		if (controlpacket){
		  controlpacket_queue.AddTail(packet); 
			return true;
		}
		else{
		  standartpacket_queue.AddTail(packet); 
			return true;
		}
	}
	bool bCheckControlQueue = false;
	if (packet->IsLastSplitted() ) {
		m_bLinkedPackets = false;
		bCheckControlQueue = true;
	} else if (packet->IsSplitted()) {
		m_bLinkedPackets = true;
	}

	Send(packet->DetachPacket(),packet->GetRealPacketSize());

	delete packet;
	if (!IsBusy() && bCheckControlQueue)
		OnSend(0);
	return true;
}

void CEMSocket::OnSend(int nErrorCode){
  if (nErrorCode) {
    OnError(nErrorCode);
    return;
  }
  if (byConnected == ES_DISCONNECTED) {
    return;
  }
  else {
    byConnected = ES_CONNECTED;
  }
  
  if (IsBusy()) {
    Send(0,0,0);
  }
  // Still busy?
  if (IsBusy()) {
    return;
  }

	while (controlpacket_queue.GetHeadPosition() != 0 && (!IsBusy()) && IsConnected() && !m_bLinkedPackets) {
		Packet* cur_packet = controlpacket_queue.GetHead();
		Send(cur_packet->DetachPacket(),cur_packet->GetRealPacketSize());
		controlpacket_queue.RemoveHead();
		delete cur_packet;
	}

	while (standartpacket_queue.GetHeadPosition() != 0 && (!IsBusy()) && IsConnected()) {
		Packet* cur_packet = standartpacket_queue.GetHead();
		if (cur_packet->IsLastSplitted() ) {
			m_bLinkedPackets = false;
		} else if (cur_packet->IsSplitted()) {
			m_bLinkedPackets = true;
		}
		Send(cur_packet->DetachPacket(),cur_packet->GetRealPacketSize());
		standartpacket_queue.RemoveHead();
		delete cur_packet;
	}

	while (controlpacket_queue.GetHeadPosition() != 0 && (!IsBusy()) && IsConnected() && !m_bLinkedPackets) {
		Packet* cur_packet = controlpacket_queue.GetHead();
		Send(cur_packet->DetachPacket(),cur_packet->GetRealPacketSize());
		controlpacket_queue.RemoveHead();
		delete cur_packet;
	} 
}

int CEMSocket::Send(char* lpBuf,int nBufLen,int WXUNUSED(nFlags))
{
	#ifdef __PACKET_SEND_DUMP__
	printf("Send\n");
	DumpMem(lpBuf,nBufLen);
	#endif
	assert (sendbuffer == NULL || lpBuf == NULL );
	if (lpBuf) {
		sendbuffer = lpBuf;
		sendblen = nBufLen;
		sent = 0;
	}
	while (true) {
		uint32 tosend = sendblen-sent;
		if (tosend > MAXFRAGSIZE) {
			tosend = MAXFRAGSIZE;
		}
		assert (tosend != 0);
		#ifndef WIN32
		signal(SIGPIPE,SIG_IGN);
		#endif
		wxSocketBase::Write(sendbuffer+sent,tosend);
		uint32 result=LastCount();
		if(Error()) {
			uint32 error = LastError(); 
			if (error == wxSOCKET_WOULDBLOCK) {
				break;
			} else {
				OnError(error);
				return -1;
			}
		}
		sent += result;
		assert (sent <= sendblen);
		if (sent == sendblen) {
			delete[] sendbuffer;
			sendbuffer = 0;
			sent = 0;
			sendblen = 0;
			break;
		}

	}
	return 0;
}

//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>

#include "EMSocket.h"		// Interface declarations.
#include "packets.h"		// Needed for Packet

#define MAX_SIZE 2000000

IMPLEMENT_DYNAMIC_CLASS(CEMSocket,wxSocketClient)

#if 0
namespace {
	inline void EMTrace(char* fmt, ...) {
#ifdef _DEBUG
		va_list argptr;
		char bufferline[512];
		va_start(argptr, fmt);
		_vsnprintf(bufferline, 512, fmt, argptr);
		va_end(argptr);
		//(Ornis+)
		char osDate[30],osTime[30]; 
		char temp[1024]; 
		_strtime( osTime );
		_strdate( osDate );
		int len = _snprintf(temp,1021,"%s %s: %s",osDate,osTime,bufferline);
		temp[len++] = 0x0d;
		temp[len++] = 0x0a;
		temp[len+1] = 0;
		HANDLE hFile = CreateFile("c:\\EMSocket.log",           // open MYFILE.TXT 
                GENERIC_WRITE,              // open for reading 
                FILE_SHARE_READ,           // share for reading 
                NULL,                      // no security 
                OPEN_ALWAYS,               // existing file only 
                FILE_ATTRIBUTE_NORMAL,     // normal file 
                NULL);                     // no attr. template 
  
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			DWORD nbBytesWritten = 0;
			SetFilePointer(hFile, 0, NULL, FILE_END);
			bool b = WriteFile(
				hFile,                    // handle to file
				temp,                // data buffer
				len,     // number of bytes to write
				&nbBytesWritten,  // number of bytes written
				NULL        // overlapped buffer
			);
			CloseHandle(hFile);
		}
#else
		va_list argptr;
		va_start(argptr, fmt);
		va_end(argptr);
#endif
	}
}
#endif

CEMSocket::CEMSocket(void)
  : wxSocketClient(wxSOCKET_NOWAIT/*wxSOCKET_BLOCK*/)
{
	byConnected = ES_NOTCONNECTED;
	
	limitenabled = false;
	downloadlimit = 0;
	pendingOnReceive = false;

	// Download partial header
	// memset(pendingHeader, 0, sizeof(pendingHeader));
	pendingHeaderSize = 0;

	// Download partial packet
	pendingPacket = NULL;
	pendingPacketSize = 0;	
	
	sendbuffer = 0;
	sendblen = 0;
	sent = 0;
	m_bLinkedPackets = false;
}

CEMSocket::~CEMSocket(){
   //EMTrace("CEMSocket::~CEMSocket() on %d",(SOCKET)this);
  //printf("CEMSocket::~CEMSocket() on %d\n",this);
  SetNotify(0);
  Notify(FALSE);
  ClearQueues();
  OnClose(0);
  //AsyncSelect(0);
}


void CEMSocket::ClearQueues(){
  //EMTrace("CEMSocket::ClearQueues on %d",(SOCKET)this);
	for (POSITION pos = controlpacket_queue.GetHeadPosition();pos != 0;controlpacket_queue.GetNext(pos))
		delete controlpacket_queue.GetAt(pos);
	controlpacket_queue.RemoveAll();
	for (POSITION pos = standartpacket_queue.GetHeadPosition();pos != 0;standartpacket_queue.GetNext(pos))
		delete standartpacket_queue.GetAt(pos);
	standartpacket_queue.RemoveAll();
	
	limitenabled = false;	
	downloadlimit=0;
	pendingOnReceive = false;

	// Download partial header
	// memset(pendingHeader, 0, sizeof(pendingHeader));
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
	
	//sendbuffer = 0;
	sendblen = 0;
	sent = 0;
	m_bLinkedPackets = false;
}

void CEMSocket::OnClose(int nErrorCode){
	byConnected = ES_DISCONNECTED;
	//CAsyncSocket::OnClose(nErrorCode);
	ClearQueues();
};

void CEMSocket::OnReceive(int nErrorCode){
	// the 2 meg size was taken from another place
	static char GlobalReadBuffer[MAX_SIZE];

	// Check for an error code
	if(nErrorCode != 0){
		OnError(nErrorCode);
		return;
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
			switch (pendingPacket->prot){
				case OP_EDONKEYPROT:
				case OP_PACKEDPROT:
				case OP_EMULEPROT:
					break;
				default:
					//("CEMSocket::OnReceive ERROR Wrong header");
					delete pendingPacket;
					pendingPacket = NULL;
					OnError(ERR_WRONGHEADER);
					return;
			}

			// Security: Check for buffer overflow (2MB)
			if(pendingPacket->size > sizeof(GlobalReadBuffer)) {
				delete pendingPacket;
				pendingPacket = NULL;
				OnError(ERR_TOOBIG);
				return;
			}

			// Init data buffer
			pendingPacket->pBuffer = new char[pendingPacket->size + 1];
			pendingPacketSize = 0;
		}

		// Bytes ready to be copied into packet's internal buffer
		wxASSERT(rptr <= rend);
		uint32 toCopy = ((pendingPacket->size - pendingPacketSize) < (uint32)(rend - rptr)) ? 
			             (pendingPacket->size - pendingPacketSize) : (uint32)(rend - rptr);

		// Copy Bytes from Global buffer to packet's internal buffer
		memcpy(&pendingPacket->pBuffer[pendingPacketSize], rptr, toCopy);
		pendingPacketSize += toCopy;
		rptr += toCopy;
		
		// Check if packet is complet
		wxASSERT(pendingPacket->size >= pendingPacketSize);
		if(pendingPacket->size == pendingPacketSize){

			// Process packet
			PacketReceived(pendingPacket);
			delete pendingPacket;	
			pendingPacket = NULL;
			pendingPacketSize = 0;
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


/*
void CEMSocket::OnReceive(int nErrorCode)
{
	// the 2 meg size was taken from another place
	static char GlobalReadBuffer[MAX_SIZE];

	if (nErrorCode){
		OnError(nErrorCode);
		return;
	}
	if (byConnected == ES_DISCONNECTED) {
		return;
	} else {
		byConnected = ES_CONNECTED;
	}

	uint32 readMax = sizeof(GlobalReadBuffer) - readbuf_size;

	// Buffer overflow
	if (readMax == 0) {
		delete readbuf;
		readbuf = NULL;
		readbuf_size = 0;
		OnError(ERR_TOOBIG);
		return;
	}

	if (limitenabled && readMax > downloadlimit) {
		readMax = downloadlimit;
	}

	// We attempt to read up to 2 megs at a time (minus whatever is in our internal read buffer)
	Read(GlobalReadBuffer+readbuf_size,readMax);
	uint32 ret=LastCount();

	if (Error() || ret == 0) {
		return;
	}

	if (limitenabled) {
		downloadlimit -= ret;
	}

	// Copy over our temporary read buffer into the global read buffer for processing
	if (readbuf) {
  		memcpy(GlobalReadBuffer, readbuf, readbuf_size);
		ret += readbuf_size;
		delete[] readbuf;
		readbuf = NULL;
		readbuf_size = 0;
	}

	char *rptr = GlobalReadBuffer;
	char *rend = GlobalReadBuffer + ret;

	// Loop, processing packets until we run out of them
	while (rend - rptr >= 6) {
		Packet *packet = new Packet(rptr);

		rptr += 6;

		if (packet->size > sizeof(GlobalReadBuffer)) {
			delete packet;
			OnError(ERR_TOOBIG);
			return;
		}

		if (packet->size > rend - rptr) {
			rptr -= 6;
			delete packet;
			break;
		}


		char *packetBuffer = new char[packet->size + 1];
		memcpy(packetBuffer, rptr, packet->size);

		rptr += packet->size;
		packet->pBuffer = packetBuffer;
		PacketReceived(packet);
		delete packet;
	}

	// Finally, if there is any data left over, save it for next time
	//ASSERT(rptr <= rend);
	if (rptr != rend) {
		readbuf_size = rend - rptr;
		readbuf = new char[readbuf_size];
		memcpy(readbuf, rptr, readbuf_size);
	}	
}

*/

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

bool CEMSocket::SendPacket(Packet* packet, bool delpacket,bool controlpacket){
	if (!delpacket){
		Packet* copy = new Packet(packet->opcode,packet->size);
		memcpy(copy->pBuffer,packet->pBuffer,packet->size);
		packet = copy;
	}			
	if ( ( (!IsConnected()) || IsBusy() ) || ( m_bLinkedPackets && controlpacket ) ){
		if (controlpacket){
		  controlpacket_queue.AddTail(packet); //AddTail(packet);
			return true;
		}
		else{
		  standartpacket_queue.AddTail(packet); //AddTail(packet);
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

	/* This assert seem to make some troubles ... BigBob
	} else if (m_bLinkedPackets) {
		assert (false);
	}
	*/
	//printf("Sending packet (%d,opcode=%x)\n",packet->GetRealPacketSize(),packet->opcode);
	//assert(packet->GetRealPacketSize()!=1300);

	Send(packet->DetachPacket(),packet->GetRealPacketSize());

	delete packet;
	if (!IsBusy() && bCheckControlQueue)
		OnSend(0);
	return true;
}

void CEMSocket::OnSend(int nErrorCode){
  if (nErrorCode) { // || Error()){
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
//		EMTrace("CEMSocket::OnSend sending control packet on %d, size=%u",(SOCKET)this, cur_packet->GetRealPacketSize());
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
		/* This assert seem to make some troubles ... BigBob
		} else if (m_bLinkedPackets) {
			assert (false);
		}
		*/
//		EMTrace("CEMSocket::OnSend sending standart packet on %d, size=%u",(SOCKET)this, cur_packet->GetRealPacketSize());
		Send(cur_packet->DetachPacket(),cur_packet->GetRealPacketSize());
		standartpacket_queue.RemoveHead();
		delete cur_packet;
	}

	while (controlpacket_queue.GetHeadPosition() != 0 && (!IsBusy()) && IsConnected() && !m_bLinkedPackets) {
		Packet* cur_packet = controlpacket_queue.GetHead();
//		EMTrace("CEMSocket::OnSend sending control packet on %d, size=%u",(SOCKET)this, cur_packet->GetRealPacketSize());
		Send(cur_packet->DetachPacket(),cur_packet->GetRealPacketSize());
		controlpacket_queue.RemoveHead();
		delete cur_packet;
	} 
}

int CEMSocket::Send(char* lpBuf,int nBufLen,int nFlags)
{
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
		//if (result == (uint32)-1) {
		if(Error()) {
			uint32 error = LastError(); //GetLastError();
			if (error == wxSOCKET_WOULDBLOCK) {
				break;
			} else {
				//OnError(error);
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

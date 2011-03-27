//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
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

#include "ECSocket.h"

#include <sstream>
#include <iostream>
#include <algorithm>

using namespace std;

#include "ECPacket.h"		// Needed for CECPacket
#include "../../../Logger.h"
#include <common/Format.h>	// Needed for CFormat

#define EC_COMPRESSION_LEVEL	Z_DEFAULT_COMPRESSION
#define EC_MAX_UNCOMPRESSED	1024

#ifndef __GNUC__
#define __attribute__(x)
#endif

// If your compiler gives errors on these lines, just remove them.
int utf8_mbtowc(wchar_t *p, const unsigned char *s, int n) __attribute__((__visibility__("internal")));
int utf8_wctomb(unsigned char *s, wchar_t wc, int maxlen) __attribute__((__visibility__("internal")));
int utf8_mb_remain(char c) __attribute__((__pure__));

/*----------=> Import from the Linux kernel <=----------*/
/*
 * linux/fs/nls_base.c
 */

/*
 * Sample implementation from Unicode home page.
 * http://www.stonehand.com/unicode/standard/fss-utf.html
 */
struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	uint32_t  lmask;
	uint32_t  lval;
};

static const struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
    {0,     0,      0,      0,              0,         /* end of table    */}
};

int utf8_mbtowc(uint32_t *p, const unsigned char *s, int n)
{
	uint32_t l;
	int c0, c, nc;
	const struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval)
				return -1;
			*p = l;
			return nc;
		}
		if (n <= nc)
			return -1;
		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
			return -1;
		l = (l << 6) | c;
	}
	return -1;
}

int utf8_wctomb(unsigned char *s, uint32_t wc, int maxlen)
{
	uint32_t l;
	int c, nc;
	const struct utf8_table *t;

	l = wc;
	nc = 0;
	for (t = utf8_table; t->cmask && maxlen; t++, maxlen--) {
		nc++;
		if (l <= t->lmask) {
			c = t->shift;
			*s = t->cval | (l >> c);
			while (c > 0) {
				c -= 6;
				s++;
				*s = 0x80 | ((l >> c) & 0x3F);
			}
			return nc;
		}
	}
	return -1;
}
/*----------=> End of Import <=----------*/

int utf8_mb_remain(char c)
{
	int i;
	for (i = 0; i < 5; ++i) {
		if ((c & utf8_table[i].cmask) == utf8_table[i].cval) break;
	}
	return i;
}


void CQueuedData::Write(const void *data, size_t len)
{
	const size_t canWrite = std::min(GetRemLength(), len);
	wxASSERT(len == canWrite);

	memcpy(m_wr_ptr, data, canWrite);
	m_wr_ptr += canWrite;
}


void CQueuedData::WriteAt(const void *data, size_t len, size_t offset)
{
	wxASSERT(len + offset <= m_data.size());
	if (offset > m_data.size()) {
		return;
	} else if (offset + len > m_data.size()) {
		len = m_data.size() - offset;
	}

	memcpy(&m_data[0] + offset, data, len);
}


void CQueuedData::Read(void *data, size_t len)
{
	const size_t canRead = std::min(GetUnreadDataLength(), len);
	wxASSERT(len == canRead);

	memcpy(data, m_rd_ptr, canRead);
	m_rd_ptr += canRead;
}


void CQueuedData::WriteToSocket(CECSocket *sock)
{
	wxCHECK_RET(m_rd_ptr < m_wr_ptr,
		wxT("Reading past written data in WriteToSocket"));
	
	sock->SocketWrite(m_rd_ptr, GetUnreadDataLength());
	m_rd_ptr += sock->GetLastCount();
}


void CQueuedData::ReadFromSocket(CECSocket *sock, size_t len)
{
	const size_t canWrite = std::min(GetRemLength(), len);
	wxASSERT(len == canWrite);

	sock->SocketRead(m_wr_ptr, canWrite);
	m_wr_ptr += sock->GetLastCount();
}


size_t CQueuedData::ReadFromSocketAll(CECSocket *sock, size_t len)
{
	size_t read_rem = std::min(GetRemLength(), len);
	wxASSERT(read_rem == len);

	//  We get here when socket is truly blocking
	do {
		// Give socket a 10 sec chance to recv more data.
		if ( !sock->WaitSocketRead(10, 0) ) {
			AddDebugLogLineN(logEC, wxT("ReadFromSocketAll: socket is blocking"));
			break;
		}

		wxASSERT(m_wr_ptr + read_rem <= &m_data[0] + m_data.size());
		sock->SocketRead(m_wr_ptr, read_rem);
		m_wr_ptr += sock->GetLastCount();
		read_rem -= sock->GetLastCount();

		if (sock->SocketRealError()) {
			AddDebugLogLineN(logEC, wxT("ReadFromSocketAll: socket error"));
			break;
		}
	} while (read_rem);

	return len - read_rem;
}


size_t CQueuedData::GetLength() const
{
	return m_data.size();
}


size_t CQueuedData::GetDataLength() const
{
	const size_t len = m_wr_ptr - &m_data[0];
	wxCHECK_MSG(len <= m_data.size(), m_data.size(),
		wxT("Write-pointer past end of buffer"));

	return len;
}


size_t CQueuedData::GetRemLength() const
{
	return m_data.size() - GetDataLength();
}


size_t CQueuedData::GetUnreadDataLength() const
{
	wxCHECK_MSG(m_wr_ptr >= m_rd_ptr, 0,
		wxT("Read position past write position."));

	return m_wr_ptr - m_rd_ptr;
}



//
// CECSocket API - User interface functions
//

CECSocket::CECSocket(bool use_events)
:
m_use_events(use_events),
m_in_ptr(EC_SOCKET_BUFFER_SIZE),
m_out_ptr(EC_SOCKET_BUFFER_SIZE),
m_curr_rx_data(new CQueuedData(EC_SOCKET_BUFFER_SIZE)),
m_curr_tx_data(new CQueuedData(EC_SOCKET_BUFFER_SIZE)),
m_rx_flags(0),
m_tx_flags(0),
// setup initial state: 4 flags + 4 length
m_bytes_needed(EC_HEADER_SIZE),
m_in_header(true),
m_my_flags(0x20),
m_haveNotificationSupport(false)
{
	
}

CECSocket::~CECSocket()
{
	while (!m_output_queue.empty()) {
		CQueuedData *data = m_output_queue.front();
		m_output_queue.pop_front();
		delete data;
	}
}

bool CECSocket::ConnectSocket(uint32_t ip, uint16_t port)
{
	bool res = InternalConnect(ip, port, !m_use_events);
	return !SocketError() && res;
}

void CECSocket::SendPacket(const CECPacket *packet)
{
	uint32 len = WritePacket(packet);
	packet->DebugPrint(false, len);
	OnOutput();
}

const CECPacket *CECSocket::SendRecvPacket(const CECPacket *packet)
{
	SendPacket(packet);
	
	if (m_curr_rx_data->ReadFromSocketAll(this, EC_HEADER_SIZE) != EC_HEADER_SIZE
		|| SocketError()		// This is a synchronous read, so WouldBlock is an error too.
		|| !ReadHeader()) {
		OnError();
		AddDebugLogLineN(logEC, wxT("SendRecvPacket: error"));
		return 0;
	}
	if (m_curr_rx_data->ReadFromSocketAll(this, m_curr_packet_len) != m_curr_packet_len
		|| SocketError()) {
		OnError();
		AddDebugLogLineN(logEC, wxT("SendRecvPacket: error"));
		return 0;
	}
	const CECPacket *reply = ReadPacket();
	m_curr_rx_data->Rewind();
	return reply;
}

std::string CECSocket::GetLastErrorMsg()
{
	int code = InternalGetLastError();
	switch(code) {
		case EC_ERROR_NOERROR:
			return "No error happened";
		case EC_ERROR_INVOP:
			return "Invalid operation";
		case EC_ERROR_IOERR:
			return "Input/Output error";
		case EC_ERROR_INVADDR:
			return "Invalid address passed to wxSocket";
		case EC_ERROR_INVSOCK:
			return "Invalid socket (uninitialized)";
		case EC_ERROR_NOHOST:
			return "No corresponding host";
		case EC_ERROR_INVPORT:
			return "Invalid port";
		case EC_ERROR_WOULDBLOCK:
			return "The socket is non-blocking and the operation would block";
		case EC_ERROR_TIMEDOUT:
			return "The timeout for this operation expired";
		case EC_ERROR_MEMERR:
			return "Memory exhausted";
	}
	ostringstream error_string;
	error_string << "Error code " << code <<  " unknown.";
	return error_string.str();
}

bool CECSocket::SocketRealError() 
{ 
	bool ret = false;
	if (InternalError()) {
		int lastError = InternalGetLastError();
		ret = lastError != EC_ERROR_NOERROR && lastError != EC_ERROR_WOULDBLOCK;
	}
	return ret;
}

void CECSocket::OnError()
{
#ifdef __DEBUG__
	cout << GetLastErrorMsg() << endl;
#endif
}

void CECSocket::OnLost()
{
}

//
// Event handlers
//
void CECSocket::OnConnect()
{
}

void CECSocket::OnInput()
{
	size_t bytes_rx = 0;
	do {
		m_curr_rx_data->ReadFromSocket(this, m_bytes_needed);
		if (SocketRealError()) {
			AddDebugLogLineN(logEC, wxT("OnInput: socket error"));
			OnError();
			// socket already disconnected in this point
			return;
		}
		bytes_rx = GetLastCount();
		m_bytes_needed -= bytes_rx;
	
		if (m_bytes_needed == 0) {
			if (m_in_header) {
				m_in_header = false;
				if (!ReadHeader()) {
					AddDebugLogLineN(logEC, wxT("OnInput: header error"));
					return;
				}
			} else {
				std::auto_ptr<const CECPacket> packet(ReadPacket());
				m_curr_rx_data->Rewind();
				if (packet.get()) {
					std::auto_ptr<const CECPacket> reply(OnPacketReceived(packet.get(), m_curr_packet_len));
					if (reply.get()) {
						SendPacket(reply.get());
					}
				} else {
					AddDebugLogLineN(logEC, wxT("OnInput: no packet"));
				}
				m_bytes_needed = EC_HEADER_SIZE;
				m_in_header = true;
			}
		}
	} while (bytes_rx);
}

void CECSocket::OnOutput()
{
	while (!m_output_queue.empty()) {
		CQueuedData* data = m_output_queue.front();
		data->WriteToSocket(this);
		if (!data->GetUnreadDataLength()) {
			m_output_queue.pop_front();
			delete data;
		}
		if (SocketError()) {
			if (!WouldBlock()) {
				// real error, abort
				AddDebugLogLineN(logEC, wxT("OnOutput: socket error"));
				OnError();
				return;
			}
			// Now it's just a blocked socket.
			if ( m_use_events ) {
				// Event driven logic: return, OnOutput() will be called again later
				return;
			}
			// Syncronous call: wait (for max 10 secs)
			if ( !WaitSocketWrite(10, 0) ) {
				// Still not through ?
				if (WouldBlock()) {
					// WouldBlock() is only EAGAIN or EWOULD_BLOCK, 
					// and those shouldn't create an infinite wait.
					// So give it another chance.
					continue;
				} else {
					AddDebugLogLineN(logEC, wxT("OnOutput: socket error in sync wait"));
					OnError();
					break;
				}
			}
		}
	}
	//
	// All outstanding data sent to socket
	// (used for push clients)
	//
	WriteDoneAndQueueEmpty();
}

bool CECSocket::DataPending()
{
	return !m_output_queue.empty();
}

//
// Socket I/O
//

size_t CECSocket::ReadBufferFromSocket(void *buffer, size_t required_len)
{
	wxASSERT(required_len);

	if (m_curr_rx_data->GetUnreadDataLength() < required_len) {
		// need more data that we have. Looks like nothing will help here
		AddDebugLogLineN(logEC, CFormat(wxT("ReadBufferFromSocket: not enough data (%d < %d)")) 
			% m_curr_rx_data->GetUnreadDataLength() % required_len);
		return 0;
	}
	m_curr_rx_data->Read(buffer, required_len);
	return required_len;
}

void CECSocket::WriteBufferToSocket(const void *buffer, size_t len)
{
	unsigned char *wr_ptr = (unsigned char *)buffer;
	while ( len ) {
		size_t curr_free = m_curr_tx_data->GetRemLength();
		if ( len > curr_free ) {

			m_curr_tx_data->Write(wr_ptr, curr_free);
			len -= curr_free;
			wr_ptr += curr_free;
			m_output_queue.push_back(m_curr_tx_data.release());
			m_curr_tx_data.reset(new CQueuedData(EC_SOCKET_BUFFER_SIZE));
		} else {
			m_curr_tx_data->Write(wr_ptr, len);
			break;
		}
	}
}


//
// ZLib "error handler"
//

void ShowZError(int zerror, z_streamp strm)
{
	const char *p = NULL;

	switch (zerror) {
		case Z_STREAM_END: p = "Z_STREAM_END"; break;
		case Z_NEED_DICT: p = "Z_NEED_DICT"; break;
		case Z_ERRNO: p = "Z_ERRNO"; break;
		case Z_STREAM_ERROR: p = "Z_STREAM_ERROR"; break;
		case Z_DATA_ERROR: p = "Z_DATA_ERROR"; break;
		case Z_MEM_ERROR: p = "Z_MEM_ERROR"; break;
		case Z_BUF_ERROR: p = "Z_BUF_ERROR"; break;
		case Z_VERSION_ERROR: p = "Z_VERSION_ERROR"; break;
	}
	printf("ZLib operation returned %s\n", p);
	printf("ZLib error message: %s\n", strm->msg);
	printf("zstream state:\n\tnext_in=%p\n\tavail_in=%u\n\ttotal_in=%lu\n\tnext_out=%p\n\tavail_out=%u\n\ttotal_out=%lu\n",
		strm->next_in, strm->avail_in, strm->total_in, strm->next_out, strm->avail_out, strm->total_out);
	AddDebugLogLineN(logEC, wxT("ZLib error"));
}


bool CECSocket::ReadHeader()
{
	m_curr_rx_data->Read(&m_rx_flags, 4);
	m_rx_flags = ENDIAN_NTOHL(m_rx_flags);
	m_curr_rx_data->Read(&m_curr_packet_len, 4);
	m_curr_packet_len = ENDIAN_NTOHL(m_curr_packet_len);
	m_bytes_needed = m_curr_packet_len;
	// packet bigger that 16Mb looks more like broken request
	if (m_bytes_needed > 16*1024*1024) {
		AddDebugLogLineN(logEC, CFormat(wxT("ReadHeader: packet too big: %d")) % m_bytes_needed);
		CloseSocket();
		return false;
	}
	m_curr_rx_data->Rewind();
	size_t currLength = m_curr_rx_data->GetLength();
	// resize input buffer if
	// a) too small or
	if (currLength < m_bytes_needed
	// b) way too large (free data again after receiving huge packets)
		|| m_bytes_needed + EC_SOCKET_BUFFER_SIZE * 10 < currLength) {
		// Client socket: IsAuthorized() is always true
		// Server socket: do not allow growing of internal buffers before succesfull login.
		// Otherwise sending a simple header with bogus length of 16MB-1 will crash an embedded
		// client with memory exhaustion.
		if (!IsAuthorized()) {
			AddDebugLogLineN(logEC, CFormat(wxT("ReadHeader: resize (%d -> %d) on non autorized socket")) % currLength % m_bytes_needed);
			CloseSocket();
			return false;
		}
		// Don't make buffer smaller than EC_SOCKET_BUFFER_SIZE
		size_t bufSize = m_bytes_needed;
		if (bufSize < EC_SOCKET_BUFFER_SIZE) {
			bufSize = EC_SOCKET_BUFFER_SIZE;
		}
		m_curr_rx_data.reset(new CQueuedData(bufSize));
	}
	if (ECLogIsEnabled()) {
		DoECLogLine(CFormat(wxT("< %d ...")) % m_bytes_needed);
	}
	return true;
}


bool CECSocket::ReadNumber(void *buffer, size_t len)
{
	if (m_rx_flags & EC_FLAG_UTF8_NUMBERS) {
		unsigned char mb[6];
		uint32_t wc;
		if (!ReadBuffer(mb, 1)) return false;
		int remains = utf8_mb_remain(mb[0]);
		if (remains) if (!ReadBuffer(&(mb[1]), remains)) return false;
		if (utf8_mbtowc(&wc, mb, 6) == -1) return false;	// Invalid UTF-8 code sequence
		switch (len) {
			case 1: PokeUInt8( buffer,  wc ); break;
			case 2: RawPokeUInt16( buffer, wc ); break;
			case 4: RawPokeUInt32( buffer, wc ); break;
		}
	} else {
		if ( !ReadBuffer(buffer, len) ) {
			return false;
		}
		switch (len) {
			case 2:
				RawPokeUInt16( buffer, ENDIAN_NTOHS( RawPeekUInt16( buffer ) ) );
				break;
			case 4:
				RawPokeUInt32( buffer, ENDIAN_NTOHL( RawPeekUInt32( buffer ) ) );
				break;
		}
	}
	return true;
}

bool CECSocket::WriteNumber(const void *buffer, size_t len)
{
	if (m_tx_flags & EC_FLAG_UTF8_NUMBERS) {
		unsigned char mb[6];
		uint32_t wc = 0;
		int mb_len;
		switch (len) {
			case 1: wc = PeekUInt8( buffer ); break;
			case 2: wc = RawPeekUInt16( buffer ); break;
			case 4: wc = RawPeekUInt32( buffer ); break;
			default: return false;
		}
		if ((mb_len = utf8_wctomb(mb, wc, 6)) == -1) return false;	// Something is terribly wrong...
		return WriteBuffer(mb, mb_len);
	} else {
		char tmp[8];

		switch (len) {
			case 1: PokeUInt8( tmp, PeekUInt8( buffer ) ); break;
			case 2: RawPokeUInt16( tmp, ENDIAN_NTOHS( RawPeekUInt16( buffer ) ) ); break;
			case 4: RawPokeUInt32( tmp, ENDIAN_NTOHL( RawPeekUInt32( buffer ) ) ); break;
		}
		return WriteBuffer(tmp, len);
	}
}

bool CECSocket::ReadBuffer(void *buffer, size_t len)
{
	if (m_rx_flags & EC_FLAG_ZLIB) {
		if ( !m_z.avail_in ) {
			// no reason for this situation: all packet should be
			// buffered by now
			AddDebugLogLineN(logEC, wxT("ReadBuffer: ZLib error"));
			return false;
		}
		m_z.avail_out = (uInt)len;
		m_z.next_out = (Bytef*)buffer;
		int zerror = inflate(&m_z, Z_SYNC_FLUSH);
		if ((zerror != Z_OK) && (zerror != Z_STREAM_END)) {
			ShowZError(zerror, &m_z);
			AddDebugLogLineN(logEC, wxT("ReadBuffer: ZLib error"));
			return false;
		}
		return true;
	} else {
		// using uncompressed buffered i/o
		size_t read = ReadBufferFromSocket(buffer, len);
		if (read == len) {
			return true;
		} else {
			AddDebugLogLineN(logEC, CFormat(wxT("ReadBuffer: %d < %d")) % read % len);
			return false;
		}
	}
}

bool CECSocket::WriteBuffer(const void *buffer, size_t len)
{
	if (m_tx_flags & EC_FLAG_ZLIB) {

		unsigned char *rd_ptr = (unsigned char *)buffer;
		do {
			unsigned int remain_in = EC_SOCKET_BUFFER_SIZE - m_z.avail_in;
			if ( remain_in >= len ) {
				memcpy(m_z.next_in+m_z.avail_in, rd_ptr, len);
				m_z.avail_in += (uInt)len;
				len = 0;
			} else {
				memcpy(m_z.next_in+m_z.avail_in, rd_ptr, remain_in);
				m_z.avail_in += remain_in;
				len -= remain_in;
				rd_ptr += remain_in;
				// buffer is full, calling zlib
				do {
				    m_z.next_out = &m_out_ptr[0];
				    m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
					int zerror = deflate(&m_z, Z_NO_FLUSH);
					if ( zerror != Z_OK ) {
						AddDebugLogLineN(logEC, wxT("WriteBuffer: ZLib error"));
						ShowZError(zerror, &m_z);
						return false;
					}
					WriteBufferToSocket(&m_out_ptr[0],
						EC_SOCKET_BUFFER_SIZE - m_z.avail_out);
				} while ( m_z.avail_out == 0 );
				// all input should be used by now
				wxASSERT(m_z.avail_in == 0);
				m_z.next_in = &m_in_ptr[0];
			}
		} while ( len );
		return true;
	} else {
		// using uncompressed buffered i/o
		WriteBufferToSocket(buffer, len);
		return true;
	}
}

bool CECSocket::FlushBuffers()
{
	if (m_tx_flags & EC_FLAG_ZLIB) {
		do {
		    m_z.next_out = &m_out_ptr[0];
		    m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
			int zerror = deflate(&m_z, Z_FINISH);
			if ( zerror == Z_STREAM_ERROR ) {
				AddDebugLogLineN(logEC, wxT("FlushBuffers: ZLib error"));
				ShowZError(zerror, &m_z);
				return false;
			}
			WriteBufferToSocket(&m_out_ptr[0],
				EC_SOCKET_BUFFER_SIZE - m_z.avail_out);
		} while ( m_z.avail_out == 0 );
	}
	if ( m_curr_tx_data->GetDataLength() ) {
		m_output_queue.push_back(m_curr_tx_data.release());
		m_curr_tx_data.reset(new CQueuedData(EC_SOCKET_BUFFER_SIZE));
	}
	return true;
}

//
// Packet I/O
//

uint32 CECSocket::WritePacket(const CECPacket *packet)
{
	if (SocketRealError()) {
		OnError();
		return 0;
	}
	// Check if output queue is empty. If not, memorize the current end.
	std::list<CQueuedData*>::iterator outputStart = m_output_queue.begin();
	uint32 outputQueueSize = m_output_queue.size();
	for (uint32 i = 1; i < outputQueueSize; i++) {
		outputStart++;
	}

	uint32_t flags = 0x20;

	if (packet->GetPacketLength() > EC_MAX_UNCOMPRESSED
		&& ((m_my_flags & EC_FLAG_ZLIB) > 0)) {
		flags |= EC_FLAG_ZLIB;
	} else {
		flags |= EC_FLAG_UTF8_NUMBERS;
	}

	flags &= m_my_flags;
	m_tx_flags = flags;
	
	if (flags & EC_FLAG_ZLIB) {
		m_z.zalloc = Z_NULL;
		m_z.zfree = Z_NULL;
		m_z.opaque = Z_NULL;
		m_z.avail_in = 0;
		m_z.next_in = &m_in_ptr[0];
		int zerror = deflateInit(&m_z, EC_COMPRESSION_LEVEL);
		if (zerror != Z_OK) {
			// don't use zlib if init failed
			flags &= ~EC_FLAG_ZLIB;
			ShowZError(zerror, &m_z);
		}
	}

	uint32_t tmp_flags = ENDIAN_HTONL(flags);
	WriteBufferToSocket(&tmp_flags, sizeof(uint32));
	
	// preallocate 4 bytes in buffer for packet length
	uint32_t packet_len = 0;
	WriteBufferToSocket(&packet_len, sizeof(uint32));
	
	packet->WritePacket(*this);

	// Finalize zlib compression and move current data to outout queue
	FlushBuffers();

	// find the beginning of our data in the output queue
	if (outputQueueSize) {
		outputStart++;
	} else {
		outputStart = m_output_queue.begin();
	}
	// now calculate actual size of data
	for(std::list<CQueuedData*>::iterator it = outputStart; it != m_output_queue.end(); it++) {
		packet_len += (uint32_t)(*it)->GetDataLength();
	}
	// header size is not counted
	packet_len -= EC_HEADER_SIZE;
	// now write actual length at offset 4
	uint32 packet_len_E = ENDIAN_HTONL(packet_len);
	(*outputStart)->WriteAt(&packet_len_E, 4, 4);
	
	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			AddDebugLogLineN(logEC, wxT("WritePacket: ZLib error"));
			ShowZError(zerror, &m_z);
		}
	}
	return packet_len;
}


const CECPacket *CECSocket::ReadPacket()
{
	CECPacket *packet = 0;

	uint32_t flags = m_rx_flags;
	
	if ( ((flags & 0x60) != 0x20) || (flags & EC_FLAG_UNKNOWN_MASK) ) {
		// Protocol error - other end might use an older protocol
		AddDebugLogLineN(logEC, wxT("ReadPacket: protocol error"));
		cout << "ReadPacket: packet have invalid flags " << flags << endl;
		CloseSocket();
		return 0;
	}

	if (flags & EC_FLAG_ZLIB) {

	    m_z.zalloc = Z_NULL;
	    m_z.zfree = Z_NULL;
	    m_z.opaque = Z_NULL;
	    m_z.avail_in = 0;
	    m_z.next_in = 0;
	    
		int zerror = inflateInit(&m_z);
		if (zerror != Z_OK) {
			AddDebugLogLineN(logEC, wxT("ReadPacket: zlib error"));
			ShowZError(zerror, &m_z);
			cout << "ReadPacket: failed zlib init" << endl;
			CloseSocket();
			return 0;
		}
	}

	m_curr_rx_data->ToZlib(m_z);
	packet = new CECPacket();
	
	if (!packet->ReadFromSocket(*this)) {
		AddDebugLogLineN(logEC, wxT("ReadPacket: error in packet read"));
		cout << "ReadPacket: error in packet read" << endl;
		delete packet;
		packet = NULL;
		CloseSocket();
	}

	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			AddDebugLogLineN(logEC, wxT("ReadPacket: zlib error"));
			ShowZError(zerror, &m_z);
			cout << "ReadPacket: failed zlib free" << endl;
			CloseSocket();
		}
	}

	return packet;
}

const CECPacket *CECSocket::OnPacketReceived(const CECPacket *, uint32)
{
	return 0;
}
// File_checked_for_headers

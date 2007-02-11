//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2006 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

using namespace std;

#include "ECPacket.h"		// Needed for CECPacket

#define EC_COMPRESSION_LEVEL	Z_BEST_COMPRESSION
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

static struct utf8_table utf8_table[] =
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
	struct utf8_table *t;

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
	struct utf8_table *t;

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

size_t CQueuedData::ReadFromSocketAll(CECSocket *sock, size_t len)
{
	size_t read_rem = len;
	//
	// We get here when socket is truly blocking
	//
	do {
		//
		// Give socket a 10 sec chance to recv more data.
		if ( !sock->WaitSocketRead(10, 0) ) {
			break;
		}
		wxASSERT(m_wr_ptr + read_rem <= &m_data[0] + m_data.size());
		sock->SocketRead(m_wr_ptr, read_rem);
		m_wr_ptr += sock->GetLastCount();
		read_rem -= sock->GetLastCount();
		if (sock->SocketError() && !sock->WouldBlock()) {
				break;
		}
	} while (read_rem);
	return len - read_rem;
}

//
// CECSocket API - User interface functions
//

CECSocket::CECSocket(bool use_events)
:
m_use_events(use_events),
m_output_queue(),
m_in_ptr(EC_SOCKET_BUFFER_SIZE),
m_out_ptr(EC_SOCKET_BUFFER_SIZE),
m_curr_rx_data(new CQueuedData(EC_SOCKET_BUFFER_SIZE)),
m_curr_tx_data(new CQueuedData(EC_SOCKET_BUFFER_SIZE)),
m_rx_flags(0),
m_tx_flags(0),
m_my_flags(0x20 | EC_FLAG_ZLIB | EC_FLAG_UTF8_NUMBERS | EC_FLAG_ACCEPTS),
// setup initial state: 4 flags + 4 length
m_bytes_needed(8),
m_in_header(true)
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
	bool res = InternalConnect(ip, port, false);
	if ( !m_use_events ) {
		res = WaitSocketConnect(10, 0);
		if ( res ) {
			OnConnect();
		} else {
			OnLost();
		}
	}
	return !SocketError();
}

void CECSocket::SendPacket(const CECPacket *packet)
{
	WritePacket(packet);
	OnOutput();
}

const CECPacket *CECSocket::SendRecvPacket(const CECPacket *packet)
{
	SendPacket(packet);
	m_curr_rx_data->ReadFromSocketAll(this, 2 * sizeof(uint32_t));
	if (SocketError() && !WouldBlock()) {
		OnError();
		return 0;
	}

	m_curr_rx_data->Read(&m_rx_flags, sizeof(m_rx_flags));
	m_rx_flags = ENDIAN_NTOHL(m_rx_flags);
	m_curr_rx_data->Read(&m_curr_packet_len, sizeof(m_curr_packet_len));
	m_curr_packet_len = ENDIAN_NTOHL(m_curr_packet_len);
	
	if ( m_curr_rx_data->GetLength() < (m_curr_packet_len+2*sizeof(uint32_t)) ) {
		m_curr_rx_data.reset(new CQueuedData(m_curr_packet_len));
	}
	m_curr_rx_data->ReadFromSocketAll(this, m_curr_packet_len);
	if (SocketError() && !WouldBlock()) {
		OnError();
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
		case EC_ERROR_DUMMY:
			return "Dummy code - should not happen";
	}
	ostringstream error_string;
	error_string << "Error code " << code <<  " unknown.";
	return error_string.str();
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
		if (m_curr_rx_data.get()) {
			m_curr_rx_data->ReadFromSocket(this, m_bytes_needed);
		} else {
			return;
		}
		if (SocketError() && !WouldBlock()) {
			OnError();
			// socket already disconnected in this point
			m_curr_rx_data.reset(0);
			return;
		}
		bytes_rx = GetLastCount();
		m_bytes_needed -= bytes_rx;
	} while (m_bytes_needed && bytes_rx);
	
	if (!m_bytes_needed) {
		if (m_in_header) {
			m_in_header = false;
			m_curr_rx_data->Read(&m_rx_flags, sizeof(m_rx_flags));
			m_rx_flags = ENDIAN_NTOHL(m_rx_flags);
			if (m_rx_flags & EC_FLAG_ACCEPTS) {
				// Client sends its capabilities, update the internal mask.
				m_curr_rx_data->Read(&m_my_flags, sizeof(m_my_flags));
				m_my_flags = ENDIAN_NTOHL(m_my_flags);
				printf("Reading accepts mask: %x\n", m_my_flags);
				wxASSERT(m_my_flags & 0x20);
				// There has to be 4 more bytes. THERE HAS TO BE, DAMN IT.
				m_curr_rx_data->ReadFromSocketAll(this, sizeof(m_curr_packet_len));
			}
			m_curr_rx_data->Read(&m_curr_packet_len, sizeof(m_curr_packet_len));
			m_curr_packet_len = ENDIAN_NTOHL(m_curr_packet_len);
			m_bytes_needed = m_curr_packet_len;
			// packet bigger that 16Mb looks more like broken request
			if (m_bytes_needed > 16*1024*1024) {
				CloseSocket();
				return;
			}
			size_t needed_size = m_bytes_needed + ((m_rx_flags & EC_FLAG_ACCEPTS) ? 12 : 8);
			if (!m_curr_rx_data.get() ||
			    m_curr_rx_data->GetLength() < needed_size) {
				m_curr_rx_data.reset(new CQueuedData(needed_size));
			}
			#warning Kry TODO: Read packet?
		} else {
			//m_curr_rx_data->DumpMem();
			std::auto_ptr<const CECPacket> packet(ReadPacket());
			m_curr_rx_data->Rewind();
			if (packet.get()) {
				std::auto_ptr<const CECPacket> reply(OnPacketReceived(packet.get()));
				if (reply.get()) {
					SendPacket(reply.get());
				}
			}
			m_bytes_needed = 8;
			m_in_header = true;
		}
	}
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
			if (WouldBlock()) {
				if ( m_use_events ) {
					return;
				} else {
					if ( !WaitSocketWrite(10, 0) ) {
						if (WouldBlock()) {
							continue;
						} else {
							OnError();
							break;
						}
	                		}
				}
			} else {
				OnError();
				return;
			}
		}
	}
}

//
// Socket I/O
//

size_t CECSocket::ReadBufferFromSocket(void *buffer, size_t required_len)
{
	wxASSERT(required_len);

	if (m_curr_rx_data->GetUnreadDataLength() < required_len) {
		// need more data that we have. Looks like nothing will help here
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
	char *p = NULL;

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
			return false;
		}
		m_z.avail_out = len;
		m_z.next_out = (Bytef*)buffer;
		int zerror = inflate(&m_z, Z_SYNC_FLUSH);
		if ((zerror != Z_OK) && (zerror != Z_STREAM_END)) {
			ShowZError(zerror, &m_z);
			return false;
		}
		return true;
	} else {
		// using uncompressed buffered i/o
		return ReadBufferFromSocket(buffer, len) == len;
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
				m_z.avail_in += len;
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

void CECSocket::WritePacket(const CECPacket *packet)
{
	if (SocketError()) {
		return;
	}

	uint32_t flags = 0x20;

	if ( packet->GetPacketLength() > EC_MAX_UNCOMPRESSED ) {
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

	uint32_t tmp_flags = ENDIAN_HTONL(flags/* | EC_FLAG_ACCEPTS*/);
	WriteBufferToSocket(&tmp_flags, sizeof(uint32));
	
/*	uint32_t tmp_accepts_flags = ENDIAN_HTONL(m_my_flags);
	WriteBufferToSocket(&tmp_accepts_flags, sizeof(uint32));*/

	// preallocate 4 bytes in buffer for packet length
	uint32_t packet_len = 0;
	WriteBufferToSocket(&packet_len, sizeof(uint32));
	
	packet->WritePacket(*this);

	FlushBuffers();

	// now calculate actual size of data
	packet_len = m_curr_tx_data->GetDataLength();
	for(std::deque<CQueuedData*>::iterator i = m_output_queue.begin(); i != m_output_queue.end(); i++) {
		packet_len += (*i)->GetDataLength();
	}
	// 4 flags and 4 length are not counted
	packet_len -= 8;
	// now write actual length @ offset 4
	packet_len = ENDIAN_HTONL(packet_len);
	
	CQueuedData *first_buff = m_output_queue.front();
	if ( !first_buff ) first_buff = m_curr_tx_data.get();
	first_buff->WriteAt(&packet_len, sizeof(uint32_t), sizeof(uint32_t));
	
	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &m_z);
			return;
		}
	}
}


const CECPacket *CECSocket::ReadPacket()
{
	CECPacket *packet = 0;

	uint32_t flags = m_rx_flags;
	
	if ( ((flags & 0x60) != 0x20) || (flags & EC_FLAG_UNKNOWN_MASK) ) {
		// Protocol error - other end might use an older protocol
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
			ShowZError(zerror, &m_z);
			cout << "ReadPacket: failed zlib init" << endl;
			CloseSocket();
			return 0;
		}
	}

	m_curr_rx_data->ToZlib(m_z);
	packet = new CECPacket(*this);
	packet->ReadFromSocket(*this);
	
	if (packet->m_error != 0) {
		cout << "ReadPacket: error " << packet->m_error << "in packet read" << endl;
		delete packet;
		packet = NULL;
		CloseSocket();
	}

	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &m_z);
			cout << "ReadPacket: failed zlib free" << endl;
			CloseSocket();
		}
	}

	return packet;
}

const CECPacket *CECSocket::OnPacketReceived(const CECPacket *)
{
	return 0;
}
// File_checked_for_headers

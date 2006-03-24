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

#include <wx/intl.h>			// Needed for i18n
#include <zlib.h>		// Needed for packet (de)compression
#include <cstring>		// Needed for memcpy()/memmove()

#include "ArchSpecific.h"	// Needed for ENDIAN_NTOHL

#include "ECVersion.h"		// Needed for EC_VERSION_ID
#include "ECCodes.h"		// Needed for the EC_FLAG_* values
#include "ECPacket.h"		// Needed for CECPacket

#include <wx/app.h>		// Needed for wxTheApp

#include "OtherFunctions.h"
#include "common/StringFunctions.h"	// Needed for unicode2char()
#include "Logger.h"
#include <common/Format.h>

#define EC_SOCKET_BUFFER_SIZE	2048
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
	uint32  lmask;
	uint32  lval;
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

int utf8_mbtowc(uint32 *p, const unsigned char *s, int n)
{
	uint32 l;
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

int utf8_wctomb(unsigned char *s, uint32 wc, int maxlen)
{
	uint32 l;
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

size_t CQueuedData::ReadFromSocketAll(wxSocketBase *sock, size_t len)
{
	size_t read_rem = len;
	//
	// We get here when socket is truly blocking
	//
	do {
		//
		// Give socket a 10 sec chance to recv more data.
		if ( !sock->WaitForRead(10, 0) ) {
			break;
		}
		sock->Read(m_wr_ptr, read_rem);
		m_wr_ptr += sock->LastCount();
		read_rem -= sock->LastCount();
		if (sock->Error()) {
			if (sock->LastError() != wxSOCKET_WOULDBLOCK) {
				break;
			}
		}
	} while ( read_rem );
	return len - read_rem;
}

//-------------------- CECSocketHandler --------------------

#define ECSOCKET_SENDPACKET	(GSOCK_MAX_EVENT + 1)
#define ECSOCKET_RECEIVEDPACKET	(GSOCK_MAX_EVENT + 2)
#define ECSOCKET_RECEIVEDDATA	(GSOCK_MAX_EVENT + 3)

#define	EC_SOCKET_HANDLER	(wxID_HIGHEST + 644)

class CECSocketHandler: public wxEvtHandler {
 public:
        CECSocketHandler() {};

 private:
        void SocketHandler(wxSocketEvent& event);

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CECSocketHandler, wxEvtHandler)
        EVT_SOCKET(EC_SOCKET_HANDLER, CECSocketHandler::SocketHandler)
END_EVENT_TABLE()

void CECSocketHandler::SocketHandler(wxSocketEvent& event)
{
        CECSocket *socket = dynamic_cast<CECSocket *>(event.GetSocket());
        wxCHECK_RET(socket, wxT("Socket event with a NULL socket!"));

        switch(event.GetSocketEvent()) {
        case wxSOCKET_LOST:
            socket->OnLost();
            break;
        case wxSOCKET_INPUT:
            socket->OnInput();
            break;
        case wxSOCKET_OUTPUT:
            socket->OnOutput();
            break;
        case wxSOCKET_CONNECTION:
            socket->OnConnect();
            break;
            
        default:
            // Nothing should arrive here...
            wxFAIL;
            break;
        }
}

static CECSocketHandler	g_ECSocketHandler;


//
// CECSocket API - User interface functions
//

CECSocket::CECSocket(bool use_events) : wxSocketClient()
{
	m_rx_flags = m_rx_flags = 0;
	m_my_flags = 0x20 | EC_FLAG_ZLIB | EC_FLAG_UTF8_NUMBERS;

	// setup initial state: 4 flags + 4 length
	m_bytes_needed = 8;
	m_in_header = true;

	m_in_ptr = new unsigned char[EC_SOCKET_BUFFER_SIZE];
	m_out_ptr = new unsigned char[EC_SOCKET_BUFFER_SIZE];
	
	m_curr_rx_data = new CQueuedData(EC_SOCKET_BUFFER_SIZE);
	m_curr_tx_data = new CQueuedData(EC_SOCKET_BUFFER_SIZE);

	m_use_events = use_events;
	
	if ( use_events ) {
		SetEventHandler(g_ECSocketHandler, EC_SOCKET_HANDLER);
		SetNotify(wxSOCKET_CONNECTION_FLAG |
			  wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG);
		Notify(true);
		SetFlags(wxSOCKET_NOWAIT);
	} else {
		SetFlags(wxSOCKET_WAITALL | wxSOCKET_BLOCK);
		Notify(false);
	}
}

CECSocket::~CECSocket()
{
	if (m_in_ptr) {
		delete [] m_in_ptr;
	}
	if (m_out_ptr) {
		delete [] m_out_ptr;
	}

	while (!m_output_queue.empty()) {
		CQueuedData *data = m_output_queue.front();
		m_output_queue.pop_front();
		delete data;
	}
}

bool CECSocket::Connect(wxSockAddress& address)
{
	bool res = wxSocketClient::Connect(address, false);
	if ( !m_use_events ) {
		res = WaitOnConnect(10, 0);
		if ( res ) {
			OnConnect();
		} else {
			OnLost();
		}
	}
	return !Error();
}

void CECSocket::SendPacket(const CECPacket *packet)
{
	WritePacket(packet);
	OnOutput();
}

const CECPacket *CECSocket::SendRecvPacket(const CECPacket *packet)
{
	wxASSERT(!m_use_events);
	
	SendPacket(packet);
	
	m_curr_rx_data->ReadFromSocketAll(this, 2 * sizeof(uint32));
	if (Error() && (LastError() != wxSOCKET_WOULDBLOCK)) {
		OnError();
		return 0;
	}

	m_curr_rx_data->Read(&m_rx_flags, sizeof(m_rx_flags));
	m_rx_flags = ENDIAN_NTOHL(m_rx_flags);
	m_curr_rx_data->Read(&m_curr_packet_len, sizeof(m_curr_packet_len));
	m_curr_packet_len = ENDIAN_NTOHL(m_curr_packet_len);
	
	if ( m_curr_rx_data->GetLength() < (m_curr_packet_len+2*sizeof(uint32)) ) {
		delete m_curr_rx_data;
		m_curr_rx_data = new CQueuedData(m_curr_packet_len);
	}
	m_curr_rx_data->ReadFromSocketAll(this, m_curr_packet_len);
	if (Error() && (LastError() != wxSOCKET_WOULDBLOCK)) {
		OnError();
		return 0;
	}
	const CECPacket *reply = ReadPacket();
	m_curr_rx_data->Rewind();
	return reply;
}

wxString CECSocket::GetErrorMsg(wxSocketError code)
{
	switch(code) {
		case wxSOCKET_NOERROR:
			return wxT("No error happened");
		case wxSOCKET_INVOP:
			return wxT("Invalid operation");
		case wxSOCKET_IOERR:
			return wxT("Input/Output error");
		case wxSOCKET_INVADDR:
			return wxT("Invalid address passed to wxSocket");
		case wxSOCKET_INVSOCK:
			return wxT("Invalid socket (uninitialized)");
		case wxSOCKET_NOHOST:
			return wxT("No corresponding host");
		case wxSOCKET_INVPORT:
			return wxT("Invalid port");
		case wxSOCKET_WOULDBLOCK:
			return wxT("The socket is non-blocking and the operation would block");
		case wxSOCKET_TIMEDOUT:
			return wxT("The timeout for this operation expired");
		case wxSOCKET_MEMERR:
			return wxT("Memory exhausted");
		case wxSOCKET_DUMMY:
			return wxT("Dummy code - should not happen");
	}
	return wxString::Format(wxT("Error code 0x%08x unknown"), code);
}

void CECSocket::OnError()
{
#ifdef __DEBUG__
	printf("CECSocket error: %s\n", (const char *)unicode2char(GetErrorMsg(LastError())));
#endif
}

void CECSocket::OnLost()
{
}

void CECSocket::Destroy()
{
	wxSocketBase::Destroy();
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
		if ( Error() ) {
			if (LastError() != wxSOCKET_WOULDBLOCK && LastError() != wxSOCKET_NOERROR) {
				OnError();
				// socket already disconnected in this point
				delete m_curr_rx_data;
				m_curr_rx_data = 0;
				return;
			}
		}
	} while ( m_bytes_needed && bytes_rx );
	bytes_rx = LastCount();
	m_bytes_needed -= bytes_rx;

	if ( !m_bytes_needed ) {
		if ( m_in_header ) {
			m_in_header = false;
			m_curr_rx_data->Read(&m_rx_flags, sizeof(m_rx_flags));
			m_rx_flags = ENDIAN_NTOHL(m_rx_flags);
			m_curr_rx_data->Read(&m_curr_packet_len, sizeof(m_curr_packet_len));
			m_curr_packet_len = ENDIAN_NTOHL(m_curr_packet_len);
			m_bytes_needed = m_curr_packet_len;
			// packet bigger that 16Mb looks more like broken request
			if ( m_bytes_needed > 16*1024*1024 ) {
				wxSocketBase::Close();
				return;
			}
			if ( !m_curr_rx_data || (m_curr_rx_data->GetLength() < (m_bytes_needed+8)) ) {
				delete m_curr_rx_data;
				m_curr_rx_data = new CQueuedData(m_bytes_needed);
			}
		} else {
			//m_curr_rx_data->DumpMem();
			std::auto_ptr<const CECPacket> packet(ReadPacket());
			m_curr_rx_data->Rewind();
			std::auto_ptr<const CECPacket> reply(OnPacketReceived(packet.get()));
			if ( reply.get() ) {
				SendPacket(reply.get());
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
		if (Error()) {
			if (LastError() == wxSOCKET_WOULDBLOCK || LastError() == wxSOCKET_NOERROR) {
				if ( m_use_events ) {
					return;
				} else {
					if ( !WaitForWrite(10, 0) ) {
                        if (LastError() == wxSOCKET_WOULDBLOCK) {
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

	if ( m_curr_rx_data->GetUnreadDataLength() < required_len ) {
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
			m_output_queue.push_back(m_curr_tx_data);
			m_curr_tx_data = new CQueuedData(EC_SOCKET_BUFFER_SIZE);
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
		uint32 wc;
		if (!ReadBuffer(mb, 1)) return false;
		int remains = utf8_mb_remain(mb[0]);
		if (remains) if (!ReadBuffer(&(mb[1]), remains)) return false;
		if (utf8_mbtowc(&wc, mb, 6) == -1) return false;	// Invalid UTF-8 code sequence
		switch (len) {
			case 1: PokeUInt8( buffer,  wc ); break;
			case 2:	RawPokeUInt16( buffer, wc ); break;
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
		uint32 wc = 0;
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
		return true;
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
				    m_z.next_out = m_out_ptr;
				    m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
					int zerror = deflate(&m_z, Z_NO_FLUSH);
					if ( zerror != Z_OK ) {
						ShowZError(zerror, &m_z);
						return false;
					}
					WriteBufferToSocket(m_out_ptr, EC_SOCKET_BUFFER_SIZE - m_z.avail_out);
				} while ( m_z.avail_out == 0 );
				// all input should be used by now
				wxASSERT(m_z.avail_in == 0);
				m_z.next_in = m_in_ptr;
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
		    m_z.next_out = m_out_ptr;
		    m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
			int zerror = deflate(&m_z, Z_FINISH);
			if ( zerror == Z_STREAM_ERROR ) {
				ShowZError(zerror, &m_z);
				return false;
			}
			WriteBufferToSocket(m_out_ptr, EC_SOCKET_BUFFER_SIZE - m_z.avail_out);
		} while ( m_z.avail_out == 0 );
	}
	if ( m_curr_tx_data->GetDataLength() ) {
		m_output_queue.push_back(m_curr_tx_data);
		m_curr_tx_data = new CQueuedData(EC_SOCKET_BUFFER_SIZE);
	}
	return true;
}

//
// Packet I/O
//

void CECSocket::WritePacket(const CECPacket *packet)
{
	if (Error() && (LastError() != wxSOCKET_NOERROR)) {
		return;
	}

	uint32 flags = 0x20;

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
	    
	    m_z.next_in = m_in_ptr;
	    
		int zerror = deflateInit(&m_z, EC_COMPRESSION_LEVEL);
		if (zerror != Z_OK) {
			// don't use zlib if init failed
			flags &= ~EC_FLAG_ZLIB;
			ShowZError(zerror, &m_z);
		}
	}

	uint32 tmp_flags = ENDIAN_HTONL(flags);
	WriteBufferToSocket(&tmp_flags, sizeof(uint32));
	
	// preallocate 4 bytes in buffer for packet length
	uint32 packet_len = 0;
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
	if ( !first_buff ) first_buff = m_curr_tx_data;
	first_buff->WriteAt(&packet_len, sizeof(uint32), sizeof(uint32));
	
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

	uint32 flags = m_rx_flags;
	
	if ( ((flags & 0x60) != 0x20) || (flags & EC_FLAG_UNKNOWN_MASK) ) {
		// Protocol error - other end might use an older protocol
		fputs((const char *)unicode2char((wxString)(CFormat(_("ReadPacket: packet have invalid flags %08x")) % flags)), stdout);
		Close();
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
			fputs((const char *)unicode2char(_("ReadPacket: failed zlib init")), stdout);
			Close();
			return 0;
		}
	}

    m_curr_rx_data->ToZlib(m_z);
	packet = new CECPacket(*this);
	packet->ReadFromSocket(*this);
	
	if (packet->m_error != 0) {
		fputs((const char *)unicode2char((wxString::Format(_("ReadPacket: error %d in packet read"), packet->m_error))), stdout);
		delete packet;
		packet = NULL;
		Close();
	}

	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &m_z);
			fputs((const char *)unicode2char(_("ReadPacket: failed zlib free")), stdout);
			Close();
		}
	}

	return packet;
}

const CECPacket *CECSocket::OnPacketReceived(const CECPacket *)
{
	return 0;
}

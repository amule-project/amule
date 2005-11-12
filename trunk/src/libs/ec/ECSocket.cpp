//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#include "ECSocket.h"

#include <memory>			// Needed for auto_ptr
using std::auto_ptr;

#include <wx/intl.h>			// Needed for i18n

#include "ECVersion.h"		// Needed for EC_VERSION_ID
#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR
#include "ArchSpecific.h"	// Needed for ENDIAN_NTOHL

#include "ECcodes.h"		// Needed for the EC_FLAG_* values
#include "ECPacket.h"		// Needed for CECPacket
#include "zlib.h"		// Needed for packet (de)compression
#include "cstring"		// Needed for memcpy()/memmove()

#define EC_SOCKET_BUFFER_SIZE	32768*4
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

wxString GetSocketError(wxSocketError code)
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

unsigned int ReadBufferFromSocket(wxSocketBase *sock, void *buffer, unsigned int required_len, unsigned int max_len, wxSocketError *ErrorCode)
{
	unsigned int LastIO;
	unsigned int ReadSoFar = 0;
	char *iobuf = (char *)buffer;
	bool error = sock->Error();
/*
	bool FirstRead = true;
*/
	wxSocketError LastErrorValue = sock->LastError();

	while (((required_len == 0) || (required_len > ReadSoFar)) && !error) {
		/*
		 * lfroen: commenting this out becouse it doesn't work this way on gui builds. On wxGTK
		 * any call to WaitFor<X> will eventually call Yield. As a result, if socket call initiated
		 * by some gui action (selecting a menu), wxYield is called recoursively.
		 * So, furure TODO: get rid of WaitFor<X> on gui builds. Thats an only way.
		 */
/*
		//
		// Give socket a 10 sec chance to recv more data.
		if (FirstRead && (required_len != max_len)) {
			FirstRead = false;
			if ( !sock->WaitForRead(0, 0) ) {
				LastErrorValue = sock->LastError();
				if ((LastErrorValue == wxSOCKET_WOULDBLOCK) || (LastErrorValue == wxSOCKET_NOERROR)) {
					if ( !sock->WaitForRead(10, 0) ) {
						error = true;
						break;
					}
				} else {
					error = true;
					break;
				}
			}
		} else {
			if ( !sock->WaitForRead(10, 0) ) {
				error = true;
				break;
			}
		}
*/
		sock->Read(iobuf, max_len);
		LastIO = sock->LastCount();
		error = sock->Error();
		if (error) {
			LastErrorValue = sock->LastError();
			if (LastErrorValue == wxSOCKET_WOULDBLOCK) {
				error = false;
				LastErrorValue = wxSOCKET_NOERROR;
			}
			continue;
		}
		ReadSoFar += LastIO;
		iobuf += LastIO;
		max_len -= LastIO;
		if (required_len == 0) {
			break;
		}
	}
	if (error) {
		if (ErrorCode) *ErrorCode = LastErrorValue;
#ifdef __DEBUG__
		wxString msg = GetSocketError(LastErrorValue);
		//
		// some better logging must be here
		printf("CECSocket::ReadBufferFromSocket error %s\n", (const char *)unicode2char(msg));
#endif
	} else {
		if (ErrorCode) *ErrorCode = wxSOCKET_NOERROR;
	}
	return ReadSoFar;
}


unsigned int WriteBufferToSocket(wxSocketBase *sock, const void *buffer, unsigned int len, wxSocketError *ErrorCode)
{
	unsigned int msgRemain = len;
	unsigned int LastIO;
	unsigned int WroteSoFar = 0;
	const char *iobuf = (const char *)buffer;
	bool error = false;
	wxSocketError LastErrorValue = wxSOCKET_NOERROR;

	while(msgRemain && !error) {
		//
		// Give socket a 10 sec chance to send more data.
		if ( !sock->WaitForWrite(10, 0) ) {
			LastErrorValue = sock->LastError();
			if (LastErrorValue == wxSOCKET_WOULDBLOCK) {
				LastErrorValue = wxSOCKET_NOERROR;
				continue;
			}
			error = true;
			break;
		}
		sock->Write(iobuf, msgRemain);
		LastIO = sock->LastCount();
		error = sock->Error();
		if (error) {
			LastErrorValue = sock->LastError();
			if (LastErrorValue == wxSOCKET_WOULDBLOCK) {
				LastErrorValue = wxSOCKET_NOERROR;
				error = false;
				continue;
			}
		}
		msgRemain -= LastIO;
		WroteSoFar += LastIO;
		iobuf += LastIO;
	}
	if (error) {
		if (ErrorCode) *ErrorCode = LastErrorValue;
#ifdef __DEBUG__
		wxString msg = GetSocketError(LastErrorValue);
		//
		// some better logging must be here
		printf("CECSocket::WriteBufferToSocket error %s\n", (const char *)unicode2char(msg));
#endif
	} else {
		if (ErrorCode) *ErrorCode = wxSOCKET_NOERROR;
	}
	return WroteSoFar;
}


////////////////////////////////////////////////////////
//////////       CECSocketHandler        ///////////////
////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CECSocketHandler, wxEvtHandler)
        EVT_SOCKET(EC_SOCKET_HANDLER, CECSocketHandler::SocketHandler)
END_EVENT_TABLE()

void CECSocketHandler::SocketHandler(wxSocketEvent& event)
{
        CECSocket *socket = dynamic_cast<CECSocket*>(event.GetSocket());
        wxASSERT(socket);
        if (!socket) {
			return;
        }

        switch(event.GetSocketEvent()) {
                case wxSOCKET_LOST:
                        socket->OnClose();
                        break;
                case wxSOCKET_INPUT:
                        socket->OnReceive();
                        break;
                case wxSOCKET_OUTPUT:
                        socket->OnSend();
                        break;
                case wxSOCKET_CONNECTION:
                        socket->OnConnect();
                        break;
                default:
                        // Nothing should arrive here...
                        wxASSERT(0);
                        break;
        }
}

////////////////////////////////////////////////////////
////////////         CECSocket      ////////////////////
////////////////////////////////////////////////////////

CECSocket::CECSocket(void) : wxSocketClient()
{
	parms.firsttransfer = true;
	parms.accepts = 0;
	parms.in_ptr = NULL;
	parms.out_ptr = NULL;
	parms.LastSocketError = wxSOCKET_NOERROR;
	parms.used_flags = 0;
	parms.z.zalloc = Z_NULL;
	parms.z.zfree = Z_NULL;
	parms.z.opaque = Z_NULL;
	SetEventHandler(handler,EC_SOCKET_HANDLER);
	SetNotify(
		wxSOCKET_CONNECTION_FLAG |
		wxSOCKET_INPUT_FLAG |
		wxSOCKET_OUTPUT_FLAG |
		wxSOCKET_LOST_FLAG);
	Notify(true);
}

CECSocket::~CECSocket(void)
{
	if (parms.in_ptr) {
		delete [] parms.in_ptr;
	}
	if (parms.out_ptr) {
		delete [] parms.out_ptr;
	}
}



void CECSocket::InitBuffers()
{
	if (!parms.in_ptr) {
		parms.in_ptr = new unsigned char[EC_SOCKET_BUFFER_SIZE];
	}
	if (!parms.out_ptr) {
		parms.out_ptr = new unsigned char[EC_SOCKET_BUFFER_SIZE];
	}

	wxASSERT(parms.in_ptr && parms.out_ptr);

	parms.z.next_in = parms.in_ptr;
	parms.z.avail_in = 0;
	parms.z.total_in = 0;
	parms.z.next_out = parms.out_ptr;
	parms.z.avail_out = EC_SOCKET_BUFFER_SIZE;
	parms.z.total_out = 0;
}

void CECSocket::OnConnect()
{

}

void CECSocket::OnSend()
{
	while ( !m_pending_tx.empty() ) {
		EC_OUTBUF &buf = m_pending_tx.front();
		
		int write_count = buf.m_size - (buf.m_current - buf.m_buf);
		Write(buf.m_current, write_count);
		int written_count = LastCount();
		if ( write_count == written_count ) {
			delete [] buf.m_buf;
			m_pending_tx.pop_front();
		} else {
			buf.m_current += written_count;
			if ( Error() ) {
				if ( LastError() == wxSOCKET_WOULDBLOCK ) {
					break;
				} else {
					OnError();
					return;
				}
			}
		}
	}
}

void CECSocket::OnReceive()
{
	Read(m_curr_ptr, m_bytes_left);
	int recv_count = LastCount();
	if ( Error() || !recv_count ) {
		if ( LastError() == wxSOCKET_WOULDBLOCK ) {
			return;
		} else {
			OnError();
			return;
		}
	}
	m_curr_ptr += recv_count;
}

void CECSocket::OnClose()
{

}

void CECSocket::OnError()
{
	OnClose();
}

bool CECSocket::ReadNumber(void *buffer, unsigned int len)
{
	if (parms.used_flags & EC_FLAG_UTF8_NUMBERS) {
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


bool CECSocket::WriteNumber(const void *buffer, unsigned int len)
{
	if (parms.used_flags & EC_FLAG_UTF8_NUMBERS) {
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


bool CECSocket::ReadBuffer(void *buffer, unsigned int len)
{
	if (parms.used_flags & EC_FLAG_ZLIB) {
		// using zlib compressed i/o
		// Here we pretty much misuse the z.next_out and z.avail_out values,
		// but it doesn't matter as long a we always call inflate() with an
		// empty output buffer.
		while (parms.z.avail_out < len) {
			if (parms.z.avail_out) {
				memcpy(buffer, parms.z.next_out, parms.z.avail_out);
				buffer = (unsigned char *)buffer + parms.z.avail_out;
				len -= parms.z.avail_out;
			}
			// consumed all output
			parms.z.next_out = parms.out_ptr;
			parms.z.avail_out = EC_SOCKET_BUFFER_SIZE;
			unsigned min_read = 1;
			if (parms.z.avail_in) {
				memmove(parms.in_ptr, parms.z.next_in, parms.z.avail_in);
				min_read = 0;
			}
			parms.z.next_in = parms.in_ptr;
			parms.z.avail_in += ReadBufferFromSocket(this, parms.z.next_in + parms.z.avail_in, min_read, EC_SOCKET_BUFFER_SIZE - parms.z.avail_in, &parms.LastSocketError);
			if (parms.LastSocketError != wxSOCKET_NOERROR) {
				return false;
			}
			int zerror = inflate(&parms.z, Z_SYNC_FLUSH);
			if ((zerror != Z_OK) && (zerror != Z_STREAM_END)) {
				ShowZError(zerror, &parms.z);
				return false;
			}
			parms.z.next_out = parms.out_ptr;
			parms.z.avail_out = EC_SOCKET_BUFFER_SIZE - parms.z.avail_out;
		}
		memcpy(buffer, parms.z.next_out, len);
		parms.z.next_out += len;
		parms.z.avail_out -= len;
		return true;
	} else {
		// using uncompressed buffered i/o
		if (parms.z.avail_in < len) {
			// get more data
			if (parms.z.avail_in) {
				memcpy(buffer, parms.z.next_in, parms.z.avail_in);
				len -= parms.z.avail_in;
				buffer = (Bytef*)buffer + parms.z.avail_in;
			}
			if (len >= EC_SOCKET_BUFFER_SIZE) {
				// read directly to app mem, to avoid unnecessary memcpy()s
				parms.z.avail_in = 0;
				parms.z.next_in = parms.in_ptr;
				return ReadBufferFromSocket(this, buffer, len, len, &parms.LastSocketError) == len;
			} else {
				parms.z.avail_in = ReadBufferFromSocket(this, parms.in_ptr, len, EC_SOCKET_BUFFER_SIZE, &parms.LastSocketError);
				parms.z.next_in = parms.in_ptr;
				if (parms.LastSocketError != wxSOCKET_NOERROR) {
					parms.z.avail_in = 0;
					return false;
				}
			}
		}
		memcpy(buffer, parms.z.next_in, len);
		parms.z.next_in += len;
		parms.z.avail_in -= len;
		return true;
	}
}


bool CECSocket::WriteBuffer(const void *buffer, unsigned int len)
{
	unsigned int remain_in;

	if (parms.used_flags & EC_FLAG_ZLIB) {
		// using zlib compressed i/o
		while ((remain_in = (EC_SOCKET_BUFFER_SIZE - (parms.z.next_in - parms.in_ptr) - parms.z.avail_in)) < len) {
			memcpy(parms.z.next_in + parms.z.avail_in, buffer, remain_in);
			buffer = (Bytef*)buffer + remain_in;
			len -= remain_in;
			parms.z.avail_in += remain_in;
			int zerror = deflate(&parms.z, Z_NO_FLUSH);
			if ( zerror != Z_OK ) {
				ShowZError(zerror, &parms.z);
				return false;
			}
			if (!parms.z.avail_out) {
				WriteBufferToSocket(this, parms.out_ptr, EC_SOCKET_BUFFER_SIZE, &parms.LastSocketError);
				if (parms.LastSocketError != wxSOCKET_NOERROR) {
					return false;
				}
				parms.z.next_out = parms.out_ptr;
				parms.z.avail_out = EC_SOCKET_BUFFER_SIZE;
			}
			if (parms.z.next_in != parms.in_ptr) {
				if (parms.z.avail_in) {
					memmove(parms.in_ptr, parms.z.next_in, parms.z.avail_in);
				}
				parms.z.next_in = parms.in_ptr;
			}
		}
		memcpy(parms.z.next_in + parms.z.avail_in, buffer, len);
		parms.z.avail_in += len;
		return true;
	} else {
		// using uncompressed buffered i/o
		if (parms.z.avail_out < len) {
			// send some data
			if (parms.z.avail_out) {
				memcpy(parms.z.next_out, buffer, parms.z.avail_out);
				len -= parms.z.avail_out;
				buffer = (Bytef*)buffer + parms.z.avail_out;
			}
			parms.z.next_out = parms.out_ptr;
			parms.z.avail_out = EC_SOCKET_BUFFER_SIZE;
			WriteBufferToSocket(this, parms.out_ptr, EC_SOCKET_BUFFER_SIZE, &parms.LastSocketError);
			if (parms.LastSocketError != wxSOCKET_NOERROR) {
				return false;
			}
			if (len >= EC_SOCKET_BUFFER_SIZE) {
				// direct write from app mem, to avoid unnecessary memcpy()s
				return WriteBufferToSocket(this, buffer, len, &parms.LastSocketError) == len;
			}
		}
		memcpy(parms.z.next_out, buffer, len);
		parms.z.next_out += len;
		parms.z.avail_out -= len;
		return true;
	}
}

bool CECSocket::FlushBuffers()
{
	if (parms.used_flags & EC_FLAG_ZLIB) {
		int zerror = Z_OK;
		while (zerror == Z_OK) {
			zerror = deflate(&parms.z, Z_FINISH);
			if (WriteBufferToSocket(this, parms.out_ptr, EC_SOCKET_BUFFER_SIZE - parms.z.avail_out, &parms.LastSocketError) == EC_SOCKET_BUFFER_SIZE - parms.z.avail_out) {
				parms.z.next_out = parms.out_ptr;
				parms.z.avail_out = EC_SOCKET_BUFFER_SIZE;
			} else {
				return false;
			}
		}
		if (zerror == Z_STREAM_END) return true;
		else {
			ShowZError(zerror, &parms.z);
			return false;
		}
	} else {
		if (parms.z.avail_out != EC_SOCKET_BUFFER_SIZE) {
			bool retval = (WriteBufferToSocket(this, parms.out_ptr, EC_SOCKET_BUFFER_SIZE - parms.z.avail_out, &parms.LastSocketError) == EC_SOCKET_BUFFER_SIZE - parms.z.avail_out);
			parms.z.next_out = parms.out_ptr;
			parms.z.avail_out = EC_SOCKET_BUFFER_SIZE;
			return retval;
		}
	}
	// report success if there's nothing to do
	return true;
}

/**
 * Reads FLAGS value from given socket.
 */
uint32 CECSocket::ReadFlags()
{
	int i = 0;
	uint32 flags = 0;
	uint8 b;

	do {
		//if (!ReadBuffer(&b, 1, NULL)) return 0;
		if (!ReadBufferFromSocket(this, &b, 1, 1, NULL)) return 0;
		flags += (uint32)b << i;
		i += 8;
	} while ((b & 0x80) && (i < 32));
	return flags;
}

/**
 * Writes FLAGS value to given socket.
 */
bool CECSocket::WriteFlags(uint32 flags)
{
	uint8 b;

	flags |= 0x20;		// Setting bit 5
	flags &= 0xffffffbf;	// Clearing bit 6
	do {
		b = flags & 0xff;
		flags >>= 8;
		if (flags) b |= 0x80;
		//if (!WriteBuffer(sock, &b, 1, NULL)) return false;
		if (!WriteBufferToSocket(this, &b, 1, NULL)) return false;
	} while (flags);
	return true;
}

/**
 * Writes \e packet to the socket.
 *
 * Writes a given packet to the socket, taking care of
 * all transmission aspects (e.g. compression).
 *
 * @param packet CECPacket class instance to be written.
 *
 * @return \b true on success, \b false on failure.
 */
bool CECSocket::WritePacket(const CECPacket *packet)
{
	
	if (!IsConnected()) {
		return false;
	}
	
	uint32 flags = 0x20;
	uint32 accepted_flags = 0x20 | EC_FLAG_ZLIB | EC_FLAG_UTF8_NUMBERS;

	if (packet->GetPacketLength() > EC_MAX_UNCOMPRESSED) flags |= EC_FLAG_ZLIB;
	else flags |= EC_FLAG_UTF8_NUMBERS;

	InitBuffers();

	// ensure we won't use anything the other end cannot accept
	flags &= parms.accepts;
	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateInit(&parms.z, EC_COMPRESSION_LEVEL);
		if (zerror != Z_OK) {
			// don't use zlib if init failed
			flags &= ~EC_FLAG_ZLIB;
			ShowZError(zerror, &parms.z);
		}
	}
	parms.used_flags = flags;
	if (parms.firsttransfer) {
		parms.firsttransfer = false;
		flags |= EC_FLAG_ACCEPTS;
		if (!WriteFlags(flags)) return false;
		if (!WriteFlags(accepted_flags)) return false;
	} else {
		if (!WriteFlags(flags)) return false;
	}
	bool retval = packet->WritePacket(*this) && FlushBuffers();
	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateEnd(&parms.z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &parms.z);
			return false;
		}
	}
	return retval;
}

/**
 * Reads a CECPacket packet from the socket.
 *
 * Reads a packet from the socket, taking care of
 * all transmission aspects (e.g. compression).
 *
 * @return A pointer to a CECPacket class instance.
 *
 * \note You must later free the packet by calling
 * \b \c delete on the returned pointer.
 */
CECPacket * CECSocket::ReadPacket()
{
	if (!IsConnected()) {
		return NULL;
	}
	
	uint32 flags = ReadFlags();

	if ((flags & 0x60) != 0x20) {
		// Protocol error - other end might use an older protocol
		return NULL;
	}

	// check if the other end sends an "accepts" value
	if (flags & EC_FLAG_ACCEPTS) {
		parms.accepts = ReadFlags();
	}

	if ((flags & EC_FLAG_UNKNOWN_MASK)) {
		// Received a packet with an unknown extension
		return NULL;
	}

	InitBuffers();
	parms.used_flags = flags;
	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateInit(&parms.z);
		if (zerror != Z_OK) {
			// unable to uncompress compressed input
			ShowZError(zerror, &parms.z);
			return NULL;
		} else {
			// misusing z parameters - read more at ReadBuffer()
			parms.z.avail_out = 0;
		}
	}
	CECPacket *p = new CECPacket(*this);
#ifndef KEEP_PARTIAL_PACKETS
	if (p->m_error != 0) {
		delete p;
		p = NULL;
	}
#endif
	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateEnd(&parms.z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &parms.z);
		}
	}
	return p;
}

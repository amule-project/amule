//
// This file is part of the aMule Project.
//
// Copyright (c) 2004 aMule Team ( http://www.amule-project.net )
// Copyright (c) 2004 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ECSocket.h"
#endif

#include "ECSocket.h"

#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR
#include "ArchSpecific.h" // Needed for ENDIAN_NTOHL

#include "ECcodes.h"		// Needed for the EC_FLAG_* values
#include "ECPacket.h"		// Needed for CECPacket
#include "zlib.h"		// Needed for packet (de)compression
#include "string.h"		// Needed for memset()
#include "stdlib.h"		// Needed for malloc()/free()

#include "StringFunctions.h"	// Needed for unicode2char()

#define EC_SOCKET_BUFFER_SIZE	32768
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

/*----------=> Socket Registry <=----------*/
struct socket_desc {
	// Global values (for whole session)
	struct socket_desc*	next;
	wxSocketBase *		socket;
	bool			firsttransfer;
	uint32			accepts;
	bool			ptrs_valid;
	void *			in_ptr;
	void *			out_ptr;
	// This transfer only
	wxSocketError	LastSocketError;
	uint32			used_flags;
	z_stream		z;
};

static socket_desc* s_registered_sockets = NULL;
static wxMutex s_registry_lock;

void RegisterSocket(wxSocketBase *socket)
{
	struct socket_desc *p = new struct socket_desc;
	memset(p, 0, sizeof(struct socket_desc));
	p->socket = socket;
	p->firsttransfer = true;
	// Just in case, if it's defined something else than zero
	p->LastSocketError = wxSOCKET_NOERROR;
// Just in case
#if Z_NULL != 0
	p->z.zalloc = Z_NULL;
	p->z.zfree = Z_NULL;
#endif

	s_registry_lock.Lock();
	p->next = s_registered_sockets;
	s_registered_sockets = p;
	s_registry_lock.Unlock();
}

void UnregisterSocket(wxSocketBase *socket)
{
	struct socket_desc *p;
	struct socket_desc *pp;

	s_registry_lock.Lock();
	p = pp = s_registered_sockets;
	while (p) {
		if (p->socket == socket) break;
		pp = p;
		p = p->next;
	}
	if (p) {
		if (p == s_registered_sockets) {
			s_registered_sockets = p->next;
		} else {
			pp->next = p->next;
		}
		free(p->in_ptr);
		free(p->out_ptr);
		delete p;
	}
	s_registry_lock.Unlock();
}

struct socket_desc *FindSocket(wxSocketBase *socket)
{
	s_registry_lock.Lock();
	struct socket_desc *p = s_registered_sockets;
	while (p) {
		if (p->socket == socket) break;
		p = p->next;
	}
	s_registry_lock.Unlock();
	return p;
}
/*----------=> End of Socket Registry <=----------*/

void InitBuffers(struct socket_desc * parms)
{
	if (!parms->in_ptr) {
		parms->in_ptr = malloc(EC_SOCKET_BUFFER_SIZE);
	}
	if (!parms->out_ptr) {
		parms->out_ptr = malloc(EC_SOCKET_BUFFER_SIZE);
	}
	if (parms->in_ptr && parms->out_ptr) {
		parms->ptrs_valid = true;
		parms->z.next_in = (Bytef*)parms->in_ptr;
		parms->z.avail_in = 0;
		parms->z.total_in = 0;
		parms->z.next_out = (Bytef*)parms->out_ptr;
		parms->z.avail_out = EC_SOCKET_BUFFER_SIZE;
		parms->z.total_out = 0;
#ifdef __DEBUG__
	} else {
		printf("EC socket buffer allocation error, using direct socket operations.\n");
#endif
	}
}

#ifdef __DEBUG__
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

// note that args value MUST be in brackets!
#define CALL_Z_FUNCTION(func, args)	{	\
	int zerror = func args ;		\
	if (zerror != Z_OK) {			\
		ShowZError(zerror, &parms->z);	\
	}					\
}

#else

#define ShowZError(x,y)
#define CALL_Z_FUNCTION(func, args)	func args

#endif /* __DEBUG__ */

unsigned int ReadBufferFromSocket(wxSocketBase *sock, void *buffer, unsigned int required_len, unsigned int max_len, wxSocketError *ErrorCode)
{
	unsigned int LastIO;
	unsigned int ReadSoFar = 0;
	char *iobuf = (char *)buffer;
	bool error = sock->Error();
	bool FirstRead = true;
	wxSocketError LastErrorValue = sock->LastError();

	while ((required_len > ReadSoFar) && !error) {
		/*
		 * lfroen: commenting this out becouse it doesn't work this way on gui builds. On wxGTK
		 * any call to WaitFor<X> will eventually call Yield. As a result, if socket call initiated
		 * by some gui action (selecting a menu), wxYield is called recoursively.
		 * So, furure TODO: get rid of WaitFor<X> on gui builds. Thats an only way.
		 */
#ifndef CLIENT_GUI
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
#endif
		sock->Read(iobuf, max_len);
		LastIO = sock->LastCount();
		error = sock->Error();
		if (error) LastErrorValue = sock->LastError();
		ReadSoFar += LastIO;
		iobuf += LastIO;
		max_len -= LastIO;
	}
	if (error) {
		if (ErrorCode) *ErrorCode = LastErrorValue;
#ifdef __DEBUG__
		wxString msg = GetSocketError(LastErrorValue);
		//
		// some better logging must be here
		printf("ECSocket::ReadBufferFromSocket error %s\n", (const char *)unicode2char(msg));
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
	bool error = sock->Error();
	wxSocketError LastErrorValue = sock->LastError();

	while(msgRemain && !error) {
		//
		// Give socket a 10 sec chance to send more data.
		if ( !sock->WaitForWrite(10, 0) ) {
			LastErrorValue = sock->LastError();
			error = true;
			break;
		}
		sock->Write(iobuf, msgRemain);
		LastIO = sock->LastCount();
		error = sock->Error();
		if (error) LastErrorValue = sock->LastError();
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
		printf("ECSocket::WriteBufferToSocket error %s\n", (const char *)unicode2char(msg));
#endif
	} else {
		if (ErrorCode) *ErrorCode = wxSOCKET_NOERROR;
	}
	return WroteSoFar;
}

ECSocket::ECSocket(void)
{
	m_type = AMULE_EC_CLIENT;
	m_sock = new wxSocketClient();
	RegisterSocket(m_sock);
}


ECSocket::ECSocket(wxSockAddress& address, wxEvtHandler *handler, int id)
{
	m_type = AMULE_EC_SERVER;
	m_sock = new wxSocketServer(address, wxSOCKET_REUSEADDR);
	if(m_sock->Ok() && handler) {
		// Setup the event handler and subscribe to connection events
		m_sock->SetEventHandler(*handler, id);
		m_sock->SetNotify(wxSOCKET_CONNECTION_FLAG);
		m_sock->Notify(true);
	}
	RegisterSocket(m_sock);
}


ECSocket::~ECSocket(void)
{
	UnregisterSocket(m_sock);
	delete m_sock;
}


bool ECSocket::ReadNumber(wxSocketBase *sock, void *buffer, unsigned int len, void *opaque)
{
	if (((struct socket_desc *)opaque)->used_flags & EC_FLAG_UTF8_NUMBERS) {
		unsigned char mb[6];
		uint32 wc;
		if (!ReadBuffer(sock, mb, 1, opaque)) return false;
		int remains = utf8_mb_remain(mb[0]);
		if (remains) if (!ReadBuffer(sock, &(mb[1]), remains, opaque)) return false;
		if (utf8_mbtowc(&wc, mb, 6) == -1) return false;	// Invalid UTF-8 code sequence
		switch (len) {
			case 1: PokeUInt8( buffer,  wc ); break;
			case 2:	RawPokeUInt16( buffer, wc ); break;
			case 4: RawPokeUInt32( buffer, wc ); break;
		}
	} else {
		if ( !ReadBuffer(sock, buffer, len, opaque) ) {
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


bool ECSocket::WriteNumber(wxSocketBase *sock, const void *buffer, unsigned int len, void *opaque)
{
	if (((struct socket_desc *)opaque)->used_flags & EC_FLAG_UTF8_NUMBERS) {
		unsigned char mb[6];
		uint32 wc;
		int mb_len;
		switch (len) {
			case 1: wc = PeekUInt8( buffer ); break;
			case 2: wc = RawPeekUInt16( buffer ); break;
			case 4: wc = RawPeekUInt32( buffer ); break;
		}
		if ((mb_len = utf8_wctomb(mb, wc, 6)) == -1) return false;	// Something is terribly wrong...
		return WriteBuffer(sock, mb, mb_len, opaque);
	} else {
		char tmp[8];

		switch (len) {
			case 1: PokeUInt8( tmp, PeekUInt8( buffer ) ); break;
			case 2: RawPokeUInt16( tmp, ENDIAN_NTOHS( RawPeekUInt16( buffer ) ) ); break;
			case 4: RawPokeUInt32( tmp, ENDIAN_NTOHL( RawPeekUInt32( buffer ) ) ); break;
		}
		return WriteBuffer(sock, tmp, len, opaque);
	}
}


bool ECSocket::ReadBuffer(wxSocketBase *sock, void *buffer, unsigned int len, void *opaque)
{
	struct socket_desc *parms = (struct socket_desc *)opaque;

	if (parms) {
		if (parms->ptrs_valid) {
			if (parms->used_flags & EC_FLAG_ZLIB) {
				// using zlib compressed i/o
				// Here we pretty much misuse the z.next_out and z.avail_out values,
				// but it doesn't matter as long a we always call inflate() with an
				// empty output buffer.
				while (parms->z.avail_out < len) {
					if (parms->z.avail_out) {
						memcpy(buffer, parms->z.next_out, parms->z.avail_out);
						buffer = (Bytef*)buffer + parms->z.avail_out;
						len -= parms->z.avail_out;
					}
					// consumed all output
					parms->z.next_out = (Bytef *)parms->out_ptr;
					parms->z.avail_out = EC_SOCKET_BUFFER_SIZE;
					unsigned min_read = 1;
					if (parms->z.avail_in) {
						memmove(parms->in_ptr, parms->z.next_in, parms->z.avail_in);
						min_read = 0;
					}
					parms->z.next_in = (Bytef *)parms->in_ptr;
					parms->z.avail_in += ReadBufferFromSocket(sock, parms->z.next_in + parms->z.avail_in, min_read, EC_SOCKET_BUFFER_SIZE - parms->z.avail_in, &parms->LastSocketError);
					if (parms->LastSocketError != wxSOCKET_NOERROR) {
						return false;
					}
					int zerror = inflate(&parms->z, Z_SYNC_FLUSH);
					if ((zerror != Z_OK) && (zerror != Z_STREAM_END)) {
						ShowZError(zerror, &parms->z);
						return false;
					}
					parms->z.next_out = (Bytef *)parms->out_ptr;
					parms->z.avail_out = EC_SOCKET_BUFFER_SIZE - parms->z.avail_out;
				}
				memcpy(buffer, parms->z.next_out, len);
				parms->z.next_out += len;
				parms->z.avail_out -= len;
				return true;
			} else {
				// using uncompressed buffered i/o
				if (parms->z.avail_in < len) {
					// get more data
					if (parms->z.avail_in) {
						memcpy(buffer, parms->z.next_in, parms->z.avail_in);
						len -= parms->z.avail_in;
						buffer = (Bytef*)buffer + parms->z.avail_in;
					}
					if (len >= EC_SOCKET_BUFFER_SIZE) {
						// read directly to app mem, to avoid unnecessary memcpy()s
						parms->z.avail_in = 0;
						parms->z.next_in = (Bytef *)parms->in_ptr;
						return ReadBufferFromSocket(sock, buffer, len, len, &parms->LastSocketError) == len;
					} else {
						parms->z.avail_in = ReadBufferFromSocket(sock, parms->in_ptr, len, EC_SOCKET_BUFFER_SIZE, &parms->LastSocketError);
						parms->z.next_in = (Bytef *)parms->in_ptr;
						if (parms->LastSocketError != wxSOCKET_NOERROR) {
							parms->z.avail_in = 0;
							return false;
						}
					}
				}
				memcpy(buffer, parms->z.next_in, len);
				parms->z.next_in += len;
				parms->z.avail_in -= len;
				return true;
			}
		} else {
			// No valid buffers, using direct socket i/o
			return ReadBufferFromSocket(sock, buffer, len, len, &parms->LastSocketError) == len;
		}
	} else {
		// Requested direct socket i/o
		return ReadBufferFromSocket(sock, buffer, len, len, NULL) == len;
	}
}


bool ECSocket::WriteBuffer(wxSocketBase *sock, const void *buffer, unsigned int len, void *opaque)
{
	struct socket_desc *parms = (struct socket_desc *)opaque;
	unsigned int remain_in;

	if (parms) {
		if (parms->ptrs_valid) {
			if (parms->used_flags & EC_FLAG_ZLIB) {
				// using zlib compressed i/o
				while ((remain_in = (EC_SOCKET_BUFFER_SIZE - (parms->z.next_in - (Bytef *)parms->in_ptr) - parms->z.avail_in)) < len) {
					memcpy(parms->z.next_in + parms->z.avail_in, buffer, remain_in);
					buffer = (Bytef*)buffer + remain_in;
					len -= remain_in;
					parms->z.avail_in += remain_in;
					CALL_Z_FUNCTION(deflate, (&parms->z, Z_NO_FLUSH));
					if (!parms->z.avail_out) {
						WriteBufferToSocket(sock, parms->out_ptr, EC_SOCKET_BUFFER_SIZE, &parms->LastSocketError);
						if (parms->LastSocketError != wxSOCKET_NOERROR) {
							return false;
						}
						parms->z.next_out = (Bytef *)parms->out_ptr;
						parms->z.avail_out = EC_SOCKET_BUFFER_SIZE;
					}
					if (parms->z.next_in != parms->in_ptr) {
						if (parms->z.avail_in) {
							memmove(parms->in_ptr, parms->z.next_in, parms->z.avail_in);
						}
						parms->z.next_in = (Bytef *)parms->in_ptr;
					}
				}
				memcpy(parms->z.next_in + parms->z.avail_in, buffer, len);
				parms->z.avail_in += len;
				return true;
			} else {
				// using uncompressed buffered i/o
				if (parms->z.avail_out < len) {
					// send some data
					if (parms->z.avail_out) {
						memcpy(parms->z.next_out, buffer, parms->z.avail_out);
						len -= parms->z.avail_out;
						buffer = (Bytef*)buffer + parms->z.avail_out;
					}
					parms->z.next_out = (Bytef *)parms->out_ptr;
					parms->z.avail_out = EC_SOCKET_BUFFER_SIZE;
					WriteBufferToSocket(sock, parms->out_ptr, EC_SOCKET_BUFFER_SIZE, &parms->LastSocketError);
					if (parms->LastSocketError != wxSOCKET_NOERROR) {
						return false;
					}
					if (len >= EC_SOCKET_BUFFER_SIZE) {
						// direct write from app mem, to avoid unnecessary memcpy()s
						return WriteBufferToSocket(sock, buffer, len, &parms->LastSocketError) == len;
					}
				}
				memcpy(parms->z.next_out, buffer, len);
				parms->z.next_out += len;
				parms->z.avail_out -= len;
				return true;
			}
		} else {
			// No valid buffers, using direct socket i/o
			return WriteBufferToSocket(sock, buffer, len, &parms->LastSocketError) == len;
		}
	} else {
		// Requested direct socket i/o
		return WriteBufferToSocket(sock, buffer, len, NULL) == len;
	}
}

bool FlushBuffers(struct socket_desc *parms)
{
	if (parms) {
		if (parms->ptrs_valid) {
			if (parms->used_flags & EC_FLAG_ZLIB) {
				int zerror = Z_OK;
				while (zerror == Z_OK) {
					zerror = deflate(&parms->z, Z_FINISH);
					if (WriteBufferToSocket(parms->socket, parms->out_ptr, EC_SOCKET_BUFFER_SIZE - parms->z.avail_out, &parms->LastSocketError) == EC_SOCKET_BUFFER_SIZE - parms->z.avail_out) {
						parms->z.next_out = (Bytef *)parms->out_ptr;
						parms->z.avail_out = EC_SOCKET_BUFFER_SIZE;
					} else {
						return false;
					}
				}
				if (zerror == Z_STREAM_END) return true;
				else {
					ShowZError(zerror, &parms->z);
					return false;
				}
			} else {
				if (parms->z.avail_out != EC_SOCKET_BUFFER_SIZE) {
					bool retval = (WriteBufferToSocket(parms->socket, parms->out_ptr, EC_SOCKET_BUFFER_SIZE - parms->z.avail_out, &parms->LastSocketError) == EC_SOCKET_BUFFER_SIZE - parms->z.avail_out);
					parms->z.next_out = (Bytef *)parms->out_ptr;
					parms->z.avail_out = EC_SOCKET_BUFFER_SIZE;
					return retval;
				}
			}
		}
	}
	// report success if there's nothing to do
	return true;
}

/**
 * Reads FLAGS value from given socket.
 */
uint32 ECSocket::ReadFlags(wxSocketBase *sock)
{
	int i = 0;
	uint32 flags = 0;
	uint8 b;

	do {
		if (!ReadBuffer(sock, &b, 1, NULL)) return 0;
		flags += (uint32)b << i;
		i += 8;
	} while ((b & 0x80) && (i < 32));
	return flags;
}

/**
 * Writes FLAGS value to given socket.
 */
bool ECSocket::WriteFlags(wxSocketBase *sock, uint32 flags)
{
	uint8 b;

	flags |= 0x20;		// Setting bit 5
	flags &= 0xffffffbf;	// Clearing bit 6
	do {
		b = flags & 0xff;
		flags >>= 8;
		if (flags) b |= 0x80;
		if (!WriteBuffer(sock, &b, 1, NULL)) return false;
	} while (flags);
	return true;
}

/**
 * Writes \e packet to \e sock.
 *
 * Writes a given packet to given socket, taking care of
 * all transmission aspects (e.g. compression).
 *
 * @param sock socket to use.
 * @param packet CECPacket class instance to be written.
 *
 * @return \b true on success, \b false on failure.
 */
bool ECSocket::WritePacket(wxSocketBase *sock, const CECPacket *packet)
{
	uint32 flags = 0x20;
	uint32 accepted_flags = 0x20 | EC_FLAG_ZLIB | EC_FLAG_UTF8_NUMBERS;
	struct socket_desc *parms = FindSocket(sock);

	if (packet->GetPacketLength() > EC_MAX_UNCOMPRESSED) flags |= EC_FLAG_ZLIB;
	else flags |= EC_FLAG_UTF8_NUMBERS;

	InitBuffers(parms);
	if (!parms->ptrs_valid) {
		// cannot use zlib without i/o buffers
		flags &= ~EC_FLAG_ZLIB;
		accepted_flags &= ~EC_FLAG_ZLIB;
	}
	// ensure we won't use anything the other end cannot accept
	flags &= parms->accepts;
	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateInit(&parms->z, EC_COMPRESSION_LEVEL);
		if (zerror != Z_OK) {
			// don't use zlib if init failed
			flags &= ~EC_FLAG_ZLIB;
			ShowZError(zerror, &parms->z);
		}
	}
	parms->used_flags = flags;
	if (parms->firsttransfer) {
		parms->firsttransfer = false;
		flags |= EC_FLAG_ACCEPTS;
		if (!WriteFlags(sock, flags)) return false;
		if (!WriteFlags(sock, accepted_flags)) return false;
	} else {
		if (!WriteFlags(sock, flags)) return false;
	}
	bool retval = packet->WritePacket(sock, *this, parms) && FlushBuffers(parms);
	if (flags & EC_FLAG_ZLIB) {
		CALL_Z_FUNCTION(deflateEnd, (&parms->z));
	}
	return retval;
}

/**
 * Reads a CECPacket packet from \e sock.
 *
 * Reads a packet from the given socket, taking care of
 * all transmission aspects (e.g. compression).
 *
 * @param sock socket to use.
 *
 * @return A pointer to a CECPacket class instance.
 *
 * \note You must later free the packet by calling
 * \b \c delete on the returned pointer.
 */
CECPacket * ECSocket::ReadPacket(wxSocketBase *sock)
{
	uint32 flags = ReadFlags(sock);
	struct socket_desc *parms = FindSocket(sock);

	if ((flags & 0x60) != 0x20) {
		// Protocol error - other end might use an older protocol
		return NULL;
	}

	// check if the other end sends an "accepts" value
	if (flags & EC_FLAG_ACCEPTS) {
		parms->accepts=ReadFlags(sock);
	}

	if ((flags & EC_FLAG_UNKNOWN_MASK)) {
		// Received a packet with an unknown extension
		return NULL;
	}

	InitBuffers(parms);
	parms->used_flags = flags;
	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateInit(&parms->z);
		if (zerror != Z_OK) {
			// unable to uncompress compressed input
			ShowZError(zerror, &parms->z);
			return NULL;
		} else {
			// misusing z parameters - read more at ReadBuffer()
			parms->z.avail_out = 0;
		}
	}
	CECPacket *p = new CECPacket(sock, *this, parms);
#ifndef KEEP_PARTIAL_PACKETS
	if (p->m_error != 0) {
		delete p;
		p = NULL;
	}
#endif
	if (flags & EC_FLAG_ZLIB) {
		CALL_Z_FUNCTION(inflateEnd, (&parms->z));
	}
	return p;
}

/*! 
 * \fn ECSocket::WritePacket(const CECPacket *packet)
 *
 * \brief Writes \e packet to the m_sock member of the class.
 *
 * Writes a given packet to given socket, taking care of
 * all transmission aspects (e.g. compression).
 *
 * \param packet CECPacket class instance to be written.
 *
 * \return \b true on success, \b false on failure.
 */

/*!
 * \fn CECPacket * ECSocket::ReadPacket(void)
 *
 * \brief Reads a CECPacket packet from the m_sock member of the class.
 *
 * Reads a packet from the given socket, taking care of
 * all transmission aspects (e.g. compression).
 *
 * \return A pointer to a CECPacket class instance.
 *
 * \note You must later free the packet by calling
 * \c delete on the returned pointer.
 */

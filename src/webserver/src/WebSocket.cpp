//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


#include "WebSocket.h"


#ifdef ENABLE_UPNP
#	include "UPnPBase.h"
#endif

CWebSocket::CWebSocket(CWebServerBase *parent)
{
	m_pHead = 0;
	m_pTail = 0;
	// Allocate one extra slot so the NUL terminator at the end of
	// OnReceive() always has a home, even if Read() exactly fills the
	// requested span and the grow loop is skipped (the off-by-one
	// scenario in #873: first Read returns m_dwBufSize bytes AND
	// LastError() is set, leaving m_dwRecv == m_dwBufSize when the
	// loop exits). The allocation is +1 byte; m_dwBufSize keeps
	// reflecting the *usable* span we hand to Read() so the
	// `m_dwBufSize - m_dwRecv` reads below still leave the spare slot
	// free for the terminator.
	m_pBuf = new char [4096 + 1];
	m_dwBufSize = 4096;
	m_dwRecv = 0;
	m_dwHttpHeaderLen = 0;
	m_dwHttpContentLen = 0;
	m_Cookie = 0;
	m_IsGet = false;
	m_IsPost = false;

	m_pParent = parent;

	Notify(true);

}

void CWebSocket::OnLost(int)
{
	Close();
	Destroy();
}

void CWebSocket::OnReceive(int)
{
	uint32 read = Read(m_pBuf + m_dwRecv, m_dwBufSize - m_dwRecv);
	m_dwRecv += read;
	while ((m_dwRecv == m_dwBufSize) && (read != 0) && (!LastError())) {
		// Buffer is too small. Make it bigger. Allocate one extra
		// slot for the NUL terminator written below, matching the
		// `+1` overhead the ctor uses (see #873).
		uint32 newsize = m_dwBufSize + (m_dwBufSize  >> 1);
		char* newbuffer = new char[newsize + 1];
		char* oldbuffer = m_pBuf;
		memcpy(newbuffer, oldbuffer, m_dwBufSize);
		delete[] oldbuffer;
		m_pBuf = newbuffer;
		m_dwBufSize = newsize;
		// And read again
		read = Read(m_pBuf + m_dwRecv, m_dwBufSize - m_dwRecv);
		m_dwRecv += read;
	}

	if (read == 0) {
		if (LastError()) {
			Close();
			return ;
		}
	}

	m_pBuf[m_dwRecv] = '\0';


	//
	// Check what kind of request is that
	if ( !m_IsGet && !m_IsPost && m_dwRecv >= 4) {
		if ( !strncasecmp(m_pBuf, "GET", 3) ) {
			m_IsGet = true;
		} else if ( !strncasecmp(m_pBuf, "POST", 4) ) {
			m_IsPost = true;
		} else {
			// unknown request - close the socket
			Close();
			return ;
		}
	}
	//
	// RFC1945:
	//

	//
	// "GET" must have last line empty
	if ( m_IsGet ) {
		if ( !strncasecmp(m_pBuf + m_dwRecv - 4, "\r\n\r\n", 4) ) {
			//
			// Process request
			OnRequestReceived(m_pBuf, 0, 0);
		}
	}
	//
	// "POST" have "Content-Length"
	if ( m_IsPost ) {
		char *cont_len = strstr(m_pBuf, "Content-Length");
		// do we have received all the line ?
		if ( cont_len && strstr(cont_len, "\r\n\r\n") ) {
			cont_len += strlen("Content-Length:");
			// can be white space following
			while ( isspace(*cont_len) ) cont_len++;
			int len = atoi(cont_len);
			if ( !len ) {
				Close();
				return ;
			}
			// do we have all of data ?
			char *cont = strstr(m_pBuf, "\r\n\r\n");
			cont += 4;
			if ( cont - m_pBuf + len <= (int)m_dwRecv ) {
				OnRequestReceived(m_pBuf, cont, len);
			}
		}
	}
}

void CWebSocket::OnSend(int)
{
	while (m_pHead && m_pHead->m_pToSend) {
		uint32 nRes = Write(m_pHead->m_pToSend, m_pHead->m_dwSize);
		if (nRes >= m_pHead->m_dwSize) {
			// erase this chunk
			CChunk* pNext = m_pHead->m_pNext;
			delete m_pHead;
			if (!(m_pHead = pNext)) {
				m_pTail = NULL;
			}
		} else {
			if (LastError()) {
				Close();
				break;
			} else if (nRes > 0) {
				m_pHead->m_pToSend += nRes;
				m_pHead->m_dwSize -= nRes;
			} else {
				// blocks
				break;
			}
		}
	}
}

void CWebSocket::OnRequestReceived(char* pHeader, char* pData, uint32 dwDataLen)
{
	bool is_post = false;
	if ( strncmp(pHeader, "GET", 3) == 0 ) {
	} else if ( strncmp(pHeader, "POST", 4) == 0 ) {
		is_post = true;
	} else {
		// invalid request
		return ;
	}
	char *path = strchr(pHeader, ' ');
	if ( !path ) {
		return;
	}
	*path++ = 0;
	pHeader = strchr(path, ' ');
	if ( !pHeader ) {
		return;
	}
	*pHeader++ = 0;

	wxString sURL(char2unicode(path));
	// Capture the URL exactly as it was on the wire, before any
	// POST-body concatenation below. The login handler in
	// CScriptWebServer::ProcessURL needs to be able to distinguish
	// "the `pass` param came from the POST body" from "the `pass`
	// param came from the URL query string"; storing the pre-concat
	// CParsedUrl gives it that signal without re-parsing or
	// regex-on-string heuristics (#872).
	wxString sOriginalURL = sURL;
	if ( is_post ) {
		// Append the POST body to the URL so CParsedUrl picks up the
		// form fields the same way it does for GET-style ?key=value
		// pairs.  Use `&` rather than `?` when the URL already has a
		// query string -- otherwise the combined string ends up as
		// `/page?a=b?pass=XYZ`, and CParsedUrl's `?`-then-`&`-split
		// truncates the first key's value to `b?pass=XYZ` and never
		// registers a `pass` entry, breaking POST login on any URL
		// that wasn't query-less (issue #724).
		wxString sData(char2unicode(pData));
		sURL += (sURL.Find('?') != wxNOT_FOUND ? "&" : "?") + sData.Left(dwDataLen);
	}

	//
	// Find session cookie.
	//
	// 64-bit so the cookie value can hold a full
	// AutoSeededRandomPool-sourced token; previously this was an
	// `int` + `atoi()` which made server-side session IDs trivially
	// guessable (#870).
	uint64_t sessid = 0;
	char *current_cookie = strstr(pHeader, "Cookie: ");
	if ( current_cookie == NULL ) {
		current_cookie = strstr(pHeader, "cookie: ");
	}
	if ( current_cookie ) {
		current_cookie = strstr(current_cookie, "amuleweb_session_id");
		if ( current_cookie ) {
			char *value = strchr(current_cookie, '=');
			if ( value ) {
				sessid = strtoull(++value, NULL, 10);
			}
		}
	}
	ThreadData Data = { CParsedUrl(sURL), CParsedUrl(sOriginalURL), sURL, sessid, this };

	wxString sFile = Data.parsedURL.File();
	if (sFile.Length() > 4 ) {
		wxString url_ext = sFile.Right( sFile.Length() - sFile.Find('.', true) ).MakeLower();
		if ( (url_ext==".gif") || (url_ext==".jpg") || (url_ext==".ico") ||
			(url_ext==".png") || (url_ext==".bmp") || (url_ext==".jpeg") ) {
			m_pParent->ProcessImgFileReq(Data);
		} else {
			m_pParent->ProcessURL(Data);
		}
	} else {
		m_pParent->ProcessURL(Data);
	}

	//
	// Done processing, reset state
	//
	m_dwRecv = 0;
	m_IsGet = 0;
	m_IsPost = 0;
}

void CWebSocket::SendContent(const char* szStdResponse, const void* pContent, uint32 dwContentSize) {
	char szBuf[0x1000]; // 0x1000 is safe because it's just used for the header
	int nLen = snprintf(szBuf, sizeof(szBuf), "HTTP/1.1 200 OK\r\n%sContent-Length: %d\r\n\r\n", szStdResponse, dwContentSize);
	SendData(szBuf, nLen);
	SendData(pContent, dwContentSize);
}

void CWebSocket::SendHttpHeaders(const char* szType, bool use_gzip, uint32 content_len, uint64_t session_id)
{
	char szBuf[0x1000];

	char cookie[256];
	if ( session_id ) {
		// HttpOnly: the cookie isn't readable from JavaScript, which
		// blunts the "steal the session via reflected XSS" path
		// (the cookie still rides on every request the browser
		// sends to amuleweb -- it just stops being readable from
		// `document.cookie` and friends).
		// SameSite=Strict: the browser refuses to attach this
		// cookie to cross-site requests, which is the lever that
		// CSRF needs in order to ride the authenticated session.
		// `Secure` is NOT set here: amuleweb has no native TLS
		// handling and doesn't know whether it's behind a TLS-
		// terminating proxy. Setting `Secure` unconditionally
		// would silently lock out every direct-HTTP user (browser
		// refuses the cookie -> infinite login loop). Wiring this
		// to a preference is a follow-up. (#871)
		snprintf(cookie, sizeof(cookie),
			"Set-Cookie: amuleweb_session_id=%llu; HttpOnly; SameSite=Strict\r\n",
			static_cast<unsigned long long>(session_id));
	} else {
		cookie[0] = 0;
	}

	snprintf(szBuf, sizeof(szBuf), "HTTP/1.1 200 OK\r\nServer: aMule\r\nPragma: no-cache\r\nExpires: 0\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"%s"
		"Connection: close\r\nContent-Type: %s\r\n"
		"Content-Length: %d\r\n%s\r\n",
		 cookie, szType, content_len, (use_gzip ? "Content-Encoding: gzip\r\n" : ""));

	SendData(szBuf, strlen(szBuf));
}

void CWebSocket::SendData(const void* pData, uint32 dwDataSize)
{
	if (!dwDataSize) {	// sanity
		return;
	}
	const char * data = (const char*) pData;
	bool outputRequired = !m_pHead;

	// push it to our tails
	CChunk* pChunk = new CChunk;
	pChunk->m_pNext = NULL;
	pChunk->m_dwSize = dwDataSize;
	pChunk->m_pData = new char[dwDataSize];
	memcpy(pChunk->m_pData, data, dwDataSize);
	// push it to the end of our queue
	pChunk->m_pToSend = pChunk->m_pData;
	if (m_pTail) {
		m_pTail->m_pNext = pChunk;
	} else {
		m_pHead = pChunk;
	}
	m_pTail = pChunk;

	if (outputRequired) {
		OnSend(0);
	}
}
// File_checked_for_headers

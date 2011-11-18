//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2011 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
	m_pBuf = new char [4096];
	m_dwBufSize = 4096;
	m_dwRecv = 0;
	m_dwHttpHeaderLen = 0;
	m_dwHttpContentLen = 0;
	m_Cookie = 0;
	m_IsGet = false;
	m_IsPost = false;
	
	m_pParent = parent;
	
	SetEventHandler(*parent, ID_WEBCLIENTSOCKET_EVENT);
	SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_LOST_FLAG);
	Notify(true);
	
}

void CWebSocket::OnError()
{
}

void CWebSocket::OnLost()
{
}

void CWebSocket::OnInput()
{
	Read(m_pBuf + m_dwRecv, m_dwBufSize - m_dwRecv);
	m_dwRecv += LastCount();
	while ((m_dwRecv == m_dwBufSize) && (LastCount()!=0) && (!Error())) {
		// Buffer is too small. Make it bigger.
		uint32 newsize = m_dwBufSize + (m_dwBufSize  >> 1);
		char* newbuffer = new char[newsize];
		char* oldbuffer = m_pBuf;
		memcpy(newbuffer, oldbuffer, m_dwBufSize);
		delete[] oldbuffer;
		m_pBuf = newbuffer;
		m_dwBufSize = newsize;
		// And read again
		Read(m_pBuf + m_dwRecv, m_dwBufSize - m_dwRecv);
		m_dwRecv += LastCount();				
	}
	
	if (LastCount() == 0) {
		if (Error()) {
			if (LastError() != wxSOCKET_WOULDBLOCK) {
				Close();
				return ;
			}
		}
	}
	
	m_pBuf[m_dwRecv] = '\0';

	
	//
	// Check what kind of request is that
	if ( !m_IsGet && !m_IsPost ) {
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
			OnOutput();
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
				OnOutput();
			}
		}
	}
}

void CWebSocket::OnOutput()
{
	while (m_pHead && m_pHead->m_pToSend) {
		Write(m_pHead->m_pToSend, m_pHead->m_dwSize);
		uint32 nRes = LastCount();
		if (nRes >= m_pHead->m_dwSize) {
			// erase this chunk
			CChunk* pNext = m_pHead->m_pNext;
			delete m_pHead;
			if (!(m_pHead = pNext)) {
				m_pTail = NULL;
			}
		} else {
			if ((nRes > 0) && (!Error())) {
				m_pHead->m_pToSend += nRes;
				m_pHead->m_dwSize -= nRes;
			} else {
				if (Error()) {
					if (LastError() != wxSOCKET_WOULDBLOCK) {
						Close();
						break;
					}
				}
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
	if ( is_post ) {
		wxString sData(char2unicode(pData));
		sURL += wxT("?") + sData.Left(dwDataLen);
	}
	
	//
	// Find session cookie.
	//
	int sessid = 0;
	char *current_cookie = strstr(pHeader, "Cookie: ");
	if ( current_cookie ) {
		current_cookie = strstr(current_cookie, "amuleweb_session_id");
		if ( current_cookie ) {
			char *value = strchr(current_cookie, '=');
			if ( value ) {
				sessid = atoi(++value);
			}			
		}
	}
	ThreadData Data = { CParsedUrl(sURL), sURL, sessid, this };

	wxString sFile = Data.parsedURL.File();
	if (sFile.Length() > 4 ) {
		wxString url_ext = sFile.Right( sFile.Length() - sFile.Find('.', true) ).MakeLower();
		if ( (url_ext==wxT(".gif")) || (url_ext==wxT(".jpg")) || (url_ext==wxT(".ico")) ||
			(url_ext==wxT(".png")) || (url_ext==wxT(".bmp")) || (url_ext==wxT(".jpeg")) ) {
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

void CWebSocket::SendHttpHeaders(const char* szType, bool use_gzip, uint32 content_len, int session_id)
{
	char szBuf[0x1000];

	char cookie[256];
	if ( session_id ) {
		snprintf(cookie, sizeof(cookie), "Set-Cookie: amuleweb_session_id=%d\r\n", session_id);
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
	const char * data = (const char*) pData;
	if (!m_pHead) {
		// try to send it directly
		Write(data, dwDataSize);
		uint32 nRes = LastCount();
		if ((nRes < dwDataSize) && 
			Error() && (LastError() != wxSOCKET_WOULDBLOCK)) {
			Close();
		} else {
			data += nRes;
			dwDataSize -= nRes;
		}
	}
	if (dwDataSize) {
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
	}
}
// File_checked_for_headers

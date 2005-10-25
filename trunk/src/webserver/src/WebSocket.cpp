//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2005 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <wx/thread.h>

#include "WebSocket.h"

#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!

#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR


WX_DEFINE_ARRAY(CWCThread*, ArrayOfCWCThread);

ArrayOfCWCThread s_wcThreads;
static wxMutex *s_mutex_wcThreads;

/*** CWSThread ***/
CWSThread::CWSThread(CWebServerBase *webserver) {
	ws = webserver;
	
	//retrieve web server listening port
	wsport = ws->webInterface->m_WebserverPort;
	if (wsport == -1) {
		wsport = ws->GetWSPrefs();
	}
	if (wsport == -1) {
		wsport = 4711;
		ws->Print(wxT("WSThread: Could not get web server port -- using default value.\n"));
	}
}

// thread execution starts here
void *CWSThread::Entry() {
	ws->Print(wxT("\nWSThread: Thread started\n"));
	// Create the address - listen on localhost:ECPort
	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(wsport);
	// Create the socket
	m_WSSocket = new wxSocketServer(addr, wxSOCKET_REUSEADDR);
	wxString msg = addr.Hostname() + wxString::Format(wxT(":%d\n"), addr.Service());
	// We use Ok() here to see if the server is really listening
	if (! m_WSSocket->Ok()) {
		ws->Print(wxT("WSThread: could not create socket on ") + msg);	
	} else {
		ws->Print(wxT("WSThread: created socket listening on ") + msg);	

		s_mutex_wcThreads = new wxMutex();

		while (!TestDestroy()) {
			bool connection_pending	= m_WSSocket->WaitForAccept(1, 0);	// 1 sec
			wxSocketBase* sock;
			if (connection_pending) {
				// Accept incoming connection
				sock = m_WSSocket->Accept(false);
			} else {
				sock = NULL;
			}
			if (sock) {
				// If there was a connection, create new CWCThread
				CWCThread *wct = new CWCThread(ws, sock);

				wxMutexLocker lock(*s_mutex_wcThreads);
				s_wcThreads.Add(wct);
				
				if ( s_wcThreads.Last()->Create() != wxTHREAD_NO_ERROR ) {
					ws->Print(wxT("WSThread: Can't create web client socket thread\n"));
					// destroy the socket
					sock->Destroy();
				} else {
					// ...and run it
					s_wcThreads.Last()->Run();
				}
			}
		}
		ws->Print(wxT("WSThread: Waiting for WCThreads to be terminated..."));
		bool should_wait = true;
		while (should_wait) {
			wxMutexLocker lock(*s_mutex_wcThreads);
			should_wait = (s_wcThreads.GetCount() != 0);
		}

		// by this time, all threads are dead
		delete s_mutex_wcThreads;

		// frees the memory allocated to the array
		s_wcThreads.Clear();

		ws->Print(wxT("done.\n"));
	}

	// Signal the webserver that we exited.
	ws->wsThread = NULL;

	// Kry - WTF to return here?
	// shakraw - it must return NULL. it is correct now.
	return NULL;
}

/*** CWCThread ***/
CWCThread::CWCThread(CWebServerBase *ws, wxSocketBase *sock) {
    stWebSocket.m_pParent = ws;
    stWebSocket.m_hSocket = sock;
    stWebSocket.m_hSocket->SetTimeout(10);
    stWebSocket.m_pHead = NULL;
    stWebSocket.m_pTail = NULL;
    stWebSocket.m_pBuf = new char [4096];
    stWebSocket.m_dwBufSize = 4096;
    stWebSocket.m_dwRecv = 0;
    stWebSocket.m_bValid = true;
    stWebSocket.m_bCanRecv = true;
    stWebSocket.m_bCanSend = true;
    stWebSocket.m_dwHttpHeaderLen = 0;
    stWebSocket.m_dwHttpContentLen = 0;
	stWebSocket.m_Cookie = 0;
}
CWCThread::~CWCThread()
{
	delete [] stWebSocket.m_pBuf;
}


// thread execution starts here
void *CWCThread::Entry() {
#ifdef __DEBUG__
	stWebSocket.m_pParent->Print(wxT("WCThread: Started a new WCThread\n"));
#endif
	bool IsGet = false, IsPost = false;
	while ( stWebSocket.m_bCanRecv ) {
		//check for connection status and return immediately
		if (stWebSocket.m_hSocket->WaitForLost(0)) {
			return 0;
		}
		if (stWebSocket.m_hSocket->WaitForRead(0)) {
			stWebSocket.m_hSocket->Read(stWebSocket.m_pBuf+stWebSocket.m_dwRecv, stWebSocket.m_dwBufSize - stWebSocket.m_dwRecv);
			stWebSocket.m_dwRecv += stWebSocket.m_hSocket->LastCount();
			while ((stWebSocket.m_dwRecv == stWebSocket.m_dwBufSize) && (stWebSocket.m_hSocket->LastCount()!=0) && (!stWebSocket.m_hSocket->Error())) {
					// Buffer is too small. Make it bigger.
					uint32 newsize = stWebSocket.m_dwBufSize + (stWebSocket.m_dwBufSize  >> 1);
					char* newbuffer = new char[newsize];
					char* oldbuffer = stWebSocket.m_pBuf;
					memcpy(newbuffer, oldbuffer, stWebSocket.m_dwBufSize);
					delete[] oldbuffer;
					stWebSocket.m_pBuf = newbuffer;
					stWebSocket.m_dwBufSize = newsize;
					// And read again
					stWebSocket.m_hSocket->Read(stWebSocket.m_pBuf + stWebSocket.m_dwRecv, stWebSocket.m_dwBufSize - stWebSocket.m_dwRecv);
					stWebSocket.m_dwRecv += stWebSocket.m_hSocket->LastCount();				
			}
			
			if (stWebSocket.m_hSocket->LastCount() == 0) {
				if (stWebSocket.m_hSocket->Error()) {
					if (stWebSocket.m_hSocket->LastError() != wxSOCKET_WOULDBLOCK) {
						//close socket&thread
						stWebSocket.m_pParent->Print(wxT("WCThread: got read error. closing socket and terminating thread\n"));
						stWebSocket.m_bValid = false;
						return 0;
					}
				}
			}
			
			stWebSocket.m_pBuf[stWebSocket.m_dwRecv] = '\0';
			
			//
			// Check what kind of request is that
			if ( !IsGet && !IsPost ) {
				if ( !strncasecmp(stWebSocket.m_pBuf, "GET", 3) ) {
					IsGet = true;
				} else if ( !strncasecmp(stWebSocket.m_pBuf, "POST", 4) ) {
					IsPost = true;
				} else {
					stWebSocket.m_pParent->Print(wxT("WCThread: request is unknown: ["));
					stWebSocket.m_pParent->Print(char2unicode(stWebSocket.m_pBuf));
					stWebSocket.m_pParent->Print(wxT("]\n"));
					return 0;
				}
			}
			// 
			// RFC1945:
			//
			
			//
			// "GET" must have last line empty
			if ( IsGet ) {
				if ( !strncasecmp(stWebSocket.m_pBuf + stWebSocket.m_dwRecv - 4, "\r\n\r\n", 4) ) {
					stWebSocket.m_bCanRecv = false;
					//
					// Process request
					stWebSocket.OnRequestReceived(stWebSocket.m_pBuf, 0, 0);
				}
			}
			//
			// "POST" have "Content-Length"
			if ( IsPost ) {
				char *cont_len = strstr(stWebSocket.m_pBuf, "Content-Length");
				// do we have received all the line ?
				if ( cont_len && strstr(cont_len, "\r\n\r\n") ) {
					cont_len += strlen("Content-Length:");
					// can be white space following
					while ( isspace(*cont_len) ) cont_len++;
					int len = atoi(cont_len);
					if ( !len ) {
						stWebSocket.m_pParent->Print(wxT("WCThread: POST method have no data\n"));
						return 0;
					}
					// do we have all of data ?
					char *cont = strstr(stWebSocket.m_pBuf, "\r\n\r\n");
					cont += 4;
					if ( cont - stWebSocket.m_pBuf + len <= (int)stWebSocket.m_dwRecv ) {
						stWebSocket.m_bCanRecv = false;
						stWebSocket.OnRequestReceived(stWebSocket.m_pBuf, cont, len);
					}
				}
			}
		} else {
			Sleep(10);
		}
	}
	//check for connection status and return immediately
	if (stWebSocket.m_hSocket->WaitForLost(0)) {
		//stWebSocket.m_pParent->Print(wxT("*** WCThread - WaitForLost\n"));
		//connection closed/lost. terminate thread
	} else {
		// send what is left in our tails
		while (stWebSocket.m_pHead && stWebSocket.m_pHead->m_pToSend) {
			if (!stWebSocket.m_hSocket->WaitForWrite()) {
				stWebSocket.m_pParent->Print(wxT("WCThread: got timeout on socket.\n"));
				stWebSocket.m_bValid = false;
				break;				
			}
			//stWebSocket.m_pParent->Print(wxString::Format(wxT("*** WCThread write:\n%s\n"), stWebSocket.m_pHead->m_pToSend));
			//WRITE
			stWebSocket.m_hSocket->Write(stWebSocket.m_pHead->m_pToSend, stWebSocket.m_pHead->m_dwSize);
			uint32 nRes = stWebSocket.m_hSocket->LastCount();
			if (nRes >= stWebSocket.m_pHead->m_dwSize) {
				// erase this chunk
				CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
				delete stWebSocket.m_pHead;
				if (!(stWebSocket.m_pHead = pNext)) {
					stWebSocket.m_pTail = NULL;
				}
			} else {
				if ((nRes > 0) && (!stWebSocket.m_hSocket->Error())) {
					stWebSocket.m_pHead->m_pToSend += nRes;
					stWebSocket.m_pHead->m_dwSize -= nRes;
				} else {
					if (stWebSocket.m_hSocket->Error()) {
						if (stWebSocket.m_hSocket->LastError() != wxSOCKET_WOULDBLOCK) {
							//got error
							stWebSocket.m_pParent->Print(wxT("WCThread: got write error.\n"));
							stWebSocket.m_bValid = false;
							break;
						}
					}
				}
			}
		}
	}
	//destroy the socket
	stWebSocket.m_hSocket->Destroy();
#ifdef __DEBUG__
	stWebSocket.m_pParent->Print(wxT("WCThread: exited [WebSocket closed]\n"));
#endif
	// remove ourself from threads array
	wxMutexLocker lock(*s_mutex_wcThreads);
	s_wcThreads.Remove(this);

	// Kry - WTF to return here?
	// shakraw - it must return NULL. it is correct now.
	return NULL;	
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
		current_cookie += strlen("Cookie: ");
		char *value = strchr(current_cookie, '=');
		if ( value ) {
			*value++ = 0;
		}
		if ( !strcmp(current_cookie, "SESSID") ) {
			sessid = atoi(value);
		}
	}
	ThreadData Data = { CParsedUrl(sURL), sURL, sessid, this };
	if (sURL.Length() > 4 ) {
		wxString url_ext = sURL.Right( sURL.Length() - sURL.Find('.', true) ).MakeLower();
		if ( (url_ext==wxT(".gif")) || (url_ext==wxT(".jpg")) || (url_ext==wxT(".ico")) ||
			(url_ext==wxT(".png")) || (url_ext==wxT(".bmp")) || (url_ext==wxT(".jpeg")) ) {
			m_pParent->ProcessImgFileReq(Data);
		} else {
			m_pParent->ProcessURL(Data);
		}
	} else {
		m_pParent->ProcessURL(Data);
	}

}

void CWebSocket::SendContent(const char* szStdResponse, const void* pContent, uint32 dwContentSize) {
	char szBuf[0x1000]; // 0x1000 is safe because it's just used for the header
	int nLen = sprintf(szBuf, "HTTP/1.1 200 OK\r\n%sContent-Length: %d\r\n\r\n", szStdResponse, dwContentSize);
	SendData(szBuf, nLen);
	SendData(pContent, dwContentSize);
}

void CWebSocket::SendHttpHeaders(bool use_gzip, uint32 content_len, int session_id)
{
	char szBuf[0x1000];

	char cookie[256];
	if ( session_id ) {
		sprintf(cookie, "Set-Cookie: SESSID=%d\r\n", session_id);
	} else {
		cookie[0] = 0;
	}

	sprintf(szBuf, "HTTP/1.1 200 OK\r\nServer: aMule\r\nPragma: no-cache\r\nExpires: 0\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"%s"
		"Connection: close\r\nContent-Type: text/html\r\n"
		"Content-Length: %d\r\n%s\r\n\r\n",
		 cookie, content_len, (use_gzip ? "Content-Encoding: gzip" : ""));

	SendData(szBuf, strlen(szBuf));
}

void CWebSocket::SendData(const void* pData, uint32 dwDataSize) {
	if (m_bValid && m_bCanSend) {
		if (!m_pHead) {
			// try to send it directly
			m_hSocket->Write((const char*) pData, dwDataSize);
			uint32 nRes = m_hSocket->LastCount();
			if ((nRes < dwDataSize) && 
				m_hSocket->Error() && 
				(m_hSocket->LastError() != wxSOCKET_WOULDBLOCK)) {
				m_bValid = false;
			} else {
				((const char*&) pData) += nRes;
				dwDataSize -= nRes;
			}
		}
		if (dwDataSize && m_bValid) {
			// push it to our tails
			CChunk* pChunk = new CChunk;
			pChunk->m_pNext = NULL;
			pChunk->m_dwSize = dwDataSize;
			pChunk->m_pData = new char[dwDataSize];
			memcpy(pChunk->m_pData, pData, dwDataSize);
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
}

//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2005 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "WebSocket.h"
#endif

#include <wx/thread.h>

#include "WebSocket.h"

#include <wx/arrimpl.cpp>	// this is a magic incantation which must be done!

#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR


WX_DEFINE_ARRAY(CWCThread*, ArrayOfCWCThread);

ArrayOfCWCThread s_wcThreads;
static wxMutex *s_mutex_wcThreads;

/*** CWSThread ***/
CWSThread::CWSThread(CWebServer *webserver) {
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
	ws->Print(wxT("WSThread: created service\n"));
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
			// Accept the incoming connection and returns immediately
			// Here we should always have a connection pending.
			wxSocketBase *sock = m_WSSocket->Accept();
			if (sock) {
				CWCThread *wct = new CWCThread(ws, sock);

				s_mutex_wcThreads->Lock();
				s_wcThreads.Add(wct);
				
				if ( s_wcThreads.Last()->Create() != wxTHREAD_NO_ERROR ) {
					ws->Print(wxT("WSThread: Can't create web client socket thread\n"));
					// destroy the socket
					sock->Destroy();
				} else {
					// ...and run it
					s_wcThreads.Last()->Run();
				}
				s_mutex_wcThreads->Unlock();
			}
		}
		ws->Print(wxT("WSThread: Waiting for WCThreads to be terminated..."));
		s_mutex_wcThreads->Lock();
		for (size_t i=0; i<s_wcThreads.GetCount(); ++i) {
			// terminate i-th thread
			s_wcThreads.Item(i)->Delete();
		}
		s_mutex_wcThreads->Unlock();

		// by this time, all threads are dead
		delete s_mutex_wcThreads;

		// frees the memory allocated to the array
		s_wcThreads.Clear();

		ws->Print(wxT("done.\n"));
	}
	
	// Kry - WTF to return here?
	// shakraw - it must return NULL. it is correct now.
	return NULL;
}

/*** CWCThread ***/
CWCThread::CWCThread(CWebServer *ws, wxSocketBase *sock) {
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
}
CWCThread::~CWCThread()
{
	delete [] stWebSocket.m_pBuf;
}


// thread execution starts here
void *CWCThread::Entry() {
#ifdef DEBUG
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
					stWebSocket.OnRequestReceived(stWebSocket.m_pBuf, stWebSocket.m_dwRecv, 0, 0);
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
						stWebSocket.OnRequestReceived(stWebSocket.m_pBuf, 
							cont - stWebSocket.m_pBuf, cont, len);
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
#ifdef DEBUG
	stWebSocket.m_pParent->Print(wxT("WCThread: exited [WebSocket closed]\n"));
#endif
	// remove ourself from threads array
	s_mutex_wcThreads->Lock();
	s_wcThreads.Remove(this);
	s_mutex_wcThreads->Unlock();

	// Kry - WTF to return here?
	// shakraw - it must return NULL. it is correct now.
	return NULL;	
}

void CWebSocket::OnRequestReceived(char* pHeader, uint32 dwHeaderLen, char* pData, uint32 dwDataLen)
{
	wxString sHeader(char2unicode(pHeader));
	sHeader = sHeader.Left(dwHeaderLen);
	wxString sData(char2unicode(pData));
	sData=sData.Left(dwDataLen);
	wxString sURL;
	
	if (sHeader.Left(3) == wxT("GET"))
		sURL = sHeader.Trim();
	else if (sHeader.Left(4) == wxT("POST"))
		sURL = wxT("?") + sData.Trim(); // '?' to imitate GET syntax for ParseURL

	if (sURL.Find(wxT(" ")) > -1)
		sURL = sURL.Mid(sURL.Find(wxT(" "))+1, sURL.Length());
	
	if (sURL.Find(wxT(" ")) > -1)
		sURL = sURL.Left(sURL.Find(wxT(" ")));

	ThreadData Data;
	Data.sURL = sURL;
	Data.pSocket = this;
	if (sURL.Length()>4 && sURL.Right(4).MakeLower()==wxT(".gif") || sURL.Right(4).MakeLower()==wxT(".jpg") || 
		sURL.Right(4).MakeLower()==wxT(".png") || sURL.Right(4).MakeLower()==wxT(".bmp") ||
		sURL.Right(4).MakeLower()==wxT(".css") ||
		sURL.Right(5).MakeLower()==wxT(".jpeg")) {
		m_pParent->ProcessImgFileReq(Data);
	} else {
		m_pParent->ProcessURL(Data);
	}

	Disconnect();
}


void CWebSocket::Disconnect() {
    if (m_bValid && m_bCanSend) {
        m_bCanSend = false;
	    
        if (m_pTail) {
            // push it as a tail
            CChunk* pChunk = new CChunk;
            if (pChunk) {
                pChunk->m_dwSize = 0;
                pChunk->m_pData = NULL;
                pChunk->m_pToSend = NULL;
                pChunk->m_pNext = NULL;

                m_pTail->m_pNext = pChunk;
            }

        }
    }
}


void CWebSocket::SendContent(const char* szStdResponse, const void* pContent, uint32 dwContentSize) {
	char szBuf[0x1000]; // 0x1000 is safe because it's just used for the header
	int nLen = sprintf(szBuf, "HTTP/1.1 200 OK\r\n%sContent-Length: %d\r\n\r\n", szStdResponse, dwContentSize);
	SendData(szBuf, nLen);
	SendData(pContent, dwContentSize);
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
			if (pChunk) {
				pChunk->m_pNext = NULL;
				pChunk->m_dwSize = dwDataSize;
				if ((pChunk->m_pData = new char[dwDataSize])) {
					memcpy(pChunk->m_pData, pData, dwDataSize);
					// push it to the end of our queue
					pChunk->m_pToSend = pChunk->m_pData;
					if (m_pTail) {
						m_pTail->m_pNext = pChunk;
					} else {
						m_pHead = pChunk;
					}
					m_pTail = pChunk;
				} else {
					delete pChunk; // oops, no memory (???)
				}
			}
		}
	}
}

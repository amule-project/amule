/*  This file is part of aMule project
 *  
 *  aMule Copyright (C)2003-2004 aMule Team ( http://www.amule-project.net )
 *  This file Copyright (C)2003 (sorry, dunno who done this before)
 *  This file Copyright (C)2004 shakraw <shakraw@users.sourceforge.net>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <wx/thread.h>

#include "WebSocket.h"

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

WX_DEFINE_ARRAY(CWCThread*, ArrayOfCWCThread);

/*** CWSThread ***/
CWSThread::CWSThread(CWebServer *ws) {
	this->ws = ws;
	
	//retrieve web server listening port
	wsport = ws->GetWSPort();
}


// thread execution starts here
void *CWSThread::Entry() {
	wxSocketBase *sock;
	ArrayOfCWCThread wcThreads;
	
	ws->Print("\nWSThread: Thread started\n");	
	// Create the address - listen on localhost:ECPort
	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(wsport);
	ws->Print("WSThread: created service\n");

	// Create the socket
	m_WSSocket = new wxSocketServer(addr);
	
	// We use Ok() here to see if the server is really listening
	if (! m_WSSocket->Ok()) {
		ws->Print("WSThread: could create socket on %s:%d\n", addr.Hostname().GetData(), addr.Service());	
	} else {
		ws->Print("WSThread: created socket listening on %s:%d\n", addr.Hostname().GetData(), addr.Service());	

		while (!TestDestroy()) {
			//check for incoming connection waiting to be accepted
			//and returns immediately
			if (m_WSSocket->WaitForAccept(0)) {
				//Accept the incoming connection and returns immediately
				//Here we should always have a connection pending.
				sock = m_WSSocket->Accept(FALSE);

				CWCThread *wct = new CWCThread(ws, sock);
				wcThreads.Add(wct);
				
				if ( wcThreads.Last()->Create() != wxTHREAD_NO_ERROR ) {
					ws->Print("WSThread: Can't create web client socket thread\n");
					sock->Destroy(); //destroy the socket
				} else {
					//...and run it
					wcThreads.Last()->Run();
				}
			}

			//ws->Print("*** WSThread: %d threads\n", wcThreads.GetCount());
			for (size_t i=0; i<wcThreads.GetCount(); i++) {
				if (!wcThreads.Item(i)->IsAlive()) {
					//ws->Print("*** WSThread: thread %d removed\n", i);
					wcThreads.RemoveAt(i); //remove terminated thread from array to save memory
				}
			}
			
			wxThread::Sleep(200);
		}
		
		ws->Print("WSThread: Waiting for WCThreads to be terminated...");
		for (size_t i=0; i<wcThreads.GetCount(); i++) {
			if (wcThreads.Item(i)->IsAlive()) {
				wcThreads.Item(i)->Delete(); //terminate i-th alive thread
			}
		}
		wcThreads.Clear(); //frees the memory allocated to the array
		ws->Print("done.\n");
	}
	
	// Kry - WTF to return here?
	// shakraw - it must return NULL. it is correct now.
	return NULL;
}


/*** CWCThread ***/
CWCThread::CWCThread(CWebServer *ws, wxSocketBase *sock) {
    stWebSocket.m_pParent = ws;
    stWebSocket.m_hSocket = sock;
    stWebSocket.m_pHead = NULL;
    stWebSocket.m_pTail = NULL;
    stWebSocket.m_pBuf = NULL;
    stWebSocket.m_dwBufSize = 0;
    stWebSocket.m_dwRecv = 0;
    stWebSocket.m_bValid = true;
    stWebSocket.m_bCanRecv = true;
    stWebSocket.m_bCanSend = true;
    stWebSocket.m_dwHttpHeaderLen = 0;
    stWebSocket.m_dwHttpContentLen = 0;
}


// thread execution starts here
void *CWCThread::Entry() {
#ifdef DEBUG
	stWebSocket.m_pParent->Print("WCThread: Started a new WCThread\n");
#endif
	//while (!TestDestroy()) {
		//check for connection status and return immediately
		if (stWebSocket.m_hSocket->WaitForLost(0)) {
			//stWebSocket.m_pParent->Print("*** WCThread - WaitForLost\n");
			//connection closed/lost. exit thread cicle
		} else {
		
			//check for read and return immediately
			if (stWebSocket.m_hSocket->WaitForRead(0)) {
				//stWebSocket.m_pParent->Print("*** WCThread - WaitForRead\n");
				char pBuf[0x1000];
				//READ
				stWebSocket.m_hSocket->Read(&pBuf, sizeof(pBuf));
				//stWebSocket.m_pParent->Print("*** WCThread read:\n%s\n", pBuf);
				if (stWebSocket.m_hSocket->LastCount() == 0) {
					if (stWebSocket.m_hSocket->Error()) {
						if (stWebSocket.m_hSocket->LastError() != wxSOCKET_WOULDBLOCK) {
							//close socket&thread
							stWebSocket.m_pParent->Print("WCThread: got read error. closing socket and terminating thread\n");			
							stWebSocket.m_bValid = false;
							//break;
						}
					} else {
						//read nothing
						stWebSocket.m_bCanRecv = false;
						stWebSocket.OnReceived(NULL, 0);
					}
				}
				stWebSocket.OnReceived(pBuf, stWebSocket.m_hSocket->LastCount());
			}
		
			//check for write and return immediately
			if (stWebSocket.m_hSocket->WaitForWrite(0)) {
				// send what is left in our tails
				while (stWebSocket.m_pHead) {
					if (stWebSocket.m_pHead->m_pToSend) {
						//stWebSocket.m_pParent->Print("*** WCThread write:\n%s\n", stWebSocket.m_pHead->m_pToSend);
						//WRITE
						stWebSocket.m_hSocket->Write(stWebSocket.m_pHead->m_pToSend, stWebSocket.m_pHead->m_dwSize);
						wxUint32 nRes = stWebSocket.m_hSocket->LastCount();
						if (nRes != stWebSocket.m_pHead->m_dwSize) {
							if (nRes > 0) {
								if (/*(nRes > 0) &&*/(nRes < stWebSocket.m_pHead->m_dwSize)) {
									stWebSocket.m_pHead->m_pToSend += nRes;
									stWebSocket.m_pHead->m_dwSize -= nRes;
								}
							} else {
								if (stWebSocket.m_hSocket->Error()) {
									if (stWebSocket.m_hSocket->LastError() != wxSOCKET_WOULDBLOCK) {
										//got error
										stWebSocket.m_pParent->Print("WCThread: got write error.\n");
										stWebSocket.m_bValid = false;
									}
								}
							}
							break;
						}
					}
	
					// erase this chunk
					CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
					delete stWebSocket.m_pHead;
					if (!(stWebSocket.m_pHead = pNext)) {
						stWebSocket.m_pTail = NULL;
					}
				}
			}
		}
		
		//wxThread::Sleep(200);
	//}
	
	//destroy the socket and terminate thread.
	stWebSocket.m_hSocket->Destroy();
#ifdef DEBUG
	stWebSocket.m_pParent->Print("WCThread: exited [WebSocket closed]\n");
#endif

	// Kry - WTF to return here?
	// shakraw - it must return NULL. it is correct now.
	return NULL;	
}


void CWebSocket::OnReceived(char* pData, wxUint32 dwSize) {
	//m_pParent->Print("*** WCThread: OnReceived\n");

	const UINT SIZE_PRESERVE = 0x1000;

	if (m_dwBufSize < dwSize + m_dwRecv) {
		// reallocate
		char* pNewBuf = new char[m_dwBufSize = dwSize + m_dwRecv + SIZE_PRESERVE];
		if (!pNewBuf) {
			m_pParent->Print("WCThread: unable to reallocate buffer. internal problem.\n");
			m_bValid = false; // internal problem
			return;
		}
		if (m_pBuf) {
			memcpy(pNewBuf, m_pBuf, m_dwRecv);
			delete[] m_pBuf;
		}
		m_pBuf = pNewBuf;
	}
	memcpy(m_pBuf + m_dwRecv, pData, dwSize);
	m_dwRecv += dwSize;

	// check if we have all that we want
	if (!m_dwHttpHeaderLen) {
		// try to find it
		bool bPrevEndl = false;
		
		for (wxUint32 dwPos = 0; dwPos < m_dwRecv; dwPos++) {
			if ('\n' == m_pBuf[dwPos]) {
				if (bPrevEndl) {
					// We just found the end of the http header
					// Now write the message's position into two first 
					// DWORDs of the buffer
					m_dwHttpHeaderLen = dwPos + 1;

					// try to find now the 'Content-Length' header
					for (dwPos = 0; dwPos < m_dwHttpHeaderLen; ) {
						PVOID pPtr = memchr(m_pBuf + dwPos, '\n', m_dwHttpHeaderLen - dwPos);
						if (!pPtr) break;

						wxUint32 dwNextPos = ((wxUint32) pPtr) - ((wxUint32) m_pBuf);

						// check this header
						char szMatch[] = "content-length";
						if (!strncasecmp(m_pBuf + dwPos, szMatch, sizeof(szMatch) - 1)) {
							dwPos += sizeof(szMatch) - 1;
							pPtr = memchr(m_pBuf + dwPos, ':', m_dwHttpHeaderLen - dwPos);
							if (pPtr) {
								m_dwHttpContentLen = atol(((char*) pPtr) + 1);
							}
							break;
						}
						dwPos = dwNextPos + 1;
					}
					break;
				} else {
					bPrevEndl = true;
				}
			} else {
				if ('\r' != m_pBuf[dwPos]) {
					bPrevEndl = false;
				}
			}
		}
	}
	
	if (m_dwHttpHeaderLen && !m_bCanRecv && !m_dwHttpContentLen) {
		m_dwHttpContentLen = m_dwRecv - m_dwHttpHeaderLen; // of course
	}

	if (m_dwHttpHeaderLen && (!m_dwHttpContentLen || (m_dwHttpHeaderLen + m_dwHttpContentLen <= m_dwRecv))) {
		OnRequestReceived(m_pBuf, m_dwHttpHeaderLen, m_pBuf + m_dwHttpHeaderLen, m_dwHttpContentLen);

		if (m_bCanRecv && (m_dwRecv > m_dwHttpHeaderLen + m_dwHttpContentLen)) {
			// move our data
			memmove(m_pBuf, m_pBuf + m_dwHttpHeaderLen + m_dwHttpContentLen, m_dwRecv - m_dwHttpHeaderLen + m_dwHttpContentLen);
			m_dwRecv -= m_dwHttpHeaderLen + m_dwHttpContentLen;
		} else {
			m_dwRecv = 0;
		}

		m_dwHttpHeaderLen = 0;
		m_dwHttpContentLen = 0;
	}
}


void CWebSocket::OnRequestReceived(char* pHeader, wxUint32 dwHeaderLen, char* pData, wxUint32 dwDataLen) {
	wxString sHeader(pHeader, dwHeaderLen);
	wxString sData(pData, dwDataLen);
	wxString sURL;
	bool imgreq=false;
	bool stylereq=false;
	
	if (sHeader.Left(3) == "GET")
		sURL = sHeader.Trim();
	else if (sHeader.Left(4) == "POST")
		sURL = "?" + sData.Trim(); // '?' to imitate GET syntax for ParseURL

	if (sURL.Find(" ") > -1)
		sURL = sURL.Mid(sURL.Find(" ")+1, sURL.Length());
	
	if (sURL.Find(" ") > -1)
		sURL = sURL.Left(sURL.Find(" "));

	if (sURL.Length()>4 && sURL.Right(4).MakeLower()==".gif" || sURL.Right(4).MakeLower()==".jpg" || 
		sURL.Right(4).MakeLower()==".png" || sURL.Right(4).MakeLower()==".bmp" ||
		sURL.Right(5).MakeLower()==".jpeg")
		imgreq=true;
	
	if (sURL.Length()>4 && sURL.Right(4).MakeLower()==".css")
		stylereq=true;

	ThreadData Data;
	Data.sURL = sURL;
	Data.pThis = m_pParent;
	Data.pSocket = this;

	if (!imgreq && !stylereq) m_pParent->ProcessURL(Data);
	else if (!imgreq) m_pParent->ProcessStyleFileReq(Data);
	else m_pParent->ProcessImgFileReq(Data);

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


void CWebSocket::SendContent(LPCSTR szStdResponse, const void* pContent, wxUint32 dwContentSize) {
    char szBuf[0x1000];
    int nLen = sprintf(szBuf, "HTTP/1.1 200 OK\r\n%sContent-Length: %d\r\n\r\n", szStdResponse, dwContentSize);
    SendData(szBuf, nLen);
    SendData(pContent, dwContentSize);
}


void CWebSocket::SendData(const void* pData, wxUint32 dwDataSize) {
    if (m_bValid && m_bCanSend) {
        if (!m_pHead) {
            // try to send it directly
			m_hSocket->Write((const char*) pData, dwDataSize);
			uint32 nRes = m_hSocket->LastCount();
            if (((nRes < 0) || (nRes > dwDataSize)) && m_hSocket->Error() && (m_hSocket->LastError() != wxSOCKET_WOULDBLOCK))
                m_bValid = false;
            else {
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
                    // push it to the end of our queue
                    pChunk->m_pToSend = pChunk->m_pData;
                    if (m_pTail)
                        m_pTail->m_pNext = pChunk;
                    else
                        m_pHead = pChunk;
					
                    m_pTail = pChunk;
                } else {
                    delete pChunk; // oops, no memory (???)
				}
            }
        }
    }
}

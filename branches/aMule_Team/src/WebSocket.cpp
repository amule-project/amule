// ws2.cpp : Defines the entry point for the application.
// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <cstdlib>
#include <cerrno>
#include <algorithm>		// Needed for std::max
#include <unistd.h>		// Needed for close(2)

#include <pthread.h>
#ifdef __WXMSW__
	#include <winsock.h>
	#include <unistd.h>
	#define EWOULDBLOCK WSAEWOULDBLOCK 
	// socket shutdown() types
	#define SHUT_WR SD_SEND
	#define SHUT_RD SD_RECEIVE
	#define SHUT_RDWR SD_BOTH
#else
	#include <netinet/in.h>
#endif

//#pragma comment(lib, "ws2_32.lib")
#include "types.h"
#include "WebSocket.h"
#include "WebServer.h"

int g_hTerminate = -1;
pthread_t g_hSocketThread = 0;

typedef struct
{
    void	*pThis;
    int	hSocket;
}
SocketData;

void CWebSocket::SetParent(CWebServer *pParent)
{
    m_pParent = pParent;
}

void CWebSocket::OnRequestReceived(char* pHeader, DWORD dwHeaderLen, char* pData, DWORD dwDataLen)
{
 	wxString sHeader(pHeader, dwHeaderLen);

	wxString sData(pData, dwDataLen);
	wxString sURL;
	bool imgreq=false;
	bool stylereq=false;

	if(sHeader.Left(3) == "GET")
		sURL = sHeader.Trim();

	else if(sHeader.Left(4) == "POST")
		sURL = "?" + sData.Trim();	// '?' to imitate GET syntax for ParseURL

	if(sURL.Find(" ") > -1)
		sURL = sURL.Mid(sURL.Find(" ")+1, sURL.Length());
	if(sURL.Find(" ") > -1)
		sURL = sURL.Left(sURL.Find(" "));

	if (sURL.Length()>4 && sURL.Right(4).MakeLower()==".gif" || sURL.Right(4).MakeLower()==".jpg" || sURL.Right(4).MakeLower()==".png" ||
		sURL.Right(4).MakeLower()==".bmp" ||sURL.Right(5).MakeLower()==".jpeg")
		imgreq=true;
	if (sURL.Length()>4 && sURL.Right(4).MakeLower()==".css")
		stylereq=true;

	ThreadData Data;
	Data.sURL = sURL;
	Data.pThis = m_pParent;
	Data.pSocket = this;

	if (!imgreq && !stylereq) 
		m_pParent->ProcessURL(Data);
	else if (!imgreq)
		m_pParent->ProcessStyleFileReq(Data);
	else
		m_pParent->ProcessImgFileReq(Data);

	Disconnect();    	
}

void CWebSocket::OnReceived(void* pData, DWORD dwSize)
{
 	const UINT SIZE_PRESERVE = 0x1000;

	if (m_dwBufSize < dwSize + m_dwRecv)
	{
		// reallocate
		char* pNewBuf = new char[m_dwBufSize = dwSize + m_dwRecv + SIZE_PRESERVE];
		if (!pNewBuf)
		{
			m_bValid = false; // internal problem
			return;
		}

		if (m_pBuf)
		{
			memcpy(pNewBuf, m_pBuf, m_dwRecv);
			delete[] m_pBuf;
		}

		m_pBuf = pNewBuf;
	}
	memcpy(m_pBuf + m_dwRecv, pData, dwSize);
	m_dwRecv += dwSize;

	// check if we have all that we want
	if (!m_dwHttpHeaderLen)
	{
		// try to find it
		bool bPrevEndl = false;
		for (DWORD dwPos = 0; dwPos < m_dwRecv; dwPos++)
			if ('\n' == m_pBuf[dwPos])
				if (bPrevEndl)
				{
					// We just found the end of the http header
					// Now write the message's position into two first DWORDs of the buffer
					m_dwHttpHeaderLen = dwPos + 1;

					// try to find now the 'Content-Length' header
					for (dwPos = 0; dwPos < m_dwHttpHeaderLen; )
					{
						PVOID pPtr = memchr(m_pBuf + dwPos, '\n', m_dwHttpHeaderLen - dwPos);
						if (!pPtr)
							break;
						DWORD dwNextPos = ((DWORD) pPtr) - ((DWORD) m_pBuf);

						// check this header
						char szMatch[] = "content-length";
						if (!strncasecmp(m_pBuf + dwPos, szMatch, sizeof(szMatch) - 1))
						{
							dwPos += sizeof(szMatch) - 1;
							pPtr = memchr(m_pBuf + dwPos, ':', m_dwHttpHeaderLen - dwPos);
							if (pPtr)
								m_dwHttpContentLen = atol(((char*) pPtr) + 1);

							break;
						}
						dwPos = dwNextPos + 1;
					}

					break;
				}
				else
				{
					bPrevEndl = true;
				}
			else
				if ('\r' != m_pBuf[dwPos])
					bPrevEndl = false;

	}

	if (m_dwHttpHeaderLen && !m_bCanRecv && !m_dwHttpContentLen)
		m_dwHttpContentLen = m_dwRecv - m_dwHttpHeaderLen; // of course

	if (m_dwHttpHeaderLen && (!m_dwHttpContentLen || (m_dwHttpHeaderLen + m_dwHttpContentLen <= m_dwRecv)))
	{
		OnRequestReceived(m_pBuf, m_dwHttpHeaderLen, m_pBuf + m_dwHttpHeaderLen, m_dwHttpContentLen);

		if (m_bCanRecv && (m_dwRecv > m_dwHttpHeaderLen + m_dwHttpContentLen))
		{
			// move our data
			memmove(m_pBuf, m_pBuf + m_dwHttpHeaderLen + m_dwHttpContentLen, m_dwRecv - m_dwHttpHeaderLen + m_dwHttpContentLen);
			m_dwRecv -= m_dwHttpHeaderLen + m_dwHttpContentLen;
		} else
			m_dwRecv = 0;

		m_dwHttpHeaderLen = 0;
		m_dwHttpContentLen = 0;
	}
}

void CWebSocket::SendData(const void* pData, DWORD dwDataSize)
{
    if (m_bValid && m_bCanSend)
    {
        if (!m_pHead)
        {
            // try to send it directly
            int nRes = send(m_hSocket, (const char*) pData, dwDataSize, 0);
            if (((nRes < 0) || (nRes > (signed) dwDataSize)) && (EWOULDBLOCK != errno))
                m_bValid = false;
            else
            {
                ((const char*&) pData) += nRes;
                dwDataSize -= nRes;
            }
        }

        if (dwDataSize && m_bValid)
        {
            // push it to our tails
            CChunk* pChunk = new CChunk;
            if (pChunk)
            {
                pChunk->m_pNext = NULL;
                pChunk->m_dwSize = dwDataSize;
                if ((pChunk->m_pData = new char[dwDataSize]))
                {
                    // push it to the end of our queue
                    pChunk->m_pToSend = pChunk->m_pData;
                    if (m_pTail)
                        m_pTail->m_pNext = pChunk;
                    else
                        m_pHead = pChunk;
                    m_pTail = pChunk;

                }
                else
                    delete pChunk; // oops, no memory (???)
            }
        }
    }

}

void CWebSocket::SendContent(LPCSTR szStdResponse, const void* pContent, DWORD dwContentSize)
{
    char szBuf[0x1000];
    int nLen = sprintf(szBuf, "HTTP/1.1 200 OK\r\n%sContent-Length: %d\r\n\r\n", szStdResponse, dwContentSize);
    SendData(szBuf, nLen);
    SendData(pContent, dwContentSize);
}

void CWebSocket::Disconnect()
{
    if (m_bValid && m_bCanSend)
    {
        m_bCanSend = false;
        if (m_pTail)
        {
            // push it as a tail
            CChunk* pChunk = new CChunk;
            if (pChunk)
            {
                pChunk->m_dwSize = 0;
                pChunk->m_pData = NULL;
                pChunk->m_pToSend = NULL;
                pChunk->m_pNext = NULL;

                m_pTail->m_pNext = pChunk;
            }

        }
        else
            if (shutdown(m_hSocket, SHUT_WR))
                m_bValid = false;
    }
}

void* WebSocketAcceptedFunc(void *pD)
{
    SocketData *pData = (SocketData *)pD;
    int hSocket = pData->hSocket;
    CWebServer *pThis = (CWebServer *)pData->pThis;
    delete pData;

    CWebSocket stWebSocket;
    stWebSocket.SetParent(pThis);
    stWebSocket.m_pHead = NULL;
    stWebSocket.m_pTail = NULL;
    stWebSocket.m_bValid = true;
    stWebSocket.m_bCanRecv = true;
    stWebSocket.m_bCanSend = true;
    stWebSocket.m_hSocket = hSocket;
    stWebSocket.m_pBuf = NULL;
    stWebSocket.m_dwRecv = 0;
    stWebSocket.m_dwBufSize = 0;
    stWebSocket.m_dwHttpHeaderLen = 0;
    stWebSocket.m_dwHttpContentLen = 0;

    while(1)
    {
        fd_set readfds,writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(hSocket,&readfds);
        FD_SET(hSocket,&writefds);
        FD_SET(g_hTerminate,&readfds);
        int rv=select(std::max(g_hTerminate,hSocket)+1,&readfds,&writefds,NULL,NULL);
        if(rv>0)
        {
            // got something
            if(FD_ISSET(g_hTerminate,&readfds))
            {
                // time to terminate
                break;
            }
            if(FD_ISSET(hSocket,&readfds))
            {
                // something receivedf
                while (true)
                {
                    char pBuf[0x1000];
                    int nRes = recv(hSocket, pBuf, sizeof(pBuf), 0);
										if (nRes <= 0)
                    {
                        if (!nRes)
                        {
                            stWebSocket.m_bCanRecv = false;
                            stWebSocket.OnReceived(NULL, 0);
                        }
                        else
                            if (EWOULDBLOCK != errno)
                                stWebSocket.m_bValid = false;
												// break would exit at wrong level..
												goto exit_select;
                        //break;
                    }
                    stWebSocket.OnReceived(pBuf, nRes);
                }
            }
            if(FD_ISSET(hSocket,&writefds))
            {
                // send what is left in our tails
                while (stWebSocket.m_pHead)
                {
                    if (stWebSocket.m_pHead->m_pToSend)
                    {
                        int nRes = send(hSocket, stWebSocket.m_pHead->m_pToSend, stWebSocket.m_pHead->m_dwSize, 0);
                        if (nRes != (signed) stWebSocket.m_pHead->m_dwSize)
                        {
                            if (nRes)
                                if ((nRes > 0) && (nRes < (signed) stWebSocket.m_pHead->m_dwSize))
                                {
                                    stWebSocket.m_pHead->m_pToSend += nRes;
                                    stWebSocket.m_pHead->m_dwSize -= nRes;

                                }
                                else
                                    if (EWOULDBLOCK != errno)
                                        stWebSocket.m_bValid = false;
                            break;
                        }
                    }
                    else
                        if (shutdown(hSocket, SHUT_WR))
                        {
                            stWebSocket.m_bValid = false;
                            break;
                        }

                    // erase this chunk
                    CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
                    delete stWebSocket.m_pHead;
                    if (!(stWebSocket.m_pHead = pNext))
                        stWebSocket.m_pTail = NULL;
                }
            }
        }
        else if(rv<0)
        {
            stWebSocket.m_bValid=false;
            break;
        }
    }
#if 0
    HANDLE hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (hEvent)
    {
        if (!WSAEventSelect(hSocket, hEvent, FD_READ | FD_CLOSE | FD_WRITE))
        {
            CWebSocket stWebSocket;
            stWebSocket.SetParent(pThis);
            stWebSocket.m_pHead = NULL;
            stWebSocket.m_pTail = NULL;
            stWebSocket.m_bValid = true;
            stWebSocket.m_bCanRecv = true;
            stWebSocket.m_bCanSend = true;
            stWebSocket.m_hSocket = hSocket;
            stWebSocket.m_pBuf = NULL;
            stWebSocket.m_dwRecv = 0;
            stWebSocket.m_dwBufSize = 0;
            stWebSocket.m_dwHttpHeaderLen = 0;
            stWebSocket.m_dwHttpContentLen = 0;

            HANDLE pWait[] = { hEvent, g_hTerminate };

            while (WAIT_OBJECT_0 == WaitForMultipleObjects(2, pWait, FALSE, INFINITE))
            {
                while (stWebSocket.m_bValid)
                {
                    WSANETWORKEVENTS stEvents;
                    if (WSAEnumNetworkEvents(hSocket, NULL, &stEvents))
                        stWebSocket.m_bValid = false;
                    else
                    {
                        if (!stEvents.lNetworkEvents)
                            break; //no more events till now

                        if (FD_READ & stEvents.lNetworkEvents)
                            while (true)
                            {
                                char pBuf[0x1000];
                                int nRes = recv(hSocket, pBuf, sizeof(pBuf), 0);
                                if (nRes <= 0)
                                {
                                    if (!nRes)
                                    {
                                        stWebSocket.m_bCanRecv = false;
                                        stWebSocket.OnReceived(NULL, 0);
                                    }
                                    else
                                        if (WSAEWOULDBLOCK != WSAGetLastError())
                                            stWebSocket.m_bValid = false;
                                    break;
                                }
                                stWebSocket.OnReceived(pBuf, nRes);
                            }

                        if (FD_CLOSE & stEvents.lNetworkEvents)
                            stWebSocket.m_bCanRecv = false;

                        if (FD_WRITE & stEvents.lNetworkEvents)
                            // send what is left in our tails
                            while (stWebSocket.m_pHead)
                            {
                                if (stWebSocket.m_pHead->m_pToSend)
                                {
                                    int nRes = send(hSocket, stWebSocket.m_pHead->m_pToSend, stWebSocket.m_pHead->m_dwSize, 0);
                                    if (nRes != (signed) stWebSocket.m_pHead->m_dwSize)
                                    {
                                        if (nRes)
                                            if ((nRes > 0) && (nRes < (signed) stWebSocket.m_pHead->m_dwSize))
                                            {
                                                stWebSocket.m_pHead->m_pToSend += nRes;
                                                stWebSocket.m_pHead->m_dwSize -= nRes;

                                            }
                                            else
                                                if (WSAEWOULDBLOCK != WSAGetLastError())
                                                    stWebSocket.m_bValid = false;
                                        break;
                                    }
                                }
                                else
                                    if (shutdown(hSocket, SD_SEND))
                                    {
                                        stWebSocket.m_bValid = false;
                                        break;
                                    }

                                // erase this chunk
                                CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
                                delete stWebSocket.m_pHead;
                                if (!(stWebSocket.m_pHead = pNext))
                                    stWebSocket.m_pTail = NULL;
                            }
                    }
                }

                if (!stWebSocket.m_bValid || (!stWebSocket.m_bCanRecv && !stWebSocket.m_pHead))
                    break;
            }

            while (stWebSocket.m_pHead)
            {
                CWebSocket::CChunk* pNext = stWebSocket.m_pHead->m_pNext;
                delete stWebSocket.m_pHead;
                stWebSocket.m_pHead = pNext;
            }
            if (stWebSocket.m_pBuf)
                delete[] stWebSocket.m_pBuf;
        }

        VERIFY(CloseHandle(hEvent));
    }
#endif
exit_select:
    //VERIFY(!closesocket(hSocket));
    close(hSocket);

    printf("*** hanska: exited (WebSocket closed)\n");
    return 0;
}

int wsport; //shakraw

void* WebSocketListeningFunc(void *pThis)
{
    int hSocket=-1;
    hSocket=socket(AF_INET,SOCK_STREAM,0);
    if(hSocket>0)
    {
        struct sockaddr_in stAddr;

        stAddr.sin_family = AF_INET;
        //stAddr.sin_port = htons(theApp.glob_prefs->GetWSPort());
	stAddr.sin_port = htons(wsport);
	//printf("*** WebServer: Using port: %i\n", theApp.glob_prefs->GetWSPort());
	printf("*** WebServer: Using port: %i\n", wsport);

        stAddr.sin_addr.s_addr = INADDR_ANY;

        if (!bind(hSocket, (sockaddr*) &stAddr, sizeof(stAddr)) &&
                !listen(hSocket, SOMAXCONN))
        {
            while(1)
            {
                int hAccepted=accept(hSocket,NULL,NULL);
                if(hAccepted==(-1))
                    break;
                // uh uh uh.. well you can do it this way too..
                if(1 /* theApp.glob_prefs->GetWSIsEnabled()*/)
                {
                    pthread_t dwThread = 0;
                    pthread_attr_t myattr;
                    SocketData *pData = new SocketData;
                    pData->hSocket = hAccepted;
                    pData->pThis = pThis;
                    pthread_attr_init(&myattr);
                    pthread_attr_setdetachstate(&myattr,PTHREAD_CREATE_DETACHED);
                    pthread_create(&dwThread,&myattr,WebSocketAcceptedFunc,(void*)pData);
                    //HANDLE hThread = CreateThread(NULL, 0, WebSocketAcceptedFunc, (void *)pData, 0, &dwThread);
                }
            }
        }

    }
#if 0
    WSADATA stData;
    if (!WSAStartup(MAKEWORD(2, 2), &stData))
    {
        SOCKET hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
        if (INVALID_SOCKET != hSocket)
        {
            SOCKADDR_IN stAddr;
            stAddr.sin_family = AF_INET;
            stAddr.sin_port = htons(theApp.glob_prefs->GetWSPort());
            stAddr.sin_addr.S_un.S_addr = INADDR_ANY;

            if (!bind(hSocket, (sockaddr*) &stAddr, sizeof(stAddr)) &&
                    !listen(hSocket, SOMAXCONN))
            {
                HANDLE hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
                if (hEvent)
                {
                    if (!WSAEventSelect(hSocket, hEvent, FD_ACCEPT))
                    {
                        HANDLE pWait[] = { hEvent, g_hTerminate };
                        while (WAIT_OBJECT_0 == WaitForMultipleObjects(2, pWait, FALSE, INFINITE))
                            while (true)
                            {
                                SOCKET hAccepted = accept(hSocket, NULL, NULL);
                                if (INVALID_SOCKET == hAccepted)
                                    break;

                                if(theApp.glob_prefs->GetWSIsEnabled())
                                {
                                    DWORD dwThread = 0;
                                    SocketData *pData = new SocketData;
                                    pData->hSocket = hAccepted;
                                    pData->pThis = pThis;
                                    HANDLE hThread = CreateThread(NULL, 0, WebSocketAcceptedFunc, (void *)pData, 0, &dwThread);
                                    if (hThread)
                                        VERIFY(CloseHandle(hThread));
                                    else
                                        VERIFY(!closesocket(hSocket));
                                }
                                else
                                    VERIFY(!closesocket(hSocket));
                            }
                    }

                    VERIFY(CloseHandle(hEvent));
                }
            }

            VERIFY(!closesocket(hSocket));
        }

        VERIFY(!WSACleanup());
    }
#endif
    return 0;
}


int hTermPipe=-1;

void StartSockets(CWebServer *pThis)
{
#warning "WebSocket::StartSockets() - Rewrite w/o the usage of pipes (doesn't work on Win32)."
#ifndef __WXMSW__ // Bah, yes, go on, use pipes for xplatform app. Very wise indeed.
    int pipes[2];
    if(pipe(pipes)==0)
    {
        //if (g_hTerminate = CreateEvent(NULL, TRUE, FALSE, NULL))
        g_hTerminate=pipes[0];
        hTermPipe=pipes[1];
        printf("*** creating web socket and starting listener!\n");
	wsport = pThis->GetWSPort();
        pthread_create(&g_hSocketThread,NULL,WebSocketListeningFunc,(void*)pThis);
        //DWORD dwThread = 0;
        //g_hSocketThread = CreateThread(NULL, 0, WebSocketListeningFunc, (void*)pThis, 0, &dwThread);
    }
#endif
}

void StopSockets()
{
    if (g_hSocketThread)
    {
        //VERIFY(SetEvent(g_hTerminate));
        write(hTermPipe,"111",4); // signal the thread

        // then we'll wait for it's termination
        //if (WAIT_TIMEOUT == WaitForSingleObject(g_hSocketThread, 300))
        //	VERIFY(TerminateThread(g_hSocketThread, -1));
        //VERIFY(CloseHandle(g_hSocketThread));
        void* retval=NULL;
        pthread_join(g_hSocketThread,&retval);
    }
    //VERIFY(CloseHandle(g_hTerminate));

}

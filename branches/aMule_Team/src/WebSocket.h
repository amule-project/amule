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

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <cstring>		// Needed for std::strlen

#include "types.h"		// Needed for DWORD

class CWebServer;

void StartSockets(CWebServer* pThis);
void StopSockets(void);

class CWebSocket {
public:
	void SetParent(CWebServer *);
	CWebServer* m_pParent;

	class CChunk 
	{
	public:
		char* m_pData;
		char* m_pToSend;
		DWORD m_dwSize;
		CChunk* m_pNext;

		~CChunk() { if (m_pData) delete[] m_pData; }
	};

	CChunk* m_pHead; // tails of what has to be sent
	CChunk* m_pTail;

	char* m_pBuf;
	DWORD m_dwRecv;
	DWORD m_dwBufSize;
	DWORD m_dwHttpHeaderLen;
	DWORD m_dwHttpContentLen;

	bool m_bCanRecv;
	bool m_bCanSend;
	bool m_bValid;
	int  m_hSocket;

	void OnReceived(void* pData, DWORD dwDataSize); // must be implemented
	void SendData(const void* pData, DWORD dwDataSize);
	void SendData(char* szText) { SendData(szText, std::strlen(szText)); }
	void SendContent(LPCSTR szStdResponse, const void* pContent, DWORD dwContentSize);
	void SendTextContent(char* szText) { SendContent("", szText, std::strlen(szText)); }
	void Disconnect();

	void OnRequestReceived(char* pHeader, DWORD dwHeaderLen, char* pData, DWORD dwDataLen);
};

#endif // WEBSOCKET_H

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

#ifndef WEBSOCKET_H
#define WEBSOCKET_H


#include "WebServer.h"
#include <LibSocket.h>


#ifdef ENABLE_UPNP
class CUPnPControlPoint;
class CUPnPPortMapping;
#endif
class CWebServer;


class CWebSocket : public CLibSocket {
	public:
		CWebSocket(CWebServerBase *parent);

		virtual void OnSend(int);
		virtual void OnReceive(int);
		virtual void OnLost();

        void OnRequestReceived(char* pHeader, char* pData, uint32 dwDataLen);

		void SendContent(const char* szStdResponse, const void* pContent, uint32 dwContentSize);
		void SendData(const void* pData, uint32 dwDataSize);
		void SendHttpHeaders(const char * szType, bool use_gzip, uint32 content_len, int session_id);

		CWebServerBase *m_pParent;

		class CChunk {
			public:
				char* m_pData;
				char* m_pToSend;
				uint32 m_dwSize;

				CChunk* m_pNext;
				~CChunk() { if (m_pData) delete[] m_pData; }
		};

		CChunk *m_pHead; // tails of what has to be sent
		CChunk *m_pTail;

		bool m_IsGet, m_IsPost;
		char *m_Cookie;
		char *m_pBuf;
		uint32 m_dwBufSize;
		uint32 m_dwRecv;
		uint32 m_dwHttpHeaderLen;
		uint32 m_dwHttpContentLen;
};

#endif //WEBSERVER_H
// File_checked_for_headers

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "Types.h"
#include "Proxy.h"

class CEncryptedDatagramSocket : public CDatagramSocketProxy
{
public:
	CEncryptedDatagramSocket( wxIPaddress &address, wxSocketFlags flags = wxSOCKET_NONE,	const CProxyData *proxyData = NULL);
	virtual ~CEncryptedDatagramSocket();

protected:
	int DecryptReceivedClient(uint8* pbyBufIn, int nBufLen, uint8** ppbyBufOut, uint32 dwIP, uint16* nReceiverVerifyKey, uint16* nSenderVerifyKey) const;
	int EncryptSendClient(uint8** ppbyBuf, int nBufLen, const uint8* pachClientHashOrKadID, bool bKad, uint16 nReceiverVerifyKey, uint16 nSenderVerifyKey) const;
	
	int DecryptReceivedServer(uint8* pbyBufIn, int nBufLen, uint8** ppbyBufOut, uint32 dwBaseKey, uint32 dbgIP) const;
	int EncryptSendServer(uint8** ppbyBuf, int nBufLen, uint32 dwBaseKey) const;

};

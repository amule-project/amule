// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef CLIENTLIST_H
#define CLIENTLIST_H

#include <wx/dynarray.h>
#include "types.h"		// Needed for uint16 and uint32
#include "CTypedPtrList.h"	// Needed for CPtrList
#include "GetTickCount.h"
#include "updownclient.h"	// Needed for CUpDownClient

#include <map>

class CUpDownClient;
class CClientReqSocket;
class CMD4Hash;

#define BAN_CLEANUP_TIME	1200000 // 20 min

//------------CDeletedClient Class----------------------
// this class / list is a bit overkill, but currently needed to avoid any exploit possibtility
// it will keep track of certain clients attributes for 2 hours, while the CUpDownClient object might be deleted already
// currently: IP, Port, UserHash


struct PORTANDHASH{
	uint16 nPort;
	void* pHash;
};

WX_DECLARE_OBJARRAY(PORTANDHASH, ArrayOfPortAndHash);

class CDeletedClient{
public:
	CDeletedClient(CUpDownClient* pClient){
		m_dwInserted = ::GetTickCount();
		PORTANDHASH porthash = { pClient->GetUserPort(), pClient->Credits()};
		m_ItemsList.Add(porthash);
	}
	ArrayOfPortAndHash	m_ItemsList;
//	CArray<PORTANDHASH,PORTANDHASH> m_ItemsList;
	uint32							m_dwInserted;
};

class CClientList
{
public:
	CClientList();
	~CClientList();
	void	AddClient(CUpDownClient* toadd,bool bSkipDupTest = false);
	void	RemoveClient(CUpDownClient* toremove);

	typedef std::map<uint16, uint32> clientmap16;
	typedef std::map<uint32, uint32> clientmap32;
	
	void	GetStatistics(uint32 &totalclient, uint32 stats[], clientmap16 *clientStatus=NULL, clientmap32 *clientVersionEDonkey=NULL, clientmap32 *clientVersionEDonkeyHybrid=NULL, clientmap32 *clientVersionEMule=NULL, clientmap32 *clientVersionAMule=NULL); // xrmb : statsclientstatus
	void	DeleteAll();
	bool	AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender);
	CUpDownClient* FindClientByIP(uint32 clientip,uint16 port);
	CUpDownClient* VUGetRandomClient();
	bool	ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash);
	void AddTrackClient(CUpDownClient* toadd);
	uint16 GetClientsFromIP(uint32 dwIP);

	void	AddBannedClient(uint32 dwIP);
	bool	IsBannedClient(uint32 dwIP);
	void	RemoveBannedClient(uint32 dwIP);
	uint16	GetBannedCount() const		{return m_bannedList.size(); }


	void	Process();
	
	bool	Debug_IsValidClient(CUpDownClient* tocheck) const;
private:
	CTypedPtrList<CPtrList, CUpDownClient*> list;
	std::map<uint32, uint32> m_bannedList;
	std::map<uint32, CDeletedClient*> m_trackedClientsList;
	uint32	m_dwLastBannCleanUp;
	uint32	m_dwLastTrackedCleanUp;
};

#endif // CLIENTLIST_H

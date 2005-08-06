//
// This file is part of the aMule Project.
//
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

#ifndef UPLOADQUEUE_H
#define UPLOADQUEUE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "UploadQueue.h"
#endif

#include "Types.h"		// Needed for uint16, uint32 and uint64
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
#include "CMD4Hash.h"		// Needed for CMD4Hash

#include <list>

class CUpDownClient;


class CUploadQueue
{
public:
	CUploadQueue();
	~CUploadQueue();
	void	Process();
	void	AddClientToQueue(CUpDownClient* client);
	bool	RemoveFromUploadQueue(CUpDownClient* client,bool updatewindow = true);
	bool	RemoveFromWaitingQueue(CUpDownClient* client,bool updatewindow = true);
	bool	IsOnUploadQueue(CUpDownClient* client)	{return GetWaitingClient(client);}
	bool	IsDownloading(CUpDownClient* client)	{return GetDownloadingClient(client);}
	bool	CheckForTimeOver(CUpDownClient* client);
	POSITION GetFirstFromUploadList()		{return uploadinglist.GetHeadPosition();}
	CUpDownClient* GetNextFromUploadList(POSITION &curpos)	{return uploadinglist.GetNext(curpos);}
	CUpDownClient* GetQueueClientAt(POSITION &curpos)	{return uploadinglist.GetAt(curpos);}

	POSITION GetFirstFromWaitingList()		{return waitinglist.GetHeadPosition();}
	CUpDownClient* GetNextFromWaitingList(POSITION &curpos) {return waitinglist.GetNext(curpos);}
	CUpDownClient* GetWaitClientAt(POSITION &curpos)	{return waitinglist.GetAt(curpos);}
	CUpDownClient* GetWaitingClientByIP(uint32 dwIP);
	CUpDownClient* GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort);
	CUpDownClient* GetNextClient(CUpDownClient* update);

	uint16	GetWaitingPosition(CUpDownClient* client);
	void	SuspendUpload( const CMD4Hash& );
	void	ResumeUpload( const CMD4Hash& );

protected:
	void	RemoveFromWaitingQueue(POSITION pos);
	POSITION GetWaitingClient(CUpDownClient* client);
	POSITION GetDownloadingClient(CUpDownClient* client);
	bool	AcceptNewClient();
	void	AddUpNextClient(CUpDownClient* directadd = 0);

private:
	CTypedPtrList<CPtrList, CUpDownClient*> waitinglist;
	CTypedPtrList<CPtrList, CUpDownClient*> uploadinglist;
	typedef std::list<CMD4Hash> suspendlist;
	suspendlist suspended_uploads_list;  //list for suspended uploads
	uint32	m_nLastStartUpload;
	bool	lastupslotHighID; // VQB lowID alternation
};

#endif // UPLOADQUEUE_H

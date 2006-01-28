//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef UPLOADQUEUE_H
#define UPLOADQUEUE_H

#include "Types.h"		// Needed for uint16, uint32 and uint64
#include "MD4Hash.h"		// Needed for CMD4Hash

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
	bool	IsOnUploadQueue(CUpDownClient* client) const;
	bool	IsDownloading(CUpDownClient* client) const;
	bool	CheckForTimeOver(CUpDownClient* client);
	
	const CClientPtrList& GetWaitingList() const { return m_waitinglist; }
	const CClientPtrList& GetUploadingList() const { return m_uploadinglist; }
	
	CUpDownClient* GetWaitingClientByIP(uint32 dwIP);
	CUpDownClient* GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort);

	uint16	GetWaitingPosition(CUpDownClient* client);
	void	SuspendUpload( const CMD4Hash& );
	void	ResumeUpload( const CMD4Hash& );

private:
	void	RemoveFromWaitingQueue(CClientPtrList::iterator pos);
	bool	AcceptNewClient();
	void	AddUpNextClient(CUpDownClient* directadd = 0);

	CClientPtrList m_waitinglist;
	CClientPtrList m_uploadinglist;
	
	typedef std::list<CMD4Hash> suspendlist;
	suspendlist suspended_uploads_list;  //list for suspended uploads
	uint32	m_nLastStartUpload;
	bool	lastupslotHighID; // VQB lowID alternation
};

#endif // UPLOADQUEUE_H

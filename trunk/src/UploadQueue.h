//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "MD4Hash.h"		// Needed for CMD4Hash
#include <set>

class CUpDownClient;

class CUploadQueue
{
public:
	CUploadQueue();
	~CUploadQueue();
	void	Process();
	void	AddClientToQueue(CUpDownClient* client);
	bool	RemoveFromUploadQueue(CUpDownClient* client);
	bool	RemoveFromWaitingQueue(CUpDownClient* client);
	bool	IsOnUploadQueue(const CUpDownClient* client) const;
	bool	IsDownloading(CUpDownClient* client) const;
	bool	CheckForTimeOver(CUpDownClient* client);
	
	const CClientPtrList& GetWaitingList() const { return m_waitinglist; }
	const CClientPtrList& GetUploadingList() const { return m_uploadinglist; }
	
	CUpDownClient* GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort, bool bIgnorePortOnUniqueIP, bool* pbMultipleIPs = NULL);

	uint16	GetWaitingPosition(const CUpDownClient *client) const;
	uint16	SuspendUpload(const CMD4Hash &, bool terminate);
	void	ResumeUpload(const CMD4Hash &);

private:
	void	RemoveFromWaitingQueue(CClientPtrList::iterator pos);
	uint16	GetMaxSlots() const;
	void	AddUpNextClient(CUpDownClient* directadd = 0);
	bool	IsSuspended(const CMD4Hash& hash) { return suspendedUploadsSet.find(hash) != suspendedUploadsSet.end(); }

	CClientPtrList m_waitinglist;
	CClientPtrList m_uploadinglist;
	
	std::set<CMD4Hash> suspendedUploadsSet;  // set for suspended uploads
	uint32	m_nLastStartUpload;
	bool	lastupslotHighID; // VQB lowID alternation
	bool	m_allowKicking;
};

#endif // UPLOADQUEUE_H
// File_checked_for_headers

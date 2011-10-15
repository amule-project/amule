//
// This file is part of the aMule Project.
//
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

#ifndef UPLOADQUEUE_H
#define UPLOADQUEUE_H

#include "ClientRef.h"		// Needed for CClientRefList
#include "MD4Hash.h"		// Needed for CMD4Hash

// Experimental extended upload queue population
//
// When a client is set up from scratch (no shares, all downloads empty)
// it takes a while after completion of the first downloaded chunks until
// uploads start. Problem is, upload queue is empty, because clients that
// find nothing to download don't stay queued.
//
// Set this to 1 for faster finding of upload slots in this case.
// aMule will then try to contact its sources for uploading if the
// upload queue is empty.
#define EXTENDED_UPLOADQUEUE 0

class CUpDownClient;
class CKnownFile;

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
	bool	IsDownloading(const CUpDownClient* client) const;
	bool	CheckForTimeOver(CUpDownClient* client);
	void	ResortQueue() { SortGetBestClient(); }
	
	const CClientRefList& GetWaitingList() const { return m_waitinglist; }
	const CClientRefList& GetUploadingList() const { return m_uploadinglist; }
	
	CUpDownClient* GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort, bool bIgnorePortOnUniqueIP, bool* pbMultipleIPs = NULL);

	uint16	SuspendUpload(const CMD4Hash &, bool terminate);
	void	ResumeUpload(const CMD4Hash &);
	CKnownFile* GetAllUploadingKnownFile() { return m_allUploadingKnownFile; }

private:
	void	RemoveFromWaitingQueue(CClientRefList::iterator pos);
	uint16	GetMaxSlots() const;
	void	AddUpNextClient(CUpDownClient* directadd = 0);
	bool	IsSuspended(const CMD4Hash& hash) { return suspendedUploadsSet.find(hash) != suspendedUploadsSet.end(); }
	void	SortGetBestClient(CClientRef * bestClient = NULL);

	CClientRefList m_waitinglist;
	CClientRefList m_uploadinglist;

#if EXTENDED_UPLOADQUEUE
	CClientRefList m_possiblyWaitingList;
	int		PopulatePossiblyWaitingList();
#endif

	std::set<CMD4Hash> suspendedUploadsSet;  // set for suspended uploads
	uint32	m_nLastStartUpload;
	uint32	m_lastSort;
	bool	lastupslotHighID; // VQB lowID alternation
	bool	m_allowKicking;
	// This KnownFile collects all currently uploading clients for display in the upload list control
	CKnownFile * m_allUploadingKnownFile;
};

#endif // UPLOADQUEUE_H
// File_checked_for_headers

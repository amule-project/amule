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

#ifndef KNOWNFILELIST_H
#define KNOWNFILELIST_H


#include <unordered_set>

#include "SharedFileList.h" // CKnownFileMap


class CKnownFile;
class CPath;

class CKnownFileList
{
public:
	CKnownFileList();
	~CKnownFileList();
	bool	SafeAddKFile(CKnownFile* toadd, bool afterHashing = false);
	bool	Init();
	void	Save();
	void	Clear();
	CKnownFile* FindKnownFile(
		const CPath& filename,
		time_t in_date,
		uint64 in_size);
	CKnownFile* FindKnownFileByID(const CMD4Hash& hash);
	void	PrepareIndex();
	void	ReleaseIndex();

	// Latch set by CSharedFileList::Reload once a full share-scan has
	// finished in this session. PruneDuplicates only runs once this is
	// true, so the cap-prune never fires before the pin set is
	// populated by FindKnownFile-during-scan.
	void	MarkInitialShareScanComplete();

	uint16 requested;
	uint32 transferred;
	uint16 accepted;

private:
	wxMutex	list_mut;

	bool	Append(CKnownFile*, bool afterHashing = false);

	CKnownFile *IsOnDuplicates(
		const CPath& filename,
		uint32 in_date,
		uint64 in_size) const;

	bool KnownFileMatches(
		CKnownFile *knownFile,
		const CPath& filename,
		uint32 in_date,
		uint64 in_size) const;

	// Drop duplicate-list records whose hash has more than
	// KNOWN_DUPLICATE_HASH_CAP variants, keeping the newest by mtime.
	// `inUse` is a snapshot of pointers currently held by
	// CSharedFileList::m_Files_map: never pruned (would dangle the
	// share-list pointer). m_pinnedDuplicates additionally protects
	// records that FindKnownFile matched against a real on-disk file
	// during this session even when AddFile rejected them as
	// content-duplicates of an already-shared file.
	void	PruneDuplicates(const std::unordered_set<CKnownFile*> & inUse);

	typedef std::list<CKnownFile*> KnownFileList;
	KnownFileList	m_duplicateFileList;
	CKnownFileMap	m_knownFileMap;
	// The filename "known.met"
	wxString	m_filename;
	// Speed up shared files reload. The key is (size, mtime) rather than
	// size alone: libraries that contain many files of the same size
	// (small text files, fixed-quality JPEGs, fixed-bitrate audio/video)
	// would otherwise collapse FindKnownFile()'s equal_range into a
	// large bucket that the inner KnownFileMatches loop walks linearly,
	// turning the whole shared-list reload into O(N^2) over the same-
	// size files. Adding mtime to the key narrows the bucket aggressively
	// in any realistic library — files added at different times have
	// different mtimes — while keeping the per-entry cost identical
	// (8 bytes of key vs the previous 4).
	typedef std::multimap<std::pair<uint32, uint32>, CKnownFile*> KnownFileSizeMap;
	KnownFileSizeMap * m_knownSizeMap;
	KnownFileSizeMap * m_duplicateSizeMap;

	// Duplicate-list records that FindKnownFile / IsOnDuplicates
	// returned during this session — i.e. their (name, date, size)
	// matched a real on-disk file. Pinned across the rest of the
	// session so the cap-prune never drops a record that we know
	// still represents a live file (avoids re-hashing on next
	// restart). Cleared on Clear() / Init() so each session starts
	// fresh.
	std::unordered_set<CKnownFile*> m_pinnedDuplicates;

	// Set to true by MarkInitialShareScanComplete() at the end of the
	// first non-aborted CSharedFileList::Reload of the session.
	bool	m_initialShareScanComplete;
};

#endif // KNOWNFILELIST_H
// File_checked_for_headers

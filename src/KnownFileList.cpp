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


#include "KnownFileList.h"	// Interface declarations

#include <common/DataFileVersion.h>

#include <algorithm>
#include <map>
#include <memory>		// Do_not_auto_remove (lionel's Mac, 10.3)
#include <vector>
#include "PartFile.h"		// Needed for CPartFile
#include "amule.h"
#include "Logger.h"
#include "MemFile.h"
#include "ScopedPtr.h"
#include "SearchList.h"		// Needed for UpdateSearchFileByHash
#include "SharedFileList.h"
#include <common/Format.h>
#include "Preferences.h"	// Needed for thePrefs

// Max duplicate-list records retained per hash. Unique hashes always
// keep their live m_knownFileMap entry; this caps only the historical
// (name/date) variants in m_duplicateFileList. 8 covers daily-touch /
// weekly-snapshot / monthly-backup cycles while bounding the on-disk
// known.met at unique_hashes × (1 + cap).
#define KNOWN_DUPLICATE_HASH_CAP 8


// This function is inlined for performance
inline bool CKnownFileList::KnownFileMatches(
	CKnownFile *knownFile,
	const CPath& filename,
	uint32 in_date,
	uint64 in_size) const
{
	return
		(knownFile->GetLastChangeDatetime() == (time_t)in_date) &&
		(knownFile->GetFileSize() == in_size) &&
		(knownFile->GetFileName() == filename);
}


CKnownFileList::CKnownFileList()
{
	accepted = 0;
	requested = 0;
	transferred = 0;
	m_filename = "known.met";
	m_knownSizeMap = NULL;
	m_duplicateSizeMap = NULL;
	m_initialShareScanComplete = false;
	Init();
}


CKnownFileList::~CKnownFileList()
{
	Clear();
}


bool CKnownFileList::Init()
{
	CFile file;

	CPath fullpath = CPath(thePrefs::GetConfigDir() + m_filename);
	if (!fullpath.FileExists()) {
		// This is perfectly normal. The file was probably either
		// deleted, or this is the first time running aMule.
		return false;
	}

	if (!file.Open(fullpath)) {
		AddLogLineC(CFormat(_("WARNING: %s cannot be opened.")) % m_filename);
		return false;
	}

	try {
		uint8 version = file.ReadUInt8();
		if ((version != MET_HEADER) && (version != MET_HEADER_WITH_LARGEFILES)) {
			AddLogLineC(_("WARNING: Known file list corrupted, contains invalid header."));
			return false;
		}

		wxMutexLocker sLock(list_mut);
		uint32 RecordsNumber = file.ReadUInt32();
		AddDebugLogLineN(logKnownFiles, CFormat("Reading %i known files from file format 0x%2.2x.")
			% RecordsNumber % version);

		// Keep the size-map index live during the load. Append() is O(log N)
		// on every record, but on each MD4 hash collision (real-world
		// libraries hit these whenever the same content was indexed under
		// two paths/names) it falls back to IsOnDuplicates(name, date, size).
		// Without a duplicate-size index, IsOnDuplicates scans
		// m_duplicateFileList linearly, so the dedup cost grows with each
		// duplicate appended — O(N^2) over the whole load. Prebuilding the
		// (empty) index here lets Append maintain it incrementally, giving
		// the O(log N) equal_range fast path on every collision check.
		// Issue #562 startup gap, ~36 s on a 200 k-file library.
		PrepareIndex();
		for (uint32 i = 0; i < RecordsNumber; i++) {
			CScopedPtr<CKnownFile> record;
			if (record->LoadFromFile(&file)) {
				AddDebugLogLineN(logKnownFiles,
					CFormat("Known file read: %s") % record->GetFileName());
				Append(record.release());
			} else {
				AddLogLineC(_("Failed to load entry in known file list, file may be corrupt"));
			}
		}
		ReleaseIndex();
		AddDebugLogLineN(logKnownFiles, "Finished reading known files");

		return true;
	} catch (const CInvalidPacket& e) {
		ReleaseIndex();
		AddLogLineC(_("Invalid entry in known file list, file may be corrupt: ") + e.what());
	} catch (const CSafeIOException& e) {
		ReleaseIndex();
		AddLogLineC(CFormat(_("IO error while reading %s file: %s")) % m_filename % e.what());
	}

	return false;
}


void CKnownFileList::Save()
{
	// Snapshot the currently-shared-files pointer set without holding
	// our own lock -- CSharedFileList::CopyFileList briefly takes its
	// own mutex (the reverse lock order from
	// CSharedFileList::SafeAddKFile, which already does
	// sharedfiles-lock -> knownfiles-lock; doing it the same way here
	// would be a textbook ABBA). Anything in the snapshot is a
	// pointer SharedFileList is currently holding; PruneDuplicates
	// will never delete those.
	std::vector<CKnownFile*> sharedSnapshot;
	if (theApp && theApp->sharedfiles) {
		theApp->sharedfiles->CopyFileList(sharedSnapshot);
	}
	std::unordered_set<CKnownFile*> inUse(
		sharedSnapshot.begin(), sharedSnapshot.end());

	// Acquire the lock before opening the .new file. Save() is called
	// from both the main thread (on shutdown / scheduled persist) and
	// the hashing worker thread (CHashingTask::OnLastTask). If two
	// callers raced past the open, both would create known.met.new at
	// the same path, the first to Close() would rename it away, and
	// the second's rename would fail with ENOENT -- producing the
	// "Impossible to get permissions for file 'known.met.new'" /
	// "couldn't be renamed 'known.met.new' -> 'known.met'" pair seen
	// in #86. Holding list_mut around the whole save serialises the
	// .new lifecycle. The list itself is read-only inside, so this
	// doesn't widen the existing critical section meaningfully.
	wxMutexLocker sLock(list_mut);

	PruneDuplicates(inUse);

	CFile file(thePrefs::GetConfigDir() + m_filename, CFile::write_safe);
	if (!file.IsOpened()) {
		return;
	}

	AddDebugLogLineN(logKnownFiles, CFormat("start saving %s") % m_filename);

	try {
		// Kry - This is the version, but we don't know it till
		// we know if any largefile is saved. This allows the list
		// to be compatible with previous versions.
		bool bContainsAnyLargeFiles = false;
		file.WriteUInt8(0);

		file.WriteUInt32(m_knownFileMap.size() + m_duplicateFileList.size());

		// Duplicates handling. Duplicates needs to be saved first,
		// since it is the last entry that gets used.
		KnownFileList::iterator itDup = m_duplicateFileList.begin();
		for ( ; itDup != m_duplicateFileList.end(); ++itDup ) {
			(*itDup)->WriteToFile(&file);
			if ((*itDup)->IsLargeFile()) {
				bContainsAnyLargeFiles = true;
			}
		}

		CKnownFileMap::iterator it = m_knownFileMap.begin();
		for (; it != m_knownFileMap.end(); ++it) {
			it->second->WriteToFile(&file);
			if (it->second->IsLargeFile()) {
				bContainsAnyLargeFiles = true;
			}
		}

		file.Seek(0);
		file.WriteUInt8(bContainsAnyLargeFiles ? MET_HEADER_WITH_LARGEFILES : MET_HEADER);
		file.Close();
	} catch (const CIOFailureException& e) {
		AddLogLineC(CFormat(_("Error while saving %s file: %s")) % m_filename % e.what());
	}
	AddDebugLogLineN(logKnownFiles, CFormat("finished saving %s") % m_filename);
}


void CKnownFileList::Clear()
{
	wxMutexLocker sLock(list_mut);

	DeleteContents(m_knownFileMap);
	DeleteContents(m_duplicateFileList);
	ReleaseIndex();
	m_pinnedDuplicates.clear();
	m_initialShareScanComplete = false;
}


void CKnownFileList::MarkInitialShareScanComplete()
{
	wxMutexLocker sLock(list_mut);
	m_initialShareScanComplete = true;
}


CKnownFile* CKnownFileList::FindKnownFile(
	const CPath& filename,
	time_t in_date,
	uint64 in_size)
{
	wxMutexLocker sLock(list_mut);

	if (m_knownSizeMap) {
		const auto key = std::make_pair((uint32) in_size, (uint32) in_date);
		std::pair<KnownFileSizeMap::const_iterator, KnownFileSizeMap::const_iterator> p;
		p = m_knownSizeMap->equal_range(key);
		for (KnownFileSizeMap::const_iterator it = p.first; it != p.second; ++it) {
			CKnownFile *cur_file = it->second;
			if (KnownFileMatches(cur_file, filename, in_date, in_size)) {
				return cur_file;
			}
		}
	} else {
		for (CKnownFileMap::const_iterator it = m_knownFileMap.begin();
			 it != m_knownFileMap.end(); ++it) {
			CKnownFile *cur_file = it->second;
			if (KnownFileMatches(cur_file, filename, in_date, in_size)) {
				return cur_file;
			}
		}
	}

	// Pin any duplicate-list match against this session's prune so a
	// real on-disk file's record isn't dropped just because its hash
	// is also held by a more-recent live entry (the dual-content-copy
	// case: same hash in two shared paths -- one becomes m_Files_map,
	// the other only ever appears here).
	CKnownFile * dup = IsOnDuplicates(filename, in_date, in_size);
	if (dup) {
		m_pinnedDuplicates.insert(dup);
	}
	return dup;
}


CKnownFile *CKnownFileList::IsOnDuplicates(
	const CPath& filename,
	uint32 in_date,
	uint64 in_size) const
{
	if (m_duplicateSizeMap) {
		const auto key = std::make_pair((uint32) in_size, (uint32) in_date);
		std::pair<KnownFileSizeMap::const_iterator, KnownFileSizeMap::const_iterator> p;
		p = m_duplicateSizeMap->equal_range(key);
		for (KnownFileSizeMap::const_iterator it = p.first; it != p.second; ++it) {
			CKnownFile *cur_file = it->second;
			if (KnownFileMatches(cur_file, filename, in_date, in_size)) {
				return cur_file;
			}
		}
	} else {
		for (KnownFileList::const_iterator it = m_duplicateFileList.begin();
			 it != m_duplicateFileList.end(); ++it) {
			CKnownFile *cur_file = *it;
			if (KnownFileMatches(cur_file, filename, in_date, in_size)) {
				return cur_file;
			}
		}
	}
	return NULL;
}


CKnownFile* CKnownFileList::FindKnownFileByID(const CMD4Hash& hash)
{
	wxMutexLocker sLock(list_mut);

	if (!hash.IsEmpty()) {
		if (m_knownFileMap.find(hash) != m_knownFileMap.end()) {
			return m_knownFileMap[hash];
		} else {
			return NULL;
		}
	}
	return NULL;

}


bool CKnownFileList::SafeAddKFile(CKnownFile* toadd, bool afterHashing)
{
	bool ret;
	{
		wxMutexLocker sLock(list_mut);
		ret = Append(toadd, afterHashing);
	}
	if (ret) {
		theApp->searchlist->UpdateSearchFileByHash(toadd->GetFileHash());
	}
	return ret;
}


bool CKnownFileList::Append(CKnownFile *Record, bool afterHashing)
{
	if (Record->GetFileSize() > 0) {
		// sanity check if the number of part hashes is correct here
		if (Record->GetHashCount() != Record->GetED2KPartHashCount()) {
			AddDebugLogLineC(logKnownFiles, CFormat("%s with size %d should have %d part hashes, but only %d are available")
				% Record->GetFileName().GetPrintable() % Record->GetFileSize() % Record->GetED2KPartHashCount() % Record->GetHashCount());
			return false;
		}
		const CMD4Hash& tkey = Record->GetFileHash();
		CKnownFileMap::iterator it = m_knownFileMap.find(tkey);
		if (it == m_knownFileMap.end()) {
			m_knownFileMap[tkey] = Record;
			if (m_knownSizeMap) {
				m_knownSizeMap->insert(
					std::make_pair(
						std::make_pair((uint32) Record->GetFileSize(),
							(uint32) Record->GetLastChangeDatetime()),
						Record));
			}
			return true;
		} else {
			CKnownFile *existing = it->second;
			if (KnownFileMatches(Record, existing->GetFileName(), existing->GetLastChangeDatetime(), existing->GetFileSize())) {
				// The file is already on the list, ignore it.
				AddDebugLogLineN(logKnownFiles, CFormat("%s is already on the list") % Record->GetFileName().GetPrintable());
				return false;
			} else if (CKnownFile * dup = IsOnDuplicates(
					Record->GetFileName(), Record->GetLastChangeDatetime(),
					Record->GetFileSize())) {
				// The file is on the duplicates list, ignore it.
				// Should not happen, at least not after hashing. Or why did it get hashed in the first place then?
				AddDebugLogLineN(logKnownFiles, CFormat("%s is on the duplicates list") % Record->GetFileName().GetPrintable());
				// Pin the duplicate -- Append got here from a real
				// on-disk file (we just hashed it), so this record
				// matches a live file and the next-session scan
				// would benefit from finding it without re-hashing.
				m_pinnedDuplicates.insert(dup);
				return false;
			} else {
				if (afterHashing && existing->GetFileSize() == Record->GetFileSize()) {
					// We just hashed a "new" shared file and find it's already known under a different name or date.
					// Guess what - it was probably renamed or touched.
					// So copy over all properties from the existing known file and just keep name/date.
					time_t newDate = Record->GetLastChangeDatetime();
					CPath newName = Record->GetFileName();
					CMemFile f;
					existing->WriteToFile(&f);
					f.Reset();
					if (!Record->LoadFromFile(&f)) {
						// this also shouldn't happen
						AddDebugLogLineC(logKnownFiles, CFormat("error copying known file: existing: %s %d %d %d  Record: %s %d %d %d")
							% existing->GetFileName().GetPrintable() % existing->GetFileSize() % existing->GetED2KPartHashCount() % existing->GetHashCount()
							% Record->GetFileName().GetPrintable() % Record->GetFileSize() % Record->GetED2KPartHashCount() % Record->GetHashCount());
						return false;
					}
					Record->SetLastChangeDatetime(newDate);
					Record->SetFileName(newName);
				}
				// The file is a duplicated hash. Add THE OLD ONE to the duplicates list.
				// (This is used when reading the known file list where the duplicates are stored in front.)
				m_duplicateFileList.push_back(existing);
				if (m_duplicateSizeMap) {
					m_duplicateSizeMap->insert(
						std::make_pair(
							std::make_pair((uint32) existing->GetFileSize(),
								(uint32) existing->GetLastChangeDatetime()),
							existing));
				}
				if (theApp->sharedfiles) {
					// Removing the old kad keywords created with the old filename
					theApp->sharedfiles->RemoveKeywords(existing);
				}
				if (m_knownSizeMap) {
					// existing is leaving m_knownFileMap for m_duplicateFileList;
					// drop its size-map entry so FindKnownFile doesn't return a
					// pointer that no longer belongs to the live map.
					const auto existingKey = std::make_pair(
						(uint32) existing->GetFileSize(),
						(uint32) existing->GetLastChangeDatetime());
					std::pair<KnownFileSizeMap::iterator,
						KnownFileSizeMap::iterator> p =
						m_knownSizeMap->equal_range(existingKey);
					for (KnownFileSizeMap::iterator hit = p.first;
						hit != p.second; ++hit) {
						if (hit->second == existing) {
							m_knownSizeMap->erase(hit);
							break;
						}
					}
					m_knownSizeMap->insert(
						std::make_pair(
							std::make_pair((uint32) Record->GetFileSize(),
								(uint32) Record->GetLastChangeDatetime()),
							Record));
				}
				m_knownFileMap[tkey] = Record;
				return true;
			}
		}
	} else {
		AddDebugLogLineN(logGeneral,
			CFormat("%s is 0-size, not added") %
			Record->GetFileName());

		return false;
	}
}

// Make a (size, mtime) index to speed up FindKnownFile. Size + mtime
// modulo 2^32 is enough here — same precision the rest of the file
// uses for FindKnownFile's inputs and KnownFileMatches' comparisons.
void CKnownFileList::PrepareIndex()
{
	ReleaseIndex();
	m_knownSizeMap = new KnownFileSizeMap;
	for (CKnownFileMap::const_iterator it = m_knownFileMap.begin(); it != m_knownFileMap.end(); ++it) {
		m_knownSizeMap->insert(std::make_pair(
			std::make_pair((uint32) it->second->GetFileSize(),
				(uint32) it->second->GetLastChangeDatetime()),
			it->second));
	}
	m_duplicateSizeMap = new KnownFileSizeMap;
	for (KnownFileList::const_iterator it = m_duplicateFileList.begin(); it != m_duplicateFileList.end(); ++it) {
		m_duplicateSizeMap->insert(std::make_pair(
			std::make_pair((uint32) (*it)->GetFileSize(),
				(uint32) (*it)->GetLastChangeDatetime()),
			*it));
	}
}


void CKnownFileList::ReleaseIndex()
{
	delete m_knownSizeMap;
	delete m_duplicateSizeMap;
	m_knownSizeMap = NULL;
	m_duplicateSizeMap = NULL;
}


void CKnownFileList::PruneDuplicates(
	const std::unordered_set<CKnownFile*> & inUse)
{
	// Caller must hold list_mut.

	// Gate on a full share-scan having run this session -- before that,
	// inUse is empty and m_pinnedDuplicates hasn't been populated by
	// FindKnownFile yet, so a prune here would drop records the next
	// scan would have legitimately pinned. Set true by
	// MarkInitialShareScanComplete() from CSharedFileList::Reload.
	if (!m_initialShareScanComplete) {
		return;
	}

	if (m_duplicateFileList.empty()) {
		return;
	}

	// Bucket the duplicate list by hash, recording the list iterator so
	// the eventual erase is O(1). Pinned/in-use records are excluded
	// from the prune candidate set up front: they're load-bearing
	// (in_use would dangle a SharedFileList pointer; m_pinnedDuplicates
	// represents a real on-disk file matched this session). The cap
	// applies only to the unprotected remainder, so the effective
	// per-hash retention is protected + min(prunable, cap).
	std::map<CMD4Hash, std::vector<KnownFileList::iterator> > prunable;
	for (KnownFileList::iterator it = m_duplicateFileList.begin();
		it != m_duplicateFileList.end(); ++it) {
		CKnownFile * record = *it;
		if (inUse.count(record) > 0) {
			continue;
		}
		if (m_pinnedDuplicates.count(record) > 0) {
			continue;
		}
		prunable[record->GetFileHash()].push_back(it);
	}

	size_t dropped = 0;
	for (std::map<CMD4Hash,
			std::vector<KnownFileList::iterator> >::iterator
		bucket = prunable.begin();
		bucket != prunable.end(); ++bucket) {
		std::vector<KnownFileList::iterator> & iters = bucket->second;
		if (iters.size() <= KNOWN_DUPLICATE_HASH_CAP) {
			continue;
		}

		// Sort by mtime descending so the K newest survive. mtime is
		// the only freshness signal we have without persisting a
		// per-record lastSeen tag -- a record whose mtime is newer is
		// more likely to "come back" via mtime-restore tools.
		std::sort(iters.begin(), iters.end(),
			[](const KnownFileList::iterator & a,
				const KnownFileList::iterator & b) {
				return (*a)->GetLastChangeDatetime()
					> (*b)->GetLastChangeDatetime();
			});

		for (size_t i = KNOWN_DUPLICATE_HASH_CAP;
			i < iters.size(); ++i) {
			CKnownFile * dead = *iters[i];

			if (m_duplicateSizeMap) {
				const auto key = std::make_pair(
					(uint32) dead->GetFileSize(),
					(uint32) dead->GetLastChangeDatetime());
				std::pair<KnownFileSizeMap::iterator,
					KnownFileSizeMap::iterator> p =
					m_duplicateSizeMap->equal_range(key);
				for (KnownFileSizeMap::iterator hit = p.first;
					hit != p.second; ++hit) {
					if (hit->second == dead) {
						m_duplicateSizeMap->erase(hit);
						break;
					}
				}
			}

			m_duplicateFileList.erase(iters[i]);
			delete dead;
			++dropped;
		}
	}

	if (dropped > 0) {
		AddDebugLogLineN(logKnownFiles,
			CFormat("Pruned %u duplicate-hash records over per-hash cap of %u")
				% (unsigned)dropped % KNOWN_DUPLICATE_HASH_CAP);
	}
}

// File_checked_for_headers

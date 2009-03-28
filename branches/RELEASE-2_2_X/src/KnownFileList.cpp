//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "KnownFileList.h"	// Interface declarations

#include <common/DataFileVersion.h>

#include <memory>		// Do_not_auto_remove (lionel's Mac, 10.3)
#include "PartFile.h"		// Needed for CPartFile
#include "amule.h"
#include "Logger.h"
#include <common/Format.h>


// This function is inlined for performance
inline bool CKnownFileList::KnownFileMatches(
	CKnownFile *knownFile,
	const CPath& filename,
	uint32 in_date,
	uint64 in_size) const
{
	return
		(knownFile->GetLastChangeDatetime() == in_date) &&
		(knownFile->GetFileSize() == in_size) &&
		(knownFile->GetFileName() == filename);
}


CKnownFileList::CKnownFileList()
{
	accepted = 0;
	requested = 0;
	transferred = 0;
	Init();
}


CKnownFileList::~CKnownFileList()
{
	Clear();
}


bool CKnownFileList::Init()
{
	CFile file;
	
	CPath fullpath = CPath(theApp->ConfigDir + wxT("known.met"));
	if (!fullpath.FileExists()) {
		// This is perfectly normal. The file was probably either
		// deleted, or this is the first time running aMule.
		return false;
	}

	if (!file.Open(fullpath)) {
		AddLogLineM(true, _("WARNING: known.met cannot be opened."));
		return false;
	}
	
	try {
		uint8 version = file.ReadUInt8();
		if ((version != MET_HEADER) && (version != MET_HEADER_WITH_LARGEFILES)) {
			AddLogLineM(true, _("WARNING: Knownfile list corrupted, contains invalid header."));
			return false;
		}
		
		wxMutexLocker sLock(list_mut);
		uint32 RecordsNumber = file.ReadUInt32();
		AddDebugLogLineM(false, logKnownFiles,
			wxString::Format(wxT("Reading %i known files from file format 0x%2.2x."),
			RecordsNumber, version));
		for (uint32 i = 0; i < RecordsNumber; i++) {
			std::auto_ptr<CKnownFile> record(new CKnownFile());
			if (record->LoadFromFile(&file)) {
				AddDebugLogLineM(false, logKnownFiles,
					CFormat(wxT("Known file read: %s")) % record->GetFileName());
				Append(record.release());
			} else {
				AddLogLineM(true,
					wxT("Failed to load entry in knownfilelist, file may be corrupt"));
			}
		}
		AddDebugLogLineM(false, logKnownFiles, wxT("Finished reading known files"));
	
		return true;
	} catch (const CInvalidPacket& e) {
		AddLogLineM(true, wxT("Invalid entry in knownfilelist, file may be corrupt: ") + e.what());
	} catch (const CSafeIOException& e) {
		AddLogLineM(true, CFormat(_("IO error while reading known.met file: %s")) % e.what());
	}	
	
	return false;
}


void CKnownFileList::Save()
{
	CFile file(theApp->ConfigDir + wxT("known.met"), CFile::write);
	if (!file.IsOpened()) {
		return;
	}

	wxMutexLocker sLock(list_mut);

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
	} catch (const CIOFailureException& e) {
		AddLogLineM(true, CFormat(_("Error while saving known.met file: %s")) % e.what());
	}
}


void CKnownFileList::Clear()
{	
	wxMutexLocker sLock(list_mut);

	for (CKnownFileMap::iterator it = m_knownFileMap.begin();
	     it != m_knownFileMap.end(); ++it) {
		delete it->second;
	}
	m_knownFileMap.clear();

	for (KnownFileList::iterator it = m_duplicateFileList.begin();
	     it != m_duplicateFileList.end(); ++it) {
		delete *it;
	}
	m_duplicateFileList.clear();
}


CKnownFile* CKnownFileList::FindKnownFile(
	const CPath& filename,
	time_t in_date,
	uint64 in_size)
{
	wxMutexLocker sLock(list_mut);
	
	for (CKnownFileMap::const_iterator it = m_knownFileMap.begin();
	     it != m_knownFileMap.end(); ++it) {
		CKnownFile *cur_file = it->second;
		if (KnownFileMatches(cur_file, filename, in_date, in_size)) {
			return cur_file;
		}
	}

	return IsOnDuplicates(filename, in_date, in_size);
}


CKnownFile *CKnownFileList::IsOnDuplicates(
	const CPath& filename,
	uint32 in_date,
	uint64 in_size) const
{
	for (KnownFileList::const_iterator it = m_duplicateFileList.begin();
	     it != m_duplicateFileList.end(); ++it) {
		CKnownFile *cur_file = *it;
		if (KnownFileMatches(cur_file, filename, in_date, in_size)) {
			return cur_file;
		}
	}
	return NULL;
}


bool CKnownFileList::IsKnownFile(const CKnownFile *file) 
{
	wxCHECK(file, false);

	wxMutexLocker sLock(list_mut);

	// For the map, search with the key
	const CMD4Hash &key = file->GetFileHash();
	CKnownFileMap::const_iterator itMap = m_knownFileMap.find(key);
	if (itMap != m_knownFileMap.end()) {
		return true;
	}
	// For the list, we have to iterate to search
	for (KnownFileList::iterator it = m_duplicateFileList.begin();
	     it != m_duplicateFileList.end(); ++it) {
		if (*it == file) {
			return true;
		}
	}
	return false;
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


bool CKnownFileList::SafeAddKFile(CKnownFile* toadd)
{
	wxMutexLocker sLock(list_mut);
	return Append(toadd);
}


bool CKnownFileList::Append(CKnownFile *Record)
{
	if (Record->GetFileSize() > 0) {
		const CMD4Hash& tkey = Record->GetFileHash();
		CKnownFileMap::iterator it = m_knownFileMap.find(tkey);
		if (it == m_knownFileMap.end()) {
			m_knownFileMap[tkey] = Record;			
			return true;
		} else {
			it->second;
			time_t in_date =  it->second->GetLastChangeDatetime();
			uint64 in_size =  it->second->GetFileSize();
			CPath filename = it->second->GetFileName();
			if (KnownFileMatches(Record, filename, in_date, in_size) ||
			    IsOnDuplicates(filename, in_date, in_size)) {
				// The file is already on the list, ignore it.
				return false;
			} else {
				// The file is a duplicated hash. Add THE OLD ONE to the duplicates list.
				m_duplicateFileList.push_back(m_knownFileMap[tkey]);
				// Is this thread-safe? If John is not sure and I'm not sure either...
				if (theApp->sharedfiles) {
					// Removing the old kad keywords created with the old filename
					theApp->sharedfiles->RemoveKeywords(it->second);
				}
				m_knownFileMap[tkey] = Record;	
				return true;
			}
		}
	} else {
		AddDebugLogLineM(false, logGeneral,
			CFormat(wxT("%s is 0-size, not added")) %
			Record->GetFileName());
		
		return false;
	}
}

// File_checked_for_headers

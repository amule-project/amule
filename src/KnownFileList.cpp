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

#include <wx/utils.h>

#include "KnownFileList.h"	// Interface declarations
#include "CFile.h"		// Needed for CFile
#include "PartFile.h"		// Needed for CPartFile
#include <common/StringFunctions.h> // Needed for unicode2char
#include "amule.h"
#include "MD4Hash.h"		// Needed for CMD4Hash
#include "Logger.h"
#include <common/Format.h>


CKnownFileList::CKnownFileList()
{
	accepted = 0;
	requested = 0;
	transfered = 0;
	Init();
}


CKnownFileList::~CKnownFileList()
{
	Clear();
}


bool CKnownFileList::Init()
{
	CFile file;
	
	wxString fullpath = theApp.ConfigDir + wxT("known.met");
	if (!wxFileExists(fullpath)) {
		return false;
	}

	if (!file.Open(fullpath)) {
		return false;
	}
	
	try {
		if (file.ReadUInt8() != MET_HEADER) {
			AddLogLineM(true, _("Warning: Knownfile list corrupted, contains invalid header."));
			return false;
		}
		
		wxMutexLocker sLock(list_mut);
		uint32 RecordsNumber = file.ReadUInt32();
		for (uint32 i = 0; i < RecordsNumber; i++) {
			CKnownFile* pRecord =  new CKnownFile();
			bool loaded = false;
			try {
				loaded = pRecord->LoadFromFile(&file);
			} catch (...) {
				delete pRecord;
				throw;
			}
				
			if (loaded) {
				Append(pRecord);
			} else {			
				delete pRecord;
			}
		}
	
		return true;
	} catch (const CSafeIOException& e) {
		AddLogLineM(true, CFormat(_("IO error while reading known.met file: %s")) % e.what());
		
		return false;
	}	
}

void CKnownFileList::Save()
{
	CFile file(theApp.ConfigDir + wxT("known.met"), CFile::write);
	if (!file.IsOpened()) {
		return;
	}

	wxMutexLocker sLock(list_mut);

	try {
		file.WriteUInt8(MET_HEADER);
		file.WriteUInt32(m_map.size() + m_duplicates.size());

		CKnownFileMap::iterator it = m_map.begin();
		for (; it != m_map.end(); ++it) {
			it->second->WriteToFile(&file);
		}
	
		// Kry - Duplicates handling.
		KnownFileList::iterator itDup = m_duplicates.begin();
		for ( ; itDup != m_duplicates.end(); ++itDup ) {
			(*itDup)->WriteToFile(&file);
		}
	} catch (const CIOFailureException& e) {
		AddLogLineM(true, CFormat(_("Error while saving known.met file: %s")) % e.what());
	}
}


void CKnownFileList::Clear()
{	
	wxMutexLocker sLock(list_mut);
	for ( CKnownFileMap::iterator it = m_map.begin(); it != m_map.end(); it++ )
		delete it->second;
	m_map.clear();

	KnownFileList::iterator it = m_duplicates.begin();
	for ( ; it != m_duplicates.end(); ++it ) {
		delete *it;
	}
	m_duplicates.clear();
}

CKnownFile* CKnownFileList::FindKnownFile(wxString filename,uint32 in_date,uint32 in_size){

	wxMutexLocker sLock(list_mut);
	
	CKnownFile* cur_file;

	for (CKnownFileMap::iterator pos = m_map.begin(); pos != m_map.end(); pos++ ) {
		cur_file = pos->second;
		if ((abs((int)cur_file->GetFileDate() - (int)in_date) < 20) && cur_file->GetFileSize() == in_size && (cur_file->GetFileName() == filename)) {
			return cur_file;
		}
	}

	return IsOnDuplicates(filename, in_date, in_size);
}

CKnownFile* CKnownFileList::FindKnownFileByID(const CMD4Hash& hash) {
	
	wxMutexLocker sLock(list_mut);
	
	if (!hash.IsEmpty())
	{
		if (m_map.find(hash) != m_map.end()) {
			return m_map[hash];
		} else {
			return NULL;	
		}
	}
	return NULL;	

}

bool CKnownFileList::SafeAddKFile(CKnownFile* toadd) {
	wxMutexLocker sLock(list_mut);
	return Append(toadd);
}

bool CKnownFileList::Append(CKnownFile* Record)
{
	if (Record->GetFileSize() > 0) {
		const CMD4Hash& tkey = Record->GetFileHash();
		CKnownFileMap::iterator it = m_map.find(tkey);
		if ( it == m_map.end() ) {
			m_map[tkey] = Record;			
			return true;
		} else {
			it->second;
			uint32 in_date =  it->second->GetFileDate();
			uint32 in_size =  it->second->GetFileSize();
			wxString filename = it->second->GetFileName();
			if (((abs((int)Record->GetFileDate() - (int)in_date) < 20) && Record->GetFileSize() == in_size && (Record->GetFileName() == filename)) || IsOnDuplicates(filename, in_date, in_size)) {
				// The file is already on the list, ignore it.
				return false;
			} else {
				// The file is a duplicated hash. Add THE OLD ONE to the duplicates list.
				m_duplicates.push_back(m_map[tkey]);
				// Is this thread-safe? If John is not sure and I'm not sure either...
				if (theApp.sharedfiles) {
					// Removing the old kad keywords created with the old filename
					theApp.sharedfiles->RemoveKeywords(it->second);
				}
				m_map[tkey] = Record;	
				return true;
			}
		}
	} else {
		AddDebugLogLineM( false, logGeneral,
			CFormat( wxT("%s is 0-size, not added") )
				% Record->GetFileName()
		);
		
		return false;
	}
}


CKnownFile* CKnownFileList::IsOnDuplicates(wxString filename,uint32 in_date,uint32 in_size) const
{
	KnownFileList::const_iterator it = m_duplicates.begin();
	for ( ; it != m_duplicates.end(); ++it ) {
		CKnownFile* cur_file = *it;
		if ((abs((int)cur_file->GetFileDate() - (int)in_date) < 20) && cur_file->GetFileSize() == in_size && (cur_file->GetFileName() == filename)) {
			return cur_file;
		}
		
	}	
	return NULL;
}

bool CKnownFileList::IsKnownFile(const CKnownFile* file) 
{
	if (file) {
		return FindKnownFileByID(file->GetFileHash()) != NULL;
	}
	return false;
}

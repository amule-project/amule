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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "KnownFileList.h"
#endif

#include <wx/utils.h>

#include "KnownFileList.h"	// Interface declarations
#include "SafeFile.h"		// Needed for CSafeFile
#include "PartFile.h"		// Needed for CPartFile
#include "StringFunctions.h" // Needed for unicode2char
#include "amule.h"
#include "CMD4Hash.h"		// Needed for CMD4Hash
#include "Logger.h"
#include "Format.h"

#include <wx/listimpl.cpp> // ye old magic incantation
WX_DEFINE_LIST(KnownFileList)

CKnownFileList::CKnownFileList() {
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
	CSafeFile file;
	uint32 result = false;
	try {
		
		wxString fullpath(theApp.ConfigDir + wxT("known.met"));
		if (!wxFileExists(fullpath)) {
			return false;
		}
		
		file.Open(fullpath);
		if (file.ReadUInt8() /*header*/ != MET_HEADER) {
			file.Close();
			return false;
		}
		wxMutexLocker sLock(list_mut);
		uint32 RecordsNumber = file.ReadUInt32();
		for (uint32 i = 0; i != RecordsNumber; i++) {
			CKnownFile* pRecord =  new CKnownFile();
			if (!pRecord->LoadFromFile(&file)){
				delete pRecord;
				continue;
			}
			Append(pRecord);
		}
		file.Close();
		result = true;
	} catch (...) {
		AddLogLineM(true,_("Error reading known.met file! (corrupted?)"));
	}
	
	return result;
}

void CKnownFileList::Save() {

	CSafeFile* file = new CSafeFile();
	wxString fullpath(theApp.ConfigDir + wxT("known.met"));
	file->Open(fullpath, CFile::write);
	if (!(file->IsOpened())) {
		delete file;
		return;
	}


	wxMutexLocker sLock(list_mut);

	file->WriteUInt8(MET_HEADER);
	uint32 RecordsNumber = m_map.size();
	file->WriteUInt32(RecordsNumber + duplicates.GetCount());

	CKnownFileMap::iterator it = m_map.begin();
	for (uint32 i = 0; i != RecordsNumber; i++,it++) {
		if ( it == m_map.end() )
			break;		// TODO: Throw an exception
		it->second->WriteToFile(file);
	}
	// Kry - Duplicates handling.
	KnownFileList::compatibility_iterator node = duplicates.GetFirst();
	while (node) {
		CKnownFile* duplicate = node->GetData();
		duplicate->WriteToFile(file);
		node = node->GetNext();
	}	
	
	file->Flush();
	file->Close();
	delete file;
}

void CKnownFileList::Clear() {	
	wxMutexLocker sLock(list_mut);
	for ( CKnownFileMap::iterator it = m_map.begin(); it != m_map.end(); it++ )
		delete it->second;
	m_map.clear();

	KnownFileList::compatibility_iterator node = duplicates.GetFirst();
	while (node) {
		CKnownFile* duplicate = node->GetData();
		delete duplicate;
		node = duplicates.GetFirst();
	}		
	
}

CKnownFile* CKnownFileList::FindKnownFile(wxString filename,uint32 in_date,uint32 in_size){

	wxMutexLocker sLock(list_mut);
	
	CKnownFile* cur_file;

	for (CKnownFileMap::iterator pos = m_map.begin(); pos != m_map.end(); pos++ ) {
		cur_file = pos->second;
		if ((abs((int)cur_file->GetFileDate() - (int)in_date) < 20) && cur_file->GetFileSize() == in_size && !cur_file->GetFileName().Cmp(filename)) {
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
			if (((abs((int)Record->GetFileDate() - (int)in_date) < 20) && Record->GetFileSize() == in_size && !Record->GetFileName().Cmp(filename)) || IsOnDuplicates(filename, in_date, in_size)) {
				// The file is already on the list, ignore it.
				return false;
			} else {
				// The file is a duplicated hash. Add THE OLD ONE to the duplicates list.
				duplicates.Append(m_map[tkey]);
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

CKnownFile* CKnownFileList::IsOnDuplicates(wxString filename,uint32 in_date,uint32 in_size) const {

	for (KnownFileList::compatibility_iterator node = duplicates.GetFirst(); node; node = node->GetNext()) {
		CKnownFile* cur_file = node->GetData();
		if ((abs((int)cur_file->GetFileDate() - (int)in_date) < 20) && cur_file->GetFileSize() == in_size && !cur_file->GetFileName().Cmp(filename)) {
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

//this file is part of the aMule Project
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


#include <wx/utils.h>

#include "KnownFileList.h"	// Interface declarations
#include "SafeFile.h"		// Needed for CSafeFile
#include "PartFile.h"		// Needed for CPartFile
#include "otherfunctions.h" // md4cmp
#include "amule.h"
#include "CMD4Hash.h"		// Needed for CMD4Hash

#include <wx/listimpl.cpp> // ye old magic incantation
WX_DEFINE_LIST(KnownFileList);

CKnownFileList::CKnownFileList() {
	accepted = 0;
	requested = 0;
	transfered = 0;
	Init();
}

CKnownFileList::~CKnownFileList() {
// TODO: Rather find the source of the lock leakage
//	list_mut.Unlock();
	Clear();
}

bool CKnownFileList::Init() {
	CSafeFile file;
	try {
		
		wxString fullpath(theApp.ConfigDir + wxT("known.met"));
		if (!wxFileExists(fullpath)) {//CFile::modeRead|CFile::osSequentialScan)) {
			return false;
		}
		
		uint8 header;
		file.Open(fullpath);
		file.Read(&header,1);
		if (header != MET_HEADER) {
			file.Close();
			return false;
		}
		//CSingleLock sLock(&list_mut,true); // to make sure that its thread-safe
		wxMutexLocker sLock(list_mut);
		uint32 RecordsNumber;
		file.Read(&RecordsNumber,4);
		ENDIAN_SWAP_I_32(RecordsNumber);
		for (uint32 i = 0; i != RecordsNumber; i++) {
			CKnownFile* pRecord =  new CKnownFile();
			if (!pRecord->LoadFromFile(&file)){
				//printf("*** Failed to load entry %u (name=%s  hash=%s  size=%u  parthashs=%u expected parthashs=%u) from known.met\n", i, pRecord->GetFileName(), md4str(pRecord->GetFileHash()).c_str(), pRecord->GetFileSize(), pRecord->GetHashCount(), pRecord->GetED2KPartHashCount());
				delete pRecord;
				continue;
			}
			Append(pRecord);
		}
		//sLock.Unlock();
		file.Close();
		return true;
	}
#if 0
	catch(CFileException* error) {
		if (error->m_cause == CFileException::endOfFile)
			AddLogLineM(true, _("Error: the file known.met is corrupted, unable to load known files"));
		else{
			char buffer[150];
			error->GetErrorMessage(buffer,150);
			AddLogLineM(true, _("Unexpected file error while reading known.met: %s, unable to load known files"), buffer);
		}
		error->Delete();	//memleak fix
		return false;
	}
#endif
	catch (...) {
		printf("Caught unknown exception on CKnownFileList::Init()\n");
		return false;
	}
}

void CKnownFileList::Save() {

	CFile* file = new CFile();
	wxString fullpath(theApp.ConfigDir + wxT("known.met"));
	file->Open(fullpath, CFile::write);
	if (!(file->IsOpened())) {
		delete file;
		return;
	}


	wxMutexLocker sLock(list_mut);
	//AddLogLineM(false,_("KnownFileList Save Starts"));
	uint8 ucHeader = MET_HEADER;
	//AddLogLineM(false,_("Saved MET_HEADER"));
	file->Write(&ucHeader, 1);
	uint32 RecordsNumber = m_map.size() + duplicates.GetCount();
	//AddLogLineF(false,_("RecordsNumber = %i"), RecordsNumber);
	ENDIAN_SWAP_I_32(RecordsNumber);
	//AddLogLineF(false,_("Endian RecordsNumber = %i"), RecordsNumber);		
	file->Write(&RecordsNumber,4);
	RecordsNumber = m_map.size();
	CKnownFileMap::iterator it = m_map.begin();
	for (uint32 i = 0; i != RecordsNumber; i++,it++) {
		if ( it == m_map.end() )
			break;		// TODO: Throw an exception
		it->second->WriteToFile(file);
	}
	// Kry - Duplicates handling.
	KnownFileList::Node* node = duplicates.GetFirst();
	while (node) {
		CKnownFile* duplicate = node->GetData();
		duplicate->WriteToFile(file);
		node = node->GetNext();
	}	
	
	file->Flush();
	file->Close();
	//AddLogLineM(false,_("KnownFileList Save Ends"));	
	delete file;
}

void CKnownFileList::Clear() {	
	wxMutexLocker sLock(list_mut);
	for ( CKnownFileMap::iterator it = m_map.begin(); it != m_map.end(); it++ )
		delete it->second;
	m_map.clear();

	KnownFileList::Node *node = duplicates.GetFirst();
	while (node) {
		CKnownFile* duplicate = node->GetData();
		delete duplicate;
		delete node;
		node = duplicates.GetFirst();
	}		
	
}

CKnownFile* CKnownFileList::FindKnownFile(wxString filename,uint32 in_date,uint32 in_size) {
	
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

CKnownFile* CKnownFileList::FindKnownFileByID(const CMD4Hash& hash)
{
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
		#ifdef __DEBUG__
		printf("%s is 0-size, not added\n",unicode2char(Record->GetFileName()));
		#endif
		return false;
	}
}

CKnownFile* CKnownFileList::IsOnDuplicates(wxString filename,uint32 in_date,uint32 in_size) {
	CKnownFile* cur_file;
	KnownFileList::Node* node = duplicates.GetFirst();
	while (node) {
		CKnownFile* duplicate = node->GetData();
		cur_file = duplicate;
		if ((abs((int)cur_file->GetFileDate() - (int)in_date) < 20) && cur_file->GetFileSize() == in_size && !cur_file->GetFileName().Cmp(filename)) {
			return cur_file;
		}
		node = node->GetNext();
	}	
	return NULL;
}

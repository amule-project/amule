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
#include "Preferences.h"	// Needed for MAX_PATH
#include "SafeFile.h"		// Needed for CSafeFile
#include "PartFile.h"		// Needed for CPartFile
#include "otherfunctions.h" // md4cmp
#include "amule.h"
#include "amuleDlg.h"
#include "MapKey.h"		// Needed for CCKey


#include <wx/listimpl.cpp> // ye old magic incantation
WX_DEFINE_LIST(KnownFileList);

CKnownFileList::CKnownFileList(CString in_appdir) {
	appdir = in_appdir;
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
		
		CString fullpath = appdir + wxT("known.met");
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
			theApp.amuledlg->AddLogLine(true, CString(_("Error: the file known.met is corrupted, unable to load known files")));
		else{
			char buffer[150];
			error->GetErrorMessage(buffer,150);
			theApp.amuledlg->AddLogLine(true, CString(_("Unexpected file error while reading known.met: %s, unable to load known files")), buffer);
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
	CString fullpath = appdir + wxT("known.met");
	file->Open(fullpath, CFile::write);
	if (!(file->IsOpened())) {
		delete file;
		return;
	}


	wxMutexLocker sLock(list_mut);
	//theApp.amuledlg->AddLogLine(false,CString(_("KnownFileList Save Starts")).GetData());
	uint8 ucHeader = MET_HEADER;
	//theApp.amuledlg->AddLogLine(false,CString(_("Saved MET_HEADER")).GetData());
	file->Write(&ucHeader, 1);
	uint32 RecordsNumber = m_map.size() + duplicates.GetCount();
	//theApp.amuledlg->AddLogLine(false,CString(_("RecordsNumber = %i")).GetData(), RecordsNumber);
	ENDIAN_SWAP_I_32(RecordsNumber);
	//theApp.amuledlg->AddLogLine(false,CString(_("Endian RecordsNumber = %i")).GetData(), RecordsNumber);		
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
	//theApp.amuledlg->AddLogLine(false,CString(_("KnownFileList Save Ends")).GetData());	
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

CKnownFile* CKnownFileList::FindKnownFile(const char* filename,uint32 in_date,uint32 in_size) {
	
	wxMutexLocker sLock(list_mut);
	
	CKnownFile* cur_file;

	for (CKnownFileMap::iterator pos = m_map.begin(); pos != m_map.end(); pos++ ) {
		cur_file = pos->second;
		if ((abs(cur_file->GetFileDate() - in_date) < 20) && cur_file->GetFileSize() == in_size && !cur_file->GetFileName().Cmp(char2unicode(filename))) {
			return cur_file;
		}
	}

	return IsOnDuplicates(filename, in_date, in_size);
}

CKnownFile* CKnownFileList::FindKnownFileByID(const uchar* hash)
{
	wxMutexLocker sLock(list_mut);
	
	if (hash)
	{
		CCKey tkey(hash);
		if (m_map.find(tkey) != m_map.end()) {
			return m_map[tkey];
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
		CCKey tkey(Record->GetFileHash());
		CKnownFileMap::iterator it = m_map.find(tkey);
		if ( it == m_map.end() ) {
			m_map[tkey] = Record;			
			return true;
		} else {
			it->second;
			uint32 in_date =  it->second->GetFileDate();
			uint32 in_size =  it->second->GetFileSize();
			const char* filename = (unicode2char(it->second->GetFileName().c_str()));
			if (((abs(Record->GetFileDate() - (in_date)) < 20) && Record->GetFileSize() == in_size && !Record->GetFileName().Cmp(char2unicode(filename))) || IsOnDuplicates(filename, in_date, in_size)) {
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
		printf("%s is 0-size, not added\n",Record->GetFileName().c_str());
		#endif
		return false;
	}
}

CKnownFile* CKnownFileList::IsOnDuplicates(const char* filename,uint32 in_date,uint32 in_size) {
	CKnownFile* cur_file;
	KnownFileList::Node* node = duplicates.GetFirst();
	while (node) {
		CKnownFile* duplicate = node->GetData();
		cur_file = duplicate;
		if ((abs(cur_file->GetFileDate() - in_date) < 20) && cur_file->GetFileSize() == in_size && !cur_file->GetFileName().Cmp(char2unicode(filename))) {
			return cur_file;
		}
		node = node->GetNext();
	}	
	return NULL;
}

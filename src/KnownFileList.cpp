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

static wxString KnownFileHash(const char* filename, uint32 date, uint32 size)
{
	return wxString::Format("%s%08x%08x", filename, date, size);
}

static wxString KnownFileHash(CKnownFile *file)
{
	return KnownFileHash(file->GetFileName().c_str(), file->GetFileDate(),
	                     file->GetFileSize());
}

CKnownFileList::CKnownFileList(char* in_appdir) {
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
	/*try*/ {
		char* fullpath = new char[strlen(appdir)+10];
		strcpy(fullpath,appdir);
		strcat(fullpath,"known.met");
		if (!wxFileExists(fullpath)) {//CFile::modeRead|CFile::osSequentialScan)) {
			delete[] fullpath;
			return false;
		}
		
		uint8 header;
		file.Open(fullpath);
		delete[] fullpath;
		fullpath = NULL;
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
}

void CKnownFileList::Save() {
	CFile* file = new CFile();
	char* fullpath = new char[strlen(appdir)+MAX_PATH];
	strcpy(fullpath,appdir);
	strcat(fullpath,"known.met");
	file->Open(fullpath, CFile::write);
	if (!(file->IsOpened())) {
		delete[] fullpath;
		fullpath=NULL;
		delete file;
		return;
	}
	delete[] fullpath;
	fullpath=NULL;

	wxMutexLocker sLock(list_mut);
	theApp.amuledlg->AddLogLine(false,CString(_("KnownFileList Save Starts")).GetData());
	uint8 ucHeader = MET_HEADER;
	theApp.amuledlg->AddLogLine(false,CString(_("Saved MET_HEADER")).GetData());
	file->Write(&ucHeader, 1);
	uint32 RecordsNumber = m_map.size();
	theApp.amuledlg->AddLogLine(false,CString(_("RecordsNumber = %i")).GetData(), RecordsNumber);
	ENDIAN_SWAP_I_32(RecordsNumber);
	theApp.amuledlg->AddLogLine(false,CString(_("Endian RecordsNumber = %i")).GetData(), RecordsNumber);		
	file->Write(&RecordsNumber,4);
	RecordsNumber = m_map.size();
	KnownFileMap::iterator it = m_map.begin();
	for (uint32 i = 0; i != RecordsNumber; i++,it++) {
		if ( it == m_map.end() )
			break;		// TODO: Throw an exception
		it->second->WriteToFile(file);
	}
	file->Flush();
	file->Close();
	theApp.amuledlg->AddLogLine(false,CString(_("KnownFileList Save Ends")).GetData());	
	delete file;
}



void CKnownFileList::Clear() {	
	wxMutexLocker sLock(list_mut);
	for ( KnownFileMap::iterator it = m_map.begin(); it != m_map.end(); it++ )
		delete it->second;
	m_map.clear();
}

CKnownFile* CKnownFileList::FindKnownFile(char* filename,uint32 in_date,uint32 in_size) {
	wxMutexLocker sLock(list_mut);
	KnownFileMap::iterator it = m_map.find(KnownFileHash(filename, in_date, in_size));
	if ( it != m_map.end() )
		return it->second;
	return 0;
}

bool CKnownFileList::SafeAddKFile(CKnownFile* toadd) {
	wxMutexLocker sLock(list_mut);
	return Append(toadd);
}

bool CKnownFileList::Append(CKnownFile* Record)
{
	wxString HashString = KnownFileHash(Record);
	KnownFileMap::iterator it = m_map.find(HashString);
	if ( it == m_map.end() ) {
		m_map[HashString] = Record;
		return true;
	} else {
		return false;
	}
}

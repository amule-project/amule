// This file is part of the aMule Project
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


#ifndef SHAREDFILELIST_H
#define SHAREDFILELIST_H

#include <map>
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/thread.h>		// Needed for wxMutex

#include "types.h"		// Needed for uint16 and uint64
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

/*
struct UnknownFile_Struct{
	char* name;
	char* directory;
	CPartFile* partfile_Owner;
};
*/
struct UnknownFile_Struct;

class CKnownFileList;
class CAddFileThread;
class CCKey;
class CKnownFile;
class CServerConnect;
class CPreferences;
class CMemFile;

#if 0
WX_DECLARE_LIST(UnknownFileStruct,UnknownList);

class CFilePtrList : public UnknownList
{
public:
  CFilePtrList() : UnknownList()
    { };
  virtual ~CFilePtrList() 
    { };
  
  int GetCount()	{ int nResult;
  wxMutexLocker locker(m_lockFilePtrList);
  nResult = UnknownList::GetCount();
  return nResult; };
  
  bool IsEmpty()		{ bool bResult;
  wxMutexLocker locker(m_lockFilePtrList);
  bResult = UnknownList::IsEmpty();
  return bResult; };
  
  UnknownList::Node* AddTail(UnknownFileStruct* pNewElement) 
    { UnknownList::Node* pos;
    wxMutexLocker locker(m_lockFilePtrList);
    pos = UnknownList::Append(pNewElement);
    return pos; };
  
  UnknownFileStruct* RemoveHead() 
    { UnknownFileStruct* pFile;
    wxMutexLocker locker(m_lockFilePtrList);
    pFile = UnknownList::GetFirst()->GetData();
    UnknownList::DeleteNode(UnknownList::GetFirst());
    return pFile; };
  
  //CRITICAL_SECTION 	m_lockFilePtrList;
  wxMutex m_lockFilePtrList;
};
#endif

typedef std::map<CCKey,CKnownFile*> CKnownFileMap;

class CSharedFileList{
	friend class CSharedFilesCtrl;
	friend class CClientReqSocket;
public:
	CSharedFileList(CPreferences* in_prefs,CServerConnect* in_server, CKnownFileList* in_filelist);
	~CSharedFileList();
	void	SendListToServer();
	void 	Reload(bool sendtoserver = true, bool firstload = false);
	void	SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd = false);
	void	SetOutputCtrl(CSharedFilesCtrl* in_ctrl);
	void	RemoveFile(CKnownFile* toremove);
	wxMutex	list_mut;
	CKnownFile*	GetFileByID(unsigned char* filehash);
	short	GetFilePriorityByID(unsigned char* filehash);
	CKnownFile*     GetFileByIndex(int index);
	CKnownFileList*		filelist;
	void	CreateOfferedFilePacket(CKnownFile* cur_file,CMemFile* files, bool toserver);
	uint64	GetDatasize();
	uint16	GetCount()	{return m_Files_map.size(); }
	void	UpdateItem(CKnownFile* toupdate);
	void	AddFilesFromDirectory(char* directory);
	void    GetSharedFilesByDirectory(const char *directory,CTypedPtrList<CPtrList, CKnownFile*>& list);
	void	ClearED2KPublishInfo();
	
private:
	void	FindSharedFiles();

	CKnownFileMap		m_Files_map;
	CPreferences*		app_prefs;
	CServerConnect*		server;
	CSharedFilesCtrl*	output;
	CAddFileThread*		m_Thread;
};

#if 0
WX_DECLARE_HASH_MAP(wxString,CKnownFile*,wxStringHash,wxStringEqual,FilesMap);
WX_DECLARE_LIST(UnknownFileStruct,WaitingForList);

class CSharedFileList{
	friend class CSharedFilesCtrl;
	friend class CClientReqSocket;
public:
	CSharedFileList(CPreferences* in_prefs,CServerConnect* in_server, CKnownFileList* in_filelist);
	~CSharedFileList();
	void	SendListToServer();
	void	Reload(bool sendtoserver = true);
	void	SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd = false);
	void	SetOutputCtrl(CSharedFilesCtrl* in_ctrl);
	void	RemoveFile(CKnownFile* toremove);
	wxMutex	list_mut;
	CKnownFile*	GetFileByID(unsigned char* filehash);
	short	GetFilePriorityByID(unsigned char* filehash);
	CKnownFileList*		filelist;
	void	CreateOfferedFilePacket(CKnownFile* cur_file,CMemFile* files);
	uint64	GetDatasize();
	uint16	GetCount()	{return m_Files_map.size(); }
	void	UpdateItem(CKnownFile* toupdate);
private:
	void	FindSharedFiles();
	void	AddFilesFromDirectory(char* directory);
	void	HashNextFile();

	//CMap<CCKey,CCKey&,CKnownFile*,CKnownFile*> m_Files_map;
	FilesMap m_Files_map;
	//CTypedPtrList<CPtrList, UnknownFileStruct*> waitingforhash_list;
	WaitingForList waitingforhash_list;
	CPreferences*		app_prefs;
	CServerConnect*		server;
	CSharedFilesCtrl*	output;
};
#endif

/*
class CAddFileThread : public wxThread
{
public:
	CAddFileThread();
public:
	virtual	bool	InitInstance() {return true;}
	virtual ExitCode Entry();
	virtual void OnExit() {}

private:
};
*/

#endif // SHAREDFILELIST_H

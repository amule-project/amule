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

struct UnknownFile_Struct;

class CKnownFileList;
class CAddFileThread;
class CKnownFile;
class CServerConnect;
class CPreferences;
class CSafeMemFile;
class CMD4Hash;

typedef std::map<CMD4Hash,CKnownFile*> CKnownFileMap;

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
	CKnownFile*	GetFileByID(const CMD4Hash& filehash);
	short	GetFilePriorityByID(const CMD4Hash& filehash);
	CKnownFile*     GetFileByIndex(int index);
	CKnownFileList*		filelist;
	void	CreateOfferedFilePacket(CKnownFile* cur_file,CSafeMemFile* files, bool toserver);
	uint64	GetDatasize();
	uint16	GetCount()	{return m_Files_map.size(); }
	void	UpdateItem(CKnownFile* toupdate);
	void	AddFilesFromDirectory(wxString directory);
	void    GetSharedFilesByDirectory(const wxString directory,CTypedPtrList<CPtrList, CKnownFile*>& list);
	void	ClearED2KPublishInfo();
	
private:
	void	FindSharedFiles();
	bool	reloading;
	
	CKnownFileMap		m_Files_map;
	CPreferences*		app_prefs;
	CServerConnect*		server;
	CSharedFilesCtrl*	output;
	CAddFileThread*		m_Thread;
};

#endif // SHAREDFILELIST_H

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

#ifndef KNOWNFILELIST_H
#define KNOWNFILELIST_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "KnownFileList.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/hashmap.h>		// Needed for WX_DECLARE_HASH_MAP, wxStringHash and wxStringEqual
#include <wx/thread.h>		// Needed for wxMutex


#include "types.h"		// Needed for uint16 and uint32
#include "SharedFileList.h" // CKnownFileMap

class CKnownFile;
	
 WX_DECLARE_LIST(CKnownFile, KnownFileList);

//WX_DECLARE_HASH_MAP(wxString,CKnownFile*,wxStringHash,wxStringEqual,KnownFileMap);

class CKnownFileList {
//	friend class CSharedFilesWnd;
//	friend class CFileStatistic;
public:
	CKnownFileList();
	~CKnownFileList();
	bool	SafeAddKFile(CKnownFile* toadd);
	bool	Init();
	void	Save();
	void	Clear();
	CKnownFile*	FindKnownFile(wxString filename,uint32 in_date,uint32 in_size);
	CKnownFile*   FindKnownFileByID(const CMD4Hash& hash);
	bool	IsKnownFile(const CKnownFile* file);

	uint16 requested;
	uint32 transfered;
	uint16 accepted;

private:
	wxMutex	list_mut;

	bool	Append(CKnownFile*);

	CKnownFile* IsOnDuplicates(wxString filename,uint32 in_date,uint32 in_size) const;

	KnownFileList	duplicates;
	CKnownFileMap	m_map;
};

#endif // KNOWNFILELIST_H

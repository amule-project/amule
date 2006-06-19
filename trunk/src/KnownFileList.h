//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef KNOWNFILELIST_H
#define KNOWNFILELIST_H



#include "SharedFileList.h" // CKnownFileMap


class CKnownFile;	

class CKnownFileList
{
public:
	CKnownFileList();
	~CKnownFileList();
	bool	SafeAddKFile(CKnownFile* toadd);
	bool	Init();
	void	Save();
	void	Clear();
	CKnownFile*	FindKnownFile(wxString filename,uint32 in_date,uint64 in_size);
	CKnownFile*   FindKnownFileByID(const CMD4Hash& hash);
	bool	IsKnownFile(const CKnownFile* file);

	uint16 requested;
	uint32 transfered;
	uint16 accepted;

private:
	wxMutex	list_mut;

	bool	Append(CKnownFile*);

	CKnownFile* IsOnDuplicates(wxString filename,uint32 in_date,uint64 in_size) const;

	typedef std::list<CKnownFile*> KnownFileList;
	KnownFileList	m_duplicates;
	CKnownFileMap	m_map;
};

#endif // KNOWNFILELIST_H
// File_checked_for_headers

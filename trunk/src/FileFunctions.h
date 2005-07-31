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


#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "FileFunctions.h"
#endif

#include <wx/string.h>
#include <dirent.h>


struct stat;

// This is to avoid wxStat
int UTF8_Stat(const wxString& file_name, struct stat *buf);

// Move file with safe UTF8 name.
bool UTF8_MoveFile(const wxString& from, const wxString& to); 

// Copy file with safe UTF8 name.
bool UTF8_CopyFile(const wxString& from, const wxString& to); 

// Makes a backup of a file, by copying the original file to filename + appendix
bool BackupFile(const wxString& filename, const wxString& appendix);

// Get the LastModificationTime for a file.
time_t GetLastModificationTime(const wxString& file);

/**
 * Returns the size of the specified file.
 *
 * @param fullPath The full path of the file to check.
 * @return The size of the file, or a negative value on failures.
 *
 * GetFileSize will fail if the file doesn't exist, if we
 * dont have read-access to it, or if the path was invalid.
 */
off_t GetFileSize(const wxString& fullPath);


// Dir iterator: needed because wxWidget's wxFindNextFile and 
// wxFindFirstFile are bugged like hell.
class CDirIterator {
public:
	enum FileType { File, Dir, Any}; 

	CDirIterator(const wxString& dir);
	~CDirIterator();

	bool IsValid() const {
		return (DirPtr != NULL);
	}

	wxString GetFirstFile(FileType search_type, const wxString& search_mask = wxEmptyString);
	wxString GetNextFile();

private:
	DIR* DirPtr;
	FileType type;
	wxString DirStr;
	wxString FileMask;
};
	
bool CheckDirExists(const wxString& dir);

#endif

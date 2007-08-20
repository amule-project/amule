//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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


#ifndef FILEFUNCTIONS_H
#define FILEFUNCTIONS_H

#include "Types.h"
#include <dirent.h>		// Needed for DIR	// Do_not_auto_remove (mingw-gcc-3.4.5)
#include <wx/dir.h>

// Remove file with safe UTF8 name.
bool UTF8_RemoveFile(const wxString& fileName);

// Move file with safe UTF8 name.
bool UTF8_MoveFile(const wxString& from, const wxString& to, bool overwrite = false); 

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
sint64 GetFileSize(const wxString& fullPath);


// Dir iterator: needed because wxWidget's wxFindNextFile and 
// wxFindFirstFile are bugged like hell.
class CDirIterator : public wxDir
{
public:
	enum FileType {
		File = wxDIR_FILES | wxDIR_HIDDEN,
		Dir  = wxDIR_DIRS  | wxDIR_HIDDEN,
		Any  = wxDIR_FILES | wxDIR_DIRS   | wxDIR_HIDDEN
	};

	CDirIterator(const wxString& dir);
	~CDirIterator();

	wxString GetFirstFile(FileType search_type, const wxString& search_mask = wxEmptyString);
	wxString GetNextFile();
	
private:
	wxString m_dir;
};
	

bool CheckDirExists(const wxString& dir);

bool CheckFileExists(const wxString& file);


//! Filetypes understood by UnpackArchive
enum EFileType
{
	//! Text files, will be left unchanged.
	EFT_Text,
	//! Zip archive, will be unpacked
	EFT_Zip,
	//! GZip archives, will be unpacked
	EFT_GZip,
	//! Met file, will be left unchanged.
	EFT_Met,
	//! Unknown filetype, will be left unchanged.
	EFT_Unknown
};


typedef std::pair<bool, EFileType> UnpackResult;

/**
 * Unpacks a single file from an archive, replacing the archive.
 *
 * @param file The archive.
 * @param files An array of filenames (terminated by a NULL entry) which should be unpacked.
 * @return The first value is true if the archive was unpacked, the second is the resulting filetype.
 *
 * If the file specified is not an archive, it will be left unchanged and
 * the file-type returned. If it is a GZip archive, the archive will be
 * unpacked and the new file will replace the archive. If the archive is a
 * Zip archive, the first file found matching any in the files array (case-
 * insensitive) will be unpacked and overwrite the archive.
 */
UnpackResult UnpackArchive(const wxString& file, const wxChar* files[]);


enum FSCheckResult {
	//! The file-system is FAT32, so certain chars have to be stripped.
	FS_IsFAT32,
	//! The file-system is not FAT32.
	FS_NotFAT32,
	//! Failed to check the specified file-system.
	FS_Failed
};


/**
 * Checks if the specified path is on a FAT32 file-system.
 *
 * Note that CheckFileSystem always returns FS_IsFAT32 on wxMSW.
 */
FSCheckResult CheckFileSystem(const wxString& path);


#endif
// File_checked_for_headers

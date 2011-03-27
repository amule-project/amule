//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "../../Types.h"
#include "Path.h"

#include <wx/dir.h>


// Dir iterator: needed because wxWidget's wxFindNextFile and 
// wxFindFirstFile are bugged like hell.
class CDirIterator : private wxDir
{
public:
	enum FileType {
		FileNoHidden = wxDIR_FILES,
		DirNoHidden  = wxDIR_DIRS,
		File = wxDIR_FILES | wxDIR_HIDDEN,
		Dir  = wxDIR_DIRS  | wxDIR_HIDDEN,
		Any  = wxDIR_FILES | wxDIR_DIRS   | wxDIR_HIDDEN
	};

	CDirIterator(const CPath& dir);
	~CDirIterator();

	CPath GetFirstFile(FileType type, const wxString& mask = wxEmptyString);
	CPath GetNextFile();

	bool HasSubDirs(const wxString& spec = wxEmptyString);
};
	

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
	EFT_Unknown,
	//! This is returned when trying to unpack a broken archive.
	EFT_Error
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
UnpackResult UnpackArchive(const CPath& file, const wxChar* files[]);

#endif
// File_checked_for_headers

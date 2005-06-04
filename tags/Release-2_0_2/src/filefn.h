//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
////////////////////////////////////////////////////////////////////////////
// Name:        filefn.h
// Purpose:     Patched wxCopyFile & wxRenameFile for chmod behaviour
// Author:      Angel Vidal - Kry
// Modified by: Angel Vidal - Kry
// Created:     29/12/03
// Copyright:   (c) 2003 Angel Vidal - Kry
// Licence:     GPL
// Based on wxWindows's filefn.cpp
/////////////////////////////////////////////////////////////////////////////

#ifndef FILEFN_H
#define FILEFN_H

class wxString;

// This function is a replacement for wxCopyFile, with the added feature,
// that chmoding of the target file is disabed when FAT32 filesystems are used. 
// The reason for this is, that FAT partitons under linux generate warnings when chmoding.
bool FS_wxCopyFile(const wxString& file1, const wxString& file2, bool overwrite = TRUE);
// Same as above, but renames rather than copies.
bool FS_wxRenameFile(const wxString& file1, const wxString& file2);


// Kry - vfat versions
bool wxCopyFile_fat32(const wxString& file1, const wxString& file2, bool overwrite = true, bool do_chmod = false);
bool wxRenameFile_fat32(const wxString& file1, const wxString& file2, bool do_chmod = false);

// Makes a backup of a file, by copying the original file to filename + appendix
bool BackupFile(const wxString& filename, const wxString& appendix);

#endif // FILEFN_H

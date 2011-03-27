//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef PLATFORMSPECIFIC_H
#define PLATFORMSPECIFIC_H

#include <common/Path.h>
#include "Types.h"


namespace PlatformSpecific {


/**
 * Create sparse file.
 *
 * This function will create the named file sparse if possible.
 *
 * @param name The file to be created (sparse if possible).
 * @param size The desired size of the file.
 * @return true, if creating the file succeeded, false otherwise.
 */
bool CreateSparseFile(const CPath& name, uint64_t size);


/**
 * Returns the max number of connections the current OS can handle.
 *
 * Currently anything but windows will return the default value (-1);
 */
#ifdef __WXMSW__
int GetMaxConnections();
#else
inline int GetMaxConnections() { return -1; }
#endif


/**
 * File system types returned by GetFilesystemType
 */
enum EFSType {
	fsFAT,		//! File Allocation Table
	fsNTFS,		//! New Technology File System
	fsHFS,		//! Hierarchical File System
	fsHPFS,		//! High Performace File System
	fsMINIX,	//! Minix file system
	fsOther		//! Unknown, other
};

/**
 * Find out the filesystem type of the given path.
 *
 * @param path The path for which the filesystem type should be checked.
 * @return The filesystem type of the given path.
 *
 * This function returns fsOther on unknown or network file systems (because the
 * real filesystem type cannot be determined).
 */
EFSType GetFilesystemType(const CPath& path);


/**
 * Checks if the filesystem can handle special chars.
 *
 * @param path The path for which the file system should be checked.
 * @return true if the underlying filesystem can handle special chars.
 *
 * This function checks if the file system of the given path can handle
 * special chars e.g. ':' in file names. This function will always return
 * false on MSW, since Windows cannot handle those characters on any file system.
 *
 * Based on http://en.wikipedia.org/wiki/Comparison_of_file_systems
 */
#ifdef __WXMSW__
inline bool CanFSHandleSpecialChars(const CPath& WXUNUSED(path)) { return false; }
#else
// Other filesystem types may be added
inline bool CanFSHandleSpecialChars(const CPath& path)
{
	switch (GetFilesystemType(path)) {
		case fsFAT:
		case fsHFS:
			return false;
		default:
			return true;
	}
}
#endif


/**
 * Check if the filesystem can handle large files.
 *
 * @param path The path for which the filesystem should be checked.
 * @return true if the underlying filesystem can handle large files.
 *
 * This function checks if the file system of the given path can handle
 * large files (>4GB).
 *
 * Based on http://en.wikipedia.org/wiki/Comparison_of_file_systems
 */
inline bool CanFSHandleLargeFiles(const CPath& path)
{
	switch (GetFilesystemType(path)) {
		case fsFAT:
		case fsHFS:
		case fsHPFS:
		case fsMINIX:
			return false;
		default:
			return true;
	}
}

/**
 * Disable / enable computer's energy saving "standby" mode.
 *
 */
#if defined __WXMSW__ || defined __WXMAC__
	#define PLATFORMSPECIFIC_CAN_PREVENT_SLEEP_MODE 1
#else
	#define PLATFORMSPECIFIC_CAN_PREVENT_SLEEP_MODE 0
#endif

void PreventSleepMode();
void AllowSleepMode();

}; /* namespace PlatformSpecific */

#endif /* PLATFORMSPECIFIC_H */

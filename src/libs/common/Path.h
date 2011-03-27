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

#ifndef FILENAME_H
#define FILENAME_H

#include "../../Types.h"
#include "Format.h"


/**
 * This class wraps a path/filename, serving a purpose much
 * like wxFileName. But in addition CPath serves to enable
 * the handling of "broken" filenames, allowing these to be
 * printed in a meaningful manner, while still allowing
 * access to the actual files in the filesystem.
 *
 * This class is thread-safe, in that the class is read-only,
 * and any function that returns wxStrings or CPath objects,
 * ensure that an entirely new wxString object is created to
 * circumvent the thread-unsafe reference counting of that
 * class.
 *
 * This class, and its static functions should be used in
 * preference of wxFileName, for the above reason, and so
 * that cross-platform issues can be worked around in a
 * single place.
 */
class CPath
{
public:
	/** Default constructor. */
	CPath();

	/** Constuctor. */
	explicit CPath(const wxString& path);

	/** Copy constructor. Creates a deep-copy of the passed object. */
	CPath(const CPath& other);


	/**
	 * Creates a path from one saved in the 'universal' format.
	 *
	 * These are to be used when the filenames/paths are saved to
	 * the local machine, and ensure that locale-changes does not
	 * affect our ability to find previously known files. This
	 * should not be used when sending filenames to other clients
	 * or (currently) the core/gui.
	 **/
	static CPath FromUniv(const wxString& path);
	/** Creates an 'universal' path from the specified CPath. */
	static wxString ToUniv(const CPath& path);


	/** Assignment operator. */
	CPath& operator=(const CPath& other);
	/** Less-than operator. */
	bool operator<(const CPath& other) const;
	/** Note that only exact matches are considered equal, see IsSameDir. */
	bool operator==(const CPath& other) const;
	/** Note that only exact matches are considered equal, see IsSameDir. */
	bool operator!=(const CPath& other) const;


	/** Returns true if the filename is valid, false otherwise. */
	bool IsOk() const;
	/** Returns true if the path exists and is a file, false otherwise. */
	bool FileExists() const;
	/** Returns true if the path exists and is a directory, false otherwise. */
	bool DirExists() const;


	enum EAccess {
		//! Only check of the file or dir exists.
		exists,
		//! If set, checks if a file is readable, or if a dir can be accessed.
		readable,
		//! If set, checks if a file or directory can be written to,
		writable,
		//! Combination of the two checks above.
		readwritable = readable | writable
	};

	/** Returns if if the path is a dir, and the checks were positive. */
	bool IsDir(EAccess mode) const;
	/** Returns if if the path is a dir, and the checks were positive. */
	bool IsFile(EAccess mode) const;


	/** Returns the contents of the object in a form suitable for use with wx system-calls. */
	wxString GetRaw() const;
	/** Returns the contents of the object in a form suitable for use in the UI. */
	wxString GetPrintable() const;
	/** Returns the (raw) last extension, empty if none is found. */
	wxString GetExt() const;

	/** Returns the full path, exluding the filename. */
	CPath GetPath() const;
	/** Returns the full filename, excluding the path. */
	CPath GetFullName() const;

	/** Returns the size of the specified file, or wxInvalidSize on failure. */
	sint64 GetFileSize() const;

	/**
	 * Compares under the assumption that both objects are dirs, even if
	 * one or the other lacks a terminal directory-seperator. However, an
	 * empty CPath object will not be considered equal to a path to the root.
	 */
	bool IsSameDir(const CPath& other) const;

	/** Returns a CPath created from joining the two objects. */
	CPath JoinPaths(const CPath& other) const;
	/** Returns a CPath with invalid chars removed, and spaces escaped if specified. */
	CPath Cleanup(bool keepSpaces = true, bool isFAT32 = false) const;
	/** Returns a CPath with a postfix before the file-extension. Must be ASCII. */
	CPath AddPostfix(const wxString& postfix) const;

	/** Returns a CPath object with the extension appended. Empty strings are ignored. */
	CPath AppendExt(const wxString& ext) const;
	/** Returns a CPath with the (last, if multiple) extension removed. */
	CPath RemoveExt() const;
	/** Returns a CPath object with all extensions removed. */
	CPath RemoveAllExt() const;


	/** Returns true if the the passed path makes up an prefix of this object. */
	bool StartsWith(const CPath& other) const;

	/**
	 * Get truncated path.
	 *
	 * @note This function truncates the @em name of the file, not the file
	 * itself.
	 *
	 * @param length	The truncated path should not exceed this length.
	 * @param isFilePath	Indicates whether the last part of the path (the
	 *			file name) should be kept or not, if possible.
	 * @return	The truncated path, at most @a length long.
	 */
	wxString TruncatePath(size_t length, bool isFilePath = false) const;

	/** 
	 * Renames the file 'src' to the file 'dst', overwriting if specified. Note that
	 * renaming cannot be done across volumes. For that CopyFile is required.
	 */
	static bool RenameFile(const CPath& src, const CPath& dst, bool overwrite = false);
	/**
	 * Copies the file 'src' to the file 'dst', overwriting if specified. 
	 * The silly name is used to avoid conflicts with the #define CopyFile,
	 * which is set on MSW.
	 */
	static bool CloneFile(const CPath& src, const CPath& dst, bool overwrite = false);

	/** Makes a backup of a file, by copying the original file to 'src' + 'appendix' */
	static bool BackupFile(const CPath& src, const wxString& appendix);

	/** Deletes the specified file, returning true on success. */
	static bool RemoveFile(const CPath& file);
	/** Deletes the specified directory, returning true on success. */
	static bool RemoveDir(const CPath& file);
	/** Creates the specified directory, returning true on success. */
	static bool MakeDir(const CPath& file);

	/** Returns true if the path exists, and is a file. */
	static bool FileExists(const wxString& file);
	/** Returns true if the path exists, and is a directory. */
	static bool DirExists(const wxString& path);

	/** Returns the size of the specified file, or wxInvalidOffset on failure. */
	static sint64 GetFileSize(const wxString& file);
	/** Returns the modification time the specified file, or (time_t)-1 on failure. */
	static time_t GetModificationTime(const CPath& file);
	/** Returns the free diskspace at the specified path, or wxInvalidOffset on failure. */
	static sint64 GetFreeSpaceAt(const CPath& path);

private:
	//! Contains the printable filename, for use in the UI.
	wxString	m_printable;
	//! Contains the "raw" filename, for use in system-calls,
	//! as well as in wxWidgets file functions.
	wxString	m_filesystem;

	// Is made a friend to avoid needless copying of strings.
	friend int CmpAny(const CPath& ArgA, const CPath& ArgB);
};

// Make CPath printable in CFormat
template<>
inline CFormat& CFormat::operator%(CPath value)
{
	return this->operator%<const wxString&>(value.GetPrintable());
}

/**
 * Overloaded version of CmpAny for use with CPaths. As this is
 * typically used in the UI, it uses the printable filename in
 * order to get visually correct results.
 */
inline int CmpAny(const CPath& ArgA, const CPath& ArgB)
{
	return ArgA.m_printable.CmpNoCase(ArgB.m_printable);
}


/**
 * Strips all path separators from the specified end of a path.
 *
 * Note: type must be either leading or trailing.
 */
wxString StripSeparators(wxString path, wxString::stripType type);


/**
 * Joins two paths with the operating system specific path-separator.
 *
 * If any of the parameters are empty, the other parameter is
 * returned unchanged.
 */
wxString JoinPaths(const wxString& path, const wxString& file);

#endif

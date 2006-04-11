//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 1998 Vadim Zeitlin ( zeitlin@dptmaths.ens-cachan.fr )
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

#ifndef CFILE_H
#define CFILE_H

#include "SafeFile.h"		// Needed for CFileDataIO
#include <wx/file.h>		// Needed for constants


/**
 * This class is a modified version of the wxFile class.
 *
 * In addition to implementing the CFileDataIO interface,
 * it offers improved support for UTF8 filenames and 64b
 * file-IO on both windows and unix-like systems. 
 *
 * @see wxFile
 */
class CFile : public CFileDataIO
{
public:
	//! Standard values for file descriptor
	enum { fd_invalid = -1, fd_stdin, fd_stdout, fd_stderr };

	/** @see wxFile::OpenMode */
	enum OpenMode { read, write, read_write, write_append, write_excl };

	
	/**
	 * Creates a closed file.
	 */
	CFile();
	
	/**
	 * Constructor, calls Open on the specified file.
	 *
	 * To check if the file was successfully opened, a
	 * call to IsOpened() is required.
	 */
	CFile(const wxString& path, OpenMode mode = read);

	/**
	 * Destructor, closes the file if opened.
	 */
	virtual ~CFile();

		
	/**
	 * Opens a file.
	 *
	 * @param path The full or relative path to the file.
	 * @param mode The opening mode.
	 * @param accessMode The permissions in case a new file is created.
	 * @return True if the file was opened, false otherwise.
	 * 
	 * Calling Open with the openmodes 'write' or 'write_append' will 
	 * create the specified file if it does not already exist.
	 *
	 * If an accessMode is not explicitly specified, the accessmode
	 * specified via CPreferences::GetFilePermissions will be used.
	 */
	bool Open(const wxString& path, OpenMode mode = read, int accessMode = wxS_DEFAULT);
	
	/**
	 * Calling Create is requivilant of calling open with OpenMode 'write'.
	 *
	 * @param overwrite Specifies if the target file should be overwritten,
	 *                  in case that it already exists.
	 *
	 * @see CFile::Open
	 */
	bool Create(const wxString& path, bool overwrite = false, int accessMode = wxS_DEFAULT);
	
	/**
	 * Closes the file.
	 *
	 * Note that calling Close on an closed file
	 * is an illegal operation.
	 */
	bool Close();


	/**
	 * Returns the file descriptior assosiated with the file.
	 *
	 * Note that direct manipulation of the descriptor should
	 * be avoided! That's what this class is for.
	 */
	int  fd() const;

	/**
	 * Flushes data not yet written.
	 *
	 * Note that calling Flush on an closed file
	 * is an illegal operation.
	 */
	bool Flush();

	
	/**
	 * @see CSafeFileIO::GetLength
	 *
	 * Note that calling GetLength on a closed file 
	 * is an illegal operation.
	 */
	virtual uint64 GetLength() const;
	
	/**
	 * Resizes the file to the specified length.
	 */
	bool SetLength(size_t newLength);
	
	/**
	 * @see CSafeFileIO::GetPosition
	 *
	 * Note that calling GetPosition on a closed file
	 * is an illegal operation.
	 */
	virtual uint64 GetPosition() const;
	
	/**
	 * Returns the current available bytes to read on the file before EOF
	 * 
	 */
	virtual uint64 GetAvailable() const;	
	
	/**
	 * Returns the path of the currently opened file.
	 * 
	 * Calling this function on an closed file is
	 * an illegal operation.
	 */
	const wxString& GetFilePath() const;
	

	/**
	 * Returns true if the file is opened, false otherwise.
	 */
	bool IsOpened() const;
	
protected:
	/** @see CFileDataIO::doRead **/
	virtual sint64 doRead(void* buffer, size_t count) const;
	/** @see CFileDataIO::doWrite **/
	virtual sint64 doWrite(const void* buffer, size_t count);
	/** @see CFileDataIO::doSeek **/
	virtual sint64 doSeek(sint64 offset) const;

private:
	//! A CMemFile is neither copyable nor assignable.
	//@{
	CFile(const CFile&);
	CFile& operator=(const CFile&);
	//@}
	
	//! File descriptor or 'fd_invalid' if not opened
	int m_fd;
	
	//! The full path to the current file.
	wxString m_filePath;
};


/**
 * This exception is thrown by CFile if a seek or tell fails.
 */
struct CSeekFailureException : public CIOFailureException {
	CSeekFailureException(const wxString& desc);
};


#endif // CFILE_H

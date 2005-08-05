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
/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:     CFile - encapsulates low-level "file descriptor"
//              wxTempFile - safely replace the old file
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#ifndef CFILE_H
#define CFILE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "CFile.h"
#endif

#include <wx/string.h>		// Needed for wxString
#include <wx/filefn.h>		// Needed for wxSeekMode and seek related stuff.

#include "SafeFile.h"		// Needed for CFileDataIO

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// we redefine these constants here because S_IREAD &c are _not_ standard
// however, we do assume that the values correspond to the Unix umask bits
#define wxS_IRUSR 00400
#define wxS_IWUSR 00200
#define wxS_IXUSR 00100

#define wxS_IRGRP 00040
#define wxS_IWGRP 00020
#define wxS_IXGRP 00010

#define wxS_IROTH 00004
#define wxS_IWOTH 00002
#define wxS_IXOTH 00001

// default mode for the new files: corresponds to umask 022
#define wxS_DEFAULT   (wxS_IRUSR | wxS_IWUSR | wxS_IRGRP | wxS_IWGRP |\
                       wxS_IROTH | wxS_IWOTH)

// ----------------------------------------------------------------------------
// class CFile: raw file IO
// ----------------------------------------------------------------------------
class CFile : public CFileDataIO
{
public:
	// more file constants
	// -------------------
		// opening mode
	enum OpenMode { read, write, read_write, write_append, write_excl };
		// standard values for file descriptor
	enum { fd_invalid = -1, fd_stdin, fd_stdout, fd_stderr };

	// static functions
	// ----------------
		// check whether a regular file by this name exists
	static bool Exists(const wxChar *name);
		// check whetther we can access the given file in given mode
		// (only read and write make sense here)
	static bool Access(const wxChar *name, OpenMode mode);

	// ctors
	// -----
		// def ctor
	CFile() { m_fd = fd_invalid; m_error = FALSE; }
		// open specified file (may fail, use IsOpened())
	CFile(const wxString& szFileName, OpenMode mode = read);
		// attach to (already opened) file
	CFile(int fd) { m_fd = fd; m_error = FALSE; }

	virtual const wxString& GetFilePath() const {return m_filePath;}; 

	// open/close
		// create a new file (with the default value of bOverwrite, it will fail if
		// the file already exists, otherwise it will overwrite it and succeed)
		// Default permissions will use the ones specified through CPreferences::GetFilePermissions
	virtual bool Create(const wxString& szFileName, bool bOverwrite = FALSE, int access = -1 );
	virtual bool Open(const wxString& szFileName, OpenMode mode = read, int access = -1 );

	virtual bool Close();  // Close is a NOP if not opened

	// assign an existing file descriptor and get it back from CFile object
	void Attach(int fd) { Close(); m_fd = fd; }
	void Detach()       { m_fd = fd_invalid;  }
	int  fd() const { return m_fd; }

		// flush data not yet written
	virtual bool Flush();

	// file pointer operations (return ofsInvalid on failure)
		// move ptr ofs bytes related to start/current off_t/end of file
	virtual off_t Seek(off_t ofs, wxSeekMode mode = wxFromStart) const;
		// get current off_t
	virtual off_t GetPosition() const;
		// get current file length
	virtual off_t GetLength() const;
		//Truncate/grow file
	virtual bool SetLength(off_t new_len);

	// simple accessors
		// is file opened?
	virtual bool IsOpened() const { return m_fd != fd_invalid; }
		// is end of file reached?
	virtual bool Eof() const;
		// has an error occured?
	virtual bool Error() const { return m_error; }

	// dtor closes the file if opened
	virtual ~CFile() { 
		if (m_fd != fd_invalid) {
			Close(); 
		}
	}
	
	// This safe read will throw a wxString on some issues
	virtual off_t SafeRead(unsigned char* pBuf, off_t nCount, int nRetries = 1) const;

protected:
	// read/write (unbuffered)
	// returns number of bytes read or ofsInvalid on error
	virtual off_t doRead(void *pBuf, off_t nCount) const;
	// returns the number of bytes written
	virtual size_t doWrite(const void *pBuf, size_t nCount);

private:
	// copy ctor and assignment operator are private because
	// it doesn't make sense to copy files this way:
	// attempt to do it will provoke a compile-time error.
	CFile(const CFile&);
	CFile& operator=(const CFile&);

	mutable int m_fd; // file descriptor or INVALID_FD if not opened
	mutable bool m_error; // error memory
	wxString m_filePath;
};


#endif // CFILE_H

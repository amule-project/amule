// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#include <wx/string.h>		// Needed for wxString
#include <wx/filefn.h>		// Needed for wxSeekMode and seek related stuff.

#include <dirent.h> // for CDirIterator

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
//
// NB: for space efficiency this class has no virtual functions, including
//     dtor which is _not_ virtual, so it shouldn't be used as a base class.
// ----------------------------------------------------------------------------
class CFile {
public:
	// more file constants
	// -------------------
		// opening mode
	enum OpenMode { read, write, read_write, write_append, write_excl };
		// standard values for file descriptor
	enum { fd_invalid = -1, fd_stdin, fd_stdout, fd_stderr };

	enum SeekMode { start = wxFromStart, current = wxFromCurrent, end = wxFromEnd}; 
	
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
	CFile(const wxChar *szFileName, OpenMode mode = read);
		// attach to (already opened) file
	CFile(int fd) { m_fd = fd; m_error = FALSE; }

	virtual const wxString& GetFilePath() const {return fFilePath;}; 

	// open/close
		// create a new file (with the default value of bOverwrite, it will fail if
		// the file already exists, otherwise it will overwrite it and succeed)
		// Default permissions will use the ones specified through CPreferences::GetFilePermissions
	virtual bool Create(const wxChar *szFileName, bool bOverwrite = FALSE, int access = -1 );
	virtual bool Open(const wxChar *szFileName, OpenMode mode = read, int access = -1 );
	// Kry -Added for windoze compatibility.
	off_t GetLength() const { return Length(); }

	virtual bool Close();  // Close is a NOP if not opened

	// assign an existing file descriptor and get it back from CFile object
	void Attach(int fd) { Close(); m_fd = fd; }
	void Detach()       { m_fd = fd_invalid;  }
	int  fd() const { return m_fd; }

	// read/write (unbuffered)
		// returns number of bytes read or ofsInvalid on error
	virtual off_t Read(void *pBuf, off_t nCount) const;
		// returns the number of bytes written
	virtual size_t Write(const void *pBuf, size_t nCount);
		// flush data not yet written
	virtual bool Flush();

	// file pointer operations (return ofsInvalid on failure)
		// move ptr ofs bytes related to start/current off_t/end of file
	virtual off_t Seek(off_t ofs, CFile::SeekMode mode = start) const;
		// move ptr to ofs bytes before the end
	virtual off_t SeekEnd(off_t ofs = 0) { return Seek(ofs, end); }
		// get current off_t
	virtual off_t GetPosition() const;
		// get current file length
	virtual off_t Length() const;
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

private:
	// copy ctor and assignment operator are private because
	// it doesn't make sense to copy files this way:
	// attempt to do it will provoke a compile-time error.
	CFile(const CFile&);
	CFile& operator=(const CFile&);

	mutable int m_fd; // file descriptor or INVALID_FD if not opened
	mutable bool m_error; // error memory
	wxString fFilePath;
};

// Dir iterator: needed because wxWidget's wxFindNextFile and 
// wxFindFirstFile are bugged like hell.

class CDirIterator {
public:
	enum FileType { File, Dir, Any}; 

	CDirIterator(const wxString dir);
	~CDirIterator();

	bool IsValid() const {
		return (DirPtr != NULL);
	}

	wxString FindFirstFile(FileType search_type, wxString search_mask = wxEmptyString);
	wxString FindNextFile();

private:
	DIR* DirPtr;
	FileType type;
	wxString DirStr;
	wxString FileMask;
};
	

#endif // CFILE_H

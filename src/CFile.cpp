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
// Name:        file.cpp
// Purpose:     wxFile - encapsulates low-level "file descriptor"
//              wxTempFile
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "CFile.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include <unistd.h>		// Needed for close(2)

#include "CFile.h"		// Interface declarations.

#include "StringFunctions.h" // unicode2char
#include "Preferences.h"
#include "Logger.h"
#include "Format.h"
#include "Packet.h"

#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for HAVE_SYS_PARAM_H
#endif

// Test if we have _GNU_SOURCE before the next step will mess up 
// setting __USE_GNU 
// (only needed for gcc-2.95 compatibility, gcc 3.2 always defines it)
#include <wx/setup.h>

// Mario Sergio Fujikawa Ferreira <lioux@FreeBSD.org>
// to detect if this is a *BSD system
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef __BORLANDC__
#	pragma hdrstop
#endif

#include <wx/log.h>

// standard
#if defined(__WXMSW__) && !defined(__GNUWIN32__) && !defined(__WXWINE__) && !defined(__WXMICROWIN__)
#	include <io.h>

#ifndef __SALFORDC__
#	define   WIN32_LEAN_AND_MEAN
#	define   NOSERVICE
#	define   NOIME
#	define   NOATOM
#	define   NOGDI
#	define   NOGDICAPMASKS
#	define   NOMETAFILE
#	define   NOMINMAX
#	define   NOMSG
#	define   NOOPENFILE
#	define   NORASTEROPS
#	define   NOSCROLL
#	define   NOSOUND
#	define   NOSYSMETRICS
#	define   NOTEXTMETRIC
#	define   NOWH
#	define   NOCOMM
#	define   NOKANJI
#	define   NOCRYPT
#	define   NOMCX
#endif

#elif (defined(__UNIX__) || defined(__GNUWIN32__))
#	include <unistd.h>
#	ifdef __GNUWIN32__
#		include <windows.h>
#	endif
#elif defined(__DOS__)
#	if defined(__WATCOMC__)
#		include <io.h>
#	elif defined(__DJGPP__)
#		include <io.h>
#		include <unistd.h>
#		include <cstdio>
#	else
#		error  "Please specify the header with file functions declarations."
#	endif
#elif (defined(__WXPM__))
#	include <io.h>
#elif (defined(__WXSTUBS__))
	// Have to ifdef this for different environments
#	include <io.h>
#elif (defined(__WXMAC__))
#if __MSL__ < 0x6000
int access( const char *path, int mode ) { return 0 ; }
#else
int _access( const char *path, int mode ) { return 0 ; }
#endif
char* mktemp( char * path ) { return path ;}
#	include <stat.h>
#	include <unistd.h>
#else
#	error  "Please specify the header with file functions declarations."
#endif  //Win/UNIX

#include <cstdio>       // SEEK_xxx constants
#include <fcntl.h>       // O_RDONLY &c

#if !defined(__MWERKS__) || defined(__WXMSW__)
#	include <sys/types.h>   // needed for stat
#	include <sys/stat.h>    // stat
#endif

// Windows compilers don't have these constants
#ifndef W_OK
enum {
	F_OK = 0,   // test for existence
	X_OK = 1,   //          execute permission
	W_OK = 2,   //          write
	R_OK = 4    //          read
};
#endif // W_OK

// there is no distinction between text and binary files under Unix, so define
// O_BINARY as 0 if the system headers don't do it already
#if defined(__UNIX__) && !defined(O_BINARY)
#	define   O_BINARY    (0)
#endif  //__UNIX__

#ifdef __SALFORDC__
#	include <unix.h>
#endif

// some broken compilers don't have 3rd argument in open() and creat()
#ifdef __SALFORDC__
#	define ACCESS(access)
#	define stat    _stat
#else // normal compiler
#	define ACCESS(access)  , (access)
#endif // Salford C

// wxWindows
#ifndef WX_PRECOMP
#	include <wx/string.h>
#	include <wx/intl.h>
#	include <wx/log.h>
#endif // !WX_PRECOMP

#ifdef __WXMSW__
#include <wx/msw/mslu.h>
#endif
#include <wx/filename.h>
#include <wx/filefn.h>

// ============================================================================
// implementation of CFile
// ============================================================================

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

bool CFile::Exists(const wxChar *name)
{
	return wxFileExists(name);
}

bool CFile::Access(const wxChar *name, OpenMode mode)
{
	int how;

	switch ( mode ) {
	default:
		wxFAIL_MSG(wxT("bad CFile::Access mode parameter."));
		// fall through
	case read:
		how = R_OK;
		break;
	
	case write:
		how = W_OK;
		break;
	case read_write:
		how = R_OK | W_OK;
		break;
	}

	return wxAccess(name, how) == 0;
}

// ----------------------------------------------------------------------------
// opening/closing
// ----------------------------------------------------------------------------

// ctors
CFile::CFile(const wxString& sFileName, OpenMode mode)
{
	wxASSERT(!sFileName.IsEmpty());

	m_fd = fd_invalid;
	m_error = false;
	Open(sFileName, mode);
}

//
// create the file, fail if it already exists and !bOverwrite
// 
// When creating files, we will always first try to create an ANSI file name,
// even if that means an extended ANSI file name. Only if it is not possible
// to do that, we fall back to  UTF-8 file names. This is unicode safe and is
// the only way to guarantee that we can later open any file in the file system,
// even if it is not an UTF-8 valid sequence.
// 
bool CFile::Create(const wxString& sFileName, bool bOverwrite, int accessMode)
{
	wxASSERT(!sFileName.IsEmpty());

	if ( accessMode == -1 ) {
		accessMode = thePrefs::GetFilePermissions();
	}
	m_filePath = sFileName;
	if (m_fd != fd_invalid) {
		Close();	
	}
	if (wxFileExists(sFileName) && !bOverwrite) {
		return false;
	}
	// Test if it is possible to use an ANSI name
	Unicode2CharBuf tmpFileName(unicode2char(sFileName));
	if (tmpFileName) {
		// Use an ANSI name
		m_fd = creat(tmpFileName, accessMode);
	} 
	
	if (m_fd == fd_invalid) { // Wrong conversion or can't create.
		// Try an UTF-8 name
		m_fd = creat(unicode2UTF8(sFileName), accessMode);
	}
	
	
	if (m_fd == fd_invalid) {
		AddDebugLogLineM( true, logCFile, 
			CFormat( wxT("Failed to created file '%s'!") )
				% m_filePath
				% m_fd );
		
		return false;
	} else {
		AddDebugLogLineM( false, logCFile, 
			CFormat( wxT("Created file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );
		
		return true;
	}
}

//
// open the file
// 
// When opening files, we will always first try to create an ANSI file name,
// even if that means an extended ANSI file name. Only if it is not possible
// to do that, we fall back to  UTF-8 file names. This is unicode safe and is
// the only way to guarantee that we can open any file in the file system,
// even if it is not an UTF-8 valid sequence.
//
bool CFile::Open(const wxString& sFileName, OpenMode mode, int accessMode)
{
	wxASSERT(!sFileName.IsEmpty());
	
	if ( accessMode == -1 ) {
		accessMode = thePrefs::GetFilePermissions();
	}
	int flags = O_BINARY;
#ifdef __linux__
	flags |=  O_LARGEFILE;
#endif
	m_filePath = sFileName;

	switch ( mode ) {
	case read:
		flags |= O_RDONLY;
		break;
	
	case write_append:
		if (CFile::Exists(sFileName))
		{
			flags |= O_WRONLY | O_APPEND;
			break;
		}
		//else: fall through as write_append is the same as write if the
		//      file doesn't exist
	
	case write:
		flags |= O_WRONLY | O_CREAT | O_TRUNC;
		break;
	
	case write_excl:
		flags |= O_WRONLY | O_CREAT | O_EXCL;
		break;

	case read_write:
		flags |= O_RDWR;
        	break;
	}

	if (m_fd != fd_invalid) {
		Close();	
	}
	// Test if it is possible to use an ANSI name
	Unicode2CharBuf tmpFileName(unicode2char(sFileName));
	if (tmpFileName) {
		// Use an ANSI name
		m_fd = open(tmpFileName, flags ACCESS(accessMode));
	} 
	
	if (m_fd == fd_invalid) { // Wrong conversion or can't open.
		// Try an UTF-8 name
		m_fd = open(unicode2UTF8(sFileName), flags ACCESS(accessMode));
	}
      
    
	if (m_fd == fd_invalid) {
		AddDebugLogLineM( true, logCFile, wxT("Failed to open file '") + sFileName + wxT("'!") );
		
		return false;
	} else {
		AddDebugLogLineM( false, logCFile,
			CFormat( wxT("Opened file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );
		
		return true;
	}    
}

//
// close
// 
bool CFile::Close() 
{
	wxASSERT(!m_filePath.IsEmpty());

	if ( IsOpened() ) {
		if (close(m_fd) == -1) {
			AddDebugLogLineM( true, logCFile,
				CFormat( wxT("Failed to close file '%s' with file descriptor '%d'.") )
					% m_filePath
					% m_fd );
			
			m_fd = fd_invalid;
			return false;
		} else {
			AddDebugLogLineM( false, logCFile,
				CFormat( wxT("Closed file '%s' with file descriptor '%d'.") )
					% m_filePath
					% m_fd );
			
			m_fd = fd_invalid;
		}
	} else {
		wxASSERT(false);
	}
	
	return true;
}

// ----------------------------------------------------------------------------
// read/write
// ----------------------------------------------------------------------------

// read
off_t CFile::Read(void *pBuf, off_t nCount) const
{
	wxCHECK( (pBuf != NULL) && IsOpened(), 0 );

#ifdef __MWERKS__
	off_t iRc = ::read(m_fd, (char*) pBuf, nCount);
#else
	off_t iRc = ::read(m_fd, pBuf, nCount);
#endif
	if ( iRc == -1 ) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't read from file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );
		
		m_error = true;
		return wxInvalidOffset;
	} else {
		return iRc;
	}
}

off_t CFile::SafeRead(unsigned char* pBuf, off_t nCount, int nRetries) const 
{
	off_t total_done = 0;
	int retries = 0; 
	while ((total_done < nCount) && (retries <= nRetries)) {
		int done = Read(pBuf+total_done,nCount-total_done);
		if (done == wxInvalidOffset) {
			// Woops, failure!
			throw wxString(wxT("Error while reading file!"));
		} else {
			total_done += done;
			wxASSERT(total_done <= nCount);
			if (total_done == nCount) {
				// This file is done.
			} else {
				retries++;
			}			
		}
	}
	
	if (total_done < nCount) {
		// The total bytes were not reached on the specified replies.
		throw wxString(wxString::Format(wxT("Error while reading file (unable to read %u bytes on two retries)!"), (unsigned)nCount));
	}
		
	wxASSERT(total_done == nCount);
	return total_done; // Which should equal nCount	
}


// write
size_t CFile::Write(const void *pBuf, size_t nCount)
{
	wxASSERT(pBuf != NULL);
	wxASSERT(IsOpened());
#ifdef __MWERKS__
#if __MSL__ >= 0x6000
	size_t iRc = ::write(m_fd, (void*) pBuf, nCount);
#else
	size_t iRc = ::write(m_fd, (const char*) pBuf, nCount);
#endif
#else
	size_t iRc = ::write(m_fd, pBuf, nCount);
#endif
	if ( ((int)iRc) == -1 ) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't write to file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );		
		
		m_error = true;
		return (size_t)0;
	} else {
		return iRc;
	}
}

// flush
bool CFile::Flush()
{
	if ( IsOpened() ) {
#ifdef __WXMSW__
		if (_commit(m_fd) == -1) {
#else
	        if ( fsync(m_fd) == -1 ) {
#endif
			AddDebugLogLineM( true, logCFile,
				CFormat( wxT("Can't flush file '%s' with file descriptor '%d'.") )
					% m_filePath
					% m_fd );

			m_error = true;			
			return false;
		}
    }
    return true;
}

// ----------------------------------------------------------------------------
// seek
// ----------------------------------------------------------------------------
#include <cerrno>
#include <cstring>

// seek
off_t CFile::Seek(off_t ofs, wxSeekMode mode) const
{
	wxASSERT( IsOpened() );

	int origin;
	switch ( mode ) {
	default:
		wxFAIL_MSG(wxT("Unknown seek origin"));

	case wxFromStart:
		origin = SEEK_SET;
		break;

	case wxFromCurrent:
		origin = SEEK_CUR;
		break;

	case wxFromEnd:
		origin = SEEK_END;
		break;
	}

	off_t iRc = lseek(m_fd, ofs, origin);
	if ( iRc == -1 ) {
		AddDebugLogLineM( true, logCFile, wxString(wxT("Error in lseek: ")) + wxString(char2unicode(strerror(errno))));
		m_error = true;
		return wxInvalidOffset;
	} else {
		return (off_t) iRc;
	}
}

// get current off_t
off_t CFile::GetPosition() const
{
	wxASSERT( IsOpened() );
	
	off_t iRc = wxTell(m_fd);
	if (iRc == -1) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't get seek position for file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );		
		
		return wxInvalidOffset;
	} else {
		return (off_t)iRc;
	}
}

// get current file length
off_t CFile::GetLength() const
{
	wxASSERT( IsOpened() );
	
#ifdef __VISUALC__
	off_t iRc = _filelength(m_fd);
#else // !VC++
	off_t iRc = wxTell(m_fd);
	if ( iRc != -1 ) {
		// @ have to use const_cast :-(
		off_t iLen = ((CFile *)this)->Seek(0, wxFromEnd);
		if ( iLen != -1 ) {
			// restore old position
			if ( ((CFile *)this)->Seek(iRc) == -1 ) {
				// error
				iLen = -1;
			}
		}
		iRc = iLen;
	}
#endif  // VC++
	
	if ( iRc == -1 ) {
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't find length of file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );		
		
		return wxInvalidOffset;
	} else {
		return (off_t)iRc;
	}
}
bool CFile::SetLength(off_t new_len) {
#ifdef __WXMSW__
	return chsize(this->fd(), new_len);
#else
	return ftruncate(this->fd(), new_len);
#endif
}	

// is end of file reached?
bool CFile::Eof() const
{
	wxASSERT( IsOpened() );
	
	off_t iRc;
	
#if defined(__DOS__) || defined(__UNIX__) || defined(__GNUWIN32__) || defined( __MWERKS__ ) || defined(__SALFORDC__)
	// @@ this doesn't work, of course, on unseekable file descriptors
	off_t ofsCur = GetPosition(),
	ofsMax = GetLength();
	if ( ofsCur == (off_t)wxInvalidOffset || ofsMax == (off_t)wxInvalidOffset ) {
		iRc = -1;
	} else {
		iRc = ofsCur == ofsMax;
	}
#else  // Windows and "native" compiler
	iRc = eof(m_fd);
#endif // Windows/Unix
	
	switch ( iRc ) {
	case 1:
		break;
	
	case 0:
		return false;
	
	case -1:
		AddDebugLogLineM( true, logCFile,
			CFormat( wxT("Can't determine if the end of file is reached for file '%s' with file descriptor '%d'.") )
				% m_filePath
				% m_fd );	
		break;
	
	default:
		wxFAIL_MSG(wxT("invalid eof() return value."));
	}
	
	return true;
}


///////////////////////////////////////////////////////////////////////////////
// CSafeFile

off_t CSafeFile::Read(void *pBuf, off_t nCount) const
{
	if (GetPosition() + nCount > GetLength()) {
		throw CEOFException(wxT("Read after end of CSafeFile"));
	}
	
	return CFile::Read( pBuf, nCount );
}

size_t CSafeFile::Write(const void *pBuf, size_t nCount)
{
	return CFile::Write( pBuf, nCount );
}


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

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <unistd.h>		// Needed for close(2)

#include "CFile.h"		// Interface declarations.
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CamuleAppBase.h"	// Needed for theApp

#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for HAVE_SYS_PARAM_H
#endif

// Test if we have _GNU_SOURCE before the next step will mess up 
// setting __USE_GNU 
// (only needed for gcc-2.95 compatibility, gcc 3.2 always defines it)
#include <wx/setup.h>

// Mario Sergio Fujikawa Ferreira <lioux@FreeBSD.org>
// to detect if this is a *BSD system
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#ifdef __BORLANDC__
#	pragma hdrstop
#endif

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

	switch ( mode )
	{
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
CFile::CFile(const wxChar *szFileName, OpenMode mode)
{
	m_fd = fd_invalid;
	m_error = FALSE;

	Open(szFileName, mode);
}

// create the file, fail if it already exists and bOverwrite
bool CFile::Create(const wxChar *szFileName, bool bOverwrite, int accessMode)
{
	fFilePath=szFileName;

	// if bOverwrite we create a new file or truncate the existing one,
	// otherwise we only create the new file and fail if it already exists
#if defined(__WXMAC__) && !defined(__UNIX__)
	// Dominic Mazzoni [dmazzoni+@cs.cmu.edu] reports that open is still broken on the mac, so we replace
	// int fd = open(wxUnix2MacFilename( szFileName ), O_CREAT | (bOverwrite ? O_TRUNC : O_EXCL), access);
	int fd = creat( szFileName , accessMode);
#else
	int fd = wxOpen( szFileName,
			O_BINARY | O_WRONLY | O_CREAT |
			(bOverwrite ? O_TRUNC : O_EXCL)
			ACCESS(accessMode) );
#endif
	if ( fd == -1 ) {
		wxLogSysError(_("can't create file '%s'"), szFileName);
		return FALSE;
	} else {
		Attach(fd);
		return TRUE;
	}
}

// open the file
bool CFile::Open(const wxChar *szFileName, OpenMode mode, int accessMode)
{
    int flags = O_BINARY;

    fFilePath=szFileName;

    switch ( mode )
    {
        case read:
            flags |= O_RDONLY;
            break;

        case write_append:
            if ( CFile::Exists(szFileName) )
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

    int fd = wxOpen( szFileName, flags ACCESS(accessMode));
    if ( fd == -1 )
    {
    	theApp.amuledlg->AddLogLine(true, CString(_("Can't open file '%s'")), szFileName);
        return FALSE;
    }
    else {
        Attach(fd);
        return TRUE;
    }
}

// close
bool CFile::Close()
{
    if ( IsOpened() ) {
        if ( close(m_fd) == -1 ) {
            wxLogSysError(_("can't close file descriptor %d"), m_fd);
            m_fd = fd_invalid;
            return FALSE;
        }
        else
            m_fd = fd_invalid;
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// read/write
// ----------------------------------------------------------------------------

// read
off_t CFile::Read(void *pBuf, off_t nCount)
{
    wxCHECK( (pBuf != NULL) && IsOpened(), 0 );

#ifdef __MWERKS__
    off_t iRc = ::read(m_fd, (char*) pBuf, nCount);
#else
    off_t iRc = ::read(m_fd, pBuf, nCount);
#endif
    if ( iRc == -1 ) {
        wxLogSysError(_("can't read from file descriptor %d"), m_fd);
        return wxInvalidOffset;
    }
    else
        return (off_t)iRc;
}

// write
size_t CFile::Write(const void *pBuf, size_t nCount)
{
    wxCHECK( (pBuf != NULL) && IsOpened(), 0 );

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
        wxLogSysError(_("can't write to file descriptor %d"), m_fd);
        m_error = TRUE;
        return (size_t)0;
    }
    else
        return iRc;
}

// flush
bool CFile::Flush()
{
    if ( IsOpened() ) {
		#ifdef __WXMSW__
		if (_commit(m_fd) == -1)
		#else
	        if ( fsync(m_fd) == -1 ) 
		#endif
		{
		    wxLogSysError(_("can't flush file descriptor %d"), m_fd);
	            return FALSE;
		}
    }
    return TRUE;
}

// ----------------------------------------------------------------------------
// seek
// ----------------------------------------------------------------------------
#include <cerrno>
#include <cstring>

// seek
off_t CFile::Seek(off_t ofs, wxSeekMode mode)
{
    wxASSERT( IsOpened() );

    int origin;
    switch ( mode ) {
        default:
            wxFAIL_MSG(_("unknown seek origin"));

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
    	printf("Error in lseek: %s\n", strerror(errno));
        return wxInvalidOffset;
    }
    else
        return (off_t) iRc;
}

// get current off_t
off_t CFile::Tell() const
{
    wxASSERT( IsOpened() );

    off_t iRc = wxTell(m_fd);
    if ( iRc == -1 ) {
        wxLogSysError(_("can't get seek position on file descriptor %d"), m_fd);
        return wxInvalidOffset;
    }
    else
        return (off_t)iRc;
}

// get current file length
off_t CFile::Length() const
{
    wxASSERT( IsOpened() );

#ifdef __VISUALC__
    off_t iRc = _filelength(m_fd);
#else // !VC++
    off_t iRc = wxTell(m_fd);
    if ( iRc != -1 ) {
        // @ have to use const_cast :-(
        off_t iLen = ((CFile *)this)->SeekEnd();
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
        wxLogSysError(_("can't find length of file on file descriptor %d"), m_fd);
        return wxInvalidOffset;
    }
    else
        return (off_t)iRc;
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
    off_t ofsCur = Tell(),
    ofsMax = Length();
    if ( ofsCur == wxInvalidOffset || ofsMax == wxInvalidOffset )
        iRc = -1;
    else
        iRc = ofsCur == ofsMax;
#else  // Windows and "native" compiler
    iRc = eof(m_fd);
#endif // Windows/Unix

    switch ( iRc ) {
        case 1:
            break;

        case 0:
            return FALSE;

        case -1:
            wxLogSysError(_("can't determine if the end of file is reached on descriptor %d"), m_fd);
                break;

        default:
            wxFAIL_MSG(_("invalid eof() return value."));
    }

    return TRUE;
}

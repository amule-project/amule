//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include <common/MuleDebug.h>			// Needed for MULE_VALIDATE_*
#include <common/StringFunctions.h>	// Needed for unicode2char
#include <common/Format.h>

#include "CFile.h"				// Interface declarations.
#include "FileFunctions.h"		// Needed for CheckFileExists
#include "Preferences.h"		// Needed for thePrefs

#include <unistd.h>				// Needed for close(2)
#include <cstdio>       		// SEEK_xxx constants
#include <fcntl.h>       		// O_RDONLY &c

#include <wx/filefn.h>

#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for HAVE_SYS_PARAM_H
#endif

// Mario Sergio Fujikawa Ferreira <lioux@FreeBSD.org>
// to detect if this is a *BSD system
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif


// standard
#if defined(__WXMSW__) && !defined(__GNUWIN32__) && !defined(__WXWINE__) && !defined(__WXMICROWIN__)
#	include <io.h>
#	ifndef __SALFORDC__
#		define   WIN32_LEAN_AND_MEAN
#		define   NOSERVICE
#		define   NOIME
#		define   NOATOM
#		define   NOGDI
#		define   NOGDICAPMASKS
#		define   NOMETAFILE
#		define   NOMINMAX
#		define   NOMSG
#		define   NOOPENFILE
#		define   NORASTEROPS
#		define   NOSCROLL
#		define   NOSOUND
#		define   NOSYSMETRICS
#		define   NOTEXTMETRIC
#		define   NOWH
#		define   NOCOMM
#		define   NOKANJI
#		define   NOCRYPT
#		define   NOMCX
#	endif
#elif (defined(__UNIX__) || defined(__GNUWIN32__))
#	ifdef __GNUWIN32__
#		include <windows.h>
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
#else
#	error  "Please specify the header with file functions declarations."
#endif  //Win/UNIX


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

#ifdef __WXMSW__
#include <wx/msw/mslu.h>
#endif


// The following defines handle different names across platforms,
// and ensures that we use 64b IO on windows (only 32b by default).
#ifdef __WXMSW__
	#define FLUSH_FD(x)			_commit(x)
	#define SEEK_FD(x, y, z)	_lseeki64(x, y, z)
	#define TELL_FD(x)			_telli64(x)
#else
	#define FLUSH_FD(x)			fsync(x)
	#define SEEK_FD(x, y, z)	lseek(x, y, z)
	#define TELL_FD(x)			wxTell(x)
#endif


CFile::CFile()
	: m_fd(fd_invalid)
{}


CFile::CFile(const wxString& fileName, OpenMode mode)
	: m_fd(fd_invalid)
{
	Open(fileName, mode);
}


CFile::~CFile()
{ 
	if (IsOpened()) {
		Close(); 
	}
}


int CFile::fd() const
{
	return m_fd;
}


bool CFile::IsOpened() const
{
	return m_fd != fd_invalid;
}


const wxString& CFile::GetFilePath() const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot return path when no file is open."));

	return m_filePath;
}


bool CFile::Create(const wxString& path, bool overwrite, int accessMode)
{
	if (!overwrite && CheckFileExists(path)) {
		return false;
	}

	return Open(path, write, accessMode);
}


bool CFile::Open(const wxString& fileName, OpenMode mode, int accessMode)
{
	MULE_VALIDATE_PARAMS(!fileName.IsEmpty(), wxT("CFile: Cannot open, empty path."));

	if ( accessMode == -1 ) {
#ifndef MULEUNIT // TODO: Remove the need for this
		accessMode = thePrefs::GetFilePermissions();
#else
		accessMode = wxS_DEFAULT;
#endif
	}

#ifdef __linux__
	int flags = O_BINARY | O_LARGEFILE;
#else
	int flags = O_BINARY;
#endif

	switch ( mode ) {
		case read:
			flags |= O_RDONLY;
			break;
	
		case write_append:
			if (CheckFileExists(fileName))
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

	if (IsOpened()) {
		Close();	
	}

	
	// When opening files, we will always first try to create an ANSI file name,
	// even if that means an extended ANSI file name. Only if it is not possible
	// to do that, we fall back to  UTF-8 file names. This is unicode safe and is
	// the only way to guarantee that we can open any file in the file system,
	// even if it is not an UTF-8 valid sequence.
	//
	
	// Test if it is possible to use an ANSI name
	Unicode2CharBuf tmpFileName = unicode2char(fileName);
	if (tmpFileName) {
		// Use an ANSI name
		m_fd = open(tmpFileName, flags, accessMode);
	} 
	
	if (m_fd == fd_invalid) { // Wrong conversion or can't open.
		// Try an UTF-8 name
		m_fd = open(unicode2UTF8(fileName), flags, accessMode);
	}
	
	m_filePath = fileName;
      
    return IsOpened();
}


bool CFile::Close() 
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot close closed file."));

	int result = close(m_fd);
	m_fd = fd_invalid;
	
	return result != wxInvalidOffset;
}


bool CFile::Flush()
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot flush closed file."));
	
	return (FLUSH_FD(m_fd) != wxInvalidOffset);
}


sint64 CFile::doRead(void* buffer, size_t count) const
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CFile: Invalid buffer in read operation."));
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot read from closed file."));
	
	size_t totalRead = 0;
	while (totalRead < count) {
		int current = ::read(m_fd, (char*)buffer + totalRead, count - totalRead);
		
		if (current == wxInvalidOffset) {
			// Read error, nothing we can do other than abort.
			return wxInvalidOffset;
		} else if ((totalRead + current < count) && Eof()) {
			// We may fail to read the specified count in a couple
			// of situations: EOF and interrupts. The check for EOF
			// is needed to avoid inf. loops.
			break;
		}
		
		totalRead += current;
	}
	
	return totalRead;
}


sint64 CFile::doWrite(const void* buffer, size_t nCount)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CFile: Invalid buffer in write operation."));
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot write to closed file."));
	
	return ::write(m_fd, buffer, nCount);
}


sint64 CFile::doSeek(sint64 offset) const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("Cannot seek on closed file."));
	MULE_VALIDATE_PARAMS(offset >= 0, wxT("Invalid position, must be positive."));
	
	return SEEK_FD(m_fd, offset, SEEK_SET);
}


uint64 CFile::GetPosition() const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("Cannot get position on closed file."));
	
	sint64 pos = TELL_FD(m_fd);

	MULE_VALIDATE_STATE(pos >= 0, wxT("Failed to retrieve position in file."));
	
	return pos;
}


uint64 CFile::GetLength() const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot get length of closed file."));

	uint64 pos = GetPosition();
	sint64 len = SEEK_FD(m_fd, 0, SEEK_END);

	MULE_VALIDATE_STATE(len >= 0, wxT("CFile: Failed to retreive length of file."));
	
	if (SEEK_FD(m_fd, pos, SEEK_SET) == wxInvalidOffset) {
		MULE_VALIDATE_STATE(false, wxT("CFile: Failed to restore pointer position."));
	}	
	
	return len;
}


bool CFile::SetLength(size_t new_len)
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot set length when no file is open."));

#ifdef __WXMSW__
	return chsize(m_fd, new_len);
#else
	return ftruncate(m_fd, new_len);
#endif
}

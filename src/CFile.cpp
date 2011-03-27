//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 1998-2011 Vadim Zeitlin ( zeitlin@dptmaths.ens-cachan.fr )
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


#include "CFile.h"		// Interface declarations.
#include "Logger.h"		// Needed for AddDebugLogLineC
#include <common/Path.h>	// Needed for CPath


#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for HAVE_SYS_PARAM_H
#endif


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
	#define SEEK_FD(x, y, z)		_lseeki64(x, y, z)
	#define TELL_FD(x)			_telli64(x)

	#if (__MSVCRT_VERSION__ < 0x0601)
		//#warning MSCVRT-Version smaller than 6.01
		#define STAT_FD(x, y)		_fstati64(x, y)
		#define STAT_STRUCT		struct _stati64
	#else
		#define STAT_FD(x, y)		_fstat64(x, y)
		#define STAT_STRUCT		struct __stat64
	#endif
#else

// We don't need to sync all meta-data, just the contents,
// so use fdatasync when possible (see man fdatasync).
	#if defined(_POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO > 0)
		#define FLUSH_FD(x)		fdatasync(x)
	#else
		#define FLUSH_FD(x)		fsync(x)
	#endif

	#define SEEK_FD(x, y, z)		lseek(x, y, z)
	#define TELL_FD(x)			wxTell(x)
	#define STAT_FD(x, y)			fstat(x, y)
	#define STAT_STRUCT			struct stat
#endif


// This function is used to check if a syscall failed, in that case
// log an appropriate message containing the errno string.
inline void syscall_check(
	bool check,
	const CPath& filePath,
	const wxString& what)
{
	if (!check) {
		AddDebugLogLineC(logCFile,
			CFormat(wxT("Error when %s (%s): %s"))
				% what % filePath % wxSysErrorMsg());
	}
}


CSeekFailureException::CSeekFailureException(const wxString& desc)
	: CIOFailureException(wxT("SeekFailure"), desc)
{}


CFile::CFile()
	: m_fd(fd_invalid)
{}


CFile::CFile(const CPath& fileName, OpenMode mode)
	: m_fd(fd_invalid)
{
	Open(fileName, mode);
}


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


const CPath& CFile::GetFilePath() const
{
	return m_filePath;
}


bool CFile::Create(const CPath& path, bool overwrite, int accessMode)
{
	if (!overwrite && path.FileExists()) {
		return false;
	}

	return Open(path, write, accessMode);
}

bool CFile::Create(const wxString& path, bool overwrite, int accessMode)
{
	return Create(CPath(path), overwrite, accessMode);
}


bool CFile::Open(const wxString& fileName, OpenMode mode, int accessMode)
{
	MULE_VALIDATE_PARAMS(fileName.Length(), wxT("CFile: Cannot open, empty path."));
	
	return Open(CPath(fileName), mode, accessMode);
}


bool CFile::Open(const CPath& fileName, OpenMode mode, int accessMode)
{
	MULE_VALIDATE_PARAMS(fileName.IsOk(), wxT("CFile: Cannot open, empty path."));

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
			if (fileName.FileExists())
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
	
	m_filePath = fileName;

	// Windows needs wide character file names
#ifdef __WXMSW__
	m_fd = _wopen(fileName.GetRaw().c_str(), flags, accessMode);
#else
	Unicode2CharBuf tmpFileName = filename2char(fileName.GetRaw());
	wxASSERT_MSG(tmpFileName, wxT("Convertion failed in CFile::Open"));
	m_fd = open(tmpFileName, flags, accessMode);
#endif
	syscall_check(m_fd != fd_invalid, m_filePath, wxT("opening file"));
	
	return IsOpened();
}


void CFile::Reopen(OpenMode mode)
{
	if (!Open(m_filePath, mode)) {
		throw CIOFailureException(wxString(wxT("Error reopening file")));
	}
}


bool CFile::Close() 
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot close closed file."));

	bool closed = (close(m_fd) != -1);
	syscall_check(closed, m_filePath, wxT("closing file"));
	
	m_fd = fd_invalid;	
	
	return closed;
}


bool CFile::Flush()
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot flush closed file."));
	
	bool flushed = (FLUSH_FD(m_fd) != -1);
	syscall_check(flushed, m_filePath, wxT("flushing file"));

	return flushed;	
}


sint64 CFile::doRead(void* buffer, size_t count) const
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CFile: Invalid buffer in read operation."));
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot read from closed file."));
	
	size_t totalRead = 0;
	while (totalRead < count) {
		int current = ::read(m_fd, (char*)buffer + totalRead, count - totalRead);
		
		if (current == -1) {
			// Read error, nothing we can do other than abort.
			throw CIOFailureException(wxString(wxT("Error reading from file: ")) + wxSysErrorMsg());
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

	sint64 result = ::write(m_fd, buffer, nCount);
	
	if (result != (sint64)nCount) {
		throw CIOFailureException(wxString(wxT("Error writing to file: ")) + wxSysErrorMsg());
	}

	return result;
}


sint64 CFile::doSeek(sint64 offset) const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("Cannot seek on closed file."));
	MULE_VALIDATE_PARAMS(offset >= 0, wxT("Invalid position, must be positive."));
	
	sint64 result = SEEK_FD(m_fd, offset, SEEK_SET);

	if (result == offset) {
		return result;
	} else if (result == wxInvalidOffset) {
		throw CSeekFailureException(wxString(wxT("Seeking failed: ")) + wxSysErrorMsg());
	} else {
		throw CSeekFailureException(wxT("Seeking returned incorrect position"));		
	}
}


uint64 CFile::GetPosition() const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("Cannot get position in closed file."));
	
	sint64 pos = TELL_FD(m_fd);
	if (pos == wxInvalidOffset) {
		throw CSeekFailureException(wxString(wxT("Failed to retrieve position in file: ")) + wxSysErrorMsg());
	}
	
	return pos;
}


uint64 CFile::GetLength() const
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot get length of closed file."));

	STAT_STRUCT buf;
	if (STAT_FD(m_fd, &buf) == -1) {
		throw CIOFailureException(wxString(wxT("Failed to retrieve length of file: ")) + wxSysErrorMsg());
	}
	
	return buf.st_size;
}


uint64 CFile::GetAvailable() const
{
	const uint64 length = GetLength();
	const uint64 position = GetPosition();

	// Safely handle seeking past EOF
	if (position < length) {
		return length - position;
	}

	// File is at or after EOF
	return 0;
}


bool CFile::SetLength(uint64 new_len)
{
	MULE_VALIDATE_STATE(IsOpened(), wxT("CFile: Cannot set length when no file is open."));

#ifdef __WXMSW__
#ifdef _MSC_VER
// MSVC has a 64bit version
	bool result = _chsize_s(m_fd, new_len) == 0;
#else
// MingW has an old runtime without it
	bool result = chsize(m_fd, new_len) == 0;
#endif
#else
	bool result = ftruncate(m_fd, new_len) != -1;
#endif

	syscall_check(result, m_filePath, wxT("truncating file"));

	return result;
}
// File_checked_for_headers

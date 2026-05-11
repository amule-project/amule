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


#include "CFile.h"			// Interface declarations.
#include "Logger.h"			// Needed for AddDebugLogLineC
#include <common/Path.h>		// Needed for CPath
#include "config.h"			// Needed for HAVE_SYS_PARAM_H

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

// standard
#if defined(__WINDOWS__) && !defined(__GNUWIN32__) && !defined(__WXWINE__) && !defined(__WXMICROWIN__)
#	include <io.h>
#	ifndef __SALFORDC__
#		define   WIN32_LEAN_AND_MEAN
#		define   NOSERVICE
#		define   NOIME
#		define   NOATOM
#		define   NOGDI
#		define   NOGDICAPMASKS
#		define   NOMETAFILE
#ifndef NOMINMAX
	#define   NOMINMAX
#endif
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


// The following defines handle different names across platforms,
// and ensures that we use 64b IO on windows (only 32b by default).
#ifdef __WINDOWS__
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
			CFormat("Error when %s (%s): %s")
				% what % filePath % wxSysErrorMsg());
	}
}


CSeekFailureException::CSeekFailureException(const wxString& desc)
	: CIOFailureException("SeekFailure", desc)
{}


CFile::CFile()
	: m_fd(fd_invalid), m_safeWrite(false),
	  m_writeBufferPending(0), m_canBuffer(false)
{}


CFile::CFile(const CPath& fileName, OpenMode mode)
	: m_fd(fd_invalid), m_writeBufferPending(0), m_canBuffer(false)
{
	Open(fileName, mode);
}


CFile::CFile(const wxString& fileName, OpenMode mode)
	: m_fd(fd_invalid), m_writeBufferPending(0), m_canBuffer(false)
{
	Open(fileName, mode);
}


CFile::~CFile()
{
	if (IsOpened()) {
		// If the writing gets aborted, dtor is still called.
		// In this case do NOT replace the original file with the
		// probably broken new one!
		m_safeWrite = false;
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
	MULE_VALIDATE_PARAMS(fileName.Length(), "CFile: Cannot open, empty path.");

	return Open(CPath(fileName), mode, accessMode);
}


bool CFile::Open(const CPath& fileName, OpenMode mode, int accessMode)
{
	MULE_VALIDATE_PARAMS(fileName.IsOk(), "CFile: Cannot open, empty path.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (IsOpened()) {
		Close();
	}

	m_safeWrite = false;
	m_filePath = fileName;
	m_writeBufferPending = 0;
	// Buffer writes for any mode that can actually write. Read-only
	// stays unbuffered so a misuse (like writing to a read-opened
	// file) still fails immediately at the doWrite call site,
	// preserving FileDataIOTest's CFile.Constructor contract.
	m_canBuffer = (mode != read);

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

		/* fall through */
		case write:
			flags |= O_WRONLY | O_CREAT | O_TRUNC;
			break;

		case write_safe:
			flags |= O_WRONLY | O_CREAT | O_TRUNC;
			m_filePath = m_filePath.AppendExt(".new");
			m_safeWrite = true;
			break;

		case write_excl:
			flags |= O_WRONLY | O_CREAT | O_EXCL;
			break;

		case read_write:
			flags |= O_RDWR;
		break;
	}

	// Windows needs wide character file names
#ifdef __WINDOWS__
	m_fd = _wopen(m_filePath.GetRaw().c_str(), flags, accessMode);
#else
	Unicode2CharBuf tmpFileName = filename2char(m_filePath.GetRaw());
	wxASSERT_MSG(tmpFileName, "Conversion failed in CFile::Open");
	m_fd = open(tmpFileName, flags, accessMode);
#endif
	syscall_check(m_fd != fd_invalid, m_filePath, "opening file");

	return IsOpened();
}


void CFile::Reopen(OpenMode mode)
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	if (!Open(m_filePath, mode)) {
		throw CIOFailureException(wxString("Error reopening file"));
	}
}


bool CFile::Close()
{
	MULE_VALIDATE_STATE(IsOpened(), "CFile: Cannot close closed file.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Flush userspace write buffer to the fd before closing — otherwise
	// any pending bytes from doWrite() would be silently dropped.
	DrainWriteBuffer();

	bool closed = (close(m_fd) != -1);
	syscall_check(closed, m_filePath, "closing file");

	m_fd = fd_invalid;

	if (m_safeWrite) {
		CPath filePathTemp(m_filePath);
		m_filePath = m_filePath.RemoveExt();	// restore m_filePath for Reopen()
		if (closed) {
			closed = CPath::RenameFile(filePathTemp, m_filePath, true);
		}
	}

	return closed;
}


bool CFile::Flush()
{
	MULE_VALIDATE_STATE(IsOpened(), "CFile: Cannot flush closed file.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Userspace buffer first, then ask the kernel to commit to disk.
	DrainWriteBuffer();

	bool flushed = (FLUSH_FD(m_fd) != -1);
	syscall_check(flushed, m_filePath, "flushing file");

	return flushed;
}


sint64 CFile::doRead(void* buffer, size_t count) const
{
	MULE_VALIDATE_PARAMS(buffer, "CFile: Invalid buffer in read operation.");
	MULE_VALIDATE_STATE(IsOpened(), "CFile: Cannot read from closed file.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Read-after-write (or interleaved read/write on a read_write file)
	// must see preceding writes. For pure-read files the buffer is
	// always empty so this is a single branch.
	DrainWriteBuffer();

	size_t totalRead = 0;
	while (totalRead < count) {
		int current = ::read(m_fd, (char*)buffer + totalRead, count - totalRead);

		if (current == -1) {
			// Read error, nothing we can do other than abort.
			throw CIOFailureException(wxString("Error reading from file: ") + wxSysErrorMsg());
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


void CFile::DrainWriteBuffer() const
{
	if (m_writeBufferPending == 0) {
		return;
	}

	// Loop to handle partial writes (e.g. interrupted by a signal).
	size_t total = 0;
	while (total < m_writeBufferPending) {
		ssize_t written = ::write(m_fd,
			m_writeBuffer.get() + total,
			m_writeBufferPending - total);
		if (written < 0) {
			throw CIOFailureException(
				wxString("Error flushing write buffer: ") + wxSysErrorMsg());
		}
		total += (size_t)written;
	}
	m_writeBufferPending = 0;
}


sint64 CFile::doWrite(const void* buffer, size_t nCount)
{
	MULE_VALIDATE_PARAMS(buffer, "CFile: Invalid buffer in write operation.");
	MULE_VALIDATE_STATE(IsOpened(), "CFile: Cannot write to closed file.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Read-only files: the kernel will reject the write with EBADF;
	// surface that immediately by going direct, the same way the
	// pre-buffering version did.
	//
	// Single payload that doesn't fit in the buffer at all also goes
	// direct — no point splitting into 64 KB chunks just to copy
	// through the buffer first. Drain pending bytes (if any) so
	// ordering is preserved.
	if (!m_canBuffer || nCount >= kWriteBufferSize) {
		DrainWriteBuffer();
		if (nCount == 0) {
			return 0;
		}
		sint64 result = ::write(m_fd, buffer, nCount);
		if (result != (sint64)nCount) {
			throw CIOFailureException(
				wxString("Error writing to file: ") + wxSysErrorMsg());
		}
		return result;
	}

	if (nCount == 0) {
		return 0;
	}

	// New write would overflow the buffer; drain first.
	if (m_writeBufferPending + nCount > kWriteBufferSize) {
		DrainWriteBuffer();
	}

	// Lazy-allocate on first buffered write: a CFile that's opened
	// write-capable but never written to (e.g. construct-then-close
	// on an early error path) shouldn't pay for the buffer.
	if (!m_writeBuffer) {
		m_writeBuffer.reset(new char[kWriteBufferSize]);
	}

	memcpy(m_writeBuffer.get() + m_writeBufferPending, buffer, nCount);
	m_writeBufferPending += nCount;
	return (sint64)nCount;
}


sint64 CFile::doSeek(sint64 offset) const
{
	if (!IsOpened()) {
		throw CSeekFailureException("Cannot seek on closed file.");
	}

	MULE_VALIDATE_PARAMS(offset >= 0, "Invalid position, must be positive.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Pending bytes belong at the pre-seek position; flush before
	// changing the fd's offset so writes don't end up in the wrong
	// place. (CSafeFile / known.met save uses Seek() to back-patch
	// the header after writing the body, so this matters in practice.)
	DrainWriteBuffer();

	sint64 result = SEEK_FD(m_fd, offset, SEEK_SET);

	if (result == offset) {
		return result;
	} else if (result == wxInvalidOffset) {
		throw CSeekFailureException(wxString("Seeking failed: ") + wxSysErrorMsg());
	} else {
		throw CSeekFailureException("Seeking returned incorrect position");
	}
}


uint64 CFile::GetPosition() const
{
	MULE_VALIDATE_STATE(IsOpened(), "Cannot get position in closed file.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// Reported position must include any pending buffered bytes.
	// Easiest is to drain so TELL_FD's answer is authoritative.
	DrainWriteBuffer();

	sint64 pos = TELL_FD(m_fd);
	if (pos == wxInvalidOffset) {
		throw CSeekFailureException(wxString("Failed to retrieve position in file: ") + wxSysErrorMsg());
	}

	return pos;
}


uint64 CFile::GetLength() const
{
	MULE_VALIDATE_STATE(IsOpened(), "CFile: Cannot get length of closed file.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// fstat reads inode metadata, not buffered bytes — drain so the
	// reported size reflects pending buffered writes.
	DrainWriteBuffer();

	STAT_STRUCT buf;
	if (STAT_FD(m_fd, &buf) == -1) {
		throw CIOFailureException(wxString("Failed to retrieve length of file: ") + wxSysErrorMsg());
	}

	return buf.st_size;
}


uint64 CFile::GetAvailable() const
{
	// Lock around both calls so length/position are taken atomically;
	// otherwise a concurrent write could land between and skew the
	// reported "available" count. Recursive so the inner GetLength /
	// GetPosition calls (which lock again) don't deadlock.
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

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
	MULE_VALIDATE_STATE(IsOpened(), "CFile: Cannot set length when no file is open.");

	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	// ftruncate / chsize operate on the kernel-side size; drain the
	// userspace buffer first so any pending bytes are part of the
	// length-resolution decision (e.g. extend-then-write patterns).
	DrainWriteBuffer();

#ifdef __WINDOWS__
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

	syscall_check(result, m_filePath, "truncating file");

	return result;
}
// File_checked_for_headers

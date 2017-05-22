//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef FILELOCK_H
#define FILELOCK_H

#include <fcntl.h>
#include <cerrno>
#ifndef _MSC_VER
#include <unistd.h> // Do_not_auto_remove (Needed for OpenBSD)
#endif
#include <string> // Do_not_auto_remove (g++-4.0.1 except win32)


/**
 * This class provides an easy way to lock non-critical
 * files used by multiple applications. However, since
 * the implementation relies on fcntl, it may not work
 * on all filesystems (NFS) and thus locking is not
 * certain.
 *
 * Currently, this lock holds an exclusive lock on the
 * file in question. It is assumed that the file will
 * be read/written by all users.
 *
 */
class CFileLock
{
public:
	/**
	 * Locks the lock-file for the specified file.
	 *
	 * The lock-file is a file named file + "_lock", and
	 * will be created if it does not already exist. This
	 * file is not removed afterwards.
	 */
	CFileLock(const std::string& file)
#ifdef _WIN32
		: m_ok(false)
	{
		hd = CreateFileA((file + "_lock").c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,		// share - shareable
			NULL,									// security - not inheritable
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_ARCHIVE,
			NULL);
		if (hd != INVALID_HANDLE_VALUE) {
			m_ok = SetLock(true);
		}
	}

	/** Releases the lock, if any is held. */
	~CFileLock() {
		if (m_ok) {
			SetLock(false);
		}

		if (hd != INVALID_HANDLE_VALUE) {
			CloseHandle(hd);
		}
	}

private:
	//! Not copyable.
	CFileLock(const CFileLock&);
	//! Not assignable.
	CFileLock& operator=(const CFileLock&);

	/** Locks or unlocks the lock-file, returning true on success. */
	bool SetLock(bool doLock) {
		// lock/unlock first byte in the file
		OVERLAPPED ov;
		ov.Offset = ov.OffsetHigh = 0;
		BOOL ret;
		if (doLock) {
			// http://msdn.microsoft.com/en-us/library/ms891385.aspx
			ret = LockFileEx(hd, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &ov);
		} else {
			// http://msdn.microsoft.com/en-us/library/ms892364.aspx
			ret = UnlockFileEx(hd, 0, 1, 0, &ov);
		}
		return ret != 0;
	}


	//! Desriptor of the file being locked.
	HANDLE hd;

	//! Specifies if the file-lock was aquired.
	bool m_ok;
#else
		: m_fd(-1),
		  m_ok(false)
	{
		// File must be open with O_WRONLY to be able to set write-locks.
		m_fd = ::open((file + "_lock").c_str(), O_CREAT | O_WRONLY, 0600);
		if (m_fd != -1) {
			m_ok = SetLock(true);
		}
	}

	/** Releases the lock, if any is held. */
	~CFileLock() {
		if (m_ok) {
			SetLock(false);
		}

		if (m_fd != -1) {
			close(m_fd);
		}
	}

private:
	//! Not copyable.
	CFileLock(const CFileLock&);
	//! Not assignable.
	CFileLock& operator=(const CFileLock&);

	/** Locks or unlocks the lock-file, returning true on success. */
	bool SetLock(bool doLock) {
		struct flock lock;
		lock.l_type = (doLock ? F_WRLCK : F_UNLCK);

		// Lock the entire file
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 0;

		// Keep trying if interrupted by a signal.
		while (true) {
			if (fcntl(m_fd, F_SETLKW, &lock) == 0) {
				return true;
			} else if ((errno != EACCES) && (errno != EAGAIN) && (errno != EINTR)) {
				// Not an error we can recover from.
				break;
			}
		}

		return false;
	}


	//! Desribtor of the file being locked.
	int m_fd;

	//! Specifies if the file-lock was aquired.
	bool m_ok;
#endif
};

#endif
// File_checked_for_headers

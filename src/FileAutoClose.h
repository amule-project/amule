//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2011 Stu Redman ( sturedman@amule.org )
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

#ifndef FILEAUTOCLOSE_H
#define FILEAUTOCLOSE_H

#include "CFile.h"			// Needed for CFile

/**
 * This class encapsulates the CFile class.
 *
 * It allows to close the used file handle and reopen
 * it on usage to minimize the number of used file handles. 
 *
 */
class CFileAutoClose
{
public:
	
	/**
	 * Creates a closed file.
	 */
	CFileAutoClose();
	
	/**
	 * Constructor, calls Open on the specified file.
	 *
	 * To check if the file was successfully opened, a
	 * call to IsOpened() is required.
	 */
	CFileAutoClose(const CPath& path, CFile::OpenMode mode = CFile::read);

	/**
	 * Request auto closing of the file handle.
	 *
	 * @param now true: close immediately  false: close when timeout has expired
	 * @return True if the file has been (or has already been) autoclosed.
	 */
	bool Release(bool now = false);

	/**
	 * Opens a file.
	 *
	 * @param path The full or relative path to the file.
	 * @param mode The opening mode (see CFile).
	 * @return True if the file was opened, false otherwise.
	 */
	bool Open(const CPath& path, CFile::OpenMode mode = CFile::read);
	
	/**
	 * Calling Create is equivilant of calling open with OpenMode 'write'.
	 *
	 * @param overwrite Specifies if the target file should be overwritten,
	 *                  in case that it already exists.
	 *
	 * @see CFile::Open
	 */
	bool Create(const CPath& path, bool overwrite = false);
	
	/**
	 * Closes the file.
	 *
	 * Note that calling Close on an closed file
	 * is an illegal operation.
	 */
	bool Close();

	/**
	 * @see CSafeFileIO::GetLength
	 *
	 * Note that calling GetLength on a closed file 
	 * is an illegal operation.
	 */
	uint64 GetLength() const;
	
	/**
	 * Resizes the file to the specified length.
	 *
	 */
	bool SetLength(uint64 newLength);
	
	/**
	 * Returns the path of the currently opened file.
	 * 
	 */
	const CPath& GetFilePath() const;

	/**
	 * Returns true if the file is opened, false otherwise.
	 */
	bool IsOpened() const;
	
	/**
	 * Reads 'count' bytes into 'buffer'.
	 *
	 * @param buffer The target buffer.
	 * @param offset The seek address in the file.
	 * @param count The number of bytes to read.
	 *
	 * See CFileDataIO::Read
	 */
	void ReadAt(void* buffer, uint64 offset, size_t count);

	/**
	 * Write 'count' bytes from 'buffer' into the file.
	 *
	 * @param buffer The source-data buffer.
	 * @param offset The seek address in the file.
	 * @param count The number of bytes to write.
	 *
	 * See CFileDataIO::Write
	 */
	void WriteAt(const void* buffer, uint64 offset, size_t count);

	/**
	 * Returns true when the file-position is past or at the end of the file.
	 */
	bool Eof();

	/**
	 * Returns the file descriptior assosiated with the file.
	 *
	 * This breaks the purpose of this class of course. 
	 * Therefore the AutoClose mechanism is disabled when fd() is called.
	 * It's required for FileArea's mmap stuff.
	 * Currently FileArea objects are shortlived enough for this not being 
	 * a problem anyway, but that might change in the future.
	 */
	int fd();

	/**
	 * Reenables AutoClose disabled by fd() before.
	 */
	void Unlock();

private:
	//! A CFileAutoClose is neither copyable nor assignable.
	//@{
	CFileAutoClose(const CFileAutoClose&);
	CFileAutoClose& operator=(const CFileAutoClose&);
	//@}
	
	/**
	 * Check if file was autoclosed, and reopen if needed.
	 */
	void Reopen();

	//! The wrapped CFile.
	CFile m_file;
	
	//! The mode used to open it.
	CFile::OpenMode m_mode;

	//! Is it temporarily closed?
	bool m_autoClosed;

	//! Autoclosing is disabled if != 0 
	uint16 m_locked;

	//! Size before it was closed.
	uint64 m_size;

	//! Last access time (s)
	uint32 m_lastAccess;
};


#endif // FILEAUTOCLOSE_H
// File_checked_for_headers

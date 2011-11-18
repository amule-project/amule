//
// This file is part of the aMule Project.
//
// Copyright (c) 2009-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2009-2011 Frediano Ziglio (freddy77@gamilc.com)
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

#ifndef FILEAREA_H
#define FILEAREA_H

#include "Types.h"		// Needed for byte

class CFileAreaSigHandler;
class CFileAutoClose;

/**
 * This class is used to optimize file read/write using mapped memory
 * if supported.
 */
class CFileArea
{
friend class CFileAreaSigHandler;
public:
	/**
	 * Creates a uninitialized file area.
	 */
	CFileArea();

	
	/**
	 * Destructor, closes the file if opened.
	 */
	virtual ~CFileArea();

	/**
	 * Closes the file.
	 */
        bool Close();

	/**
	 * Init area with a given piece of file.
	 *
	 * @param file   file to read.
	 * @param offset seek address in file.
	 * @param count  bytes to read.
	 *
	 * Initialize buffer. Buffer will contain data from current file
	 * position for count length. Buffer will be a memory mapped area
	 * or a allocated buffer depending on systems.
	 */
	void ReadAt(CFileAutoClose& file, uint64 offset, size_t count);

	/**
	 * Start a new write
	 */
	void StartWriteAt(CFileAutoClose& file, uint64 offset, size_t count);

        /**
	 * Flushes data not yet written.
	 */
	bool FlushAt(CFileAutoClose& file, uint64 offset, size_t count);

	/**
	 * Get buffer that contains data readed or to write.
	 * @return allocated buffer or NULL if not initialized
	 */
	byte *GetBuffer() const { return m_buffer; };

	/**
	 * Report error pending
	 */
	void CheckError();

private:
	//! A CFileArea is neither copyable nor assignable.
	//@{
	CFileArea(const CFileArea&);
	CFileArea& operator=(const CFileArea&);
	//@}

	/**
	 * Pointer to buffer used for read/write operations.
	 * If mapped points inside m_mmap_buffer area otherwise
	 * point to an allocated buffer to be freed.
	 */
	byte *m_buffer;
	/**
	 * Pointer to memory mapped area or NULL if not mapped.
	 */
	byte *m_mmap_buffer;
	/**
	 * Length of the mapped region, currently used only for munmap.
	 */
	size_t m_length;
	/**
	 * Global chain
	 */
	CFileArea* m_next;
	/**
	 * File handle to release
	 */
	CFileAutoClose * m_file;
	/**
	 * true if error detected
	 */
	bool m_error;
};

#endif // FILEAREA_H
// File_checked_for_headers

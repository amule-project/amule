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

#ifndef MEMFILE_H
#define MEMFILE_H

#include "Types.h"		// Needed for uint8, uint16, uint32
#include "SafeFile.h"	// Needed for CFileDataIO


/**
 * CMemFile handles virtual files stored in memory.
 * 
 * This class allows for manipulation of binary data in memory such
 * as data sent over networks. Using this class rather than writing
 * the stream onto a struct confers the following advantages:
 *  - Contents may be read dynamically in case of various versions
 *    of the same packet.
 *  - Endian correction is handled transparently. When reading and
 *    writing values, CMemFile converts to and from little-endian,
 *    so that no explicit endian convertions are nescesarry.
 *  - Strings of dynamic length can be read.
 *
 * Most of these advantages also hold for writing packets.
 * 
 * @see CFileDataIO
 */
class CMemFile : public CFileDataIO
{
public:
	/**
	 * Creates a dynamic file object.
	 * 
	 * @param growthRate The growth-rate of the buffer.
	 *
	 * The growth-rate specified by how much the buffer-size will
	 * be increased when the memfile runs out of space. Normally
	 * this means that the amount of re-allocations is cut down
	 * at the expence of slightly higher mem-usage.
	 * 
	 * If the size of the entire file to be written is known
	 * in advance, one can avoid needless re-allocations by
	 * specifying the exact length as the growth-rate.
	 *
	 * If the growth-rate is set to zero, the memfile will allocate
	 * exactly the needed amount of memory and no more when resizing.
	 */
	CMemFile(unsigned int growthRate = 1024);

	/**
	 * Creates a mem-file attached to an already existing buffer.
	 *
	 * @param buffer A pre-existing buffer.
	 * @param bufferSize The size of the buffer.
	 *
	 * A buffer attached to a memfile is assumed to already contain
	 * data and therefore the file-size is set to match the size of
	 * of the buffer.
	 *
	 * Note that while it is valid to resize the buffer to a length
	 * between zero and 'bufferSize', it is not valid to resize it
	 * to a length greater than the length specified in the 
	 * constructor. This also holds for writes that would increase
	 * the length.
	 *
	 * The buffer is _not_ freed by CMemFile upon destruction.
	 */
	CMemFile(byte* buffer, size_t bufferSize);

	/** Destructor. */
	virtual ~CMemFile();
	

	/** @see CFileDataIO::GetPosition */
	virtual uint64 GetPosition() const;
	
	/** @see CFileDataIO::GetLength */
	virtual uint64 GetLength() const;

	
	/** 
	 * Changes the length of the file, possibly resizing the buffer.
	 *
	 * @param newLen The new length of the file.
	 * 
	 * If the current position is greater than the new length, it 
	 * will be set to the end of the file.
	 * 
	 * Note that changing the lenght of a file with an attached buffer 
	 * to a value greater than the actual buffer size is an illegal 
	 * operation.
	 */
	virtual void SetLength(size_t newLen);
	
protected:
	/** @see CFileDataIO::doRead */
	virtual sint64 doRead(void* buffer, size_t count) const;

	/** @see CFileDataIO::doWrite */
	virtual sint64 doWrite(const void* buffer, size_t count);
	
	/** @see CFileDataIO::doSeek */
	virtual sint64 doSeek(sint64 offset) const;
	
private:
	//! A CMemFile is neither copyable nor assignable.
	//@{
	CMemFile(const CMemFile&);
	CMemFile& operator=(const CMemFile&);
	//@}

	/** Enlarges the buffer to at least 'size' length. */
	void enlargeBuffer(size_t size);
	
	//! The growth-rate for the buffer.
	unsigned int m_growthRate;
	//! The current position in the file. 
	mutable size_t m_position;
	//! The actual size of the buffer.
	size_t	m_BufferSize;
	//! The size of the virtual file, may be less than the buffer-size.
	size_t	m_fileSize;
	//! If true, the buffer will be freed upon termination.
	bool	m_delete;
	//! The actual buffer.
	byte*	m_buffer;
};

#endif // MEMFILE_H

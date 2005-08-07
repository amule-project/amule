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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "MemFile.h"
#endif

#include "Types.h"		// Needed for uint8, uint16, uint32
#include "SafeFile.h"	// Needed for CFileDataIO


class CMemFile : public CFileDataIO
{
public:
	CMemFile( unsigned int growBytes = 1024 );
	CMemFile(byte* buffer, unsigned int bufferSize);
	virtual ~CMemFile();

	
	virtual off_t GetPosition() const 		{ return m_position; };
	virtual off_t Seek(off_t offset, wxSeekMode from = wxFromStart) const;
	virtual bool Eof() const;
	virtual bool SetLength(off_t newLen);
	virtual off_t GetLength() const { return m_FileSize; };
	
protected:
	virtual off_t  doRead(void* buf, off_t length) const;
	virtual size_t doWrite(const void* buf, size_t length);

private:
	//! A CMemFile is neither copyable nor assignable.
	//@{
	CMemFile(const CMemFile&);
	CMemFile& operator=(const CMemFile&);
	//@}
	
	void enlargeBuffer(unsigned long size);
	
	unsigned int m_GrowBytes;
	mutable off_t m_position;
	off_t	m_BufferSize;
	off_t	m_FileSize;
	bool	m_delete;
	byte*	m_buffer;
};

#endif // MEMFILE_H

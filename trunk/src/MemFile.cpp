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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "MemFile.h"
#endif

#include "MemFile.h"		// Needed for CMemFile


CMemFile::CMemFile(unsigned int growBytes)
{
	m_buffer		= NULL;
	m_BufferSize	= 0;
	m_FileSize		= 0;
	m_GrowBytes		= growBytes;
	m_position		= 0;
	m_delete		= true;
}


CMemFile::CMemFile(byte* buffer, off_t bufferSize)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempted to resize to attach to invalid buffer."));
	MULE_VALIDATE_STATE(bufferSize >= 0, wxT("CMemFile: Attempted to attach to buffer with invalid size."));
	
	m_buffer		= buffer;
	m_BufferSize	= bufferSize;
	m_FileSize		= bufferSize;
	m_GrowBytes		= 0;
	m_position		= 0;
	m_delete		= false;
}


CMemFile::~CMemFile()
{
	if (m_delete) {
		free(m_buffer);
	}
}


off_t CMemFile::GetPosition() const
{
	return m_position;
}


bool CMemFile::Eof() const
{
	return (m_position >= m_FileSize);

}


void CMemFile::SetLength(off_t newLen)
{
	MULE_VALIDATE_STATE(newLen >= 0, wxT("CMemFile: Attempted to resize to invalid length."));

	if (newLen > m_BufferSize) {
		enlargeBuffer(newLen);
	}
	
	if (newLen < m_position) {
		m_position = newLen;
	}
	
	m_FileSize = newLen;
}


off_t CMemFile::GetLength() const
{
	return m_FileSize;
}


void CMemFile::enlargeBuffer(off_t size)
{
	MULE_VALIDATE_PARAMS(size >= m_BufferSize, wxT("CMemFile: Attempted to shrink buffer."));
	MULE_VALIDATE_STATE(m_delete, wxT("CMemFile: Attempted to grow an attached buffer."));

	off_t newsize = m_BufferSize;
	if (m_GrowBytes) {
		while (newsize < size) {
			newsize += m_GrowBytes;
		}
	} else {
		// No growth-rate specified. Change to exactly the size specified.
		newsize = size;
	}

	m_buffer = (byte*)realloc(m_buffer, newsize);
	m_BufferSize = newsize;

	MULE_VALIDATE_STATE(m_buffer, wxT("CMemFile: Failed to (re)allocate buffer"));
}


off_t CMemFile::doRead(void* buffer, size_t count) const
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempting to read to invalid buffer"));
	MULE_VALIDATE_PARAMS(m_position + count <= m_FileSize, wxT("Invalid read"));
	
	memcpy(buffer, m_buffer + m_position, count);
	m_position += count;

	return count;
}


size_t CMemFile::doWrite(const void* buffer, size_t count)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempting to write from invalid buffer"));

	// Needs more space?
	if ((off_t)(m_position + count) > m_BufferSize) {
		enlargeBuffer(m_position + count);
	}
	
	MULE_VALIDATE_STATE(m_position + count <= m_BufferSize, wxT("CMemFile: Buffer not resized to needed size."));
	
	memcpy(m_buffer + m_position, buffer, count);
	m_position += count;

	if (m_position > m_FileSize) {
		m_FileSize = m_position;
	}

	return count;
}


off_t CMemFile::doSeek(off_t offset) const
{
	MULE_VALIDATE_PARAMS(offset >= 0, wxT("Invalid position, must be positive."));
	
	return m_position = offset;
}


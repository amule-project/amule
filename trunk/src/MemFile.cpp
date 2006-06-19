//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "MemFile.h"	// Interface declarations


CMemFile::CMemFile(unsigned int growthRate)
{
	m_buffer		= NULL;
	m_BufferSize	= 0;
	m_fileSize		= 0;
	m_growthRate	= growthRate;
	m_position		= 0;
	m_delete		= true;
	m_readonly		= false;
}


CMemFile::CMemFile(byte* buffer, size_t bufferSize)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempted to resize to attach to invalid buffer."));
	
	m_buffer		= buffer;
	m_BufferSize	= bufferSize;
	m_fileSize		= bufferSize;
	m_growthRate	= 0;
	m_position		= 0;
	m_delete		= false;
	m_readonly		= false;
}

CMemFile::CMemFile(const byte* buffer, size_t bufferSize)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempted to resize to attach to invalid buffer."));
	
	m_buffer		= const_cast<byte*>(buffer);
	m_BufferSize	= bufferSize;
	m_fileSize		= bufferSize;
	m_growthRate	= 0;
	m_position		= 0;
	m_delete		= false;
	m_readonly		= true;
}

CMemFile::~CMemFile()
{
	if (m_delete) {
		free(m_buffer);
	}
}


uint64 CMemFile::GetPosition() const
{
	return m_position;
}


void CMemFile::SetLength(size_t newLen)
{
	MULE_VALIDATE_STATE(!m_readonly, wxT("CMemFile: Attempted to change lenght on a read-only buffer."));
	
	if (newLen > m_BufferSize) {
		enlargeBuffer(newLen);
	}
	
	if (newLen < m_position) {
		m_position = newLen;
	}
	
	m_fileSize = newLen;
}


uint64 CMemFile::GetLength() const
{
	return m_fileSize;
}


void CMemFile::enlargeBuffer(size_t size)
{
	MULE_VALIDATE_PARAMS(size >= m_BufferSize, wxT("CMemFile: Attempted to shrink buffer."));
	MULE_VALIDATE_STATE(m_delete, wxT("CMemFile: Attempted to grow an attached buffer."));
	MULE_VALIDATE_STATE(!m_readonly, wxT("CMemFile: Attempted to grow a read-only buffer."));
	
	size_t newsize = m_BufferSize;
	
	if (m_growthRate) {
		newsize = ((size + m_growthRate - 1) / m_growthRate) * m_growthRate;
	} else {
		// No growth-rate specified. Change to exactly the size specified.
		newsize = size;
	}

	m_buffer = (byte*)realloc(m_buffer, newsize);
	m_BufferSize = newsize;

	MULE_VALIDATE_STATE(m_buffer, wxT("CMemFile: Failed to (re)allocate buffer"));
}


sint64 CMemFile::doRead(void* buffer, size_t count) const
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempting to read to invalid buffer"));
	
	// Handle reads past EOF
	if (m_position > m_fileSize) {
		return 0;
	} else if (m_position + count > m_fileSize) {
		count -= (m_position + count) - m_fileSize;
	}
	
	if (count) {
		memcpy(buffer, m_buffer + m_position, count);
		m_position += count;
	}

	return count;
}


sint64 CMemFile::doWrite(const void* buffer, size_t count)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("CMemFile: Attempting to write from invalid buffer"));
	MULE_VALIDATE_STATE(!m_readonly, wxT("CMemFile: Attempted to write to a read-only buffer."));
	
	// Needs more space?
	if (m_position + count > m_BufferSize) {
		enlargeBuffer(m_position + count);
	}
	
	MULE_VALIDATE_STATE(m_position + count <= m_BufferSize, wxT("CMemFile: Buffer not resized to needed size."));
	
	memcpy(m_buffer + m_position, buffer, count);
	m_position += count;

	if (m_position > m_fileSize) {
		m_fileSize = m_position;
	}

	return count;
}


sint64 CMemFile::doSeek(sint64 offset) const
{
	MULE_VALIDATE_PARAMS(offset >= 0, wxT("Invalid position, must be positive."));
	
	return m_position = offset;
}
// File_checked_for_headers

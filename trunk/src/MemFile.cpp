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
#include "Packet.h"			// Needed for CInvalidPacket

CMemFile::CMemFile(unsigned int growBytes)
{
	m_GrowBytes		= growBytes;
	m_position		= 0;
	m_BufferSize	= 0;
	m_FileSize		= 0;
	m_delete		= true;
	m_buffer		= NULL;
}


CMemFile::CMemFile(const byte *buffer, unsigned int bufferSize, unsigned int growBytes)
{
	m_buffer		= NULL;
	m_delete		= true;
	
	Attach(buffer, bufferSize, growBytes);
}


void CMemFile::Attach(const byte* buffer, unsigned int bufferSize, unsigned int growBytes)
{
	// Should we free the old buffer if one such exists
	if ( m_buffer && m_delete ) {
		free(m_buffer);
	}
	m_buffer =  (byte*)buffer;
	
	m_GrowBytes		= growBytes;
	m_position		= 0;
	m_BufferSize	= bufferSize;
	m_delete		= false;

	// According to the MSDN reference, attaching with a non-zero growBytes value
	// will mean that the current contents is to be ignored
	m_FileSize 	= ( growBytes ? 0 : bufferSize );
}


byte* CMemFile::Detach()
{
	byte *retval	= m_buffer;
	
	m_position		= 0;
	m_BufferSize	= 0;
	m_FileSize		= 0;
	m_buffer		= NULL;
	m_delete		= false;
	
	return retval;
}


CMemFile::~CMemFile()
{
	if ( m_buffer && m_delete ) {
		free(m_buffer);
	}
	
	m_buffer = NULL;
}


off_t CMemFile::Seek(off_t offset, wxSeekMode from)
{
	off_t newpos = 0;
	
	switch (from) {
		case wxFromStart:
			newpos = offset;
			break;
		case wxFromCurrent:
			newpos = m_position + offset;
			break;
		case wxFromEnd:
			newpos = m_FileSize - offset;
			break;
		default:
			MULE_VALIDATE_PARAMS(false, wxT("Using an invalid seek-mode in CMemFile::Seek!"));
	}
	
	MULE_VALIDATE_PARAMS(newpos < 0, wxT("Position after seeking in CMemFile is less than zero!"));

	// If the new position is greater than current filesize, then the 
	// file-size is increased to match the position
	if ( newpos > m_FileSize ) {
		SetLength( newpos );
	}

	m_position = newpos;

	return m_position;
}


bool CMemFile::Eof() const
{
	return ( m_position >= m_FileSize );

}


void CMemFile::enlargeBuffer(unsigned long size)
{
	off_t newsize = m_BufferSize;

	// Avoid infinate loops and ensure that we dont try to grow attached
	// buffers with growlength == 0
	if ( m_GrowBytes ) {
		// Everything is fine if m_GrowBytes is non-zero
		while ( newsize < (off_t)size )
			newsize += m_GrowBytes;
	} else {
		// Attempting to delete an attached buffer is an invalid operation,
		// as we could be attempting to free/alloc a heap allocated variable
		MULE_VALIDATE_STATE(m_delete, wxT("CMemFile: Attempted to grow an attached buffer."));
		
		// Non-attached. Change to exactly the size specified.
		newsize = size;
	}

	if ( m_buffer ) {
		m_buffer = (byte*)realloc((void*)m_buffer, newsize);
	} else {
		m_buffer = (byte*)malloc(newsize);
	}

	// Check for memory errors
	if ( m_buffer == NULL ) {
		printf("Unable to allocate memory in CMemFile!\n");
		exit(1);
	}

	m_BufferSize = newsize;
}


bool CMemFile::SetLength(off_t newLen)
{
	if ( newLen > m_BufferSize ) {
		enlargeBuffer(newLen);
	}
	
	if ( newLen < m_position ) {
		m_position = newLen;
	}
	
	m_FileSize = newLen;

	return true;
}


off_t CMemFile::Read(void* buf, off_t length) const
{
	if ( length == 0 )
		return 0;
	
	// Combined test, which will fail in most cases, so we can just figoure out
	// what really happened in case it triggers, which is cheaper than doing 2
	// tests every single time
	if ( length + m_position > m_FileSize ) {
		if ( m_position > m_FileSize ) {
			throw CEOFException(wxT("Position is greater than length in CMemFile"));
		} else {
			throw CEOFException(wxT("Attempted to read past end of CMemFile"));
		}
	}

	memcpy(buf, m_buffer + m_position, length);
	m_position += length;

	return length;
}


size_t CMemFile::Write(const void* buf, size_t length)
{
	wxASSERT(buf);
	
	if ( length == 0 || buf == NULL)
		return 0;

	// Needs more space?
	if ((off_t)(m_position + length) > m_BufferSize) {
		enlargeBuffer(m_position + length);
	}
	
	memcpy(m_buffer + m_position, buf, length);
	m_position += length;

	if ( m_position > m_FileSize )
		m_FileSize = m_position;

	return length;
}



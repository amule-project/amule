// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef CMEMFILE_H
#define CMEMFILE_H


#include "CFile.h"		// Needed for CFile
#include "types.h"		// Needed for uint8, uint16, uint32

class CMemFile : public CFile {
public:  
	
	CMemFile( unsigned int growBytes = 1024 );
	CMemFile( BYTE* buffer, unsigned int bufferSize, unsigned int growBytes = 0 );
	virtual ~CMemFile();


	void Attach(BYTE* buffer, unsigned int buffserSize, unsigned int growBytes = 0 );
	BYTE* Detach();	
	
	virtual off_t GetPosition() const 		{ return m_position; };
	virtual bool GetStatus(unsigned long) const 	{ return true; };
	virtual off_t Seek(off_t offset, wxSeekMode from = wxFromStart);
	virtual bool Eof() const;
	virtual bool SetLength(off_t newLen);
	virtual off_t Length() const { return m_FileSize; };
	
	virtual off_t  Read(void* buf, off_t length) const;
	virtual size_t Write(const void* buf, size_t length);
	
	
	// These functions should not be used as they make no sense for a MemFile.
	// However using them has no effect and is safe, though not advisable.
	virtual bool Create(const wxChar *szFileName, bool bOverwrite = FALSE, int access = wxS_DEFAULT);
	virtual bool Open(const wxChar *szFileName, OpenMode mode = read, int access = wxS_DEFAULT);
	virtual bool Close() const;
	virtual bool Flush();
	virtual bool IsOpened() const;
	virtual bool Error() const;
	
protected:
	void enlargeBuffer(unsigned long size);
	
	unsigned int m_GrowBytes;
	mutable off_t m_position;
	off_t	m_BufferSize;
	off_t	m_FileSize;
	bool	m_delete;
	BYTE*	m_buffer;
};

#endif // CMEMFILE_H

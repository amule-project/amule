//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef SAFEFILE_H
#define SAFEFILE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "SafeFile.h"
#endif

#include "CFile.h"		// Needed for CFile
#include "Types.h"		// Needed for LPCSTR
#include "MemFile.h"		// Needed for CMemFile
#include "StringFunctions.h"		// Needed for the utf8 types.

namespace Kademlia{
	class CUInt128;
};

///////////////////////////////////////////////////////////////////////////////
class CFileDataIO
{
public:
	virtual off_t Read(void *pBuf, off_t nCount) const = 0;
	virtual size_t Write(const void *pBuf, size_t nCount) = 0;
	virtual off_t GetPosition() const = 0;
	virtual off_t GetLength() const = 0;
 
	virtual uint8		ReadUInt8() const;
	virtual uint16		ReadUInt16() const;
	virtual uint32		ReadUInt32() const;
	virtual void		ReadUInt128(Kademlia::CUInt128 *pVal) const;
	virtual void		ReadHash16(unsigned char* pVal) const;
 	virtual wxString ReadString(bool bOptUTF8, uint8 SizeLen = 2 /* bytes */, bool SafeRead = false) const;
	virtual wxString ReadOnlyString(bool bOptUTF8, uint16 raw_len) const;

	virtual void WriteUInt8(uint8 nVal);
	virtual void WriteUInt16(uint16 nVal);
	virtual void WriteUInt32(uint32 nVal);
	virtual void WriteUInt128(const Kademlia::CUInt128 *pVal);
	virtual void WriteHash16(const unsigned char* pVal);
	virtual void WriteString(const wxString& rstr, EUtf8Str eEncode = utf8strNone, uint8 SizeLen = 2 /* bytes */);
private:
	void CFileDataIO::WriteStringCore(const char *s, EUtf8Str eEncode, uint8 SizeLen);
 };
 


///////////////////////////////////////////////////////////////////////////////
class CSafeFile : public CFile, public CFileDataIO
{
 public:
	CSafeFile() {}
	CSafeFile(const wxChar* lpszFileName, OpenMode mode = read)
		: CFile(lpszFileName, mode) {}

	virtual off_t Read(void *pBuf, off_t nCount) const;
	virtual size_t Write(const void *pBuf, size_t nCount);
 	virtual off_t GetPosition() const {
		return CFile::GetPosition();
	}
	virtual off_t GetLength() const {
		return CFile::GetLength();
	}
	virtual off_t Seek(off_t lOff, CFile::SeekMode nFrom = CFile::start) const {
		return CFile::Seek(lOff, nFrom);
	}
};
 


///////////////////////////////////////////////////////////////////////////////
class CSafeMemFile : public CMemFile, public CFileDataIO
{
public:
	CSafeMemFile(UINT nGrowBytes = 512)
		: CMemFile(nGrowBytes) {}
	CSafeMemFile(BYTE* lpBuffer, UINT nBufferSize, UINT nGrowBytes = 0)
		: CMemFile(lpBuffer, nBufferSize, nGrowBytes) {}

	// CMemFile already does the needed checks
	virtual off_t Read(void *pBuf, off_t nCount) const {
		return CMemFile::Read( pBuf, nCount );
	}
	
	virtual size_t Write(const void *pBuf, size_t nCount) {
		return CMemFile::Write( pBuf, nCount );
	}

	virtual off_t GetPosition() const {
		return CMemFile::GetPosition();
	}
	virtual off_t GetLength() const {
		return CMemFile::GetLength();
	}
	virtual uint8		ReadUInt8() const;
	virtual uint16		ReadUInt16() const;
	virtual uint32		ReadUInt32() const;
	virtual void		ReadUInt128(Kademlia::CUInt128 *pVal) const;
	virtual void		ReadHash16(unsigned char* pVal) const;
	
	// We override the default buffer-growth behavior in these functions
	virtual void WriteUInt8(uint8 nVal);
	virtual void WriteUInt16(uint16 nVal);
	virtual void WriteUInt32(uint32 nVal);
	virtual void WriteUInt128(const Kademlia::CUInt128 *pVal);
	virtual void WriteHash16(const unsigned char* pVal);

};



///////////////////////////////////////////////////////////////////////////////
// This is just a workaround
class CSafeBufferedFile : public CFile, public CFileDataIO
{
 public:
	CSafeBufferedFile() {}
	CSafeBufferedFile(const wxChar* lpszFileName, OpenMode mode = read)
		: CFile(lpszFileName, mode) {}

	virtual off_t Read(void *pBuf, off_t nCount) const;
	virtual size_t Write(const void *pBuf, size_t nCount);
	virtual off_t Seek(off_t lOff, CFile::SeekMode nFrom = CFile::start) {
		return CFile::Seek(lOff, nFrom);
	}
	virtual off_t GetPosition() const {
		return CFile::GetPosition();
	}
	virtual off_t GetLength() const {
		return CFile::GetLength();
	}
};
 

#endif // SAFEFILE_H

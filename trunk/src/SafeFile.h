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

#ifndef SAFEFILE_H
#define SAFEFILE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "SafeFile.h"
#endif

#include "CFile.h"		// Needed for CFile
#include "Types.h"		// Needed for LPCSTR
#include "StringFunctions.h"		// Needed for the utf8 types.

namespace Kademlia{
	class CUInt128;
}

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
	virtual void WriteUInt128(const Kademlia::CUInt128& pVal);
	virtual void WriteHash16(const unsigned char* pVal);
	virtual void WriteString(const wxString& rstr, EUtf8Str eEncode = utf8strNone, uint8 SizeLen = 2 /* bytes */);
protected:
	virtual ~CFileDataIO() {};
private:
	void WriteStringCore(const char *s, EUtf8Str eEncode, uint8 SizeLen);
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
	virtual off_t Seek(off_t lOff, wxSeekMode nFrom = wxFromStart) const {
		return CFile::Seek(lOff, nFrom);
	}
};
 

#endif // SAFEFILE_H

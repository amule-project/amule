// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "SafeFile.h"		// Interface declarations.
#include "otherfunctions.h"
#include "packets.h"

///////////////////////////////////////////////////////////////////////////////
// CFileDataIO

uint8 CFileDataIO::ReadUInt8() const
{
	uint8 nVal = 0;
	Read(&nVal, sizeof(nVal));

	return nVal;
}


uint16 CFileDataIO::ReadUInt16() const
{
	uint16 nVal = 0;
	Read(&nVal, sizeof(nVal));

	return ENDIAN_SWAP_16(nVal);
}


uint32 CFileDataIO::ReadUInt32() const
{
	uint32 nVal = 0;
	Read(&nVal, sizeof(nVal));

	return ENDIAN_SWAP_32(nVal);
}


/*
void CFileDataIO::ReadUInt128(Kademlia::CUInt128 *pVal) const
{
	Read(pVal->getDataPtr(), 16);
}
*/


void CFileDataIO::ReadHash16(uchar* pVal) const
{
	Read(pVal, 16);
}


wxString CFileDataIO::ReadString() const
{
	uint16 length = ReadUInt16();

	char* val = NULL;
	try {
		val = new char[length + 1];
		// We only need to set the the NULL terminator, since we know that
		// reads will either succeed or throw an exception, in which case
		// we wont be returning anything
		val[length] = 0;
		
		Read(val, length);

		wxString str = char2unicode(val);
		delete[] val;

		return str;
	} catch ( ... ) {
		// Have to avoid mem-leaks
		delete[] val;
		
		// Re-throw
		throw;
	}
}


void CFileDataIO::WriteUInt8(uint8 nVal)
{
	Write(&nVal, sizeof nVal);
}

void CFileDataIO::WriteUInt16(uint16 nVal)
{
	ENDIAN_SWAP_I_16(nVal);
	
	Write(&nVal, sizeof nVal);
}

void CFileDataIO::WriteUInt32(uint32 nVal)
{
	ENDIAN_SWAP_I_32(nVal);

	Write(&nVal, sizeof nVal);
}

/*
void CFileDataIO::WriteUInt128(const Kademlia::CUInt128 *pVal)
{
	Write(pVal->getData(), 16);
}
*/

void CFileDataIO::WriteHash16(const uchar* pVal)
{
	Write(pVal, 16);
}


void CFileDataIO::WriteString(const wxString& rstr)
{
	//
	// We dont include the NULL terminator. Dont know why.
	// It is because we write the size, so the NULL is not necessary.
	// 
	const char *s = unicode2char(rstr);
	//
	// This avoids a crash in case unicode2char cannot perform the conversion,
	// e.g., original string is an unicode string that cannot be converted to
	// the current character set, in which case it will return NULL. Returning
	// a NULL should not happen if UTF-8 was beeing used.
	// 
	unsigned int sLength = s ? strlen(s) : 0;
	//
	// Write the size of the string
	//
	WriteUInt16(sLength);
	//
	// If this is a NULL string, there is nothing to write, only the size.
	// 
	if (sLength) {
		Write(s, sLength);
	}
}


///////////////////////////////////////////////////////////////////////////////
// CSafeFile

off_t CSafeFile::Read(void *pBuf, off_t nCount) const
{
	if ( GetPosition() + nCount > GetLength() )
		// For lack of better
		throw wxString::Format(wxT("Read after end of CSafeFile: %s\n"), GetFilePath().c_str());
		// AfxThrowFileException(CFileException::endOfFile, 0, GetFileName());
	
	return CFile::Read( pBuf, nCount );
}

size_t CSafeFile::Write(const void *pBuf, size_t nCount)
{
	return CFile::Write( pBuf, nCount );
}

///////////////////////////////////////////////////////////////////////////////
// CSafeBufferedFile

off_t CSafeBufferedFile::Read(void *pBuf, off_t nCount) const
{
	if ( GetPosition() + nCount > GetLength() )
		throw CInvalidPacket("Reading past end of CSafeBufferedFile!");
//		AfxThrowFileException(CFileException::endOfFile, 0, GetFileName());

	return CFile::Read( pBuf, nCount );
}

size_t CSafeBufferedFile::Write(const void *pBuf, size_t nCount)
{
	return CFile::Write( pBuf, nCount );
}


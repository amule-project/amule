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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "SafeFile.h"
#endif

#include "SafeFile.h"		// Interface declarations.
#include "otherfunctions.h"
#include "StringFunctions.h"		// Needed for unicode2char

#include "packets.h"
#include "kademlia/utils/UInt128.h"

#define CHECK_BOM(size,x) ((size > 3)  && (x[0] == (char)0xEF) && (x[1] == (char)0xBB) && (x[2] == (char)0xBF))

const char BOMHeader[3] = {0xEF,0xBB,0xBF};

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



void CFileDataIO::ReadUInt128(Kademlia::CUInt128 *pVal) const
{
	Read(pVal->getDataPtr(), 16);
}


void CFileDataIO::ReadHash16(uchar* pVal) const
{
	Read(pVal, 16);
}


wxString CFileDataIO::ReadString(bool bOptUTF8) const
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
		wxString str;
		
		if (CHECK_BOM(length,val)) {
			// This is a UTF8 string with a BOM header, skip header.
			str = UTF82unicode(val+3);
		} else {
			if (bOptUTF8) {
				str = UTF82unicode(val);
				if (str.IsEmpty()) {
					// Fallback to system locale
					//printf("Failed UTF8 conversion (READ), going for current locale: %s\n",val);
					str = char2unicode(val);
				}					
			} else {
				str = char2unicode(val);
			}
		}
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


void CFileDataIO::WriteUInt128(const Kademlia::CUInt128 *pVal)
{
	Write(pVal->getData(), 16);
}


void CFileDataIO::WriteHash16(const uchar* pVal)
{
	Write(pVal, 16);
}


void CFileDataIO::WriteString(const wxString& rstr,  EUtf8Str eEncode)
{
	//
	// We dont include the NULL terminator. Dont know why.
	// It is because we write the size, so the NULL is not necessary.
	// 
	// From wx docs: 
	// The macro wxWX2MBbuf reflects the correct return value of cWX2MB 
	// (either char* or wxCharBuffer), except for the const.
	
	if ((eEncode == utf8strRaw) || (eEncode == utf8strOptBOM)) {
		wxCharBuffer s = wxConvUTF8.cWC2MB(rstr.wc_str(aMuleConv));
		unsigned int sLength = s ? strlen(s) : 0;
		if (sLength == 0) {
			//wxASSERT(sLength);
			// Something failed on UTF8 enconding.
			//printf("Failed UTF8 conversion (WRITE), going for current locale: %s\n",unicode2char(rstr));			
			wxCharBuffer s2 = aMuleConv.cWX2MB(rstr);
			sLength = s2 ? strlen(s2) : 0;
			WriteUInt16(sLength);
			if (sLength) {
				Write(s2, sLength);
			}
		} else {
			if (eEncode == utf8strOptBOM) {
				WriteUInt16(sLength + 3 ); // For BOM header.
			} else {
				WriteUInt16(sLength);
			}
			if (sLength) {
				if (eEncode == utf8strOptBOM) {
					Write(BOMHeader,3);
				}
				Write(s, sLength);
			}
		}
	} else {
		wxCharBuffer s = aMuleConv.cWX2MB(rstr);
		unsigned int sLength = s ? strlen(s) : 0;
		WriteUInt16(sLength);
		if (sLength) {
			Write(s, sLength);
		}
	}

	//
	// This avoids a crash in case unicode2char cannot perform the conversion,
	// e.g., original string is an unicode string that cannot be converted to
	// the current character set, in which case it will return NULL. Returning
	// a NULL should not happen if UTF-8 was beeing used.
	// 
	// unsigned int sLength = s ? strlen(s) : 0;
	//
	// Write the size of the string
	//
	// WriteUInt16(sLength);
	//
	// If this is a NULL string, there is nothing to write, only the size.
	// 
	// if (sLength) {
	//	Write(s, sLength);
	// }
}


///////////////////////////////////////////////////////////////////////////////
// CSafeFile

off_t CSafeFile::Read(void *pBuf, off_t nCount) const
{
	if ( GetPosition() + nCount > GetLength() )
		// For lack of better
		throw CInvalidPacket("Read after end of CSafeFile");
		// AfxThrowFileException(CFileException::endOfFile, 0, GetFileName());
	
	return CFile::Read( pBuf, nCount );
}

size_t CSafeFile::Write(const void *pBuf, size_t nCount)
{
	return CFile::Write( pBuf, nCount );
}

///////////////////////////////////////////////////////////////////////////////
// CSafeMemFile


void CSafeMemFile::ReadUInt128(Kademlia::CUInt128* pVal) const
{
	if (m_position + sizeof(uint32)*4 > m_BufferSize)
		throw CInvalidPacket("EOF");
	CFileDataIO::ReadUInt128(pVal);
}


void CSafeMemFile::WriteUInt8(uint8 nVal)
{
	if (m_position + sizeof(uint8) > m_BufferSize)
		enlargeBuffer(m_position + sizeof(uint8));
	CFileDataIO::WriteUInt8(nVal);
}

void CSafeMemFile::WriteUInt16(uint16 nVal)
{
	if (m_position + sizeof(uint16) > m_BufferSize)
		enlargeBuffer(m_position + sizeof(uint16));
	CFileDataIO::WriteUInt16(nVal);
}

void CSafeMemFile::WriteUInt32(uint32 nVal)
{
	if (m_position + sizeof(uint32) > m_BufferSize)
		enlargeBuffer(m_position + sizeof(uint32));
	CFileDataIO::WriteUInt32(nVal);
}


void CSafeMemFile::WriteUInt128(const Kademlia::CUInt128* pVal)
{
	if (m_position + sizeof(uint32)*4 > m_BufferSize)
		enlargeBuffer(m_position + sizeof(uint32)*4);
	CFileDataIO::WriteUInt128(pVal);
}


void CSafeMemFile::WriteHash16(const uchar* pVal)
{
	if (m_position + sizeof(uint32)*4 /* 16 bytes */> m_BufferSize)
		enlargeBuffer(m_position + sizeof(uint32)*4);
	CFileDataIO::WriteHash16(pVal);
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

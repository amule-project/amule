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
#pragma implementation "SafeFile.h"
#endif

#include "SafeFile.h"				// Interface declarations.
#include "CMD4Hash.h"				// Needed for CMD4Hash
#include "ArchSpecific.h"			// Needed for ENDIAN_SWAP_*
#include "StringFunctions.h"		// Needed for unicode2char, etc.
#include "kademlia/utils/UInt128.h"	// Needed for CUInt128


#define CHECK_BOM(size,x) ((size >= 3)  && (x[0] == (char)0xEF) && (x[1] == (char)0xBB) && (x[2] == (char)0xBF))

const char BOMHeader[3] = {0xEF,0xBB,0xBF};


CSafeIOException::CSafeIOException(const wxString& what)
	: CMuleException(wxT("CSafeIOException"), what) {}


CEOFException::CEOFException(const wxString& what)
	: CSafeIOException(what) {}


CIOFailureException::CIOFailureException(const wxString& what)
	: CSafeIOException(what) {}



///////////////////////////////////////////////////////////////////////////////
// CFileDataIO


CFileDataIO::~CFileDataIO()
{
}

	
void CFileDataIO::Read(void *pBuf, off_t nCount) const
{
	MULE_VALIDATE_PARAMS(pBuf || nCount == 0, wxT("Attempting to write to NULL buffer."));
	MULE_VALIDATE_PARAMS(nCount >= 0, wxT("Number of bytes to read must not be negative."));

	// Check for read past EOF
	if (GetLength() < GetPosition() + nCount) {
		throw CEOFException(wxT("Attempt to read past end of file."));
	}

	// Check that we read everything we wanted.
	if (doRead(pBuf, nCount) != nCount) {
		throw CIOFailureException(wxT("Read error, failed to read from file."));
	}
}


void CFileDataIO::Write(const void *pBuf, size_t nCount)
{
	MULE_VALIDATE_PARAMS(pBuf || nCount == 0, wxT("Attempting to read from NULL buffer."));

	if (doWrite(pBuf, nCount) != nCount) {
		throw CIOFailureException(wxT("Read error, failed to write to file."));
	}
}


off_t CFileDataIO::Seek(off_t offset, wxSeekMode from) const
{
	off_t newpos = 0;
	switch (from) {
		case wxFromStart:
			newpos = offset;
			break;
			
		case wxFromCurrent:
			newpos = GetPosition() + offset;
			break;
			
		case wxFromEnd:
			newpos = GetLength() + offset;
			break;
			
		default:
			MULE_VALIDATE_PARAMS(false, wxT("Invalid seek-mode specified."));
	}
	
	MULE_VALIDATE_PARAMS(newpos >= 0, wxT("Position after seeking would be negative"));

	off_t result = doSeek(newpos);
	MULE_VALIDATE_STATE(result >= 0, wxT("Seeking resulted in invalid offset."));
	MULE_VALIDATE_STATE(result == newpos, wxT("Target position and actual position disagree."));
	
	return result;
}


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

CUInt128 CFileDataIO::ReadUInt128() const
{
	CUInt128 value;
	uint32* data = (uint32*)value.getDataPtr();
	for (int i = 0; i < 4; i++) {
		data[i] = ReadUInt32();
	}

	return value;
}

CMD4Hash CFileDataIO::ReadHash() const
{
	unsigned char hash[16];
	Read(hash, 16);

	return CMD4Hash(hash);
}


wxString CFileDataIO::ReadOnlyString(bool bOptUTF8, uint16 raw_len) const
{
	// The name is just to confuse people
	
	// We only need to set the the NULL terminator, since we know that
	// reads will either succeed or throw an exception, in which case
	// we wont be returning anything
	char val[raw_len + 1];
	val[raw_len] = 0;
		
	Read(val, raw_len);
	wxString str;
	
	if (CHECK_BOM(raw_len,val)) {
		// This is a UTF8 string with a BOM header, skip header.
		str = UTF82unicode(val + 3);
	} else {
		if (bOptUTF8) {
			str = UTF82unicode(val);
			if (str.IsEmpty()) {
				// Fallback to system locale
				str = char2unicode(val);
			}					
		} else {
			str = char2unicode(val);
		}
	}

	return str;
}


wxString CFileDataIO::ReadString(bool bOptUTF8, uint8 SizeLen, bool SafeRead) const
{
	
	uint32 length;
	switch (SizeLen) {
		case 2:
			length = ReadUInt16();
			break;
		case 4:
			length = ReadUInt32();			
			break;
		default:
			// Not uint16 neither uint32. BAD THING.
			// Let's assume uint16 for not to crash.
			wxASSERT(0);
			printf("Unexpected string len size %d on ReadString! Report on forum.amule.org!\n", SizeLen);
			length = ReadUInt16();				
			break;					
	}	

	if (SafeRead) {
		if ( length > GetLength() - GetPosition() ) {
			length = GetLength() - GetPosition();
		}	
	}
	
	return ReadOnlyString(bOptUTF8, length);

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

void CFileDataIO::WriteUInt128(const Kademlia::CUInt128& pVal)
{
	for (int i = 0; i < 4; i++) {
		WriteUInt32(pVal.get32BitChunk(i));
	}
}

void CFileDataIO::WriteHash(const CMD4Hash& value)
{
	Write(value.GetHash(), 16);
}


void CFileDataIO::WriteStringCore(const char *s, EUtf8Str eEncode, uint8 SizeLen)
{
	unsigned int sLength = s ? strlen(s) : 0;
	uint32 real_length = 0;
	if (eEncode == utf8strOptBOM) {
		real_length = sLength + 3; // For BOM header.
	} else {
		real_length = sLength;
	}			
	switch (SizeLen) {
		case 0:
			// don't write size :)
			break;
		case 2:
			wxASSERT(real_length < (uint16)0xFFFF); // Can't be higher than a uint16
			WriteUInt16(real_length);
			break;
		case 4:
			wxASSERT(real_length < (uint32)0xFFFFFFFF); // Can't be higher than a uint32
			WriteUInt32(real_length);
			break;
		default:
			// Not uint16 neither uint32. BAD THING.
			// Let's assume uint16 for not to crash.
			wxASSERT(0);
			printf(	"Unexpected string len size %d on WriteString!"
				" Report on forum.amule.org!\n", SizeLen);
			WriteUInt16(real_length);
			break;					
	}		
		
	if (eEncode == utf8strOptBOM) {
		Write(BOMHeader, 3);
	}

	// Only attempt to write non-NULL strings.
	if (sLength) {
		// No NULL terminator is written since we explicitly specify the length
		Write(s, sLength);
	}
}


void CFileDataIO::WriteString(const wxString& rstr, EUtf8Str eEncode, uint8 SizeLen)
{
	switch (eEncode) {
		case utf8strRaw:
		case utf8strOptBOM: {
			Unicode2CharBuf s(unicode2UTF8(rstr));
			if (s) {
				WriteStringCore(s, eEncode, SizeLen);
				break;
			}
		}
		default: {
			Unicode2CharBuf s1(unicode2char(rstr));
			WriteStringCore(s1, utf8strNone, SizeLen);			
		}
	}
}

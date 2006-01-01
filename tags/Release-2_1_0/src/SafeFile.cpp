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

#include "SafeFile.h"				// Interface declarations.
#include "MD4Hash.h"				// Needed for CMD4Hash
#include "ArchSpecific.h"			// Needed for ENDIAN_SWAP_*
#include <common/StringFunctions.h>		// Needed for unicode2char, etc.
#include "kademlia/utils/UInt128.h"	// Needed for CUInt128

#include <algorithm>		// Needed for std::min


#define CHECK_BOM(size, x) ((size >= 3)  && (x[0] == (char)0xEF) && (x[1] == (char)0xBB) && (x[2] == (char)0xBF))

const char BOMHeader[3] = {0xEF, 0xBB, 0xBF};


CSafeIOException::CSafeIOException(const wxString& type, const wxString& desc)
	: CMuleException(wxT("SafeIO::") + type, desc) {}


CEOFException::CEOFException(const wxString& desc)
	: CSafeIOException(wxT("EOF"), desc) {}


CIOFailureException::CIOFailureException(const wxString& desc)
	: CSafeIOException(wxT("IOFailure"), desc) {}

CIOFailureException::CIOFailureException(const wxString& type, const wxString& desc)
	: CSafeIOException(wxT("IOFailure::") + type, desc) {}
	

///////////////////////////////////////////////////////////////////////////////
// CFileDataIO


CFileDataIO::~CFileDataIO()
{
}


bool CFileDataIO::Eof() const
{
	return GetPosition() >= GetLength();
}


void CFileDataIO::Read(void *buffer, size_t count) const
{
	MULE_VALIDATE_PARAMS(buffer, wxT("Attempting to write to NULL buffer."));

	// Check that we read everything we wanted.
	if (doRead(buffer, count) == count) {
		return;
	}

	// To reduce potential system calls, we only do EOF checks when reads fail.
	if (Eof()) {
		throw CEOFException(wxT("Attempt to read past end of file."));
	} else {
		throw CIOFailureException(wxT("Read error, failed to read from file."));
	}
}


void CFileDataIO::Write(const void* buffer, size_t count)
{
	MULE_VALIDATE_PARAMS(buffer, wxT("Attempting to read from NULL buffer."));

	if (doWrite(buffer, count) != count) {
		throw CIOFailureException(wxT("Write error, failed to write to file."));
	}
}


uint64 CFileDataIO::Seek(sint64 offset, wxSeekMode from) const
{
	sint64 newpos = 0;
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
	
	MULE_VALIDATE_PARAMS(newpos >= 0, wxT("Position after seeking would be less than zero!"));

	sint64 result = doSeek(newpos);
	MULE_VALIDATE_STATE(result >= 0, wxT("Seeking resulted in invalid offset."));
	MULE_VALIDATE_STATE(result == newpos, wxT("Target position and actual position disagree."));
	
	return result;
}


uint8 CFileDataIO::ReadUInt8() const
{
	uint8 value = 0;
	Read(&value, 1);

	return value;
}


uint16 CFileDataIO::ReadUInt16() const
{
	uint16 value = 0;
	Read(&value, 2);
	
	return ENDIAN_SWAP_16(value);
}


uint32 CFileDataIO::ReadUInt32() const
{
	uint32 value = 0;
	Read(&value, 4);
	
	return ENDIAN_SWAP_32(value);
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
	CMD4Hash value;
	Read(value.GetHash(), 16);

	return value;
}


wxString CFileDataIO::ReadString(bool bOptUTF8, uint8 SizeLen, bool SafeRead) const
{
	uint32 readLen;
	switch (SizeLen) {
		case 2:	readLen = ReadUInt16();	break;
		case 4:	readLen = ReadUInt32();	break;
			
		default:
			MULE_VALIDATE_PARAMS(false, wxT("Invalid SizeLen value in ReadString"));
	}	

	if (SafeRead) {
		readLen = std::min<uint64>(readLen, GetLength() - GetPosition());
	}
	
	return ReadOnlyString(bOptUTF8, readLen);
}


wxString CFileDataIO::ReadOnlyString(bool bOptUTF8, uint16 raw_len) const
{
	// We only need to set the the NULL terminator, since we know that
	// reads will either succeed or throw an exception, in which case
	// we wont be returning anything
	char val[raw_len + 1];
	val[raw_len] = 0;
		
	Read(val, raw_len);
	wxString str;
	
	if (CHECK_BOM(raw_len, val)) {
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


void CFileDataIO::WriteUInt8(uint8 value)
{
	Write(&value, 1);
}


void CFileDataIO::WriteUInt16(uint16 value)
{
	ENDIAN_SWAP_I_16(value);
	
	Write(&value, 2);
}


void CFileDataIO::WriteUInt32(uint32 value)
{
	ENDIAN_SWAP_I_32(value);
	
	Write(&value, 4);
}


void CFileDataIO::WriteUInt128(const Kademlia::CUInt128& value)
{
	for (int i = 0; i < 4; i++) {
		WriteUInt32(value.get32BitChunk(i));
	}
}


void CFileDataIO::WriteHash(const CMD4Hash& value)
{
	Write(value.GetHash(), 16);
}


void CFileDataIO::WriteString(const wxString& str, EUtf8Str eEncode, uint8 SizeLen)
{
	switch (eEncode) {
		case utf8strRaw:
		case utf8strOptBOM: {
			Unicode2CharBuf s(unicode2UTF8(str));
			if (s) {
				WriteStringCore(s, eEncode, SizeLen);
				break;
			}
		}
		default: {
			Unicode2CharBuf s1(unicode2char(str));
			WriteStringCore(s1, utf8strNone, SizeLen);			
		}
	}
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
			// Don't write size.
			break;
			
		case 2:
			// Can't be higher than a uint16
			wxASSERT(real_length < (uint16)0xFFFF);
			WriteUInt16(real_length);
			break;
			
		case 4:
			WriteUInt32(real_length);
			break;
			
		default:
			MULE_VALIDATE_PARAMS(false, wxT("Invalid length for string-length field."));
	}		
		
	// The BOM header must be written even if the string is empty.
	if (eEncode == utf8strOptBOM) {
		Write(BOMHeader, 3);
	}

	// Only attempt to write non-NULL strings.
	if (sLength) {
		// No NULL terminator is written since we explicitly specify the length
		Write(s, sLength);
	}
}


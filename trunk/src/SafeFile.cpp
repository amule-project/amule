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
#include "ScopedPtr.h"				// Needed for CScopedPtr and CScopedArray

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
	if (doRead(buffer, count) == (signed)count) {
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

	if (doWrite(buffer, count) != (signed)count) {
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
	Read(&value, sizeof(uint8));

	return value;
}


uint16 CFileDataIO::ReadUInt16() const
{
	uint16 value = 0;
	Read(&value, sizeof(uint16));
	
	return ENDIAN_SWAP_16(value);
}


uint32 CFileDataIO::ReadUInt32() const
{
	uint32 value = 0;
	Read(&value, sizeof(uint32));
	
	return ENDIAN_SWAP_32(value);
}


uint64 CFileDataIO::ReadUInt64() const
{
	uint64 value = 0;
	Read(&value, sizeof(uint64));
	
	return ENDIAN_SWAP_64(value);
}


CUInt128 CFileDataIO::ReadUInt128() const
{
	CUInt128 value;
	uint32* data = (uint32*)value.GetDataPtr();
	for (int i = 0; i < (128/32); i++) {
		// Four 32bits chunks
		data[i] = ReadUInt32();
	}

	return value;
}


CMD4Hash CFileDataIO::ReadHash() const
{
	CMD4Hash value;
	Read(value.GetHash(), MD4HASH_LENGTH);

	return value;
}


float CFileDataIO::ReadFloat() const
{
	float retVal;
	Read(&retVal, sizeof(float));
	return retVal;
}


unsigned char* CFileDataIO::ReadBsob(uint8* puSize)
{
	MULE_VALIDATE_PARAMS(puSize, wxT("NULL pointer argument in ReadBsob"));

	*puSize = ReadUInt8();
	
	CScopedArray<unsigned char> bsob(new unsigned char[*puSize]);
	Read(bsob.get(), *puSize);
	
	return bsob.release();
}


wxString CFileDataIO::ReadString(bool bOptUTF8, uint8 SizeLen, bool SafeRead) const
{
	uint32 readLen;
	switch (SizeLen) {
		case sizeof(uint16):	readLen = ReadUInt16();	break;
		case sizeof(uint32):	readLen = ReadUInt32();	break;
			
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
	Write(&value, sizeof(uint8));
}


void CFileDataIO::WriteUInt16(uint16 value)
{
	ENDIAN_SWAP_I_16(value);
	
	Write(&value, sizeof(uint16));
}


void CFileDataIO::WriteUInt32(uint32 value)
{
	ENDIAN_SWAP_I_32(value);
	
	Write(&value, sizeof(uint32));
}


void CFileDataIO::WriteUInt64(uint64 value)
{
	ENDIAN_SWAP_I_64(value);
	
	Write(&value, sizeof(uint64));
}


void CFileDataIO::WriteUInt128(const Kademlia::CUInt128& value)
{
	for (int i = 0; i < (128/32); i++) {
		// Four 32bits chunks
		WriteUInt32(value.Get32BitChunk(i));
	}
}


void CFileDataIO::WriteHash(const CMD4Hash& value)
{
	Write(value.GetHash(), MD4HASH_LENGTH);
}


void CFileDataIO::WriteFloat(float value)
{
	Write(&value, sizeof(float));
}


void CFileDataIO::WriteBsob(const unsigned char* value, uint8 size)
{
	WriteUInt8(size);
	Write(value, size);
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
			
		case sizeof(uint16):
			// Can't be higher than a uint16
			wxASSERT(real_length < (uint16)0xFFFF);
			WriteUInt16(real_length);
			break;
			
		case sizeof(uint32):
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


CTag *CFileDataIO::ReadTag(bool bOptACP)
{
	CTag *retVal = NULL;
	wxString name;
	byte type = 0;
	try {
		type = ReadUInt8();
		name = ReadString(false);

		switch (type)
		{
			// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade 
			// the net in some months.
			//
			// And still.. it doesnt't work this way without breaking backward compatibility. To properly
			// do this without messing up the network the following would have to be done:
			//	 -	those tag types have to be ignored by any client, otherwise those tags would also be sent (and 
			//		that's really the problem)
			//
			//	 -	ignoring means, each client has to read and right throw away those tags, so those tags get
			//		get never stored in any tag list which might be sent by that client to some other client.
			//
			//	 -	all calling functions have to be changed to deal with the 'nr. of tags' attribute (which was 
			//		already parsed) correctly.. just ignoring those tags here is not enough, any taglists have to 
			//		be built with the knowledge that the 'nr. of tags' attribute may get decreased during the tag 
			//		reading..
			// 
			// If those new tags would just be stored and sent to remote clients, any malicious or just bugged
			// client could let send a lot of nodes "corrupted" packets...
			//
			case TAGTYPE_HASH:
			{
				retVal = new CTagHash(name, ReadHash());
				break;
			}

			case TAGTYPE_STRING:
				retVal = new CTagString(name, ReadString(bOptACP));
				break;

			case TAGTYPE_UINT64:
				retVal = new CTagInt64(name, ReadUInt64());
				break;

			case TAGTYPE_UINT32:
				retVal = new CTagInt32(name, ReadUInt32());
				break;

			case TAGTYPE_UINT16:
				retVal = new CTagInt16(name, ReadUInt16());
				break;

			case TAGTYPE_UINT8:
				retVal = new CTagInt8(name, ReadUInt8());
				break;

			case TAGTYPE_FLOAT32:
				retVal = new CTagFloat(name, ReadFloat());
				break;

			// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade 
			// the net in some months.
			//
			// And still.. it doesnt't work this way without breaking backward compatibility
			case TAGTYPE_BSOB:
			{
				uint8 size = 0;
				CScopedArray<unsigned char> value(ReadBsob(&size));
				
				retVal = new CTagBsob(name, value.get(), size);
			}

			default:
				throw wxString(wxT("Invalid Kad tag type on packet"));
		}
	} catch (...) {
		printf("Invalid Kad tag; type=0x%02x name=0x%02x\n",
			type, ((const char *)unicode2char(name))[0]);
		delete retVal;
		throw;
	}
	
	return retVal;
}


void CFileDataIO::ReadTagPtrList(TagPtrList* taglist, bool bOptACP)
{
	MULE_VALIDATE_PARAMS(taglist, wxT("NULL pointer argument in ReadTagPtrList"));

	uint32 count = ReadUInt8();
	for (uint32 i = 0; i < count; i++)
	{
		CTag* tag = ReadTag(bOptACP);
		taglist->push_back(tag);
	}
}


void CFileDataIO::WriteTag(const CTag& tag)
{
	try
	{
		WriteUInt8(tag.GetType());
		
		if (!tag.GetName().IsEmpty()) {
			WriteString(tag.GetName(),utf8strNone);
		} else {
			WriteUInt16(1);
			WriteUInt8(tag.GetNameID());
		}
		
		switch (tag.GetType())
		{
			case TAGTYPE_HASH:
				// Do NOT use this to transfer any tags for at least half a year!!
				WriteHash(CMD4Hash(tag.GetHash()));
				break;
			case TAGTYPE_STRING:
				WriteString(tag.GetStr(), utf8strRaw); // Always UTF8
				break;
			case TAGTYPE_UINT64:
				WriteUInt64(tag.GetInt());
				break;
			case TAGTYPE_UINT32:
				WriteUInt32(tag.GetInt());
				break;
			case TAGTYPE_FLOAT32:
				WriteFloat(tag.GetFloat());
				break;
			case TAGTYPE_BSOB:
				// Used for uint128 on Kad now
				WriteBsob(tag.GetBsob(), tag.GetBsobSize());
				break;
			case TAGTYPE_UINT16:
				WriteUInt16(tag.GetInt());
				break;
			case TAGTYPE_UINT8:
				WriteUInt8(tag.GetInt());
				break;
			case TAGTYPE_BLOB:
				// NOTE: This will break backward compatibility with met files for eMule versions prior to 0.44a
				// and any aMule prior to CVS 26/02/2005
				WriteUInt32(tag.GetBlobSize());
				Write(tag.GetBlob(), tag.GetBlobSize());
				break;
			default:
				//TODO: Support more tag types
				// With the if above, this should NEVER happen.
				printf("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, tag.GetType());
				wxASSERT(0);
				break;
		}				
	} catch (...) {
		//AddDebugLogLine(false, wxT("Exception in CDataIO:WriteTag"));
		printf("Exception in CDataIO:WriteTag");
		throw;
	}
}


void CFileDataIO::WriteTagPtrList(const TagPtrList& tagList)
{
	uint32 count = tagList.size();
	wxASSERT( count <= 0xFF );
	
	WriteUInt8(count);
	TagPtrList::const_iterator it;
	for (it = tagList.begin(); it != tagList.end(); it++) {
		WriteTag(**it);
	}
}

uint64 CFileDataIO::GetIntTagValue() const {

	uint8 type = ReadUInt8();
	
	ReadString(false);	
	
	switch (type) {
		
		case TAGTYPE_UINT64:
			return ReadUInt64();
			break;

		case TAGTYPE_UINT32:
			return ReadUInt32();
			break;

		case TAGTYPE_UINT16:
			return ReadUInt16();
			break;

		case TAGTYPE_UINT8:
			return ReadUInt8();
			break;
		
		default:
			throw wxString(wxT("Wrong tag type reading int tag"));
	}
}

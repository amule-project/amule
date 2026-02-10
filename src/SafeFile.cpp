//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "SafeFile.h"			// Interface declarations.
#include "MD4Hash.h"			// Needed for CMD4Hash
#include "kademlia/utils/UInt128.h"	// Needed for CUInt128
#include "ScopedPtr.h"			// Needed for CScopedPtr and CScopedArray
#include "Logger.h"
#include <common/Format.h>		// Needed for CFormat
#include "CompilerSpecific.h"		// Needed for __FUNCTION__


#define CHECK_BOM(size, x) ((size >= 3)  && (x[0] == (char)0xEF) && (x[1] == (char)0xBB) && (x[2] == (char)0xBF))

const char BOMHeader[3] = { '\xEF', '\xBB', '\xBF'};


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


// UInt128 values are stored a little weird way...
// Four little-endian 32-bit numbers, stored in
// big-endian order
CUInt128 CFileDataIO::ReadUInt128() const
{
	CUInt128 value;
	for (int i = 0; i < 4; i++) {
		// Four 32bits chunks
		value.Set32BitChunk(i, ReadUInt32());
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


unsigned char* CFileDataIO::ReadBsob(uint8* puSize) const
{
	MULE_VALIDATE_PARAMS(puSize, wxT("NULL pointer argument in ReadBsob"));

	*puSize = ReadUInt8();

	CScopedArray<unsigned char> bsob(*puSize);
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
	std::vector<char> val_array(raw_len + 1);
	val_array[raw_len] = 0;

	char* val = &(val_array[0]);

	Read(val, raw_len);
	wxString str;

	if (CHECK_BOM(raw_len, val)) {
		// This is a UTF8 string with a BOM header, skip header.
		str = UTF82unicode(val + 3);
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec detected: UTF-8+BOM for string of length %d"), raw_len));
	} else if (bOptUTF8) {
		str = UTF82unicode(val);
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec detected: UTF-8 for string of length %d"), raw_len));
		if (str.IsEmpty()) {
			// Fallback to Latin-1
			str = wxString(val, wxConvISO8859_1, raw_len);
			AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec fallback: ISO-8859-1 for string of length %d"), raw_len));
		}
	} else {
		// Raw strings are written as Latin-1 (see CFileDataIO::WriteStringCore)
		// However, for Kad search results, we should detect if the string is actually UTF-8
		// even when bOptUTF8 is false (backward compatibility)
		
		// Try multiple encodings in order of likelihood
		// 1. Check for BOMs and skip them
		// 2. Try UTF-8 (most common for modern Kad clients)
		// 3. Use ICU to detect the actual encoding (best for non-UTF encodings)
		// 4. Try UTF-16 LE/BE (only if UTF-8 fails and ICU fails)
		// 5. System locale (LANG/LANGUAGE environment variables)
		// 6. Latin-1 (fallback)
		
		// Check for BOMs and determine skip bytes
		int skipBytes = 0;
		bool hasBOM = false;
		
		if (raw_len >= 2) {
			if ((unsigned char)val[0] == 0xFF && (unsigned char)val[1] == 0xFE) {
				// UTF-16 LE BOM
				skipBytes = 2;
				hasBOM = true;
			} else if ((unsigned char)val[0] == 0xFE && (unsigned char)val[1] == 0xFF) {
				// UTF-16 BE BOM
				skipBytes = 2;
				hasBOM = true;
			}
		}
		
		if (!hasBOM && raw_len >= 3) {
			if ((unsigned char)val[0] == 0xEF && (unsigned char)val[1] == 0xBB && (unsigned char)val[2] == 0xBF) {
				// UTF-8 BOM
				skipBytes = 3;
				hasBOM = true;
			}
		}
		
		// Step 2: Try UTF-8 (most common for modern Kad clients)
		str = UTF82unicode(val + skipBytes);
		
		// Check if the UTF-8 decoding produced a valid string without replacement characters
		bool isValidUTF8 = !str.IsEmpty();
		if (isValidUTF8) {
			// Check for replacement characters (U+FFFD) which indicate invalid UTF-8
			wxString replacementChar = wxChar(0xFFFD);
			if (str.Contains(replacementChar)) {
				isValidUTF8 = false;
			}
		}
		
		if (isValidUTF8) {
			AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec detected: UTF-8 for string of length %d"), raw_len));
			return str;
		}
		
		// Step 3: Use ICU to detect the actual encoding (best for non-UTF encodings)
#ifdef HAVE_ICU
		UErrorCode status = U_ZERO_ERROR;
		
		UCharsetDetector* csd = ucsdet_open(&status);
		if (U_SUCCESS(status)) {
			// Set the input text for detection
			ucsdet_setText(csd, val + skipBytes, raw_len - skipBytes, &status);
			
			if (U_SUCCESS(status)) {
				// Detect the charset
				const UCharsetMatch* match = ucsdet_detect(csd, &status);
				
				if (U_SUCCESS(status) && match) {
					// Get the detected charset name
					const char* charset = ucsdet_getName(match, &status);
					
					if (U_SUCCESS(status) && charset) {
						// Get confidence level
						int32_t confidence = ucsdet_getConfidence(match, &status);
						
						if (U_SUCCESS(status) && confidence >= 30) {
							// Use ICU detection if confidence is >= 30%
							// Lower threshold to better detect encodings like GB2312
							// Use ICU to convert directly from detected charset to UTF-8
							UErrorCode convStatus = U_ZERO_ERROR;
							
							// Calculate output buffer size (UTF-8 can be up to 4 bytes per char)
							int32_t utf8Capacity = (raw_len - skipBytes) * 4;
							std::vector<char> utf8Dest(utf8Capacity);
							
							// Convert using ucnv_convert (returns length on success)
							int32_t utf8Length = ucnv_convert("UTF-8", charset, utf8Dest.data(), utf8Capacity, val + skipBytes, raw_len - skipBytes, &convStatus);
							
							if (U_SUCCESS(convStatus)) {
								// Convert UTF-8 to wxString
								str = wxString::FromUTF8(utf8Dest.data(), utf8Length);
								
								// Check if the conversion produced valid results
								if (!str.IsEmpty()) {
									AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec detected: %s (confidence %d%%) for string of length %d"), wxString(charset).Upper().c_str(), confidence, raw_len));
									ucsdet_close(csd);
									return str;
								}
							}
						}
					}
				}
			}
			
			ucsdet_close(csd);
		}
#endif
		
		// Step 4: Try UTF-16 (only if UTF-8 and ICU failed)
		if (!hasBOM && raw_len >= 2) {
			// Try UTF-16 LE (without BOM)
			const uint16_t* utf16le = reinterpret_cast<const uint16_t*>(val);
			size_t utf16leLen = raw_len / 2;
			
			// Convert UTF-16 LE to wxString
			str = wxString(reinterpret_cast<const char*>(utf16le), wxMBConvUTF16LE(), utf16leLen * 2);
			
			if (!str.IsEmpty()) {
				// Check for replacement characters
				wxString replacementChar = wxChar(0xFFFD);
				if (!str.Contains(replacementChar)) {
					// Successfully decoded UTF-16 LE
					AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec detected: UTF-16LE for string of length %d"), raw_len));
					return str;
				}
				str.Clear();
			}
			
			// Try UTF-16 BE (without BOM)
			if (str.IsEmpty() && raw_len >= 2) {
				const uint16_t* utf16be = reinterpret_cast<const uint16_t*>(val);
				size_t utf16beLen = raw_len / 2;
				
				// Convert UTF-16 BE to wxString
				str = wxString(reinterpret_cast<const char*>(utf16be), wxMBConvUTF16BE(), utf16beLen * 2);
				
				if (!str.IsEmpty()) {
					// Check for replacement characters
					wxString replacementChar = wxChar(0xFFFD);
					if (!str.Contains(replacementChar)) {
						// Successfully decoded UTF-16 BE
						AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec detected: UTF-16BE for string of length %d"), raw_len));
						return str;
					}
					str.Clear();
				}
			}
		}
		
		// Step 5: System locale (LANG/LANGUAGE environment variables)
		// This handles cases where the string is in the local encoding
		str = wxString(val + skipBytes, wxConvLocal);
		
		if (!str.IsEmpty()) {
			AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec fallback: LOCALE for string of length %d"), raw_len));
			return str;
		}
		
		// Step 6: Latin-1 fallback
		str = wxString(val + skipBytes, wxConvISO8859_1, raw_len - skipBytes);
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Codec fallback: ISO-8859-1 for string of length %d"), raw_len));
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


// UInt128 values are stored a little weird way...
// Four little-endian 32-bit numbers, stored in
// big-endian order
void CFileDataIO::WriteUInt128(const Kademlia::CUInt128& value)
{
	for (int i = 0; i < 4; i++) {
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
			if (s.data()) {
				WriteStringCore(s, eEncode, SizeLen);
				break;
			}
		}
		/* fall through */
		default: {
			// Non UTF-8 strings are saved as Latin-1
			wxCharBuffer s1 = str.mb_str(wxConvISO8859_1);
			WriteStringCore(s1, utf8strNone, SizeLen);
		}
	}
}


void CFileDataIO::WriteStringCore(const char *s, EUtf8Str eEncode, uint8 SizeLen)
{
	uint32 sLength = s ? strlen(s) : 0;
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
			// We must not allow too long strings to be written,
			// as this would allow for a buggy clients to "poison"
			// us, by sending ISO8859-1 strings that expand to a
			// greater than 16b length when converted as UTF-8.
			if (real_length > 0xFFFF) {
				wxFAIL_MSG(wxT("String is too long to be saved"));

				real_length = std::min<uint32>(real_length, 0xFFFF);
				if (eEncode == utf8strOptBOM) {
					sLength = real_length - 3;
				} else {
					sLength = real_length;
				}
			}

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


CTag *CFileDataIO::ReadTag(bool bOptACP) const
{
	CTag *retVal = NULL;
	wxString name;
	uint8_t type = 0;
	try {
		type = ReadUInt8();
		name = ReadString(false);

		switch (type)
		{
			// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade
			// the net in some months.
			//
			// And still.. it doesn't work this way without breaking backward compatibility. To properly
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
			case TAGTYPE_HASH16:
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
			// And still.. it doesn't work this way without breaking backward compatibility
			case TAGTYPE_BSOB:
			{
				uint8 size = 0;
				CScopedArray<unsigned char> value(ReadBsob(&size));

				retVal = new CTagBsob(name, value.get(), size);
				break;
			}

			default:
				throw wxString(CFormat(wxT("Invalid Kad tag type; type=0x%02x name=%s\n")) % type % name);
		}
	} catch(const CMuleException& e) {
		AddLogLineN(e.what());
		delete retVal;
		throw;
	} catch(const wxString& e) {
		AddLogLineN(e);
		throw;
	}

	return retVal;
}


void CFileDataIO::ReadTagPtrList(TagPtrList* taglist, bool bOptACP) const
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
			case TAGTYPE_HASH16:
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
				// and any aMule prior to SVN 26/02/2005
				WriteUInt32(tag.GetBlobSize());
				Write(tag.GetBlob(), tag.GetBlobSize());
				break;
			default:
				//TODO: Support more tag types
				// With the if above, this should NEVER happen.
				AddLogLineNS(CFormat(wxT("CFileDataIO::WriteTag: Unknown tag: type=0x%02X")) % tag.GetType());
				wxFAIL;
				break;
		}
	} catch (...) {
		AddLogLineNS(wxT("Exception in CDataIO:WriteTag"));
		throw;
	}
}


void CFileDataIO::WriteTagPtrList(const TagPtrList& tagList)
{
	uint32 count = tagList.size();
	wxASSERT( count <= 0xFF );

	WriteUInt8(count);
	TagPtrList::const_iterator it;
	for (it = tagList.begin(); it != tagList.end(); ++it) {
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
// File_checked_for_headers

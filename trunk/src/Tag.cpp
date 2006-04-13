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

#include <zlib.h>		// Needed for uLongf

#include "Tag.h"				// Interface declarations

#include <common/MuleDebug.h>		// Needed for CInvalidPacket

#include "SafeFile.h"		// Needed for CFileDataIO
#include "MD4Hash.h"			// Needed for CMD4Hash

///////////////////////////////////////////////////////////////////////////////
// CTag

CTag::CTag(const wxString& Name)
{
	m_uType = 0;
	m_uName = 0;
	m_Name = Name;
	m_uVal = 0;
	m_nSize = 0;
}

CTag::CTag(uint8 uName)
{
	m_uType = 0;
	m_uName = uName;
	m_uVal = 0;
	m_nSize = 0;
}

CTag::CTag(const CTag& rTag)
{
	m_uType = rTag.m_uType;
	m_uName = rTag.m_uName;
	m_Name = rTag.m_Name;
	m_nSize = 0;
	if (rTag.IsStr()) {
		m_pstrVal = new wxString(rTag.GetStr());
	} else if (rTag.IsInt()) {
		m_uVal = rTag.GetInt();
	} else if (rTag.IsFloat()) {
		m_fVal = rTag.GetFloat();
	} else if (rTag.IsHash()) {
		m_hashVal = new CMD4Hash(rTag.GetHash());
	} else if (rTag.IsBlob()) {
		m_nSize = rTag.GetBlobSize();
		m_pData = new unsigned char[rTag.GetBlobSize()];
		memcpy(m_pData, rTag.GetBlob(), rTag.GetBlobSize());
	} else {
		wxASSERT(0);
		m_uVal = 0;
	}
}


CTag::CTag(const CFileDataIO& data, bool bOptUTF8)
{
	// Zero variables to allow for safe deletion
	m_uType = m_uName = m_nSize = m_uVal = 0;
	m_pData = NULL;	
	
	try {
		m_uType = data.ReadUInt8();
		if (m_uType & 0x80) {
			m_uType &= 0x7F;
			m_uName = data.ReadUInt8();
		} else {
			uint16 length = data.ReadUInt16();
			if (length == 1) {
				m_uName = data.ReadUInt8();
			} else {
				m_uName = 0;
				m_Name = data.ReadOnlyString(utf8strNone,length);
			}
		}
	
		// NOTE: It's very important that we read the *entire* packet data,
		// even if we do not use each tag. Otherwise we will get in trouble
		// when the packets are returned in a list - like the search results
		// from a server. If we cannot do this, then we throw an exception.
		switch (m_uType) {
			case TAGTYPE_STRING:
				m_pstrVal = new wxString(data.ReadString(bOptUTF8));
				break;
			
			case TAGTYPE_UINT32:
				m_uVal = data.ReadUInt32();
				break;

			case TAGTYPE_UINT64:
				m_uVal = data.ReadUInt64();
				break;
			
			case TAGTYPE_UINT16:
				m_uVal = data.ReadUInt16();
				m_uType = TAGTYPE_UINT32;
				break;
			
			case TAGTYPE_UINT8:
				m_uVal = data.ReadUInt8();
				m_uType = TAGTYPE_UINT32;
				break;
			
			case TAGTYPE_FLOAT32:
				#warning Endianess problem?
				data.Read(&m_fVal, 4);
				break;
			
			case TAGTYPE_HASH:
				m_hashVal = new CMD4Hash(data.ReadHash());
				break;
			
			case TAGTYPE_BOOL:
				printf("***NOTE: %s; Reading BOOL tag\n", __FUNCTION__);
				data.ReadUInt8();
				break;
			
			case TAGTYPE_BOOLARRAY: {
				printf("***NOTE: %s; Reading BOOL Array tag\n", __FUNCTION__);
				uint16 len = data.ReadUInt16();
			
				// 07-Apr-2004: eMule versions prior to 0.42e.29 used the formula "(len+7)/8"!
				#warning This seems to be off by one! 8 / 8 + 1 == 2, etc.
				data.Seek((len / 8) + 1, wxFromCurrent);
				break;
			}
	
			case TAGTYPE_BLOB:
				// 07-Apr-2004: eMule versions prior to 0.42e.29 handled the "len" as int16!
				m_nSize = data.ReadUInt32();
				
				// Since the length is 32b, this check is needed to avoid
				// huge allocations in case of bad tags.
				if (m_nSize > data.GetLength() - data.GetPosition()) {
					throw CInvalidPacket(wxT("Malformed tag"));
				}
					
				m_pData = new unsigned char[m_nSize];
				data.Read(m_pData, m_nSize);
				break;
		
			default:
				if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16) {
					uint8 length = m_uType - TAGTYPE_STR1 + 1;
					m_pstrVal = new wxString(data.ReadOnlyString(bOptUTF8, length));
					m_uType = TAGTYPE_STRING;
				} else {
					// Since we cannot determine the length of this tag, we
					// simply have to abort reading the file.
					throw CInvalidPacket(wxString::Format(wxT("Unknown tag type encounted %x, cannot proceed!"),m_uType));
				}
		}
	} catch (...) {
		if (m_uType == TAGTYPE_BLOB) {
			delete[] m_pData;
		}

		throw;
	}
}


CTag::~CTag()
{
	if (IsStr()) {
		delete m_pstrVal;
	} else if (IsHash()) {
		delete m_hashVal;
	} else if (IsBlob() || IsBsob()) {
		delete[] m_pData;
	} 
}


#define CHECK_TAG_TYPE(check, expected) \
	if (!(check)) { \
		throw CInvalidPacket(wxT(#expected) wxT(" tag expected, but found ") + GetFullInfo()); \
	}

uint64 CTag::GetInt() const
{
	CHECK_TAG_TYPE(IsInt(), Integer);
	
	return m_uVal; 
}


const wxString& CTag::GetStr() const
{
	CHECK_TAG_TYPE(IsStr(), String);
	
	return *m_pstrVal; 	
}


float CTag::GetFloat() const
{
	CHECK_TAG_TYPE(IsFloat(), Float);

	return m_fVal;
}


const CMD4Hash& CTag::GetHash() const
{
	CHECK_TAG_TYPE(IsHash(), Hash);
	
	return *m_hashVal;
}
	

uint32 CTag::GetBlobSize() const
{
	CHECK_TAG_TYPE(IsBlob(), Blob);
	
	return m_nSize;
}
	

const byte* CTag::GetBlob() const
{
	CHECK_TAG_TYPE(IsBlob(), Blob);
	
	return m_pData;
}


uint32 CTag::GetBsobSize() const
{
	CHECK_TAG_TYPE(IsBsob(), Bsob);
	
	return m_nSize;
}
	

const byte* CTag::GetBsob() const
{
	CHECK_TAG_TYPE(IsBsob(), Bsob);
	
	return m_pData;
}

bool CTag::WriteNewEd2kTag(CFileDataIO* data, EUtf8Str eStrEncode) const
{

	// Write tag type
	uint8 uType;
	
	if (IsInt()) {
		if (m_uVal <= 0xFF) {
			uType = TAGTYPE_UINT8;
		} else if (m_uVal <= 0xFFFF) {
			uType = TAGTYPE_UINT16;
		} else {
			uType = TAGTYPE_UINT32;
		}
	} else if (IsStr()) {
		uint16 uStrValLen = GetRawSize(*m_pstrVal, eStrEncode);
		if (uStrValLen >= 1 && uStrValLen <= 16) {
			uType = TAGTYPE_STR1 + uStrValLen - 1;
		} else {
			uType = TAGTYPE_STRING;
		}
	} else {
		uType = m_uType;
	}

	// Write tag name
	if (!m_Name.IsEmpty()) {
		data->WriteUInt8(uType);
		data->WriteString(m_Name,utf8strNone);
	} else {
		wxASSERT( m_uName != 0 );
		data->WriteUInt8(uType | 0x80);
		data->WriteUInt8(m_uName);
	}

	// Write tag data
	switch (uType) {
		case TAGTYPE_STRING:
			data->WriteString(*m_pstrVal,eStrEncode);
			break;
		case TAGTYPE_UINT32:
			data->WriteUInt32(m_uVal);
			break;
		case TAGTYPE_UINT16:
			data->WriteUInt16(m_uVal);
			break;
		case TAGTYPE_UINT8:
			data->WriteUInt8(m_uVal);
			break;
		case TAGTYPE_FLOAT32:
			#warning Endianess problem?
			data->Write(&m_fVal, 4);
			break;
		case TAGTYPE_HASH:
			data->WriteHash(*m_hashVal);
			break;
		case TAGTYPE_BLOB:
			data->WriteUInt32(m_nSize);
			data->Write(m_pData, m_nSize);
			break;
		default:
			// See comment on the default: of CTag::CTag(const CFileDataIO& data, bool bOptUTF8)
			if (uType >= TAGTYPE_STR1 && uType <= TAGTYPE_STR16) {
				// Sending '0' as len size makes it not write the len on the IO file.
				// This is because this tag types send the len as their type.
				data->WriteString(*m_pstrVal,eStrEncode,0); 
			} else {
				printf("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, uType);
				wxASSERT(0);
				return false;
			}
			break;
	}

	return true;
}

bool CTag::WriteTagToFile(CFileDataIO* file, EUtf8Str WXUNUSED(eStrEncode), bool restrictive) const
{
	
	// Don't write tags of unknown types, we wouldn't be able to read them in again 
	// and the met file would be corrupted
	if (!restrictive || (IsStr() || IsInt() || IsFloat() || IsBlob())) {
	
		// If this fails, it'll throw.
		file->WriteTag(*this);
		return true;

	} else {
		printf("%s; Ignored tag with unknown type=0x%02X\n", __FUNCTION__, m_uType);
		return false;
	}
}


wxString CTag::GetFullInfo() const
{
	wxString strTag;
	if (!m_Name.IsEmpty()) {
		strTag = wxT('\"');
		strTag += m_Name;
		strTag += wxT('\"');
	} else {
		strTag = wxString::Format(wxT("0x%02X"), m_uName);
	}
	strTag += wxT("=");
	if (m_uType == TAGTYPE_STRING) {
		strTag += wxT("\"");
		strTag += *m_pstrVal;
		strTag += wxT("\"");
	} else if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16) {
		strTag += wxString::Format(wxT("(Str%u)\""), m_uType - TAGTYPE_STR1 + 1)
					+  *m_pstrVal + wxT("\"");
	} else if (m_uType == TAGTYPE_UINT32) {
		strTag += wxString::Format(wxT("(Int32)%u"), m_uVal);
	} else if (m_uType == TAGTYPE_UINT16) {
		strTag += wxString::Format(wxT("(Int16)%u"), m_uVal);
	} else if (m_uType == TAGTYPE_UINT8) {
		strTag += wxString::Format(wxT("(Int8)%u"), m_uVal);
	} else if (m_uType == TAGTYPE_FLOAT32) {
		strTag += wxString::Format(wxT("(Float32)%f"), m_fVal);
	} else if (m_uType == TAGTYPE_BLOB) {
		strTag += wxString::Format(wxT("(Blob)%u"), m_nSize);
	} else {
		strTag += wxString::Format(wxT("Type=%u"), m_uType);
	}
	return strTag;
}

CTagHash::CTagHash(const wxString& name, const CMD4Hash& value)
	: CTag(name) {
		m_hashVal = new CMD4Hash(value);
		m_uType = TAGTYPE_HASH;
	}

CTagHash::CTagHash(uint8 name, const CMD4Hash& value)
	: CTag(name) {
		m_hashVal = new CMD4Hash(value);
		m_uType = TAGTYPE_HASH;
	}

void deleteTagPtrListEntries(TagPtrList* taglist)
{
	TagPtrList::const_iterator it;
	for (it = taglist->begin(); it != taglist->end(); it++) {
		delete *it;
	}
	taglist->clear();
}

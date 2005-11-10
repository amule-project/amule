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

#include <zlib.h>		// Needed for uLongf

#include "Tag.h"				// Interface declarations

#include <wx/string.h>

#include "SafeFile.h"		// Needed for CFileDataIO
#include "MuleDebug.h"		// Needed for CInvalidPacket
#include "MD4Hash.h"			// Needed for CMD4Hash

///////////////////////////////////////////////////////////////////////////////
// CTag

CTag::CTag(char* pszName, uint32 uVal)
{
	m_uType = TAGTYPE_UINT32;
	m_uName = 0;
	m_pszName = nstrdup(pszName);
	m_uVal = uVal;
	m_nBlobSize = 0;
}

CTag::CTag(uint8 uName, uint32 uVal)
{
	m_uType = TAGTYPE_UINT32;
	m_uName = uName;
	m_pszName = NULL;
	m_uVal = uVal;
	m_nBlobSize = 0;
}

CTag::CTag(char* pszName, const wxString& rstrVal)
{
	m_uType = TAGTYPE_STRING;
	m_uName = 0;
	m_pszName = nstrdup(pszName);
	m_pstrVal = new wxString(rstrVal);
	m_nBlobSize = 0;
}

CTag::CTag(uint8 uName, const wxString& rstrVal)
{
	m_uType = TAGTYPE_STRING;
	m_uName = uName;
	m_pszName = NULL;
	m_pstrVal = new wxString(rstrVal);
	m_nBlobSize = 0;
}

CTag::CTag(uint8 uName, const CMD4Hash& hash)
{
	m_uType = TAGTYPE_HASH;
	m_uName = uName;
	m_pszName = NULL;
	m_hashVal = new CMD4Hash(hash);
	m_nBlobSize = 0;
}

CTag::CTag(uint8 uName, uint32 nSize, const unsigned char* pucData){
	m_uType = TAGTYPE_BLOB;
	m_uName = uName;
	m_pszName = NULL;
	m_pData = new unsigned char[nSize];
	memcpy(m_pData, pucData, nSize);
	m_nBlobSize = nSize;
}

CTag::CTag(const CTag& rTag)
{
	m_uType = rTag.m_uType;
	m_uName = rTag.m_uName;
	m_pszName = rTag.m_pszName!=NULL ? nstrdup(rTag.m_pszName) : NULL;
	m_nBlobSize = 0;
	if (rTag.IsStr()) {
		m_pstrVal = new wxString(rTag.GetStr());
	} else if (rTag.IsInt()) {
		m_uVal = rTag.GetInt();
	} else if (rTag.IsFloat()) {
		m_fVal = rTag.GetFloat();
	} else if (rTag.IsHash()) {
		m_hashVal = new CMD4Hash(rTag.GetHash());
	} else if (rTag.IsBlob()) {
		m_nBlobSize = rTag.GetBlobSize();
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
	m_uType = m_uName = m_nBlobSize = 0;
	m_pszName = NULL;
	m_pData = NULL;	
	
	try {
		m_uType = data.ReadUInt8();
		if (m_uType & 0x80) {
			m_uType &= 0x7F;
			m_uName = data.ReadUInt8();
			m_pszName = NULL;
		} else {
			uint16 length = data.ReadUInt16();
			if (length == 1) {
				m_uName = data.ReadUInt8();
				m_pszName = NULL;
			} else {
				m_uName = 0;
				m_pszName = new char[length + 1];
				data.Read(m_pszName, length);
				m_pszName[length] = '\0';
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
				m_nBlobSize = data.ReadUInt32();
				
				// Since the length is 32b, this check is needed to avoid
				// huge allocations in case of bad tags.
				if (m_nBlobSize > data.GetLength() - data.GetPosition()) {
					throw CInvalidPacket(wxT("Malformed tag"));
				}
					
				m_pData = new unsigned char[m_nBlobSize];
				data.Read(m_pData, m_nBlobSize);
				break;
		
			default:
				if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16) {
					uint8 length = m_uType - TAGTYPE_STR1 + 1;
					m_pstrVal = new wxString(data.ReadOnlyString(bOptUTF8, length));
					m_uType = TAGTYPE_STRING;
				} else {
					// Since we cannot determine the length of this tag, we
					// simply have to abort reading the file.
					throw CInvalidPacket(wxT("Unknown tag type encounted, cannot proceed"));
				}
		}
	} catch (...) {
		delete[] m_pszName;
		if (m_uType == TAGTYPE_BLOB) {
			delete[] m_pData;
		}

		throw;
	}
}


CTag::~CTag()
{
	delete[] m_pszName;
	if (IsStr()) {
		delete m_pstrVal;
	} else if (IsHash()) {
		delete m_hashVal;
	} else if (IsBlob()) {
		delete[] m_pData;
	}
}


#define CHECK_TAG_TYPE(check, expected) \
	if (!(check)) { \
		throw CInvalidPacket(wxT(#expected) wxT(" tag expected, but found ") + GetFullInfo()); \
	}

uint32 CTag::GetInt() const
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
	
	return m_nBlobSize;
}
	

const byte* CTag::GetBlob() const
{
	CHECK_TAG_TYPE(IsBlob(), Blob);
	
	return m_pData;
}


void CTag::SetInt(uint32 uVal)
{
	CHECK_TAG_TYPE(IsInt(), Integer);
		
	m_uVal = uVal;
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
	if (m_pszName) {
		data->WriteUInt8(uType);
		uint16 uTagNameLen = strlen(m_pszName);
		data->WriteUInt16(uTagNameLen);
		data->Write(m_pszName, uTagNameLen);
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
			data->WriteUInt32(m_nBlobSize);
			data->Write(m_pData, m_nBlobSize);
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

bool CTag::WriteTagToFile(CFileDataIO* file, EUtf8Str eStrEncode) const
{
	// Don't write tags of unknown types, we wouldn't be able to read them in again 
	// and the met file would be corrupted
	if (IsStr() || IsInt() || IsFloat() || IsBlob()) {
		
		file->WriteUInt8(m_uType);
		
		if (m_pszName) {
			// Don't change this for a WriteString, will only make things slower.
			uint16 tagname_len = strlen(m_pszName);
			file->WriteUInt16(tagname_len);
			file->Write(m_pszName, tagname_len);
		} else {
			file->WriteUInt16(1);
			file->WriteUInt8(m_uName);
		}

		if (IsStr()) {
			file->WriteString(GetStr(), eStrEncode);
		} else if (IsInt()) {
			file->WriteUInt32(m_uVal);
		} else if (IsFloat()) {
			#warning Endianess problem?
			file->Write(&m_fVal, 4);
		} else if (IsBlob()) {
			// NOTE: This will break backward compatibility with met files for eMule versions prior to 0.44a
			// and any aMule prior to CVS 26/02/2005
			file->WriteUInt32(m_nBlobSize);
			file->Write(m_pData, m_nBlobSize);
		} else { //TODO: Support more tag types
			// With the if above, this should NEVER happen.
			printf("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, m_uType);
			wxASSERT(0);
			return false;
		}
		//TODO: Support more tag types
		return true;
	} else {
		printf("%s; Ignored tag with unknown type=0x%02X\n", __FUNCTION__, m_uType);
		wxASSERT(0);
		return false;
	}
}


wxString CTag::GetFullInfo() const
{
	wxString strTag;
	if (m_pszName) {
		strTag = wxT('\"');
		strTag += char2unicode(m_pszName);
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
		strTag += wxString::Format(wxT("(Blob)%u"), m_nBlobSize);
	} else {
		strTag += wxString::Format(wxT("Type=%u"), m_uType);
	}
	return strTag;
}

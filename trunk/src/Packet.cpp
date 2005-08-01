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
#pragma implementation "Packet.h"
#endif

#include <zlib.h>		// Needed for uLongf

#include "Packet.h"		// Interface declarations
#include "StringFunctions.h"	// Needed for nstrdup
#include "MemFile.h"		// Needed for CMemFile
#include "OtherStructs.h"	// Needed for Header_Struct
#include "Types.h"		// Needed for wxFileSize_t

// Copy constructor
CPacket::CPacket(CPacket &p)
{
	size 		= p.size;
	opcode		= p.opcode;
	prot 		= p.prot;
	m_bSplitted 	= p.m_bSplitted;
	m_bLastSplitted = p.m_bLastSplitted;
	m_bPacked 	= p.m_bPacked;
	m_bFromPF 	= p.m_bFromPF;
	memcpy(head, p.head, sizeof head);
	tempbuffer	= NULL;
	if (p.completebuffer) {
		completebuffer 	= new char[size + 10];;
		pBuffer 	= completebuffer + sizeof(Header_Struct);
	} else {
		completebuffer 	= NULL;
		if (p.pBuffer) {
			pBuffer = new char[size];
		} else {
			pBuffer = NULL;
		}
	}
	if (pBuffer)
		memcpy( pBuffer, p.pBuffer, size );
}

CPacket::CPacket(uint8 protocol)
{
	size 		= 0;
	opcode		= 0;
	prot 		= protocol;
	m_bSplitted 	= false;
	m_bLastSplitted = false;
	m_bPacked 	= false;
	m_bFromPF 	= false;
	memset(head, 0, sizeof head);
	tempbuffer	= NULL;
	completebuffer 	= NULL;
	pBuffer 	= NULL;
}

// only used for receiving packets
CPacket::CPacket(char* header)
{
	memset(head, 0, sizeof head);
	Header_Struct* head = (Header_Struct*) header;
	size 		= ENDIAN_SWAP_32(head->packetlength) - 1;
	opcode		= head->command;
	prot		= head->eDonkeyID;
	m_bSplitted 	= false;
	m_bLastSplitted = false;
	m_bPacked 	= false;
	m_bFromPF 	= false;
	tempbuffer	= NULL;
	completebuffer 	= NULL;
	pBuffer 	= NULL;
}

CPacket::CPacket(CMemFile* datafile, uint8 protocol, uint8 ucOpcode, bool detach)
{
	size		= datafile->GetLength();
	opcode		= ucOpcode;
	prot		= protocol;
	m_bSplitted 	= false;
	m_bLastSplitted = false;
	m_bPacked 	= false;
	m_bFromPF 	= false;
	memset(head, 0, sizeof head);
	tempbuffer = NULL;
	completebuffer = new char[size + sizeof(Header_Struct)/*Why this 4?*/];
	pBuffer = completebuffer + sizeof(Header_Struct);
	
	
	if (detach) {
		//		pBuffer = (char*)datafile->Detach();
		byte* tmp = datafile->Detach();
		memcpy(pBuffer, tmp, size);
		free(tmp);
	} else {
		memcpy(pBuffer, datafile->GetBuffer(), size);
	}
}

CPacket::CPacket(int8 in_opcode, uint32 in_size, uint8 protocol, bool bFromPF)
{
	size		= in_size;
	opcode		= in_opcode;
	prot		= protocol;
	m_bSplitted 	= false;
	m_bLastSplitted = false;
	m_bPacked 	= false;
	m_bFromPF	= bFromPF;
	memset(head, 0, sizeof head);
	tempbuffer	= NULL;
	if (in_size) {
		completebuffer = new char[in_size + sizeof(Header_Struct) + 4 /*Why this 4?*/];
		pBuffer = completebuffer + sizeof(Header_Struct);
		memset(completebuffer, 0, in_size + sizeof(Header_Struct) + 4 /*Why this 4?*/);
	} else {
		completebuffer = NULL;
		pBuffer = NULL;
	}
}

// only used for splitted packets!
CPacket::CPacket(char* pPacketPart, uint32 nSize, bool bLast, bool bFromPF)
{
	size		= nSize - sizeof(Header_Struct);
	opcode		= 0;
	prot		= 0;
	m_bSplitted	= true;
	m_bLastSplitted	= bLast;
	m_bPacked	= false;
	m_bFromPF	= bFromPF;
	memset(head, 0, sizeof head);
	tempbuffer	= NULL;
	completebuffer	= pPacketPart;
	pBuffer		= NULL;
}

CPacket::~CPacket()
{
	// Never deletes pBuffer when completebuffer is not NULL
	if (completebuffer) {
		delete [] completebuffer;
	} else if (pBuffer) {
	// On the other hand, if completebuffer is NULL and pBuffer is not NULL 
		delete [] pBuffer;
	}
	
	if (tempbuffer) {
		delete [] tempbuffer;
	}
}

void CPacket::AllocDataBuffer(void)
{
	wxASSERT(completebuffer == NULL);
	pBuffer = new char[size + 1];
}

void CPacket::CopyToDataBuffer(unsigned int offset, const char *data, unsigned int n)
{
	wxASSERT(offset + n <= size + 1);
	memcpy(pBuffer + offset, data, n);
}

char* CPacket::GetPacket() {
	if (completebuffer) {
		if (!m_bSplitted) {
			memcpy(completebuffer, GetHeader(), sizeof(Header_Struct));
		}
		return completebuffer;
	} else {
		if (tempbuffer){
			delete [] tempbuffer;
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+sizeof(Header_Struct) + 4 /* why this 4?*/];
		memcpy(tempbuffer    , GetHeader(), sizeof(Header_Struct));
		memcpy(tempbuffer + sizeof(Header_Struct), pBuffer    , size);
		return tempbuffer;
	}
}

char* CPacket::DetachPacket() {
	if (completebuffer) {
		if (!m_bSplitted) {
			memcpy(completebuffer, GetHeader(), sizeof(Header_Struct));
		}
		char* result = completebuffer;
		completebuffer = pBuffer = NULL;
		return result;
	} else{
		if (tempbuffer){
			delete[] tempbuffer;
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+sizeof(Header_Struct)+4 /* Why this 4?*/];
		memcpy(tempbuffer,GetHeader(),sizeof(Header_Struct));
		memcpy(tempbuffer+sizeof(Header_Struct),pBuffer,size);
		char* result = tempbuffer;
		tempbuffer = 0;
		return result;
	}
}

char* CPacket::GetHeader() {
	wxASSERT( !m_bSplitted );

	Header_Struct* header = (Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;
	header->packetlength = ENDIAN_SWAP_32(size + 1);

	return head;
}

char* CPacket::GetUDPHeader() {
	wxASSERT( !m_bSplitted );

	memset(head, 0, 6);
	UDP_Header_Struct* header = (UDP_Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;

	return head;
}

void CPacket::PackPacket() {
	wxASSERT(!m_bSplitted);

	uLongf newsize = size + 300;
	byte* output = new byte[newsize];

	uint16 result = compress2(output, &newsize, (byte*) pBuffer, size, Z_BEST_COMPRESSION);

	if (result != Z_OK || size <= newsize) {
		delete[] output;
		return;
	}

	prot = OP_PACKEDPROT;
	memcpy(pBuffer, output, newsize);
	delete[] output;
	m_bPacked = true;
	
	size = newsize;
}

bool CPacket::UnPackPacket(uint32 uMaxDecompressedSize) {
	wxASSERT( prot == OP_PACKEDPROT );

	uint32 nNewSize = size * 10 + 300;

	if (nNewSize > uMaxDecompressedSize){
		nNewSize = uMaxDecompressedSize;
	}

	byte* unpack = new byte[nNewSize];
	uLongf unpackedsize = nNewSize;
	uint16 result = uncompress(unpack, &unpackedsize, (byte*) pBuffer, size);

	if (result == Z_OK) {
		wxASSERT( completebuffer == NULL );
		wxASSERT( pBuffer != NULL );

		size = unpackedsize;
		delete[] pBuffer;
		pBuffer = (char *) unpack;
		prot = OP_EMULEPROT;
		return true;
	}

	delete[] unpack;
	return false;
}

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

CTag::CTag(uint8 uName, const unsigned char* pucHash)
{
	m_uType = TAGTYPE_HASH;
	m_uName = uName;
	m_pszName = NULL;
	m_pData = new unsigned char[16];
	md4cpy(m_pData, pucHash);
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
		m_pData = new unsigned char[16];
		md4cpy(m_pData, rTag.GetHash());
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
	#define BAD_TAG_MSG "Error reading tag from met file"
	
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
			m_pszName = new char[length+1];
			try {
				data.Read(m_pszName, length);
			} catch(...) {
				delete[] m_pszName;
				m_pszName = NULL;
				throw CInvalidPacket(wxT(BAD_TAG_MSG));
			}
			m_pszName[length] = '\0';
		}
	}
	
	m_nBlobSize = 0;

	// NOTE: It's very important that we read the *entire* packet data, even if we do
	// not use each tag. Otherwise we will get troubles when the packets are returned in 
	// a list - like the search results from a server.
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
			m_pData = new byte[16];
			try{
				data.ReadHash16(m_pData);
			} catch (...){
				delete[] m_pData;
				m_pData = NULL;
				throw CInvalidPacket(wxT(BAD_TAG_MSG));
			}
			break;
		case TAGTYPE_BOOL: {
			printf("***NOTE: %s; Reading BOOL tag\n", __FUNCTION__);
			data.ReadUInt8();
			break;
		}
		case TAGTYPE_BOOLARRAY: {
			printf("***NOTE: %s; Reading BOOL Array tag\n", __FUNCTION__);
			uint16 len = data.ReadUInt16();
			// 07-Apr-2004: eMule versions prior to 0.42e.29 used the formula "(len+7)/8"!
			char* discard = new char[ (len/8)+1 ];
			data.Read(discard, (len/8)+1);
			delete[] discard;
			break;
		}
		case TAGTYPE_BLOB:
			// 07-Apr-2004: eMule versions prior to 0.42e.29 handled the "len" as int16!
			m_nBlobSize = data.ReadUInt32();
			if (m_nBlobSize <= data.GetLength() - data.GetPosition()) {
				m_pData = new unsigned char[m_nBlobSize];
				data.Read(m_pData, m_nBlobSize);
			} else{
				wxASSERT( false );
				m_nBlobSize = 0;
				m_pData = NULL;
			}
			break;
		default:
			// <Kry> I do really HATE that switch() doesn't handle ranges
			// <Kry> :(
			// <pho3nix> yes, switches should handle ranges.
			// <pho3nix> after all, what are compilers for? :)
			if (m_uType >= TAGTYPE_STR1 && m_uType <= TAGTYPE_STR16) {
				uint8 length = m_uType - TAGTYPE_STR1 + 1;
				m_pstrVal = new wxString(data.ReadOnlyString(bOptUTF8, length));
				m_uType = TAGTYPE_STRING;
			} else {
				if (m_uName != 0) {
					printf("%s; Unknown tag: type=0x%02X  specialtag=%u\n", __FUNCTION__, m_uType, m_uName);
				} else {
					printf("%s; Unknown tag: type=0x%02X  name=\"%s\"\n", __FUNCTION__, m_uType, m_pszName);
				}
				m_uVal = 0;
			}
	}
}

CTag::~CTag()
{
	delete[] m_pszName;
	if (IsStr()) {
		delete m_pstrVal;
	} else if (IsHash()) {
		delete[] m_pData;
	} else if (IsBlob()) {
		delete[] m_pData;
	}
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
			data->WriteHash16(m_pData);
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

void CTag::SetInt(uint32 uVal)
{
	wxASSERT( IsInt() );
	if (IsInt()) {
		m_uVal = uVal;
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

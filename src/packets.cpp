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


#include <zlib.h>		// Needed for uLongf

#include "packets.h"		// Interface declarations
#include "otherfunctions.h"	// Needed for nstrdup
#include "CMemFile.h"		// Needed for CMemFile
#include "otherstructs.h"	// Needed for Header_Struct

// Copy constructor
Packet::Packet(Packet &p)
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
		pBuffer 	= completebuffer + 6;
	} else {
		completebuffer 	= NULL;
		if (p.pBuffer) {
			pBuffer = new char[size + 1];
		} else {
			pBuffer = NULL;
		}
	}
	if (pBuffer)
		memcpy( pBuffer, p.pBuffer, size + 1);
}

Packet::Packet(uint8 protocol)
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
Packet::Packet(char* header)
{
	Header_Struct* head = (Header_Struct*) header;
	size 		= head->packetlength - 1;
	opcode		= head->command;
	prot		= head->eDonkeyID;
	m_bSplitted 	= false;
	m_bLastSplitted = false;
	m_bPacked 	= false;
	m_bFromPF 	= false;
	memset(head, 0, sizeof head);
	tempbuffer	= NULL;
	completebuffer 	= NULL;
	pBuffer 	= NULL;
}

Packet::Packet(CMemFile* datafile, uint8 protocol)
{
	size		= datafile->GetLength();
	opcode		= 0;
	prot		= protocol;
	m_bSplitted 	= false;
	m_bLastSplitted = false;
	m_bPacked 	= false;
	m_bFromPF 	= false;
	memset(head, 0, sizeof head);
	tempbuffer = NULL;
	completebuffer = new char[size + 10];
	pBuffer = completebuffer + 6;

	BYTE* tmp = datafile->Detach();
	memcpy(pBuffer, tmp, size);
	#warning check where is the malloc of this free
	free(tmp); 
}

Packet::Packet(int8 in_opcode, wxInt32 in_size, uint8 protocol, bool bFromPF)
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
		completebuffer = new char[in_size + 10];
		pBuffer = completebuffer + 6;
		memset(completebuffer, 0, in_size + 10);
	} else {
		completebuffer = NULL;
		pBuffer = NULL;
	}
}

// only used for splitted packets!
Packet::Packet(char* pPacketPart, wxUint32 nSize, bool bLast, bool bFromPF)
{
	size		= nSize - 6;
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

Packet::~Packet()
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

void Packet::AllocDataBuffer(void)
{
	wxASSERT(completebuffer == NULL);
	pBuffer = new char[size + 1];
}

void Packet::CopyToDataBuffer(unsigned int offset, const char *data, unsigned int n)
{
	wxASSERT(offset + n <= size + 1);
	memcpy(pBuffer + offset, data, n);
}

char* Packet::GetPacket() {
	if (completebuffer) {
		if (!m_bSplitted) {
			memcpy(completebuffer, GetHeader(), 6);
		}
		return completebuffer;
	} else {
		if (tempbuffer){
			delete [] tempbuffer;
			#warning Phoenix - why??? Check this info and clean!
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+10];
		memcpy(tempbuffer    , GetHeader(), 6   );
		memcpy(tempbuffer + 6, pBuffer    , size);
		return tempbuffer;
	}
}

char* Packet::DetachPacket() {
	if (completebuffer) {
		if (!m_bSplitted) {
			memcpy(completebuffer, GetHeader(), 6);
		}
		char* result = completebuffer;
		completebuffer = pBuffer = NULL;
		return result;
	} else{
		if (tempbuffer){
			delete[] tempbuffer;
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+10];
		memcpy(tempbuffer,GetHeader(),6);
		memcpy(tempbuffer+6,pBuffer,size);
		char* result = tempbuffer;
		tempbuffer = 0;
		return result;
	}
}

char* Packet::GetHeader() {
	wxASSERT( !m_bSplitted );

	Header_Struct* header = (Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;
	header->packetlength = size + 1;

	return head;
}

char* Packet::GetUDPHeader() {
	wxASSERT( !m_bSplitted );

	memset(head, 0, 6);
	UDP_Header_Struct* header = (UDP_Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;

	return head;
}

void Packet::PackPacket() {
	wxASSERT(!m_bSplitted);

	uLongf newsize = size + 300;
	BYTE* output = new BYTE[newsize];

	wxUint16 result = compress2(output, &newsize, (BYTE*) pBuffer, size, Z_BEST_COMPRESSION);

	if (result != Z_OK || size <= newsize) {
		delete[] output;
		return;
	}

	prot = OP_PACKEDPROT;
	memcpy(pBuffer, output, newsize);
	delete[] output;
	m_bPacked = true;
}

bool Packet::UnPackPacket(UINT uMaxDecompressedSize) {
	wxASSERT( prot == OP_PACKEDPROT );

	wxUint32 nNewSize = size * 10 + 300;

	if (nNewSize > uMaxDecompressedSize){
		//ASSERT(0);
		nNewSize = uMaxDecompressedSize;
	}

	BYTE* unpack = new BYTE[nNewSize];
	uLongf unpackedsize = nNewSize;
	wxUint16 result = uncompress(unpack, &unpackedsize, (BYTE *) pBuffer, size);

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
// STag

STag::STag()
{
	type = 0;
	tagname = NULL;
	stringvalue = NULL;
	intvalue = 0;
	specialtag = 0;
}

STag::STag(const STag& in)
{
	type = in.type;
	tagname = in.tagname!=NULL ? nstrdup(in.tagname) : NULL;
	if (in.type == 2)
		stringvalue = in.stringvalue!=NULL ? nstrdup(in.stringvalue) : NULL;
	else if (in.type == 3)
		intvalue = in.intvalue;
	else if (in.type == 4)
		floatvalue = in.floatvalue;
	else{
		//wxASSERT(0);
		intvalue = 0;
	}

	specialtag = in.specialtag;
}

STag::~STag()
{
	if (tagname) {
		delete[] tagname;
	}
	if (type == 2 && stringvalue) {
		delete[] stringvalue;
	}
}


///////////////////////////////////////////////////////////////////////////////
// CTag

CTag::CTag(LPCSTR name,uint32 intvalue){
	tag.tagname = nstrdup(name);
	tag.type = 3;
	tag.intvalue = intvalue;
}

CTag::CTag(int8 special,uint32 intvalue){
	tag.type = 3;
	tag.intvalue = intvalue;
	tag.specialtag = special;
}

CTag::CTag(LPCSTR name,LPCSTR strvalue){
	tag.tagname = nstrdup(name);
	tag.type = 2;
	tag.stringvalue = nstrdup(strvalue);
}

CTag::CTag(int8 special, LPCSTR strvalue){
	tag.type = 2;
	tag.stringvalue = nstrdup(strvalue);
	tag.specialtag = special;
}

CTag::CTag(LPCSTR name, const wxString& strvalue){
	tag.tagname = nstrdup(name);
	tag.type = 2;
	tag.stringvalue = nstrdup(unicode2char(strvalue));
}

CTag::CTag(uint8 special, const wxString& strvalue){
	tag.type = 2;
	tag.stringvalue = nstrdup(unicode2char(strvalue));
	tag.specialtag = special;
}

CTag::CTag(const STag& in_tag)
	: tag(in_tag)
{
}

#if defined( UNDEFINED )
CTag::CTag(CFile *file)
{
	// Use the constructor below
	CTag(*file);
}
#endif // UNDEDFINED

CTag::CTag(const CFile &in_data)
{
	in_data.Read(&tag.type,1);
	uint16 length;
	in_data.Read(&length,2);
	ENDIAN_SWAP_I_16(length);
	if (length == 1)
		in_data.Read(&tag.specialtag,1);
	else {
		tag.tagname = new char[length+1];
		in_data.Read(tag.tagname,length);
		tag.tagname[length] = 0;
	}

	// NOTE: It's very important that we read the *entire* packet data, even if we do
	// not use each tag. Otherwise we will get troubles when the packets are returned in 
	// a list - like the search results from a server.
	
	if (tag.type == 2){ // STRING
		in_data.Read(&length,2);
		ENDIAN_SWAP_I_16(length);
		tag.stringvalue = new char[length+1];
		in_data.Read(tag.stringvalue,length);
		tag.stringvalue[length] = '\0';
	}
	else if (tag.type == 3){ // DWORD
		in_data.Read(&tag.intvalue,4);
		ENDIAN_SWAP_I_32(tag.intvalue);
	}
	else if (tag.type == 4){ // FLOAT (used by Hybrid 0.48)
		// What to do with them?
		in_data.Read(&tag.floatvalue,4);
	}
	else if (tag.type == 1){ // HASH (never seen)
		printf("CTag::CTag(CFile*); Reading *unverified* HASH tag\n");
		in_data.Seek(16, wxFromCurrent);
	}
	else if (tag.type == 5){ // BOOL (never seen; propably 1 bit)
		// NOTE: This is preventive code, it was never tested
		printf("CTag::CTag(CFile*); Reading *unverified* BOOL tag\n");
		in_data.Seek(1, wxFromCurrent);
	}
	else if (tag.type == 6){ // BOOL Array (never seen; propably <numbits> <bits>)
		// NOTE: This is preventive code, it was never tested
		printf("CTag::CTag(CFile*); Reading *unverified* BOOL Array tag\n");
		uint16 len;
		in_data.Read(&len,2);
		ENDIAN_SWAP_I_16(len);
		in_data.Seek((len+7)/8, wxFromCurrent);
	}
	else if (tag.type == 7){ // BLOB (never seen; propably <len> <byte>)
		// NOTE: This is preventive code, it was never tested
		printf("CTag::CTag(CFile*); Reading *unverified* BLOB tag\n");
		uint16 len;
		in_data.Read(&len,2);
		ENDIAN_SWAP_I_16(len);
		in_data.Seek(len, wxFromCurrent);
	}
	else{
		if (tag.type==0x00) wxASSERT(0);
		if (length == 1)
			printf("CTag::CTag(CFile*); Unknown tag: type=0x%02X  specialtag=%u\n", tag.type, tag.specialtag);
		else
			printf("CTag::CTag(CFile*); Unknown tag: type=0x%02X  name=\"%s\"\n", tag.type, tag.tagname);
	}
}

CTag::~CTag()
{
}

bool CTag::WriteTagToFile(CFile* file)
{
	// don't write tags of unknown types, we wouldn't be able to read them in again 
	// and the met file would be corrupted
	if (tag.type==2 || tag.type==3 || tag.type==4){
		file->Write(&tag.type,1);
		
		if (tag.tagname){
			uint16 taglen= (uint16)strlen(tag.tagname);
			ENDIAN_SWAP_I_16(taglen);
			file->Write(&taglen,2);
			file->Write(tag.tagname,taglen);
		}
		else{
			uint16 taglen = 1;
			ENDIAN_SWAP_I_16(taglen);
			file->Write(&taglen,2);
			file->Write(&tag.specialtag,taglen);
		}

		if (tag.type == 2){
			uint16 len = (uint16)strlen(tag.stringvalue);
			ENDIAN_SWAP_I_16(len);
			file->Write(&len,2);
			file->Write(tag.stringvalue,len);
		}
		else if (tag.type == 3){
			uint32 intvalue_endian = ENDIAN_SWAP_32(tag.intvalue);
			file->Write(&intvalue_endian,4);
		}
		else if (tag.type == 4){
			// What to to with them on ppc?
			file->Write(&tag.floatvalue,4);
		}
		//TODO: Support more tag types
		else{
			printf("CTag::WriteTagToFile(CFile*); Unknown tag: type=0x%02X\n", tag.type);
			//wxASSERT(0);
			return false;
		}
		return true;
	}
	else{
		printf("CTag::WriteTagToFile(CFile*); Ignored tag with unknown type=0x%02X\n", tag.type);
		wxASSERT(0);
		return false;
	}
}

wxString CTag::GetFullInfo() const
{
	wxString strTag;
	if (tag.tagname){
		strTag = _T('\"');
		strTag += (char2unicode(tag.tagname));
		strTag += _T('\"');
	}
	else{
		strTag.Printf(_T("0x%02X"), tag.specialtag);
	}
	strTag += _T("=");
	if (tag.type == 2){
		strTag += _T("\"");
		strTag += (char2unicode(tag.stringvalue));
		strTag += _T("\"");
	}
	else if (tag.type == 3){
		TCHAR szBuff[16];
		snprintf(szBuff, 10, "%i",tag.intvalue);
		strTag += (char2unicode(szBuff));
	}
	else if (tag.type == 4){
		TCHAR szBuff[16];
		snprintf(szBuff, ELEMENT_COUNT(szBuff), "%f", tag.floatvalue);
		strTag += (char2unicode(szBuff));
	}
	else{
		wxString strBuff;
		strBuff.Printf(_T("Type=%u"), tag.type);
		strTag += strBuff;
	}
	return strTag;
}

bool CTag::WriteTagToFile(FILE* file) {
	fputc(tag.type, file);

	if (tag.tagname && (!tag.specialtag)) {
		uint16 taglen = (uint16) strlen(tag.tagname);
		ENDIAN_SWAP_I_16(taglen);
		fwrite(&taglen, 2, 1, file);
		fwrite(tag.tagname, taglen, 1, file);
	} else {
		uint16 taglen = 1;
		ENDIAN_SWAP_I_16(taglen);
		fwrite(&taglen, 2, 1, file);
		fwrite(&tag.specialtag, taglen, 1, file);
	}

	if ( tag.type == 2) {
		uint16 len = (uint16) strlen(tag.stringvalue);
		ENDIAN_SWAP_I_16(len);
		fwrite(&len, 2, 1, file);
		fwrite(tag.stringvalue, len, 1, file);
	} else if (tag.type == 3) {
		uint32 intvalue_endian = ENDIAN_SWAP_32(tag.intvalue);
		fwrite(&intvalue_endian, 4, 1, file);
	}
	
	return ferror(file);
}

CInvalidPacket::CInvalidPacket(const char* reason)
{
	// that was _bad_ practice unless length is guaranteed - Unleashed
	// strcpy(m_acWhat, "(NULL)");

	strncpy(m_acWhat, reason ? reason : "(NULL)", sizeof(m_acWhat));
	m_acWhat[sizeof(m_acWhat) - 1] = '\0';
}

const char* CInvalidPacket::what() const throw()
{
	return m_acWhat;
}

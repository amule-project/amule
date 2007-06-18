//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "RC4Encrypt.h"
#include <common/MD5Sum.h>

#include <stdexcept>

CRC4EncryptableBuffer::CRC4EncryptableBuffer() 
:
m_encrypted(false),
m_hasKey(false),
m_key()
{
}


CRC4EncryptableBuffer::~CRC4EncryptableBuffer()
{
}

void CRC4EncryptableBuffer::Append(const uint8* buffer, int n)
{
	wxASSERT(!m_encrypted);
	if (!m_encrypted) {
		CMemFile::Append(buffer, n);
	} else {
		throw std::runtime_error(
			"(CRC4EncryptableBuffer::Append): "
			"Tryed to append data to an encrypted buffer.");
	}
}


void CRC4EncryptableBuffer::Encrypt()
{
	wxASSERT(!m_encrypted);
	// This is not optimal. At all.
	int n = GetLength();
	byte orig_buffer[n];
	memcpy(orig_buffer, GetRawBuffer(), n);
	RC4Crypt(orig_buffer, GetRawBuffer(), n);
	//DumpMem(orig_buffer, n, wxT("Orig buffer: "));
	//DumpMem(GetRawBuffer() ,n, wxT("Encrypted buffer: "));
	m_encrypted = true;
}

void CRC4EncryptableBuffer::RC4Crypt( const uint8 *pachIn, uint8 *pachOut, uint32 nLen)
{
	wxASSERT( m_hasKey && nLen > 0 );
	
	if (m_hasKey) {
		uint8 byX = m_key.byX;;
		uint8 byY = m_key.byY;
		uint8* pabyState = &m_key.abyState[0];;
		uint8 byXorIndex;
		
		for (uint32 i = 0; i < nLen; ++i) {
			byX = (byX + 1) % 256;
			byY = (pabyState[byX] + byY) % 256;
			std::swap(pabyState[byX], pabyState[byY]);
			byXorIndex = (pabyState[byX] + pabyState[byY]) % 256;
			
			if (pachIn != NULL) {
				pachOut[i] = pachIn[i] ^ pabyState[byXorIndex];
			}
		}
		
		m_key.byX = byX;
		m_key.byY = byY;
	} else {
		throw std::runtime_error(
			"(CRC4EncryptableBuffer::RC4Crypt): "
			"Encrypt() has been called without a previous call"
			"to SetKey().");		
	}
}

uint8 *CRC4EncryptableBuffer::Detach()
{
	int n = GetLength();
	uint8 *ret = new uint8[n];
	memcpy(ret, GetRawBuffer(), n);
	ResetData();
	m_encrypted = false;
	return ret;
}


void CRC4EncryptableBuffer::SetKey(const MD5Sum& keyhash, bool bSkipDiscard)
{
	wxASSERT(!m_hasKey);
	if (!m_hasKey) {
		m_hasKey = true;
		RC4CreateKey(
			keyhash.GetRawHash(),
			16, bSkipDiscard);
	} else {
		throw std::runtime_error(
			"(CRC4EncryptableBuffer::SetKey): "
			"SetKey() has been called twice.");
	}
}


void CRC4EncryptableBuffer::RC4CreateKey(const uint8* pachKeyData, uint32 nLen, bool bSkipDiscard)
{
	uint8 index1;
	uint8 index2;
	uint8* pabyState;

	pabyState= &m_key.abyState[0];
	for (int i = 0; i < 256; ++i) {
		pabyState[i] = (uint8)i;
	}

	m_key.byX = 0;
	m_key.byY = 0;
	index1 = 0;
	index2 = 0;
	
	for (int i = 0; i < 256; ++i) {
		index2 = (pachKeyData[index1] + pabyState[i] + index2) % 256;
		std::swap(pabyState[i], pabyState[index2]);
		index1 = (uint8)((index1 + 1) % nLen);
	}
	
	if (!bSkipDiscard) {
		RC4Crypt(NULL, NULL, 1024);
	}
}

void CRC4EncryptableBuffer::ResetData()
{
	m_encrypted = false;
	// Should we clear the keys?
	CMemFile::ResetData();
}

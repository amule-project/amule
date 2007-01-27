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


#include "RC4Encrypt.h"


#include <stdexcept>


///////////////////////////////////////////////////////////////////////////////
// RC4 Encryption
//


RC4_Key_Struct* RC4CreateKey(
	const uint8* pachKeyData,
	uint32 nLen,
	RC4_Key_Struct *key,
	bool bSkipDiscard);


void RC4Crypt(
	const uint8 *pachIn,
	uint8 *pachOut,
	uint32 nLen,
	RC4_Key_Struct *key);


RC4_Key_Struct* RC4CreateKey(
	const uint8* pachKeyData,
	uint32 nLen,
	RC4_Key_Struct *key,
	bool bSkipDiscard)
{
	uint8 index1;
	uint8 index2;
	uint8* pabyState;

	if (key == NULL) {
		key = new RC4_Key_Struct;
	}

	pabyState= &key->abyState[0];
	for (int i = 0; i < 256; ++i) {
		pabyState[i] = (uint8)i;
	}

	key->byX = 0;
	key->byY = 0;
	index1 = 0;
	index2 = 0;
	
	for (int i = 0; i < 256; ++i) {
		index2 = (pachKeyData[index1] + pabyState[i] + index2) % 256;
		std::swap(pabyState[i], pabyState[index2]);
		index1 = (uint8)((index1 + 1) % nLen);
	}
	
	if (!bSkipDiscard) {
		RC4Crypt(NULL, NULL, 1024, key);
	}
	
	return key;
}


void RC4Crypt(
	const uint8 *pachIn,
	uint8 *pachOut,
	uint32 nLen,
	RC4_Key_Struct *key)
{
	wxASSERT( key != NULL && nLen > 0 );
	
	if (key == NULL) {
		return;
	}
	
	uint8 byX = key->byX;;
	uint8 byY = key->byY;
	uint8* pabyState = &key->abyState[0];;
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
	
	key->byX = byX;
	key->byY = byY;
}


CRC4EncryptableBuffer::CRC4EncryptableBuffer()
:
std::vector<uint8>(0),
m_encrypted(false),
m_hasKey(false),
m_key(),
m_tmpBuf(0)
{
}


CRC4EncryptableBuffer::~CRC4EncryptableBuffer()
{
}


bool CRC4EncryptableBuffer::IsEmpty()
{
	return empty();
}


void CRC4EncryptableBuffer::Append(uint8* buffer, int n)
{
	wxASSERT(!m_encrypted);
	if (!m_encrypted) {
		for(int i = 0; i < n; ++i) {
			push_back(buffer[i]);
		}
	} else {
		throw std::runtime_error(
			"(CRC4EncryptableBuffer::Append): "
			"Tryed to append data to an encrypted buffer.");
	}
}


void CRC4EncryptableBuffer::SetKey(MD5Sum keyhash)
{
	wxASSERT(!m_hasKey);
	if (!m_hasKey) {
		m_hasKey = true;
		RC4CreateKey(
			(const uint8 *)((const char *)keyhash.GetRawHash()),
			16, &m_key, false);
	} else {
		throw std::runtime_error(
			"(CRC4EncryptableBuffer::SetKey): "
			"SetKey() has been called twice.");
	}
}


void CRC4EncryptableBuffer::Encrypt()
{
	wxASSERT(m_hasKey);
	if (m_hasKey) {
		uint32 n = GetSize();
		std::vector<uint8> &outBuf = dynamic_cast<std::vector<uint8> &>(*this);
		std::swap(m_tmpBuf, outBuf);
		RC4Crypt(&m_tmpBuf[0], &outBuf[0], n, &m_key);
	} else {
		throw std::runtime_error(
			"(CRC4EncryptableBuffer::Encrypt): "
			"Encrypt() has been called without a previous call"
			"to SetKey().");
	}
}


size_t CRC4EncryptableBuffer::GetSize()
{
	return size();
}


uint8 *CRC4EncryptableBuffer::Detach()
{
	int n = GetSize();
	uint8 *ret = new uint8[n];
	memcpy(ret, &this[0], n);
	resize(0);
	m_encrypted = false;

	return ret;
}


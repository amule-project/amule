//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "RLE.h"
#include "ArchSpecific.h"
#include "ScopedPtr.h"
#include <ec/cpp/ECTag.h>		// Needed for CECTag

/*
 * RLE encoder implementation. This is RLE implementation for very specific
 * purpose: encode DIFFERENCE between subsequent states of status bar.
 * 
 * This difference is calculated by xor-ing with previous data
 * 
 * We can't use implementation with "control char" since this encoder
 * will process binary data - not ascii (or unicode) strings
 */
void RLE_Data::setup(int len, bool use_diff, uint8 * content)
{
	m_len = len;
	m_use_diff = use_diff;

	if (m_len) {
		m_buff = new uint8[m_len];
		if (content) {
			memcpy(m_buff, content, m_len);
		} else {
			memset(m_buff, 0, m_len);
		}
		//
	} else {
		m_buff = 0;
	}
}

RLE_Data &RLE_Data::operator=(const RLE_Data &obj)
{
	if (this == &obj)
		return *this;

	delete [] m_buff;
	setup(obj.m_len, obj.m_use_diff, obj.m_buff);

	return *this;
}

RLE_Data::~RLE_Data()
{
	delete [] m_buff;
}

void RLE_Data::ResetEncoder()
{
	delete m_buff;
	m_len = 0;
	m_buff = 0;
}

bool RLE_Data::Realloc(int size)
{
	if ( size == m_len ) {
		return false;
	}
	if (size == 0) {
		delete [] m_buff;
		m_buff = 0;
		m_len = 0;
		return true;
	}
	uint8 *buff = new uint8[size];
	if (m_len == 0) {
		memset(buff, 0, size);
	} else if ( size > m_len ) {
		memset(buff + m_len, 0, size - m_len);
		memcpy(buff, m_buff, m_len);
	} else {
		memcpy(buff, m_buff, size);
	}
	delete [] m_buff;
	m_buff = buff;
	
	m_len = size;
	return true;
}

const uint8 *RLE_Data::Decode(const uint8 *buff, int len)
{
	uint8 * decBuf = m_len ? new uint8[m_len] : 0;

	// If data exceeds the buffer, switch to counting only.
	// Then resize and make a second pass.
	for (bool overrun = true; overrun;) {
		overrun = false;
		int j = 0;
		for (int i = 0; i < len;) {
			if (i < len - 2 && buff[i+1] == buff[i]) {
				// This is a sequence.
				uint8 seqLen = buff[i + 2];
				if (j + seqLen <= m_len) {
					memset(decBuf + j, buff[i], seqLen);
				}
				j += seqLen;
				i += 3;
			} else {
				// This is a single byte.
				if (j < m_len) {
					decBuf[j] = buff[i];
				}
				j++;
				i++;
			}
		}
		if (j != m_len) {
			overrun = j > m_len;	// overrun, make a second pass
			Realloc(j);				// size has changed, adjust
			if (overrun) {
				delete[] decBuf;
				decBuf = new uint8[m_len];
			}
		}
	}
	//
	// Recreate data from diff
	//
	if ( m_use_diff ) {
		for (int k = 0; k < m_len; k++) {
			m_buff[k] ^= decBuf[k];
		}
	} else {
		memcpy(m_buff, decBuf, m_len);
	}
	delete[] decBuf;
	return m_buff;
}

const uint8 * RLE_Data::Encode(const uint8 *data, int inlen, int &outlen, bool &changed)
{
	changed = Realloc(inlen);		// adjust size if necessary

	if (m_len == 0) {
		outlen = 0;
		return NULL;
	}
	//
	// calculate difference from prev
	//
	if ( m_use_diff ) {
		for (int i = 0; i < m_len; i++) {
			m_buff[i] ^= data[i];
			if (m_buff[i]) {
				changed = true;
			}
		}
	} else {
		memcpy(m_buff, data, m_len);
		changed = true;
	}
	
	//
	// now RLE
	//
	// In worst case 2-byte sequence is encoded as 3. So, data can grow by 50%.
	uint8 * enc_buff = new uint8[m_len * 3/2 + 1];
	int i = 0, j = 0;
	while ( i != m_len ) {
		uint8 curr_val = m_buff[i];
		int seq_start = i;
		while ( (i != m_len) && (curr_val == m_buff[i]) && ((i - seq_start) < 0xff)) {
			i++;
		}
		if (i - seq_start > 1) {
			// if there's 2 or more equal vals - put it twice in stream
			enc_buff[j++] = curr_val;
			enc_buff[j++] = curr_val;
			enc_buff[j++] = i - seq_start;
		} else {
			// single value - put it as is
			enc_buff[j++] = curr_val;
		}
	}

	outlen = j;
	
	//
	// If using differential encoder, remember current data for
	// later use
	if ( m_use_diff ) {
		memcpy(m_buff, data, m_len);
	}
	
	return enc_buff;
}

const uint8 * RLE_Data::Encode(const ArrayOfUInts16 &data, int &outlen, bool &changed)
{
	// To encode, first copy the UInts16 to a uint8 array
	// and limit them to 0xff.
	// The encoded size is the size of data.
	int size = (int) data.size();
	if (size == 0) {
		return Encode(0, 0, outlen, changed);
	}
	CScopedArray<uint8> buf(size);
	uint8 * bufPtr = buf.get();

	for (int i = 0; i < size; i++) {
		uint16 ui = data[i];
		bufPtr[i] = (ui > 0xff) ? 0xff : (uint8) ui;
	}
	return Encode(bufPtr, size, outlen, changed);
}

const uint8 * RLE_Data::Encode(const ArrayOfUInts64 &data, int &outlen, bool &changed)
{
	// uint64 is copied to a uint8 buffer
	// first all low bytes, then all second low bytes and so on
	// so inital RLE will benefit from high bytes being equal (zero)
	// 0x000003045A6A7A8A, 0x000003045B6B7B8B
	// 8A8B7A7B6A6B5A5B0404030300000000
	int size = (int) data.size();
	if (size == 0) {
		return Encode(0, 0, outlen, changed);
	}
	CScopedArray<uint8> buf(size * 8);
	uint8 * bufPtr = buf.get();
	for (int i = 0; i < size; i++) {
		uint64 u = data[i];
		for (int j = 0; j < 8; j++) {
			bufPtr[i + j * size] = u & 0xff;
			u >>= 8;
		}
	}
	return Encode(bufPtr, size * 8, outlen, changed);
}

void RLE_Data::Decode(const uint8 *data, int len, ArrayOfUInts64 &outdata)
{
	const uint8 * decoded = Decode(data, len);
	wxASSERT(m_len % 8 == 0);
	int size = m_len / 8;
	outdata.resize(size);
	for (int i = 0; i < size; i++) {
		uint64 u = 0;
		for (int j = 8; j--;) {
			u <<= 8;
			u |= decoded[i + j * size];
		}
		outdata[i] = u;
	}
}

void PartFileEncoderData::DecodeParts(const CECTag * tag, ArrayOfUInts16 &outdata)
{
	const uint8 * buf = m_part_status.Decode((uint8 *)tag->GetTagData(), tag->GetTagDataLen());
	int size = m_part_status.Size();
	outdata.resize(size);
	for (int i = 0; i < size; i++) {
		outdata[i] = buf[i];
	}
}

void PartFileEncoderData::DecodeGaps(const CECTag * tag, ArrayOfUInts64 &outdata)
{
	m_gap_status.Decode((uint8 *)tag->GetTagData(), tag->GetTagDataLen(), outdata);
}

void PartFileEncoderData::DecodeReqs(const CECTag * tag, ArrayOfUInts64 &outdata)
{
	m_req_status.Decode((uint8 *)tag->GetTagData(), tag->GetTagDataLen(), outdata);
}


// File_checked_for_headers

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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
		// in worst case 2-byte sequence encoded as 3. So, data can grow by 50%
		m_enc_buff = new uint8[m_len*3/2 + 1];
	} else {
		m_buff = m_enc_buff = 0;
	}
}

RLE_Data &RLE_Data::operator=(const RLE_Data &obj)
{
	if (this == &obj)
		return *this;

	delete [] m_buff;
	delete [] m_enc_buff;
	setup(obj.m_len, obj.m_use_diff, obj.m_buff);

	return *this;
}

RLE_Data::~RLE_Data()
{
	delete [] m_buff;
	delete [] m_enc_buff;
}

void RLE_Data::Realloc(int size)
{
	if ( size == m_len ) {
		return;
	}

	uint8 *buff = new uint8[size];
	if ( size > m_len ) {
		memset(buff + m_len, 0, size - m_len);
		memcpy(buff, m_buff, m_len);
	} else {
		memcpy(buff, m_buff, size);
	}
	delete [] m_buff;
	m_buff = buff;
	
	// m_enc_buff doesn't need to be copied over
	delete [] m_enc_buff;
	m_enc_buff = new uint8[size*3/2 + 1];

	m_len = size;
}

const uint8 *RLE_Data::Decode(const uint8 *buff, int len)
{
	//
	// Open RLE
	//

	int i = 0, j = 0;
	while ( j != m_len ) {

		if ( i < (len -1) ) {
			if (buff[i+1] == buff[i]) {
				// this is sequence
				memset(m_enc_buff + j, buff[i], buff[i + 2]);
				j += buff[i + 2];
				i += 3;
			} else {
				// this is single byte
				m_enc_buff[j++] = buff[i++];
			}
		} else {
			// only 1 byte left in encoded data - it can't be sequence
			m_enc_buff[j++] = buff[i++];
			// if there's no more data, but buffer end is not reached,
			// it must be error in some point
			if ( j != m_len ) {
				printf("RLE_Data: decoding error. %d bytes decoded to %d instead of %d\n", len, j, m_len);
			}
			break;
		}
	}	
	//
	// Recreate data from diff
	//
	if ( m_use_diff ) {
		for (int k = 0; k < m_len; k++) {
			m_buff[k] ^= m_enc_buff[k];
		}
		return m_buff;
	} else {
		return m_enc_buff;
	}
}

const uint8 * RLE_Data::Encode(const uint8 *data, int &outlen)
{
	//
	// calculate difference from prev
	//
	if ( m_use_diff ) {
		for (int i = 0; i < m_len; i++) {
			m_buff[i] ^= data[i];
		}
	} else {
		memcpy(m_buff, data, m_len);
	}
	
	//
	// now RLE
	//
	int i = 0, j = 0;
	while ( i != m_len ) {
		uint8 curr_val = m_buff[i];
		int seq_start = i;
		while ( (i != m_len) && (curr_val == m_buff[i]) && ((i - seq_start) < 0xff)) {
			i++;
		}
		if (i - seq_start > 1) {
			// if there's 2 or more equal vals - put it twice in stream
			m_enc_buff[j++] = curr_val;
			m_enc_buff[j++] = curr_val;
			m_enc_buff[j++] = i - seq_start;
		} else {
			// single value - put it as is
			m_enc_buff[j++] = curr_val;
		}
	}

	outlen = j;
	
	//
	// If using differential encoder, remember current data for
	// later use
	if ( m_use_diff ) {
		memcpy(m_buff, data, m_len);
	}
	
	return m_enc_buff;
}

const uint8 * RLE_Data::Encode(const ArrayOfUInts16 &data, int &outlen)
{
	// To encode, first copy the UInts16 to a uint8 array
	// and limit them to 0xff.
	// The encoded size is always m_len.
	int size = (int) data.size();
	CScopedPtr<uint8> buf(new uint8[m_len]);
	uint8 * bufPtr = buf.get();

	for (int i = 0; i < m_len; i++) {
		uint16 ui = (i < size) ? data[i] : 0;
		bufPtr[i] = (ui > 0xff) ? 0xff : (uint8) ui;
	}
	return Encode(bufPtr, outlen);
}

void PartFileEncoderData::Decode(uint8 *gapdata, int gaplen, uint8 *partdata, int partlen)
{
	m_part_status.Decode(partdata, partlen);

	// in a first dword - real size
	uint32 gapsize = ENDIAN_NTOHL( RawPeekUInt32( gapdata ) );
	gapdata += sizeof(uint32);
	m_gap_status.Realloc(gapsize*2*sizeof(uint64));

	m_gap_status.Decode(gapdata, gaplen - sizeof(uint32));
}


// File_checked_for_headers

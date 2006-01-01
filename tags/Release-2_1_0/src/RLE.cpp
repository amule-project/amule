//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


/*
 * RLE encoder implementation. This is RLE implementation for very specific
 * purpose: encode DIFFERENCE between subsequent states of status bar.
 * 
 * This difference is calculated by xor-ing with previous data
 * 
 * We can't use implementation with "control char" since this encoder
 * will process binary data - not ascii (or unicode) strings
 */
RLE_Data::RLE_Data(int len, bool use_diff)
{
	m_len = len;
	m_use_diff = use_diff;
	
	m_buff = new unsigned char[m_len];
	memset(m_buff, 0, m_len);
	//
	// in worst case 2-byte sequence encoded as 3. So, data can grow at 1/3
	m_enc_buff = new unsigned char[m_len*4/3 + 1];
}

RLE_Data::RLE_Data()
{
	m_buff = 0;
	m_enc_buff = 0;
	m_len = 0;
	m_use_diff = 0;
}

RLE_Data::RLE_Data(const RLE_Data &obj)
{
	m_len = obj.m_len;
	m_use_diff = obj.m_use_diff;

	m_buff = new unsigned char[m_len];
	memcpy(m_buff, obj.m_buff, m_len);
	
	m_enc_buff = new unsigned char[m_len*4/3 + 1];
}

RLE_Data &RLE_Data::operator=(const RLE_Data &obj)
{
	m_len = obj.m_len;
	
	m_use_diff = obj.m_use_diff;

	m_buff = new unsigned char[m_len];
	memcpy(m_buff, obj.m_buff, m_len);
	
	m_enc_buff = new unsigned char[m_len*4/3 + 1];
	
	return *this;
}

RLE_Data::~RLE_Data()
{
	if ( m_buff ) {
		delete [] m_buff;
	}
	if ( m_enc_buff ) {
		delete [] m_enc_buff;
	}
}

void RLE_Data::Realloc(int size)
{
	if ( size == m_len ) {
		return;
	}

	unsigned char *buff = new unsigned char[size];
	if ( size > m_len ) {
		memset(buff + m_len, 0, size - m_len);
		memcpy(buff, m_buff, m_len);
	} else {
		memcpy(buff, m_buff, size);
	}
	delete [] m_buff;
	m_buff = buff;
	
	buff = new unsigned char[size*4/3 + 1];
	if ( size > m_len ) {
		memset(buff + m_len*4/3 + 1, 0, (size - m_len)*4/3);
		memcpy(buff, m_enc_buff, m_len*4/3 + 1);
	} else {
		memcpy(buff, m_enc_buff, size*4/3 + 1);
	}
	delete [] m_enc_buff;
	m_enc_buff = buff;

	m_len = size;
}

const unsigned char *RLE_Data::Decode(const unsigned char *buff, int len)
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
	}
		
	return m_buff;
}

void PartFileEncoderData::Decode(unsigned char *gapdata, int gaplen, unsigned char *partdata, int partlen)
{
	m_part_status.Decode(partdata, partlen);

	// in a first dword - real size
	uint32 gapsize = ENDIAN_NTOHL( RawPeekUInt32( gapdata ) );
	gapdata += sizeof(uint32);
	m_gap_status.Realloc(gapsize*2*sizeof(uint32));

	m_gap_status.Decode(gapdata, gaplen - sizeof(uint32));
}

unsigned char RLE_Data_BV::m_buff[256];

RLE_Data_BV::RLE_Data_BV(int len) : m_last_buff(len)
{
}

int RLE_Data_BV::Encode(std::vector<bool> &data)
{
	unsigned char *curr = m_buff;
	std::vector<bool>::const_iterator i = data.begin();
	std::vector<bool>::const_iterator j = m_last_buff.begin();
	while( i != data.end() ) {
		unsigned char count = 0;
		while ( (i != data.end()) && ( (*i ^ *j) == false) ) {
			count++;
			i++;
			j++;
		}
		*curr++ = count;
	}
	m_last_buff = data;
	return 0;
}


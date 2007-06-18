//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef RLE_H
#define RLE_H


#include "Types.h"

/*!
 * General purpose RLE implementation. Just encode or create
 * differential data with previous
 */
class RLE_Data
{
public:
	RLE_Data(int len, bool use_diff);
	
	// those constructors are for stl containers
	RLE_Data();
	RLE_Data(const RLE_Data &);
	RLE_Data &operator=(const RLE_Data &);
	
	~RLE_Data();
	
	const unsigned char *Encode(unsigned char *data, int &outlen)
	{
		return EncodeT<unsigned char *>(data, m_len, outlen);
	}
	
	const unsigned char *Encode(ArrayOfUInts16 &data, int &outlen)
	{
		return EncodeT<ArrayOfUInts16>(data, data.size(), outlen);
	}
	
	const unsigned char *Decode(const unsigned char *data, int len);	
	
	void ResetEncoder()
	{
		memset(m_buff, 0, m_len);
	}

	// change size of internal buffers
	void Realloc(int size);
	
	// decoder will need access to data
	const unsigned char *Buffer() { return m_buff; }
	int Size() { return m_len; }

private:	
	unsigned char *m_buff, *m_enc_buff;
	bool m_use_diff;
	int m_len, m_enc_len;
	
	// data is bounded by srclen. everything above considered == 0
	template <class T> const unsigned char *EncodeT(T &buff, int srclen, int &outlen)
	{
		//
		// calculate difference from prev
		//
		if ( m_use_diff ) {
			for (int i = 0; i < m_len; i++) {
				m_buff[i] ^= (i < srclen ) ? ((unsigned char)buff[i]) : 0;
			}
		} else {
			//
			// can't use memcpy - in case of generic class T this
			// will rely on "operator []" implementation
			for(int i = 0; i < m_len;i++) {
				m_buff[i] = (i < srclen ) ? ((unsigned char)buff[i]) : 0;
			}
		}
		
		//
		// now RLE
		//
		int i = 0, j = 0;
		while ( i != m_len ) {
			unsigned char curr_val = m_buff[i];
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
			//
			// can't use memcpy - in case of generic class T this
			// will rely on "operator []" implementation
			for(int k = 0; k < m_len;k++) {
				m_buff[k] = (k < srclen ) ? ((unsigned char)buff[k]) : 0;
			}
		}
		
		return m_enc_buff;
	}
};


/*
 * Another implementation of RLE, optimized for bit-vector. In this RLE flavor we
 * have only 2 values, so we don't need to transmit the value itself. Since most
 * of the time, bitmap will contail all zeros with few 1's, only zeros will be encoded.
 * Meaning that '0000110010000' is encoded as '4024'
 */
class RLE_Data_BV
{
public:
	RLE_Data_BV(int len);
	RLE_Data_BV();
	RLE_Data_BV(const RLE_Data_BV &);
	
	~RLE_Data_BV();
	
	RLE_Data_BV &operator=(const RLE_Data_BV &);
	
	int Encode(std::vector<bool> &data);
	void Decode(unsigned char *data, int datalen, std::vector<bool> &outbuff);
	
	const unsigned char *Buffer() { return m_buff; }
private:
	// maximum file size in amule is 4G since it uses uint32 as filesize. So, it
	// can be up to 4Gb/PARTSIZE=442 parts. Worst case is 1/0 interleaving,
	// producing 221 byte RLE encoded output.
	static unsigned char m_buff[256];

	std::vector<bool> m_last_buff;
	
	void Realloc(int size);
};


/*!
 * Data difference is different for each EC client
 */
class PartFileEncoderData {
public:
	RLE_Data m_part_status;
	RLE_Data m_gap_status;
	
	//
	// Encoder may reset history if full info requested
	void ResetEncoder()
	{
		m_part_status.ResetEncoder();
		m_gap_status.ResetEncoder();
	}
	
	//
	// decoder side - can be used everywhere
	void Decode(unsigned char *gapdata, int gaplen, unsigned char *partdata, int partlen);
	
	PartFileEncoderData() { }
	PartFileEncoderData(int part_count, int gap_count) :
		m_part_status(part_count, true), m_gap_status(gap_count*sizeof(uint64), true)
	{
	}
		
	// for stl
	PartFileEncoderData(const PartFileEncoderData &obj) :
		m_part_status(obj.m_part_status), m_gap_status(obj.m_gap_status)
	{
	}
	
	PartFileEncoderData &operator=(const PartFileEncoderData &obj)
	{
		m_part_status = obj.m_part_status;
		m_gap_status = obj.m_gap_status;
		return *this;
	}
};

#endif

// File_checked_for_headers

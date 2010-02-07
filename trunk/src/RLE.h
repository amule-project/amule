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
	RLE_Data(int len, bool use_diff)	{ setup(len, use_diff); }
	
	// those constructors are for stl containers
	RLE_Data()	{ setup(0, 0); }
	RLE_Data(const RLE_Data & obj)	{ setup(obj.m_len, obj.m_use_diff, obj.m_buff); }
	RLE_Data &operator=(const RLE_Data &);
	
	~RLE_Data();

	const uint8 *Encode(const uint8 *data, int inlen, int &outlen);

	const uint8 *Encode(const ArrayOfUInts16 &data, int &outlen);
	
	const uint8 *Decode(const uint8 *data, int len);	
	
	void ResetEncoder()
	{
		memset(m_buff, 0, m_len);
	}

	// change size of internal buffers
	void Realloc(int size);
	
	// decoder will need access to data
	const uint8 *Buffer() { return m_buff; }
	int Size() { return m_len; }

private:
	void setup(int len, bool use_diff, uint8 * content = 0);
	
	// Encode: source data (original or diff in diff mode)
	// Decode: store last data (differential only)
	uint8 *m_buff;
	// Encode: stores packed data
	// Decode: input gets unpacked here
	uint8 *m_enc_buff;
	// Unpacked size
	int m_len;
	// Use differential encoding
	bool m_use_diff;
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
	void DecodeParts(uint8 *partdata, int partlen) { m_part_status.Decode(partdata, partlen); }
	void DecodeGaps(uint8 *gapdata, int gaplen);
	
	PartFileEncoderData() :
		m_part_status(0, true), m_gap_status(0, true)
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

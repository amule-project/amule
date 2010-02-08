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
	RLE_Data(int len = 0, bool use_diff = true)	{ setup(len, use_diff); }
	
	// those constructors are for stl containers
	RLE_Data(const RLE_Data & obj)	{ setup(obj.m_len, obj.m_use_diff, obj.m_buff); }
	RLE_Data &operator=(const RLE_Data &);
	
	~RLE_Data();

	const uint8 *Encode(const uint8 *data, int inlen, int &outlen, bool &changed);

	const uint8 *Encode(const ArrayOfUInts16 &data, int &outlen, bool &changed);
	const uint8 *Encode(const ArrayOfUInts64 &data, int &outlen, bool &changed);
	
	const uint8 *Decode(const uint8 *data, int len);
	void Decode(const uint8 *data, int len, ArrayOfUInts64 &outdata);
	
	void ResetEncoder()
	{
		if (m_len) {
			memset(m_buff, 0, m_len);
		}
	}
	
	// decoder will need access to data
	const uint8 *Buffer() const	{ return m_buff; }
	int Size() const	{ return m_len; }

private:
	void setup(int len, bool use_diff, uint8 * content = 0);

	// change size of internal buffers
	// returns true if size was changed
	bool Realloc(int size);
	
	// Encode: source data (original or diff in diff mode)
	// Decode: store decoded data
	uint8 *m_buff;
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
	void DecodeGaps(const class CECTag * tag, ArrayOfUInts64 &outdata);
};

#endif

// File_checked_for_headers

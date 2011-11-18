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

	const uint8 *Encode(const ArrayOfUInts16 &data, int &outlen, bool &changed);
	const uint8 *Encode(const ArrayOfUInts64 &data, int &outlen, bool &changed);
	
	const uint8 *Decode(const uint8 *data, int len);
	void Decode(const uint8 *data, int len, ArrayOfUInts64 &outdata);
	
	void ResetEncoder();

	// decoder will need access to data
	const uint8 *Buffer() const	{ return m_buff; }
	int Size() const	{ return m_len; }

private:
	void setup(int len, bool use_diff, uint8 * content = 0);

	// change size of internal buffers
	// returns true if size was changed
	bool Realloc(int size);

	//
	// Encode some raw data
	//
	// data:	block to encode
	// inlen:	number of bytes to encode. May be zero, then data can also be 0.
	// outlen:	here the number of encoded bytes gets stored (0 if inlen is 0)
	// changed:	becomes true if the size has changed or a change in the data occured,
	//          so the differential data (before encoding) is not all zero
	//
	// return:	new buffer with encoded data, must be deleted after use!
	//
	const uint8 *Encode(const uint8 *data, int inlen, int &outlen, bool &changed);

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
protected:
	// number of sources for each part for progress bar colouring
	RLE_Data m_part_status;
	// gap list
	RLE_Data m_gap_status;
	// blocks requested for download
	RLE_Data m_req_status;

public:
	//
	// decoder side - can be used everywhere
	void DecodeParts(const class CECTag * tag, ArrayOfUInts16 &outdata);
	void DecodeGaps(const class CECTag * tag, ArrayOfUInts64 &outdata);
	void DecodeReqs(const class CECTag * tag, ArrayOfUInts64 &outdata);
};

#endif

// File_checked_for_headers

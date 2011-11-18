//
// This file is part of the aMule Project.
//
// Copyright (c) 2009-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2009-2011 Stu Redman ( sturedman@amule.org )
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

#ifndef BITVECTOR_H
#define BITVECTOR_H

//
// Packed bit vector
//
class BitVector {
public:
	BitVector()
	{
		m_bits	= 0;
		m_bytes = 0;
		m_allTrue = 0;
		m_vector = NULL;
	}

	~BitVector() { clear();	}

	// number of bits
	uint32 size() const	{ return m_bits; }

	// is it empty?
	bool empty() const { return m_bits == 0; }

	// get bit at index
	bool get(uint32 idx) const
	{
		if (idx >= m_bits) {
			wxFAIL;
			return false;
		}
		return (m_vector[idx / 8] & s_posMask[idx & 7]) != 0;
	}

	// set bit at index
	void set(uint32 idx, bool value)
	{
		if (idx >= m_bits) {
			wxFAIL;
			return;
		}
		uint32 bidx = idx / 8;
		if (value) {
			m_vector[bidx] = m_vector[bidx] | s_posMask[idx & 7];
			// If it was not all true before, then we don't know now.
			if (m_allTrue == 0) {
				m_allTrue = 2;
			}
		} else {
			m_vector[bidx] = m_vector[bidx] & s_negMask[idx & 7];
			m_allTrue = 0;
		}
	}

	// set number of bits to zero and free memory
	void clear()
	{
		m_bits	= 0;
		m_bytes = 0;
		m_allTrue = 0;
		delete[] m_vector;
		m_vector = NULL;
	}

	// set number of bits and initialize them with value
	void setsize(uint32 newsize, bool value)
	{
		if (newsize == 0) {
			clear();
			return;
		}
		m_bits = newsize;
		m_bytes = newsize / 8;
		if (newsize & 7) {
			m_bytes++;
		}
		delete[] m_vector;
		m_vector = new uint8[m_bytes];
		memset(m_vector, value ? 0xFF : 0, m_bytes);
		m_allTrue = value ? 1 : 0;
	}

	// are all bits true ?
	bool AllTrue() const
	{
		if (m_allTrue == 2) {
			// don't know, have to check
			bool foundFalse = false;
			uint32 lastByte = m_bytes;
			if (m_bits & 7) {
				// uneven: check bits of last byte individually
				lastByte--;
				for (uint32 i = m_bits & 0xfffffff8; !foundFalse && i < m_bits; i++) {
					foundFalse = !get(i);
				}
			}
			// check bytewise
			for (uint32 i = 0; !foundFalse && i < lastByte; i++) {
				foundFalse = m_vector[i] != 0xff;
			}
			// This is really just a caching of information, 
			// so m_allTrue is mutable and AllTrue() still const.
			m_allTrue = foundFalse ? 0 : 1;
		}
		return m_allTrue == 1;
	}

	// set all bits to true
	void SetAllTrue() { if (m_bytes) { memset(m_vector, 0xFF, m_bytes); } }

	// handling of the internal buffer (for EC)
	// get size
	uint32 SizeBuffer() const { return m_bytes; }
	// get buffer
	const void* GetBuffer() const { return m_vector; }
	// set buffer
	void SetBuffer(const void* src) { memcpy(m_vector, src, m_bytes); m_allTrue = 2; }

private:
	uint32	m_bits;			// number of bits
	uint32	m_bytes;		// number of bytes in the vector
	uint8 *	m_vector;		// the storage
	mutable uint8 m_allTrue;// All true ? 0: no  1: yes  2: don't know
	static const uint8 s_posMask[]; // = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}; implemented in OtherFunctions.cpp
	static const uint8 s_negMask[]; // = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};
};

#endif

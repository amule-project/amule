//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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


// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "UInt128.h"

#include "../../ArchSpecific.h"
#include <common/Format.h>	// Needed for CFormat


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CUInt128::CUInt128(const CUInt128 &value, unsigned numBits)
{
	// Copy the whole uint32s
	unsigned numULONGs = numBits / 32;
	for (unsigned i = 0; i < numULONGs; i++) {
		Set32BitChunk(i, value.Get32BitChunk(i));
	}

	// Copy the remaining bits
	for (unsigned i = numULONGs * 32; i < numBits; i++) {
		SetBitNumber(i, value.GetBitNumber(i));
	}

	// Fill the remaining bits of the current 32-bit chunk with random bits
	// (Not seeding based on time to allow multiple different ones to be created in quick succession)
	numULONGs = (numBits + 31) / 32;
	for (unsigned i = numBits; i < numULONGs * 32; i++) {
		SetBitNumber(i, rand() % 2);
	}

	// Pad with random bytes
	for (unsigned i = numULONGs; i < 3; i++) {
		Set32BitChunk(i, rand());
	}
}

wxString CUInt128::ToHexString() const
{
	wxString str;

	for (int i = 3; i >= 0; i--) {
		str.Append(CFormat(wxT("%08X")) % m_data.u32_data[i]);
	}

	return str;
}

wxString CUInt128::ToBinaryString(bool trim) const
{
	wxString str;
	str.Alloc(128);
	int b;
	for (int i = 0; i < 128; i++) {
		b = GetBitNumber(i);
		if ((!trim) || (b != 0)) {
			str.Append(b ? wxT("1") : wxT("0"));
			trim = false;
		}
	}
	if (str.Len() == 0) {
		str = wxT("0");
	}
	return str;
}

CUInt128& CUInt128::SetValueBE(const uint8_t *valueBE) throw()
{
	m_data.u32_data[3] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE));
	m_data.u32_data[2] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE + 4));
	m_data.u32_data[1] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE + 8));
	m_data.u32_data[0] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE + 12));

	return *this;
}

void CUInt128::ToByteArray(uint8_t *b) const
{
	wxCHECK_RET(b != NULL, wxT("Destination buffer missing."));

	RawPokeUInt32(b,      wxUINT32_SWAP_ON_LE(m_data.u32_data[3]));
	RawPokeUInt32(b + 4,  wxUINT32_SWAP_ON_LE(m_data.u32_data[2]));
	RawPokeUInt32(b + 8,  wxUINT32_SWAP_ON_LE(m_data.u32_data[1]));
	RawPokeUInt32(b + 12, wxUINT32_SWAP_ON_LE(m_data.u32_data[0]));
}

void CUInt128::StoreCryptValue(uint8_t *buf) const
{
	wxCHECK_RET(buf != NULL, wxT("Destination buffer missing."));

	RawPokeUInt32(buf,      wxUINT32_SWAP_ON_BE(m_data.u32_data[3]));
	RawPokeUInt32(buf + 4,  wxUINT32_SWAP_ON_BE(m_data.u32_data[2]));
	RawPokeUInt32(buf + 8,  wxUINT32_SWAP_ON_BE(m_data.u32_data[1]));
	RawPokeUInt32(buf + 12, wxUINT32_SWAP_ON_BE(m_data.u32_data[0]));
}

int CUInt128::CompareTo(const CUInt128 &other) const throw()
{
	for (int i = 3; i >= 0; i--) {
	    if (m_data.u32_data[i] < other.m_data.u32_data[i])
			return -1;
	    if (m_data.u32_data[i] > other.m_data.u32_data[i])
			return 1;
	}
	return 0;
}

int CUInt128::CompareTo(uint32_t value) const throw()
{
	if ((m_data.u64_data[1] > 0) || (m_data.u32_data[1] > 0) || (m_data.u32_data[0] > value))
		return 1;
	if (m_data.u32_data[0] < value)
		return -1;
	return 0;
}

CUInt128& CUInt128::Add(const CUInt128 &value) throw()
{
	if (value.IsZero()) return *this;

	int64_t sum = 0;
	for (int i = 0; i < 4; i++) {
		sum += m_data.u32_data[i];
		sum += value.m_data.u32_data[i];
		m_data.u32_data[i] = (uint32_t)sum;
		sum >>= 32;
	}
	return *this;
}

CUInt128& CUInt128::Subtract(const CUInt128 &value) throw()
{
	if (value.IsZero()) return *this;

	int64_t sum = 0;
	for (int i = 0; i < 4; i++) {
		sum += m_data.u32_data[i];
		sum -= value.m_data.u32_data[i];
		m_data.u32_data[i] = (uint32_t)sum;
		sum >>= 32;
	}
	return *this;
}

CUInt128& CUInt128::ShiftLeft(unsigned bits) throw()
{
	if ((bits == 0) || IsZero())
		return *this;

	if (bits > 127) {
		SetValue((uint32_t)0);
		return *this;
	}

	union {
		uint32_t u32_data[4];
		uint64_t u64_data[2];
	} result = {{ 0, 0, 0, 0 }};
	int indexShift = (int)bits / 32;
	int64_t shifted = 0;
	for (int i = 3; i >= indexShift; i--)
	{
		shifted += ((int64_t)m_data.u32_data[3 - i]) << (bits % 32);
		result.u32_data[3 - i + indexShift] = (uint32_t)shifted;
		shifted = shifted >> 32;
	}
	m_data.u64_data[0] = result.u64_data[0];
	m_data.u64_data[1] = result.u64_data[1];

	return *this;
}
// File_checked_for_headers

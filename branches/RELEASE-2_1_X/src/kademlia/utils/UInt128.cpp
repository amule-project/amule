//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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

#include "../../CryptoPP_Inc.h"
#include "../../ArchSpecific.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
using namespace CryptoPP;
////////////////////////////////////////

CUInt128::CUInt128()
{
	setValue((uint32)0);
}

CUInt128::CUInt128(bool fill)
{
	if( fill )
	{
		// Endian safe (-1 = 0xFFFF)
		m_data[0] = (uint32)-1;
		m_data[1] = (uint32)-1;
		m_data[2] = (uint32)-1;
		m_data[3] = (uint32)-1;
	}
	else
		setValue((uint32)0);
}

CUInt128::CUInt128(uint32 value)
{
	setValue(value);
}

CUInt128::CUInt128(const byte *valueBE)
{
	setValueBE(valueBE);
}

CUInt128::CUInt128(const CUInt128 &value, uint32 numBits)
{
	// Copy the whole uint32s
	uint32 numULONGs = numBits / 32;
	for (uint32 i=0; i<numULONGs; ++i)
		m_data[i] = value.m_data[i];

	// Copy the remaining bits
	for (uint32 i=(32*numULONGs); i<numBits; ++i)
		setBitNumber(i, value.getBitNumber(i));

	// Pad with random bytes (Not seeding based on time to allow multiple different ones to be created in quick succession)
	for (uint32 i=numBits; i<128; ++i)
		setBitNumber(i, (rand()%2));
}

CUInt128& CUInt128::setValue(const CUInt128 &value)
{
	m_data[0] = value.m_data[0];
	m_data[1] = value.m_data[1];
	m_data[2] = value.m_data[2];
	m_data[3] = value.m_data[3];
	return *this;
}

CUInt128& CUInt128::setValue(uint32 value)
{
	m_data[0] = 0;
	m_data[1] = 0;
	m_data[2] = 0;
	m_data[3] = value;
	return *this;
}


#define SWAP_ULONG_LE(x) wxUINT32_SWAP_ON_LE(x) 

CUInt128& CUInt128::setValueBE(const byte *valueBE)
{
	m_data[0] = SWAP_ULONG_LE(RawPeekUInt32(valueBE+0));
	m_data[1] = SWAP_ULONG_LE(RawPeekUInt32(valueBE+4));
	m_data[2] = SWAP_ULONG_LE(RawPeekUInt32(valueBE+8));
	m_data[3] = SWAP_ULONG_LE(RawPeekUInt32(valueBE+12));
	return *this;
}

CUInt128& CUInt128::setValueRandom(void)
{
	AutoSeededRandomPool rng;
	byte randomBytes[16];
	rng.GenerateBlock(randomBytes, 16);
	setValueBE( randomBytes );
	return *this;
}

uint32 CUInt128::getBitNumber(uint32 bit) const
{
	if (bit > 127)
		return 0;
	int ulongNum = bit / 32;
	
	int shift = 31 - (bit % 32);
	return ((m_data[ulongNum] >> shift) & 1);
}

CUInt128& CUInt128::setBitNumber(uint32 bit, uint32 value) 
{
	int ulongNum = bit / 32;
	int shift = 31 - (bit % 32);
	m_data[ulongNum] |= (1 << shift);
	if (value == 0)
		m_data[ulongNum] ^= (1 << shift);
	return *this;
}

CUInt128& CUInt128::XOR(const CUInt128 &value)
{
	for (int i=0; i<4; ++i)
		m_data[i] ^= value.m_data[i];
	return *this;
}

CUInt128& CUInt128::XORBE(const byte *valueBE)
{
	CUInt128 temp(valueBE);
	return XOR(temp);
}

wxString CUInt128::toHexString(void) const
{
	wxString str;
	for (int i=0; i<4; ++i)
	{
		str.Append(wxString::Format(wxT("%08X"), m_data[i]));
	}
	return str;
}

wxString CUInt128::toBinaryString(bool trim) const
{
	wxString str;
	int b;
	for (int i=0; i<128; ++i)
	{
		b = getBitNumber(i);
		if ((!trim) || (b != 0)) {
			str.Append(wxString::Format(wxT("%d"), b));
			trim = false;
		}
	}
	if (str.Len() == 0) {
		str = wxT("0");
	}
	return str;
}

void CUInt128::toByteArray(byte *b) const
{
	RawPokeUInt32(b+0, SWAP_ULONG_LE(m_data[0]));
	RawPokeUInt32(b+4, SWAP_ULONG_LE(m_data[1]));
	RawPokeUInt32(b+8, SWAP_ULONG_LE(m_data[2]));
	RawPokeUInt32(b+12,SWAP_ULONG_LE(m_data[3]));
}

int CUInt128::compareTo(const CUInt128 &other) const
{
	for (int i=0; i<4; ++i) 
	{
	    if (m_data[i] < other.m_data[i])
			return -1;
	    if (m_data[i] > other.m_data[i])
			return 1;
	}
	return 0;
}

int CUInt128::compareTo(uint32 value) const
{
	if ((m_data[0] > 0) || (m_data[1] > 0) || (m_data[2] > 0) || (m_data[3] > value))
		return 1;
	if (m_data[3] < value)
		return -1;
	return 0;
}

CUInt128& CUInt128::add(const CUInt128 &value)
{
	if (value == 0)
		return *this;
	int64 sum = 0;
	for (int i=3; i>=0; i--)
	{
		sum += m_data[i];
		sum += value.m_data[i];
		m_data[i] = (uint32)sum;
		sum = sum >> 32;
	}
	return *this;
}

CUInt128& CUInt128::add(uint32 value)
{
	if (value == 0)
		return *this;
	CUInt128 temp(value);
	add(temp);
	return *this;
}

CUInt128& CUInt128::subtract(const CUInt128 &value)
{
	if (value == 0)
		return *this;
	int64 sum = 0;
	for (int i=3; i>=0; i--)
	{
		sum += m_data[i];
		sum -= value.m_data[i];
		m_data[i] = (uint32)sum;
		sum = sum >> 32;
	}
	return *this;
}

CUInt128& CUInt128::subtract(uint32 value)
{
	if (value == 0)
		return *this;
	CUInt128 temp(value);
	subtract(temp);
	return *this;
}

CUInt128& CUInt128::shiftLeft(uint32 bits)
{
    if ((bits == 0) || (compareTo(0) == 0))
        return *this;
	if (bits > 127)
	{
		setValue((uint32)0);
		return *this;
	}

	uint32 result[] = {0,0,0,0};
	int indexShift = (int)bits / 32;
	int64 shifted = 0;
	for (int i=3; i>=indexShift; i--)
	{
		shifted += ((int64)m_data[i]) << (bits % 32);
		result[i-indexShift] = (uint32)shifted;
		shifted = shifted >> 32;
	}
	for (int i=0; i<4; ++i)
		m_data[i] = result[i];
	return *this;
}

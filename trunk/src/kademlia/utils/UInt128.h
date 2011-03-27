//								-*- C++ -*-
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

#ifndef __UINT128_H__
#define __UINT128_H__

#include "../../Types.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128
{
public:
	CUInt128(const CUInt128& value) throw()
	{
		SetValue(value);
	}

	explicit CUInt128(bool fill = false) throw()
	{
		m_data[0] = m_data[1] = m_data[2] = m_data[3] = (fill ? (uint32_t)-1 : 0);
	}

	explicit CUInt128(uint32_t value) throw()
	{
		SetValue(value);
	}

	explicit CUInt128(const uint8_t *valueBE) throw()
	{
		SetValueBE(valueBE);
	}

	/**
	 * Generates a new number, copying the most significant 'numBits' bits from 'value'.
	 * The remaining bits are randomly generated.
	 */
	CUInt128(const CUInt128& value, uint32_t numBits);

	/** Bit at level 0 being most significant. */
	unsigned GetBitNumber(unsigned bit) const throw()
	{
		return bit <= 127 ? (m_data[bit / 32] >> (31 - (bit % 32))) & 1 : 0;
	}

	int CompareTo(const CUInt128& other) const throw();
	int CompareTo(uint32_t value) const throw();

	wxString ToHexString() const;
	wxString ToBinaryString(bool trim = false) const;
	void ToByteArray(uint8_t *b) const;

	uint32_t Get32BitChunk(unsigned val) const throw()
	{
		return val < 4 ? m_data[val] : 0;
	}

	void Set32BitChunk(unsigned chunk, uint32_t value)
	{
		wxCHECK2(chunk < 4, return);

		m_data[chunk] = value;
	}

	CUInt128& SetValue(const CUInt128& value) throw()
	{
		m_data[0] = value.m_data[0];
		m_data[1] = value.m_data[1];
		m_data[2] = value.m_data[2];
		m_data[3] = value.m_data[3];
		return *this;
	}

	CUInt128& SetValue(uint32_t value) throw()
	{
		m_data[0] = m_data[1] = m_data[2] = 0;
		m_data[3] = value;
		return *this;
	}

	CUInt128& SetValueBE(const uint8_t *valueBE) throw();

	/**
	 * Stores value used by the crypt functions.
	 *
	 * Since eMule started to use the value as-is (four little-endian 32-bit integers in big-endian order),
	 * we have to reproduce that same representation on every platform.
	 *
	 * @param buf Buffer to hold the value. Must be large enough to hold the data (16 bytes at least),
	 *	and must not be NULL.
	 */
	void StoreCryptValue(uint8_t *buf) const;

	/** Bit at level 0 being most significant. */
	CUInt128& SetBitNumber(unsigned bit, unsigned value)
	{
		wxCHECK(bit <= 127, *this);

		if (value)
			m_data[bit / 32] |= 1 << (31 - (bit % 32));
		else
			m_data[bit / 32] &= ~(1 << (31 - (bit % 32)));

		return *this;
	}

	CUInt128& ShiftLeft(unsigned bits) throw();

	CUInt128& Add(const CUInt128& value) throw();
	CUInt128& Add(uint32_t value) throw()
	{
		return value ? Add(CUInt128(value)) : *this;
	}

	CUInt128& Subtract(const CUInt128& value) throw();
	CUInt128& Subtract(uint32_t value) throw()
	{
		return value ? Subtract(CUInt128(value)) : *this;
	}

	CUInt128& XOR(const CUInt128& value) throw()
	{
		m_data[0] ^= value.m_data[0];
		m_data[1] ^= value.m_data[1];
		m_data[2] ^= value.m_data[2];
		m_data[3] ^= value.m_data[3];
		return *this;
	}

	CUInt128& XORBE(const uint8_t *valueBE) throw() { return XOR(CUInt128(valueBE)); }

	bool operator<  (const CUInt128& value) const throw() {return (CompareTo(value) <  0);}
	bool operator>  (const CUInt128& value) const throw() {return (CompareTo(value) >  0);}
	bool operator<= (const CUInt128& value) const throw() {return (CompareTo(value) <= 0);}
	bool operator>= (const CUInt128& value) const throw() {return (CompareTo(value) >= 0);}
	bool operator== (const CUInt128& value) const throw() {return (CompareTo(value) == 0);}
	bool operator!= (const CUInt128& value) const throw() {return (CompareTo(value) != 0);}

	CUInt128& operator= (const CUInt128& value) throw() { return SetValue(value); }
	CUInt128& operator+=(const CUInt128& value) throw() { return Add(value); }
	CUInt128& operator-=(const CUInt128& value) throw() { return Subtract(value); }
	CUInt128& operator^=(const CUInt128& value) throw() { return XOR(value); }
	CUInt128  operator+ (const CUInt128& value) const throw() { return CUInt128(*this).Add(value); }
	CUInt128  operator- (const CUInt128& value) const throw() { return CUInt128(*this).Subtract(value); }
	CUInt128  operator^ (const CUInt128& value) const throw() { return CUInt128(*this).XOR(value); }

	bool operator<  (uint32_t value) const throw() {return (CompareTo(value) <  0);}
	bool operator>  (uint32_t value) const throw() {return (CompareTo(value) >  0);}
	bool operator<= (uint32_t value) const throw() {return (CompareTo(value) <= 0);}
	bool operator>= (uint32_t value) const throw() {return (CompareTo(value) >= 0);}
	bool operator== (uint32_t value) const throw() {return (CompareTo(value) == 0);}
	bool operator!= (uint32_t value) const throw() {return (CompareTo(value) != 0);}

	CUInt128& operator= (uint32_t value) throw() { return SetValue(value); }
	CUInt128& operator+=(uint32_t value) throw() { return Add(value); }
	CUInt128& operator-=(uint32_t value) throw() { return Subtract(value); }
	CUInt128& operator^=(uint32_t value) throw() { return value ? XOR(CUInt128(value)) : *this; }
	CUInt128  operator+ (uint32_t value) const throw() { return CUInt128(*this).Add(value); }
	CUInt128  operator- (uint32_t value) const throw() { return CUInt128(*this).Subtract(value); }
	CUInt128  operator^ (uint32_t value) const throw() { return value ? CUInt128(*this).XOR(CUInt128(value)) : *this; }

	CUInt128  operator<< (uint32_t bits) const throw() { return CUInt128(*this).ShiftLeft(bits); }
	CUInt128& operator<<=(uint32_t bits) throw() { return ShiftLeft(bits); }

private:
	bool IsZero() const throw()
	{
		return (m_data[0] | m_data[1] | m_data[2] | m_data[3]) == 0;
	}

	uint32_t m_data[4];
};

inline bool operator==(uint32_t x, const CUInt128& y) throw() { return y.operator==(x); }
inline bool operator!=(uint32_t x, const CUInt128& y) throw() { return y.operator!=(x); }
inline bool operator<(uint32_t x, const CUInt128& y) throw() { return y.operator>(x); }
inline bool operator>(uint32_t x, const CUInt128& y) throw() { return y.operator<(x); }
inline bool operator<=(uint32_t x, const CUInt128& y) throw() { return y.operator>=(x); }
inline bool operator>=(uint32_t x, const CUInt128& y) throw() { return y.operator<=(x); }
inline CUInt128 operator+(uint32_t x, const CUInt128& y) throw() { return y.operator+(x); }
inline CUInt128 operator-(uint32_t x, const CUInt128& y) throw() { return CUInt128(x).Subtract(y); }
inline CUInt128 operator^(uint32_t x, const CUInt128& y) throw() { return y.operator^(x); }

} // End namespace

#endif
// File_checked_for_headers

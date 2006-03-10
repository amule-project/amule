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

#ifndef __UINT128_H__
#define __UINT128_H__

#include "../../Types.h"
#include <wx/string.h>

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128 
{
public:

	CUInt128();
	explicit CUInt128(bool fill);
	explicit CUInt128(uint32 value);
	explicit CUInt128(const byte *valueBE);
	/**
	 * Generates a new number, copying the most significant 'numBits' bits from 'value'.
	 * The remaining bits are randomly generated.
	 */
	CUInt128(const CUInt128 &value, uint32 numBits = 128);

	~CUInt128() {};
	
	const byte* GetData() const { return (byte*)m_data; }
	byte* GetDataPtr() const { return (byte*)m_data; }

	/** Bit at level 0 being most significant. */
	uint32 GetBitNumber(uint32 bit) const;
	int CompareTo(const CUInt128 &other) const;
	int CompareTo(uint32 value) const;

	wxString ToHexString(void) const;
	wxString ToBinaryString(bool trim = false) const;
	void ToByteArray(byte *b) const;

	uint32 Get32BitChunk(int val) const {return m_data[val];}

	CUInt128& SetValue(const CUInt128 &value);
	CUInt128& SetValue(uint32 value);
	CUInt128& SetValueBE(const byte *valueBE);

	CUInt128& SetValueRandom(void);
//	CUInt128& SetValueGUID(void);

	CUInt128& SetBitNumber(uint32 bit, uint32 value);
	CUInt128& ShiftLeft(uint32 bits);

	CUInt128& Add(const CUInt128 &value);
	CUInt128& Add(uint32 value);
	CUInt128& Subtract(const CUInt128 &value);
	CUInt128& Subtract(uint32 value);

	CUInt128& XOR(const CUInt128 &value);
	CUInt128& XORBE(const byte *valueBE);

	void operator+  (const CUInt128 &value) {Add(value);}
	void operator-  (const CUInt128 &value) {Subtract(value);}
	void operator=  (const CUInt128 &value) {SetValue(value);}
	bool operator<  (const CUInt128 &value) const {return (CompareTo(value) <  0);}
	bool operator>  (const CUInt128 &value) const {return (CompareTo(value) >  0);}
	bool operator<= (const CUInt128 &value) const {return (CompareTo(value) <= 0);}
	bool operator>= (const CUInt128 &value) const {return (CompareTo(value) >= 0);}
	bool operator== (const CUInt128 &value) const {return (CompareTo(value) == 0);}
	bool operator!= (const CUInt128 &value) const {return (CompareTo(value) != 0);}

	void operator+  (uint32 value) {Add(value);}
	void operator-  (uint32 value) {Subtract(value);}
	void operator=  (uint32 value) {SetValue(value);}
	bool operator<  (uint32 value) const {return (CompareTo(value) <  0);}
	bool operator>  (uint32 value) const {return (CompareTo(value) >  0);}
	bool operator<= (uint32 value) const {return (CompareTo(value) <= 0);}
	bool operator>= (uint32 value) const {return (CompareTo(value) >= 0);}
	bool operator== (uint32 value) const {return (CompareTo(value) == 0);}
	bool operator!= (uint32 value) const {return (CompareTo(value) != 0);}

private:

	uint32 m_data[4];
};

} // End namespace

#endif

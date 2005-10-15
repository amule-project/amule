//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "UInt128.h"
#endif

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
	
	const byte* getData() const { return (byte*)m_data; }
	byte* getDataPtr() const { return (byte*)m_data; }

	/** Bit at level 0 being most significant. */
	uint32 getBitNumber(uint32 bit) const;
	int compareTo(const CUInt128 &other) const;
	int compareTo(uint32 value) const;

	wxString toHexString(void) const;
	wxString toBinaryString(bool trim = false) const;
	void toByteArray(byte *b) const;

	uint32 get32BitChunk(int val) const {return m_data[val];}

	CUInt128& setValue(const CUInt128 &value);
	CUInt128& setValue(uint32 value);
	CUInt128& setValueBE(const byte *valueBE);

	CUInt128& setValueRandom(void);
//	CUInt128& setValueGUID(void);

	CUInt128& setBitNumber(uint32 bit, uint32 value);
	CUInt128& shiftLeft(uint32 bits);

	CUInt128& add(const CUInt128 &value);
	CUInt128& add(uint32 value);
	CUInt128& subtract(const CUInt128 &value);
	CUInt128& subtract(uint32 value);

	CUInt128& XOR(const CUInt128 &value);
	CUInt128& XORBE(const byte *valueBE);

	void operator+  (const CUInt128 &value) {add(value);}
	void operator-  (const CUInt128 &value) {subtract(value);}
	void operator=  (const CUInt128 &value) {setValue(value);}
	bool operator<  (const CUInt128 &value) const {return (compareTo(value) <  0);}
	bool operator>  (const CUInt128 &value) const {return (compareTo(value) >  0);}
	bool operator<= (const CUInt128 &value) const {return (compareTo(value) <= 0);}
	bool operator>= (const CUInt128 &value) const {return (compareTo(value) >= 0);}
	bool operator== (const CUInt128 &value) const {return (compareTo(value) == 0);}
	bool operator!= (const CUInt128 &value) const {return (compareTo(value) != 0);}

	void operator+  (uint32 value) {add(value);}
	void operator-  (uint32 value) {subtract(value);}
	void operator=  (uint32 value) {setValue(value);}
	bool operator<  (uint32 value) const {return (compareTo(value) <  0);}
	bool operator>  (uint32 value) const {return (compareTo(value) >  0);}
	bool operator<= (uint32 value) const {return (compareTo(value) <= 0);}
	bool operator>= (uint32 value) const {return (compareTo(value) >= 0);}
	bool operator== (uint32 value) const {return (compareTo(value) == 0);}
	bool operator!= (uint32 value) const {return (compareTo(value) != 0);}

private:

	uint32 m_data[4];
};

} // End namespace

#endif

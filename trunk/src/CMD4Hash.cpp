// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef CMD4HASH_CPP
#define CMD4HASH_CPP

#include <wx/string.h>
#include <ctype.h>

#include "types.h"		// Needed for uint16, uint32 and uint64
#include "CMD4Hash.h"

// Base 16 chars for encoding and decoding functions
#define MD4_LOOKUP_MAX 23
static byte MD4Chars[17] = "0123456789ABCDEF";
static byte MD4Lookup[MD4_LOOKUP_MAX][2] = {
	{ '0', 0x0 },
	{ '1', 0x1 },
	{ '2', 0x2 },
	{ '3', 0x3 },
	{ '4', 0x4 },
	{ '5', 0x5 },
	{ '6', 0x6 },
	{ '7', 0x7 },
	{ '8', 0x8 },
	{ '9', 0x9 },
	{ ':', 0x9 },
	{ ';', 0x9 },
	{ '<', 0x9 },
	{ '=', 0x9 },
	{ '>', 0x9 },
	{ '?', 0x9 },
	{ '@', 0x9 },
	{ 'A', 0xA },
	{ 'B', 0xB },
	{ 'C', 0xC },
	{ 'D', 0xD },
	{ 'E', 0xE },
	{ 'F', 0xF }
};


CMD4Hash::CMD4Hash()
{
	memset( m_hash, 0, MD4HASH_LENGTH );
}


CMD4Hash::CMD4Hash(unsigned char hash[])
{
	wxASSERT(hash);
	memcpy(m_hash, hash, MD4HASH_LENGTH);
}


CMD4Hash::CMD4Hash(const CMD4Hash& hash)
{
	memcpy(m_hash, hash.m_hash, MD4HASH_LENGTH);
}


CMD4Hash::CMD4Hash(const wxString& hash)
{
	Decode( hash );
}


bool CMD4Hash::operator == (const CMD4Hash& other_hash) const
{
	return (((uint32*)m_hash)[0] == ((uint32*)other_hash.m_hash)[0] &&
			((uint32*)m_hash)[1] == ((uint32*)other_hash.m_hash)[1] &&
			((uint32*)m_hash)[2] == ((uint32*)other_hash.m_hash)[2] &&
			((uint32*)m_hash)[3] == ((uint32*)other_hash.m_hash)[3]);
}


bool CMD4Hash::operator != (const CMD4Hash& other_hash) const
{ 
	return !(*this == other_hash); 
}


bool CMD4Hash::IsEmpty() const
{
	return (((uint32*)m_hash)[0] == 0 &&
			((uint32*)m_hash)[1] == 0 &&
			((uint32*)m_hash)[2] == 0 &&
			((uint32*)m_hash)[3] == 0);
}


void CMD4Hash::Clear()
{
	memset( m_hash, 0, MD4HASH_LENGTH );
}


void CMD4Hash::Decode(const wxString& hash)
{
	wxASSERT(hash.Length() == MD4HASH_LENGTH * 2);

	for ( uint16 i = 0; i < MD4HASH_LENGTH * 2; i++ ) {
		int lookup = toupper(hash[i]) - '0';

		// Check to make sure that the given word falls inside a valid range
		byte word = 0;

		if ( lookup < 0 || lookup >= MD4_LOOKUP_MAX)
			word = 0xFF;
		else
			word = MD4Lookup[lookup][1];

		if(i % 2 == 0) {
			m_hash[i/2] = word << 4;
		} else {
			m_hash[(i-1)/2] |= word;
		}
	}
}


wxString CMD4Hash::Encode() const
{
	wxString Base16Buff;

	for( uint16 i = 0; i < MD4HASH_LENGTH; i++ ) {
		Base16Buff += MD4Chars[m_hash[i] >> 4];
		Base16Buff += MD4Chars[m_hash[i] & 0xf];
	}

	return Base16Buff;
}


#endif

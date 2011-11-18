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

// The backtrace functions contain modified code from libYaMa, (c) Venkatesha Murthy G.
// You can check libYaMa at http://personal.pavanashree.org/libyama/

#include "RandomFunctions.h"	// Interface declarations
#include "CryptoPP_Inc.h"	// Needed for Crypto functions

static CryptoPP::AutoSeededRandomPool cryptRandomGen;

const CryptoPP::AutoSeededRandomPool& GetRandomPool() { return cryptRandomGen; }

uint8_t GetRandomUint8()
{
	return cryptRandomGen.GenerateByte();
}

uint16_t GetRandomUint16()
{
	return (uint16_t)cryptRandomGen.GenerateWord32(0x0000, 0xFFFF);
}

uint32_t GetRandomUint32()
{
	return cryptRandomGen.GenerateWord32();
}

uint64_t GetRandomUint64()
{
	return ((uint64_t)GetRandomUint32() << 32) + GetRandomUint32();
}

namespace Kademlia {
	CUInt128 GetRandomUint128()
	{
		uint8_t randomBytes[16];
		cryptRandomGen.GenerateBlock(randomBytes, 16);
		return CUInt128(randomBytes);
	}
}

// File_checked_for_headers

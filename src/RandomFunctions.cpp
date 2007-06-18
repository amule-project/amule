//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

static CryptoPP::AutoSeededRandomPool cryptRandomGen;

const CryptoPP::AutoSeededRandomPool& GetRandomPool() { return cryptRandomGen; }

uint8 GetRandomUint8()
{
	return cryptRandomGen.GenerateByte();
}

uint16 GetRandomUint16()
{
	return (uint16)cryptRandomGen.GenerateWord32(0x0000, 0xFFFF);
}

uint32 GetRandomUint32()
{
	return cryptRandomGen.GenerateWord32();
}

// File_checked_for_headers

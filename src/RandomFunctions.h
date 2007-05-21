//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef RANDOMFUNCTIONS_H
#define RANDOMFUNCTIONS_H

#include "Types.h"		// Needed for uint16, uint32 and uint64
#include "CryptoPP_Inc.h"	// Needed for Crypto functions

/* 
 * Random numbers generation
 */
 
const CryptoPP::AutoSeededRandomPool& GetRandomPool();

uint8 GetRandomUint8();
uint16 GetRandomUint16();
uint32 GetRandomUint32();

#endif // RANDOMFUNCTIONS_H
// File_checked_for_headers

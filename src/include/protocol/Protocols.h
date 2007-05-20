//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef ED2KPROTOCOLS_H
#define ED2KPROTOCOLS_H

// For MuleInfoPacket (OLD - DEPRECATED.)
#define	EMULE_PROTOCOL				0x01

// Known protocols
enum Protocols {
	OP_EDONKEYHEADER		= 0xE3,
	OP_EDONKEYPROT			= OP_EDONKEYHEADER,
	OP_PACKEDPROT			= 0xD4,
	OP_EMULEPROT			= 0xC5,

	// Reserved for later UDP headers (important for EncryptedDatagramSocket)	
	OP_UDPRESERVEDPROT1 = 0xA3,
	OP_UDPRESERVEDPROT2 = 0xB2,

	// Kademlia 1/2
	OP_KADEMLIAHEADER		= 0xE4,
	OP_KADEMLIAPACKEDPROT	= 0xE5,
	
	// Kry tests
	OP_ED2KV2HEADER			= 0xF4,
	OP_ED2KV2PACKEDPROT		= 0xF5,
	
	OP_MLDONKEYPROT			= 0x00
};

#endif // ED2KPROTOCOLS_H

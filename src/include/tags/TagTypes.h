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

#ifndef COMMONTAGTYPES_H
#define COMMONTAGTYPES_H

enum Tag_Types {
	TAGTYPE_HASH16		= 0x01,
	TAGTYPE_STRING		= 0x02,
	TAGTYPE_UINT32		= 0x03,
	TAGTYPE_FLOAT32		= 0x04,
	TAGTYPE_BOOL		= 0x05,
	TAGTYPE_BOOLARRAY	= 0x06,
	TAGTYPE_BLOB		= 0x07,
	TAGTYPE_UINT16		= 0x08,
	TAGTYPE_UINT8		= 0x09,
	TAGTYPE_BSOB		= 0x0A,
	TAGTYPE_UINT64		= 0x0B,

	// Compressed string types
	TAGTYPE_STR1		= 0x11,
	TAGTYPE_STR2,
	TAGTYPE_STR3,
	TAGTYPE_STR4,
	TAGTYPE_STR5,
	TAGTYPE_STR6,
	TAGTYPE_STR7,
	TAGTYPE_STR8,
	TAGTYPE_STR9,
	TAGTYPE_STR10,
	TAGTYPE_STR11,
	TAGTYPE_STR12,
	TAGTYPE_STR13,
	TAGTYPE_STR14,
	TAGTYPE_STR15,
	TAGTYPE_STR16,
	TAGTYPE_STR17,	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
			// only because of a flaw, those tags are handled correctly,
			// but should not be handled at all
	TAGTYPE_STR18,	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
			//  only because of a flaw, those tags are handled correctly,
			// but should not be handled at all
	TAGTYPE_STR19,	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
			// only because of a flaw, those tags are handled correctly,
			// but should not be handled at all
	TAGTYPE_STR20,	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
			// only because of a flaw, those tags are handled correctly,
			// but should not be handled at all
	TAGTYPE_STR21,	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
			// only because of a flaw, those tags are handled correctly,
			// but should not be handled at all
	TAGTYPE_STR22	// accepted by eMule 0.42f (02-Mai-2004) in receiving code
			// only because of a flaw, those tags are handled correctly,
			// but should not be handled at all
};

#endif // COMMONTAGTYPES_H

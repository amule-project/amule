// 
//  This file is part of the aMule Project.
// 
//  Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// 
//  Any parts of this program derived from the xMule, lMule or eMule project,
//  or contributed by third-party developers are copyrighted by their
//  respective authors.
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

// Purpose:
// EC tag types for use on the ec library.

#ifndef __ECTAGTYPES_H__
#define __ECTAGTYPES_H__

enum ECTagTypes {
	EC_TAGTYPE_UNKNOWN = 0,
	EC_TAGTYPE_CUSTOM = 1,
	EC_TAGTYPE_UINT8 = 2,
	EC_TAGTYPE_UINT16 = 3,
	EC_TAGTYPE_UINT32 = 4,
	EC_TAGTYPE_UINT64 = 5,
	EC_TAGTYPE_STRING = 6,
	EC_TAGTYPE_DOUBLE = 7,
	EC_TAGTYPE_IPV4 = 8,
	EC_TAGTYPE_HASH16 = 9
};

#endif // __ECTAGTYPES_H__

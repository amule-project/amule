//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SERVERTAGS_H
#define SERVERTAGS_H

// server.met and server status.
enum Server_tags {
	ST_SERVERNAME						=	0x01,	// <string>
	// Unused (0x02-0x0A)
	ST_DESCRIPTION						=	0x0B,	// <string>
	ST_PING									=	0x0C,	// <uint32>
	ST_FAIL									=	0x0D,	// <uint32>
	ST_PREFERENCE						=	0x0E,	// <uint32>
	// Unused (0x0F-0x84)
	ST_DYNIP									=	0x85,
	ST_LASTPING_DEPRECATED	=	0x86,	// <uint32> // DEPRECATED, use 0x90
	ST_MAXUSERS							=	0x87,
	ST_SOFTFILES							=	0x88,
	ST_HARDFILES							= 0x89,
	// Unused (0x8A-0x8F)
	ST_LASTPING							=	0x90,	// <uint32>
	ST_VERSION							=	0x91,	// <string>
	ST_UDPFLAGS							=	0x92,	// <uint32>
	ST_AUXPORTSLIST					= 0x93,	// <string>
	ST_LOWIDUSERS						= 0x94,	// <uint32>
	ST_UDPKEY								= 0x95,	// <uint32>
	ST_UDPKEYIP							= 0x96,	// <uint32>
	ST_TCPPORTOBFUSCATION		= 0x97,	// <uint16>
	ST_UDPPORTOBFUSCATION	= 0x98	// <uint16>	
};

#endif // SERVERTAGS_H

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

#ifndef ED2KC2SUDP_H
#define ED2KC2SUDP_H

enum OP_ClientToServerUDP {
	OP_GLOBGETSOURCES2			= 0x94,	// <HASH 16><FILESIZE 4>
										// largefiles only: <HASH 16><FILESIZE 4(0)><FILESIZE 8> (17.8)		
	OP_GLOBSERVSTATREQ			= 0x96,	// (null)
	OP_GLOBSERVSTATRES			= 0x97,	// <USER 4><FILES 4>
	OP_GLOBSEARCHREQ			= 0x98,	// <search_tree>
	OP_GLOBSEARCHRES			= 0x99,	// 
	OP_GLOBGETSOURCES			= 0x9A,	// <HASH 16>
	OP_GLOBFOUNDSOURCES			= 0x9B,	//
	OP_GLOBCALLBACKREQ			= 0x9C,	// <IP 4><PORT 2><client_ID 4>
	OP_INVALID_LOWID			= 0x9E,	// <ID 4>
	OP_SERVER_LIST_REQ			= 0xA0,	// <IP 4><PORT 2>
	OP_SERVER_LIST_RES			= 0xA1,	// <count 1> (<ip 4><port 2>)[count]
	OP_SERVER_DESC_REQ			= 0xA2,	// (null)
	OP_SERVER_DESC_RES			= 0xA3,	// <name_len 2><name name_len><desc_len 2 desc_en>
	OP_SERVER_LIST_REQ2			= 0xA4	// (null)
};

// Server UDP flags
#define SRV_UDPFLG_EXT_GETSOURCES       0x00000001
#define SRV_UDPFLG_EXT_GETFILES         0x00000002
#define SRV_UDPFLG_NEWTAGS                      0x00000008
#define SRV_UDPFLG_UNICODE                      0x00000010
#define SRV_UDPFLG_EXT_GETSOURCES2      0x00000020
#define SRV_UDPFLG_LARGEFILES           0x00000100
#define SRV_UDPFLG_UDPOBFUSCATION	0x00000200
#define SRV_UDPFLG_TCPOBFUSCATION	0x00000400

#endif // ED2KC2SUDP_H

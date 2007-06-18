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

#ifndef KAD2C2CUDP_H
#define KAD2C2CUDP_H

enum KademliaV2OPCodes {
	KADEMLIA2_BOOTSTRAP_REQ			=	0x01,
	KADEMLIA2_BOOTSTRAP_RES				=	0x09,
	KADEMLIA2_HELLO_REQ						=	0x11,
	KADEMLIA2_HELLO_RES						=	0x19,
	KADEMLIA2_REQ									=	0x21,
	KADEMLIA2_RES									=	0x29,
	KADEMLIA2_SEARCH_KEY_REQ			=	0x33,
	KADEMLIA2_SEARCH_NOTES_REQ		=	0x35,
	KADEMLIA2_SEARCH_RES					=	0x3B,
	KADEMLIA2_PUBLISH_KEY_REQ			=	0x43,
	KADEMLIA2_PUBLISH_SOURCE_REQ	=	0x44,
	KADEMLIA2_PUBLISH_NOTES_REQ		=	0x45,
	KADEMLIA2_PUBLISH_RES					=	0x4B
};

#endif // KAD2C2CUDP_H

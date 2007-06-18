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

#ifndef KADC2CUDP_H
#define KADC2CUDP_H

enum KademliaV1OPcodes {
	KADEMLIA_BOOTSTRAP_REQ 	= 0x00,	// <PEER (sender) [25]>
	KADEMLIA_BOOTSTRAP_RES	= 0x08,	// <CNT [2]> <PEER [25]>*(CNT)

	KADEMLIA_HELLO_REQ	 	= 0x10,	// <PEER (sender) [25]>
	KADEMLIA_HELLO_RES     	= 0x18,	// <PEER (receiver) [25]>

	KADEMLIA_REQ		   	= 0x20,	// <TYPE [1]> <HASH (target) [16]> <HASH (receiver) 16>
	KADEMLIA_RES			= 0x28,	// <HASH (target) [16]> <CNT> <PEER [25]>*(CNT)

	KADEMLIA_SEARCH_REQ		= 0x30,	// <HASH (key) [16]> <ext 0/1 [1]> <SEARCH_TREE>[ext]
	// UNUSED				= 0x31,	// Old Opcode, don't use.
	KADEMLIA_SRC_NOTES_REQ	= 0x32,	// <HASH (key) [16]>
	KADEMLIA_SEARCH_RES		= 0x38,	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
	// UNUSED				= 0x39,	// Old Opcode, don't use.
	KADEMLIA_SRC_NOTES_RES	= 0x3A,	// <HASH (key) [16]> <CNT1 [2]> (<HASH (answer) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)

	KADEMLIA_PUBLISH_REQ	= 0x40,	// <HASH (key) [16]> <CNT1 [2]> (<HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
	// UNUSED				= 0x41,	// Old Opcode, don't use.
	KADEMLIA_PUB_NOTES_REQ	= 0x42,	// <HASH (key) [16]> <HASH (target) [16]> <CNT2 [2]> <META>*(CNT2))*(CNT1)
	KADEMLIA_PUBLISH_RES	= 0x48,	// <HASH (key) [16]>
	// UNUSED				= 0x49,	// Old Opcode, don't use.
	KADEMLIA_PUB_NOTES_RES	= 0x4A,	// <HASH (key) [16]>

	KADEMLIA_FIREWALLED_REQ	= 0x50,	// <TCPPORT (sender) [2]>
	KADEMLIA_FINDBUDDY_REQ	= 0x51,	// <TCPPORT (sender) [2]>
	KADEMLIA_CALLBACK_REQ	= 0x52,	// <TCPPORT (sender) [2]>
	KADEMLIA_FIREWALLED_RES	= 0x58,	// <IP (sender) [4]>
	KADEMLIA_FIREWALLED_ACK	= 0x59,	// (null)
	KADEMLIA_FINDBUDDY_RES	= 0x5A	// <TCPPORT (sender) [2]>
};

#endif // KADC2CUDP_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef KAD2_C2C_TCP_H
#define KAD2_C2C_TCP_H

enum Ed2kTCPOpcodesForKademliaV2 {
	OP_FWCHECKUDPREQ	= 0xA7,	// <Inter_Port 2><Extern_Port 2><KadUDPKey 4> *Support required for Kadversion >= 6
	OP_KAD_FWTCPCHECK_ACK	= 0xA8	// (null/reserved), replaces KADEMLIA_FIREWALLED_ACK_RES, *Support required for Kadversion >= 7
};

#endif // KAD2_C2C_TCP_H

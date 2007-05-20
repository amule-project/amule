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

#ifndef ED2KC2CTCP_H
#define ED2KC2CTCP_H

// Client <-> Client
enum ED2KStandardClientTCP {	
	OP_HELLO					= 0x01,	// 0x10<HASH 16><ID 4><PORT 2><1 Tag_set>
	OP_SENDINGPART				= 0x46,	// <HASH 16><von 4><bis 4><Daten len:(von-bis)>
	OP_REQUESTPARTS				= 0x47,	// <HASH 16><von[3] 4*3><bis[3] 4*3>
	OP_FILEREQANSNOFIL			= 0x48,	// <HASH 16>
	OP_END_OF_DOWNLOAD     		= 0x49,	// <HASH 16> // Unused for sending
	OP_ASKSHAREDFILES			= 0x4A,	// (null)
	OP_ASKSHAREDFILESANSWER 	= 0x4B,	// <count 4>(<HASH 16><ID 4><PORT 2><1 Tag_set>)[count]
	OP_HELLOANSWER				= 0x4C,	// <HASH 16><ID 4><PORT 2><1 Tag_set><SERVER_IP 4><SERVER_PORT 2>
	OP_CHANGE_CLIENT_ID 		= 0x4D,	// <ID_old 4><ID_new 4> // Unused for sending
	OP_MESSAGE					= 0x4E,	// <len 2><Message len>
	OP_SETREQFILEID				= 0x4F,	// <HASH 16>
	OP_FILESTATUS				= 0x50,	// <HASH 16><count 2><status(bit array) len:((count+7)/8)>
	OP_HASHSETREQUEST			= 0x51,	// <HASH 16>
	OP_HASHSETANSWER			= 0x52,	// <count 2><HASH[count] 16*count>
	OP_STARTUPLOADREQ			= 0x54,	// <HASH 16>
	OP_ACCEPTUPLOADREQ			= 0x55,	// (null)
	OP_CANCELTRANSFER			= 0x56,	// (null)	
	OP_OUTOFPARTREQS			= 0x57,	// (null)
	OP_REQUESTFILENAME			= 0x58,	// <HASH 16>	(more correctly file_name_request)
	OP_REQFILENAMEANSWER		= 0x59,	// <HASH 16><len 4><NAME len>
	OP_CHANGE_SLOT				= 0x5B,	// <HASH 16> // Not used for sending
	OP_QUEUERANK				= 0x5C,	// <wert  4> (slot index of the request) // Not used for sending
	OP_ASKSHAREDDIRS			= 0x5D,	// (null)
	OP_ASKSHAREDFILESDIR		= 0x5E,	// <len 2><Directory len>
	OP_ASKSHAREDDIRSANS			= 0x5F,	// <count 4>(<len 2><Directory len>)[count]
	OP_ASKSHAREDFILESDIRANS		= 0x60,	// <len 2><Directory len><count 4>(<HASH 16><ID 4><PORT 2><1 T
	OP_ASKSHAREDDENIEDANS		= 0x61	// (null)
};

// Extended prot client <-> Extended prot client
enum ED2KExtendedClientTCP {
	OP_EMULEINFO				= 0x01,	//
	OP_EMULEINFOANSWER			= 0x02,	//
	OP_COMPRESSEDPART			= 0x40,	//
	OP_QUEUERANKING				= 0x60,	// <RANG 2>
	OP_FILEDESC					= 0x61,	// <len 2><NAME len>
	OP_VERIFYUPSREQ				= 0x71,	// (never used)
	OP_VERIFYUPSANSWER			= 0x72,	// (never used)
	OP_UDPVERIFYUPREQ			= 0x73,	// (never used)
	OP_UDPVERIFYUPA				= 0x74,	// (never used)
	OP_REQUESTSOURCES			= 0x81,	// <HASH 16>
	OP_ANSWERSOURCES			= 0x82,	//
	OP_REQUESTSOURCES2			= 0x83,	// <HASH 16>
	OP_ANSWERSOURCES2			= 0x84,	//	
	OP_PUBLICKEY				= 0x85,	// <len 1><pubkey len>
	OP_SIGNATURE				= 0x86,	// v1: <len 1><signature len>
										// v2:<len 1><signature len><sigIPused 1>
	OP_SECIDENTSTATE			= 0x87,	// <state 1><rndchallenge 4>
	OP_REQUESTPREVIEW			= 0x90,	// <HASH 16> // Never used for sending on aMule
	OP_PREVIEWANSWER			= 0x91,	// <HASH 16><frames 1>{frames * <len 4><frame len>} // Never used for sending on aMule
	OP_MULTIPACKET				= 0x92,
	OP_MULTIPACKETANSWER		= 0x93,
//	OP_PEERCACHE_QUERY			= 0x94, // Unused on aMule - no PeerCache
//	OP_PEERCACHE_ANSWER			= 0x95, // Unused on aMule - no PeerCache
//	OP_PEERCACHE_ACK			= 0x96, // Unused on aMule - no PeerCache
	OP_PUBLICIP_REQ				= 0x97,
	OP_PUBLICIP_ANSWER			= 0x98,
	OP_CALLBACK					= 0x99,	// <HASH 16><HASH 16><uint 16>
	OP_REASKCALLBACKTCP			= 0x9A,
	OP_AICHREQUEST				= 0x9B,	// <HASH 16><uint16><HASH aichhashlen>
	OP_AICHANSWER				= 0x9C,	// <HASH 16><uint16><HASH aichhashlen> <data>
	OP_AICHFILEHASHANS			= 0x9D,	  
	OP_AICHFILEHASHREQ			= 0x9E,
	OP_BUDDYPING				= 0x9F,
	OP_BUDDYPONG				= 0xA0,
	OP_COMPRESSEDPART_I64		= 0xA1,	// <HASH 16><von 8><size 4><Data len:size>
	OP_SENDINGPART_I64			= 0xA2,	// <HASH 16><start 8><end 8><Data len:(end-start)>
	OP_REQUESTPARTS_I64			= 0xA3,	// <HASH 16><start[3] 8*3><end[3] 8*3>
	OP_MULTIPACKET_EXT			= 0xA4	
};

#endif // ED2KC2CTCP_H

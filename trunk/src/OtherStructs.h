//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef OTHERSTRUCTS_H
#define OTHERSTRUCTS_H

#include "Types.h"		// Needed for int8, int32, uint8, uint16 and uint32

// Defined in <zlib.h>
struct z_stream_s;

//			SERVER TO CLIENT
#pragma pack(1)
struct Header_Struct{
	int8	eDonkeyID;
	int32	packetlength;
	int8	command;
};
#pragma pack()

#pragma pack(1)
struct UDP_Header_Struct{
	int8	eDonkeyID;
	int8	command;
};
#pragma pack()

#pragma pack(1)
struct Requested_Block_Struct{
	uint32	StartOffset;
	uint32	EndOffset;
	uint32	packedsize;
	unsigned char	FileID[16];
	uint32  transferred; // Barry - This counts bytes completed
};
#pragma pack()

#pragma pack(1)
struct Requested_File_Struct{
	unsigned char	  fileid[16];
	uint32	  lastasked;
	uint8	  badrequests;
};
#pragma pack()

struct Pending_Block_Struct{
	Requested_Block_Struct*	block;
	struct z_stream_s*       zStream;       // Barry - Used to unzip packets
	uint32		totalUnzipped; // Barry - This holds the total unzipped bytes for all packets so far
	UINT					fZStreamError : 1,
							fRecovered    : 1;	
};

struct Gap_Struct{
	uint32 start;
	uint32 end;
};
#pragma pack(1)
struct ServerMet_Struct {
	uint32	ip;
	uint16	port;
	uint32	tagcount;
};
#pragma pack()
struct TransferredData {
	uint32	datalen;
	DWORD	timestamp;
};


//Kry import of 0.30d 
// Maella -Enhanced Chunk Selection- (based on jicxicmic)
#pragma pack(1)
struct Chunk {
	uint16 part;      // Index of the chunk
		union {
			uint16 frequency; // Availability of the chunk
			uint16 rank;      // Download priority factor (highest = 0, lowest = 0xffff)
	};
};
#pragma pack()

#endif // OTHERSTRUCTS_H

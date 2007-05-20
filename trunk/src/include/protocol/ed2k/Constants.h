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

#ifndef ED2KCONSTANTS_H
#define ED2KCONSTANTS_H

#include <include/common/Macros.h>

// MOD Note: Do not change this part - Merkur

// Timeouts & Reask times
#define	CONNECTION_TIMEOUT			40000	// set this lower if you want less connections at once, set  it higher if you have enough sockets (edonkey has its own timout too, so a very high value won't effect this)
#define	FILEREASKTIME				1300000	// 1300000 <- original value ***
#define	SERVERREASKTIME				800000  // don't set this too low, it wont speed up anything, but it could kill amule or your internetconnection
#define	UDPSERVERREASKTIME			1300000	// 1300000 <- original value ***
#define	SOURCECLIENTREASKS			MIN2MS(40)	//40 mins
#define	SOURCECLIENTREASKF			MIN2MS(5)	//5 mins
#define	UDPSERVERSTATTIME		SEC2MS(5)	//5 secs
#define	UDPSERVSTATREASKTIME	HR2MS(4)		//4 hours - eMule uses HR2S, we are based on GetTickCount, hence MS

#define	MINCOMMONPENALTY		4 // For file sources reask

#define	ED2KREPUBLISHTIME		MIN2MS(1)	//1 min

#define	UDPMAXQUEUETIME			SEC2MS(30)	//30 Seconds

#define	RSAKEYSIZE				384			//384 bits

#define	MAX_SOURCES_FILE_SOFT	500
#define	MAX_SOURCES_FILE_UDP	50

#define	MAXFILECOMMENTLEN		50

#define	MIN_UP_CLIENTS_ALLOWED			2	// min. clients allowed to download regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too high

#define	SOURCEEXCHANGE2_VERSION			4		// replaces the version sent in MISC_OPTIONS flag fro SX1

// MOD Note: end

#define	MAXCONPER5SEC				20
#define	UPLOAD_CLIENT_DATARATE			3072
#define	MAX_UP_CLIENTS_ALLOWED			250	// max. clients allowed regardless UPLOAD_CLIENT_DATARATE or any other factors. Don't set this too low, use DATARATE to adjust uploadspeed per client
#define	DOWNLOADTIMEOUT				100000
#define	CONSERVTIMEOUT				25000	// agelimit for pending connection attempts
#define	RARE_FILE				50
#define	MIN_REQUESTTIME				590000
#define	MAX_PURGEQUEUETIME			HR2MS(1)
#define	PURGESOURCESWAPSTOP			MIN2MS(15)	// How long forbid swapping a source to a certain file (NNP,...)
#define	CONNECTION_LATENCY			22050	// latency for responces
#define	CLIENTBANTIME				HR2MS(2) // 2h
#define	TRACKED_CLEANUP_TIME			HR2MS(1) 
#define	KEEPTRACK_TIME				HR2MS(2) // how long to keep track of clients which were once in the uploadqueue
#define	CLIENTLIST_CLEANUP_TIME	MIN2MS(34)	// 34 min

// (4294967295/PARTSIZE)*PARTSIZE = ~4GB
#define OLD_MAX_FILE_SIZE 4290048000ull

enum FileConstants { 
	// = 2^38 = 256GB
	MAX_FILE_SIZE	= 0x4000000000ull,
	PARTSIZE		= 9728000ull,
	BLOCKSIZE		= 184320u,
	EMBLOCKSIZE		= 184320u
};

#define INV_SERV_DESC_LEN			0xF0FF	// Used as an 'invalid' string len for OP_SERVER_DESC_REQ/RES

// This 'identifier' is used for referencing shared part (incomplete) files with the OP_ASKSHAREDDIRS and related opcodes
// it was introduced with eDonkeyHybrid and is considered as part of the protocol.
#define OP_INCOMPLETE_SHARED_FILES wxT("!Incomplete Files")

// ed2k search expression comparison operators
// kad operators used to be different, but are the same since eMule 0.47a
enum ed2k_search_compare {
	ED2K_SEARCH_OP_EQUAL		=	0, // eserver 16.45+
	ED2K_SEARCH_OP_GREATER,		// dserver
	ED2K_SEARCH_OP_LESS,		// dserver
	ED2K_SEARCH_OP_GREATER_EQUAL,	// eserver 16.45+
	ED2K_SEARCH_OP_LESS_EQUAL,	// eserver 16.45+
	ED2K_SEARCH_OP_NOTEQUAL
};

#endif // ED2KCONSTANTS_H

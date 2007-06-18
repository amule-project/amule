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

#ifndef KADCONSTANTS_H
#define KADCONSTANTS_H

// Kad search expression comparison operators
enum kad_search_compare {
	KAD_SEARCH_OP_GREATER_EQUAL	=	1, // eMule 0.40+; NOTE: this different than ED2K!
	KAD_SEARCH_OP_LESS_EQUAL 					// eMule 0.40+; NOTE: this different than ED2K!
};

// MOD Note: Do not change this part - Merkur

#define	KADEMLIAASKTIME			SEC2MS(1)	//1 second
#define	KADEMLIATOTALFILE		7			//Total files to search sources for.
#define	KADEMLIAREASKTIME		HR2MS(1)	//1 hour
#define	KADEMLIAPUBLISHTIME		SEC(2)		//2 second
#define	KADEMLIATOTALSTORENOTES	1			//Total hashes to store.
#define	KADEMLIATOTALSTORESRC	2			//Total hashes to store.
#define	KADEMLIATOTALSTOREKEY	1			//Total hashes to store.
#define	KADEMLIAREPUBLISHTIMES	HR2S(5)		//5 hours
#define	KADEMLIAREPUBLISHTIMEN	HR2S(24)	//24 hours
#define	KADEMLIAREPUBLISHTIMEK	HR2S(24)	//24 hours
#define	KADEMLIADISCONNECTDELAY	MIN2S(20)	//20 mins
#define	KADEMLIAMAXINDEX		50000		//Total keyword indexes.
#define	KADEMLIAMAXENTRIES		60000		//Total keyword entries.
#define	KADEMLIAMAXSOUCEPERFILE	300			//Max number of sources per file in index.
#define	KADEMLIAMAXNOTESPERFILE	50			//Max number of notes per entry in index.
#define	KADEMLIABUDDYTIMEOUT	MIN2MS(10)	// 10 min to receive the buddy
// MOD Note: end

// Kad parameters
#define	KADEMLIA_FIND_VALUE		0x02
#define	KADEMLIA_STORE			0x04
#define	KADEMLIA_FIND_NODE		0x0B

#endif // KADCONSTANTS_H

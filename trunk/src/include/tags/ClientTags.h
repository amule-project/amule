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

#ifndef CLIENTTAGS_H
#define CLIENTTAGS_H

enum client_tags {
	CT_NAME									=	0x01,
	CT_PORT									= 0x0F,
	CT_VERSION							=	0x11,
	CT_SERVER_FLAGS					=	0x20,	// currently only used to inform a server about supported features
	CT_EMULECOMPAT_OPTIONS	=	0xEF,
	CT_EMULE_RESERVED1			=	0xF0,
	CT_EMULE_RESERVED2			= 0xF1,
	CT_EMULE_RESERVED3			= 0xF2,
	CT_EMULE_RESERVED4			= 0xF3,
	CT_EMULE_RESERVED5			=	0xF4,
	CT_EMULE_RESERVED6			=	0xF5,
	CT_EMULE_RESERVED7			= 0xF6,
	CT_EMULE_RESERVED8			=	0xF7,
	CT_EMULE_RESERVED9			=	0xF8,
	CT_EMULE_UDPPORTS				=	0xF9,
	CT_EMULE_MISCOPTIONS1		=	0xFA,
	CT_EMULE_VERSION				=	0xFB,
	CT_EMULE_BUDDYIP				=	0xFC,
	CT_EMULE_BUDDYUDP			=	0xFD,
	CT_EMULE_MISCOPTIONS2		=	0xFE,
	CT_EMULE_RESERVED13			= 0xFF
};

// Old MuleInfo tags
enum MuleInfo_tags {
	ET_COMPRESSION					= 0x20u,
	ET_UDPPORT							= 0x21u,
	ET_UDPVER								= 0x22u,
	ET_SOURCEEXCHANGE			= 0x23u,
	ET_COMMENTS						= 0x24u,
	ET_EXTENDEDREQUEST			= 0x25u,
	ET_COMPATIBLECLIENT			= 0x26u,
	ET_FEATURES							= 0x27u,		//! bit 0: SecIdent v1 - bit 1: SecIdent v2
	ET_MOD_VERSION					= 0x55u,
	// ET_FEATURESET					= 0x54u,	// int - [Bloodymad Featureset] // UNUSED
	ET_OS_INFO								= 0x94u		// Reused rand tag (MOD_OXY), because the type is unknown
};

#endif // CLIENTTAGS_H

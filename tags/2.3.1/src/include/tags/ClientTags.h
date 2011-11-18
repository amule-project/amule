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

#ifndef CLIENTTAGS_H
#define CLIENTTAGS_H

enum client_tags {
	CT_NAME				= 0x01,
	CT_SERVER_UDPSEARCH_FLAGS	= 0x0E,
	CT_PORT				= 0x0F,
	CT_VERSION			= 0x11,
	CT_SERVER_FLAGS			= 0x20,	// currently only used to inform a server about supported features
	CT_EMULECOMPAT_OPTIONS		= 0xEF,
	CT_EMULE_RESERVED1		= 0xF0,
	CT_EMULE_RESERVED2		= 0xF1,
	CT_EMULE_RESERVED3		= 0xF2,
	CT_EMULE_RESERVED4		= 0xF3,
	CT_EMULE_RESERVED5		= 0xF4,
	CT_EMULE_RESERVED6		= 0xF5,
	CT_EMULE_RESERVED7		= 0xF6,
	CT_EMULE_RESERVED8		= 0xF7,
	CT_EMULE_RESERVED9		= 0xF8,
	CT_EMULE_UDPPORTS		= 0xF9,
	CT_EMULE_MISCOPTIONS1		= 0xFA,
	CT_EMULE_VERSION		= 0xFB,
	CT_EMULE_BUDDYIP		= 0xFC,
	CT_EMULE_BUDDYUDP		= 0xFD,
	CT_EMULE_MISCOPTIONS2		= 0xFE,
	CT_EMULE_RESERVED13		= 0xFF
};

// Old MuleInfo tags
enum MuleInfo_tags {
	ET_COMPRESSION		= 0x20u,
	ET_UDPPORT		= 0x21u,
	ET_UDPVER		= 0x22u,
	ET_SOURCEEXCHANGE	= 0x23u,
	ET_COMMENTS		= 0x24u,
	ET_EXTENDEDREQUEST	= 0x25u,
	ET_COMPATIBLECLIENT	= 0x26u,
	ET_FEATURES		= 0x27u,	//! bit 0: SecIdent v1 - bit 1: SecIdent v2
	ET_MOD_VERSION		= 0x55u,
	// ET_FEATURESET	= 0x54u,	// int - [Bloodymad Featureset] // UNUSED
	ET_OS_INFO		= 0x94u		// Reused rand tag (MOD_OXY), because the type is unknown
};

// Server capabilities, values for CT_SERVER_FLAGS
enum ServerCapabilites {
	SRVCAP_ZLIB		= 0x0001,
	SRVCAP_IP_IN_LOGIN	= 0x0002,
	SRVCAP_AUXPORT		= 0x0004,
	SRVCAP_NEWTAGS		= 0x0008,
	SRVCAP_UNICODE		= 0x0010,
	SRVCAP_LARGEFILES	= 0x0100,
	SRVCAP_SUPPORTCRYPT	= 0x0200,
	SRVCAP_REQUESTCRYPT	= 0x0400,
	SRVCAP_REQUIRECRYPT	= 0x0800
};

// aMule used to use these names
#define CAPABLE_ZLIB			SRVCAP_ZLIB
#define CAPABLE_IP_IN_LOGIN_FRAME	SRVCAP_IP_IN_LOGIN
#define CAPABLE_AUXPORT			SRVCAP_AUXPORT
#define CAPABLE_NEWTAGS			SRVCAP_NEWTAGS
#define CAPABLE_UNICODE			SRVCAP_UNICODE
#define CAPABLE_LARGEFILES		SRVCAP_LARGEFILES

// Server capabilities, values for CT_SERVER_UDPSEARCH_FLAGS
enum ServerUDPCapabilities {
	SRVCAP_UDP_NEWTAGS_LARGEFILES	= 0x01
};

#endif // CLIENTTAGS_H

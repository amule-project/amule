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

#ifndef CLIENTSOFTWARE_H
#define CLIENTSOFTWARE_H

enum EClientSoftware {
	SO_EMULE			= 0,
	SO_CDONKEY			= 1,
	SO_LXMULE			= 2,
	SO_AMULE			= 3,
	SO_SHAREAZA			= 4,
	SO_EMULEPLUS		= 5,
	SO_HYDRANODE		= 6,
	SO_NEW2_MLDONKEY	= 0x0a,
	SO_LPHANT			= 0x14,
	SO_NEW2_SHAREAZA	= 0x28,
	SO_EDONKEYHYBRID	= 0x32,
	SO_EDONKEY			= 0x33,
	SO_MLDONKEY			= 0x34,
	SO_OLDEMULE			= 0x35,
	SO_UNKNOWN			= 0x36,
	SO_NEW_SHAREAZA		= 0x44,
	SO_NEW_MLDONKEY		= 0x98,
	SO_COMPAT_UNK		= 0xFF
};

#endif // CLIENTSOFTWARE_H

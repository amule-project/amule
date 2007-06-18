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

#ifndef DATAFILEVERSION_H
#define DATAFILEVERSION_H

enum PreferencesDatFileVersions {
	PREFFILE_VERSION		= 0x14 //<<-- last change: reduced .dat, by using .ini
};

enum PartMetFileVersions {
	PARTFILE_VERSION			= 0xe0,
	PARTFILE_SPLITTEDVERSION	= 0xe1, // For edonkey part files importing.
	PARTFILE_VERSION_LARGEFILE	= 0xe2
};

enum CreditFileVersions {
	CREDITFILE_VERSION		= 0x12
};

enum KnownFileListVersions {
	MET_HEADER					= 0x0E,
	MET_HEADER_WITH_LARGEFILES	= 0x0F
};

#endif // DATAFILEVERSION_H

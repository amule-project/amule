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

#ifndef CLIENTVERSION_H
#define CLIENTVERSION_H

#ifdef HAVE_CONFIG_H
	#include "config.h"	// Needed for VERSION
#endif


// eMule version used on old MuleInfo packet (unimportant).
#define	CURRENT_VERSION_SHORT			0x47

// This is only used to server login. It has no real "version" meaning anymore.
#define	EDONKEYVERSION				0x3c 

// aMule version 

// No more Mod Version unless we're cvs

// RELEASERS: REMOVE THE DEFINE ON THE RELEASES, PLEASE
// AND FIX THE MOD_VERSION_LONG

#define __SVN__

#ifndef VERSION
	#define VERSION "2.3.0"
#endif

#ifdef __SVN__
	#define	MOD_VERSION_LONG		wxT("aMule SVN")
#else
	#define	MOD_VERSION_LONG		(wxT("aMule ") wxT(VERSION))
#endif

#define	VERSION_MJR		2
#define	VERSION_MIN		3
#define	VERSION_UPDATE		0

#ifndef PACKAGE
#define PACKAGE "amule"
#endif

#endif // CLIENTVERSION_H

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

// RC_INVOKED is defined by windres when processing a .rc resource
// file. The Windows version-info resource (version.rc.in) only needs
// the VERSION_MJR/MIN/UPDATE macros below; pulling in config.h drags
// in -I${CMAKE_BINARY_DIR} which isn't on the windres command line for
// every Windows target (e.g. ed2k).
#ifndef RC_INVOKED
#include "config.h"		// Needed for VERSION
#endif

// eMule version used on old MuleInfo packet (unimportant).
#define	CURRENT_VERSION_SHORT			0x47

// This is only used to server login. It has no real "version" meaning anymore.
#define	EDONKEYVERSION				0x3c

// aMule version

// No more Mod Version unless we're cvs

// __SVN__ marks dev builds (MOD_VERSION_LONG = "aMule SVN").  CMake
// derives AMULE_TAGGED_RELEASE from `git describe --tags --exact-match`
// at configure time and forwards it via config.h, which makes the
// __SVN__ flag self-managing: tagged release builds suppress it
// automatically; source/dev builds keep it.  Manual override is still
// possible by setting -DAMULE_TAGGED_RELEASE on the cmake command line
// (e.g. when building from a tarball with no .git directory).
#ifndef AMULE_TAGGED_RELEASE
#define __SVN__
#endif

#ifndef VERSION
	#define VERSION "3.0.0-dev"
#endif

#ifdef __SVN__
	#define	MOD_VERSION_LONG		"aMule SVN"
#else
	#define	MOD_VERSION_LONG		("aMule " VERSION)
#endif

#define	VERSION_MJR		3
#define	VERSION_MIN		0
#define	VERSION_UPDATE		0

#ifndef PACKAGE
#define PACKAGE "amule"
#endif

#endif // CLIENTVERSION_H

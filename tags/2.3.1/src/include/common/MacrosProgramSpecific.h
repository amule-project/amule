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

#ifndef MACROSPROGRAMSPECIFIC_H
#define MACROSPROGRAMSPECIFIC_H

// Built program specific macros

#ifdef AMULE_DAEMON
#	define ONLY_ON_DAEMON(x)	x
#	define NOT_ON_DAEMON(x)
#else
#	define ONLY_ON_DAEMON(x)
#	define NOT_ON_DAEMON(x)		x
#endif

#ifdef CLIENT_GUI
#	define ONLY_ON_REMOTEGUI(x)	x
#	define NOT_ON_REMOTEGUI(x)
#else
#	define ONLY_ON_REMOTEGUI(x)
#	define NOT_ON_REMOTEGUI(x)	x
#endif

#define GUI_ONLY(x)	NOT_ON_DAEMON(x)
#define CORE_ONLY(x)	NOT_ON_REMOTEGUI(x)

#endif // MACROSPROGRAMSPECIFIC_H

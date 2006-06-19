//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Alo Sarv <madcat_@users.sourceforge.net>
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Timo Kujala <tiku@users.sourceforge.net>
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

#ifndef GETTICKCOUNT_H
#define GETTICKCOUNT_H

#include "Types.h"		// Needed for uint32

#ifndef __WINDOWS__
uint32 GetTickCount();
#else
#include <winbase.h> // Do_not_auto_remove
#endif

// Ideally, same than GetTickCount.
// However, on GUI, GetTickCount does only work in
// 20 msecs increment, and some classes need better.

uint32 GetTickCountFullRes(); 

uint64 GetTickCount64();

// Functions used to init the timer on GUI

void StartTickTimer();

void StopTickTimer();

#endif // GETTICKCOUNT_H
// File_checked_for_headers

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Alo Sarv ( madcat_@users.sourceforge.net )
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Copyright (c) 2002-2011 Timo Kujala ( tiku@users.sourceforge.net )
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

#include "Types.h"		// Needed for uint32, uint64

#ifdef _WIN32
	// GetTickCount64 is a system function in Vista so rename it
	#define GetTickCount64 GetTickCount_64
#endif


//return units are milliseconds, not seconds!!!
uint64 GetTickCount64();

// Functions used to init the timer on GUI

void StartTickTimer();

void StopTickTimer();

// A cheap global time (in s) without any function calls updated in OnCoreTimer
// It counts from app startup, so uint32 shall be enough for a 136 years long session
extern uint32 TheTime;

#endif // GETTICKCOUNT_H
// File_checked_for_headers

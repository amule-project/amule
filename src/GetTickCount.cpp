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

#include "GetTickCount.h" // Interface

//seconds from app start-up, check GetTickCount.h
uint32 TheTime = 0;

#ifdef __WINDOWS__

void StartTickTimer(){};

void StopTickTimer(){};

/**
 * Returns the tickcount in milliseconds in 64bits.
 */
uint64 GetTickCount_64()
{
	// Use highres timer for all operations on Windows
	// The Timer starts at system boot and runs (on a Intel Quad core)
	// with 14 million ticks per second. So it won't overflow for
	// 35000 years.

	// Convert hires ticks to milliseconds
	static double tickFactor;
	_LARGE_INTEGER li;

	static bool first = true;
	if (first) {
		// calculate the conversion factor for the highres timer
		QueryPerformanceFrequency(&li);
		tickFactor = 1000.0 / li.QuadPart;
		first = false;
	}

	QueryPerformanceCounter(&li);
	return li.QuadPart * tickFactor;
}

#else

#include <time.h>		// Needed for clock_gettime

// in milliseconds, not seconds
// avoids 32bit rollover error for differences above 50days
// since 2**32 milliseconds = 50 days aprox
uint64 GetTickCount64(void) {
	struct timespec ts;
	uint64 msecs;

	// Fetch time (Y2038-safe)
	// CLOCK_MONOTONIC: tick count is for timeouts / deltas, must not
	// jump when wall clock is adjusted. Callers that need a Unix
	// timestamp (CClientCredits::SetLastSeen, partfile source-seeds
	// serialization, wxCas defaults) should use time(NULL) instead.
	clock_gettime(CLOCK_MONOTONIC, &ts);
	msecs = (uint64) ts.tv_sec * 1000;
	msecs += ts.tv_nsec / 1000000;
	return msecs;
}

/**
 * Copyright (c) 2002-2011 Timo Kujala ( tiku@users.sourceforge.net )
 * gettimeofday() syscall based implementation. Upon request to GetTickCount(),
 * gettimeofday syscall is being used to retrieve system time and returned. This
 * means EACH GetTickCount() call will produce a new syscall, thus becoming
 * increasingly heavy on CPU as the program uptime increases and more things
 * need to be done.
 */
	void StartTickTimer() {}

	void StopTickTimer() {}

#endif
// File_checked_for_headers

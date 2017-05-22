//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 Alo Sarv ( madcat_@users.sourceforge.net )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

uint32 TheTime = 0;

#ifdef __WINDOWS__

void StartTickTimer(){};

void StopTickTimer(){};

/**
 * Returns the tickcount in full resolution using the highres timer.
 * This function replaces calls to the low res system function GetTickCOunt
 * (which can also be messed up when an app changes the system timer resolution)
 */
uint32 GetTickCountFullRes() {
	return GetTickCount_64();
}

/**
 * Returns the tickcount in 64bits.
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

#include <sys/time.h>		// Needed for gettimeofday

uint32 GetTickCountFullRes(void) {
	struct timeval aika;
	gettimeofday(&aika,NULL);
	unsigned long msecs = aika.tv_sec * 1000;
	msecs += (aika.tv_usec / 1000);
	return msecs;
}

#if wxUSE_GUI && wxUSE_TIMER && !defined(AMULE_DAEMON)
/**
 * Copyright (c) 2003-2011 Alo Sarv ( madcat_@users.sourceforge.net )
 * wxTimer based implementation. wxGetLocalTimeMillis() is called every 2
 * milliseconds and values stored in local variables. Upon requests for current
 * time, values of those variables are returned. This means wxGetLocalTimeMillis
 * is being called exactly 50 times per second at any case, no more no less.
 */
	#include <wx/timer.h>

	class MyTimer : public wxTimer
	{
	public:
		MyTimer() { tic32 = tic64 = wxGetLocalTimeMillis().GetValue(); Start(20); }
		static uint32 GetTickCountNow() { return tic32; }
		static uint64 GetTickCountNow64() { return tic64; }
	private:
		void Notify() { tic32 = tic64 = wxGetLocalTimeMillis().GetValue(); }

		static uint32 tic32;
		static uint64 tic64;
	};

	static class MyTimer* mytimer;

	// Initialization of the static MyTimer member variables.
	uint32 MyTimer::tic32 = 0;
	uint64 MyTimer::tic64 = 0;

	void StartTickTimer() {
		wxASSERT(mytimer == NULL);
		mytimer = new MyTimer();
	}

	void StopTickTimer() {
		wxASSERT(mytimer != NULL);
		delete mytimer;
		mytimer = NULL;
	}

	uint32 GetTickCount(){
		wxASSERT(mytimer != NULL);
		return MyTimer::GetTickCountNow();
	}

	uint64 GetTickCount64(){
		wxASSERT(mytimer != NULL);
		return MyTimer::GetTickCountNow64();
	}

#else
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

	uint32 GetTickCount() { return GetTickCountFullRes(); }

	// avoids 32bit rollover error for differences above 50days
	uint64 GetTickCount64() {
		struct timeval aika;
		gettimeofday(&aika,NULL);
		return aika.tv_sec * (uint64)1000 + aika.tv_usec / 1000;
	}

#endif

#endif
// File_checked_for_headers

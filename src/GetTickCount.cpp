//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2007 Alo Sarv <madcat_@users.sourceforge.net>
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "GetTickCount.h" // Interface

#ifdef __WINDOWS__

void StartTickTimer(){};

void StopTickTimer(){};

uint32 GetTickCountFullRes() {
	return GetTickCount();
}
	
/**
 * Returns the tickcount in 64bits.
 */
uint64 GetTickCount64()
{
	// The base value, can store more than 49 days worth of ms.
	static uint64 tick = 0;
	// The current tickcount, may have overflown any number of times.
	static uint32 lastTick = 0;

	uint32 curTick = GetTickCount();

	// Check for overflow
	if ( curTick < lastTick ) {
		// Change the base value to contain the overflown value.
		tick += (uint64)1 << 32;
	}
		
	lastTick = curTick;
	return tick + lastTick;
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
 * Copyright (c) 2004-2007 Alo Sarv <madcat_@users.sourceforge.net>
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
 * Copyright (c) 2003-2007 Timo Kujala <tiku@users.sourceforge.net>
 * gettimeofday() syscall based implementation. Upon request to GetTickCount(),
 * gettimeofday syscall is being used to retrieve system time and returned. This
 * means EACH GetTickCount() call will produce a new syscall, thus becoming
 * increasingly heavy on CPU as the program uptime increases and more things
 * need to be done.
 */
	void StartTickTimer(){};

	void StopTickTimer(){};

	uint32 GetTickCount() { return GetTickCountFullRes(); }
		
	// avoids 32bit rollover error for differences above 50days
	uint64 GetTickCount64() {
		struct timeval aika;
		gettimeofday(&aika,NULL);
		return aika.tv_sec * 1000 + aika.tv_usec / 1000;
	}

#endif

#endif
// File_checked_for_headers

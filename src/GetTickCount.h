//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Alo Sarv <madcat_@users.sourceforge.net>
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef GETTICKCOUNT_H
#define GETTICKCOUNT_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
// implementation in amule.cpp & WebServer.cpp & amule-remote-gui.cpp
#pragma interface "GetTickCount.h"
#endif

#include "Types.h"		// Needed for uint32

#ifdef __WXMSW__
#include <winbase.h>
#endif

#if wxUSE_GUI && wxUSE_TIMER && !defined(AMULE_DAEMON)
/**
 * Copyright (c) 2004-2005 Alo Sarv <madcat_@users.sourceforge.net>
 * wxTimer based implementation. wxGetLocalTimeMillis() is called every 2
 * milliseconds and values stored in local variables. Upon requests for current
 * time, values of those variables are returned. This means wxGetLocalTimeMillis
 * is being called exactly 50 times per second at any case, no more no less.
 */
	#include <wx/timer.h>
	#ifndef __WXMSW__ // already defined in winbase.h
	#define GetTickCount() MyTimer::GetTickCountNow()
	#endif
	#define GetTickCount64() MyTimer::GetTickCountNow64()
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


#else
/**
 * Copyright (c) 2003-2005 Timo Kujala <tiku@users.sourceforge.net>
 * gettimeofday() syscall based implementation. Upon request to GetTickCount(),
 * gettimeofday syscall is being used to retrieve system time and returned. This
 * means EACH GetTickCount() call will produce a new syscall, thus becoming
 * increasingly heavy on CPU as the program uptime increases and more things
 * need to be done.
 */
	#include <cstddef>		// Needed for NULL
	#include <sys/time.h>		// Needed for gettimeofday

	inline uint32 GetTickCount(void) {
		struct timeval aika;
		gettimeofday(&aika,NULL);
		unsigned long secs = aika.tv_sec * 1000;
		secs += (aika.tv_usec / 1000);
		return secs;
	}

	// avoids 32bit rollover error for differences above 50days
	inline uint64 GetTickCount64() {
		struct timeval aika;
		gettimeofday(&aika,NULL);
		return aika.tv_sec * 1000 + aika.tv_usec / 1000;
	}

#endif

#endif

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef TYPES_H
#define TYPES_H

#include <wx/defs.h>
#include <inttypes.h>
#include <wx/dynarray.h>
/* 
 * Backwards compatibility with emule.
 * Note that the int* types are indeed unsigned.
 */
typedef uint8_t		int8;
typedef uint8_t		uint8;
typedef uint16_t	int16;
typedef uint16_t	uint16;
typedef uint32_t	int32;
typedef uint32_t	uint32;
typedef uint64_t	int64;
typedef uint64_t	uint64;
typedef int8_t		sint8;
typedef int16_t		sint16;
typedef int32_t		sint32;
typedef int64_t		sint64;
typedef uint8_t		uchar;
typedef uint8_t		BYTE;

WX_DEFINE_ARRAY_SHORT(uint16, ArrayOfUInts16);

#ifndef __cplusplus
	typedef int bool;
#endif

#ifdef __WXMSW__
	#include <windef.h>
	#include <wingdi.h>
	#include <winuser.h>
	#include <wx/msw/winundef.h>	/* Needed to be able to include mingw headers */

	typedef uint32_t	in_addr_t;
	typedef uint8_t		byte;
	typedef uint32_t	uint;
	typedef uint16_t	WORD;
	typedef uint32_t	UINT;
#else 
	typedef bool		BOOL;

	typedef uint32_t	UINT;
	typedef uint32_t*	UINT_PTR;
	typedef uint32_t*	PUINT;

	typedef uint8_t		byte;
	typedef uint32_t	DWORD;
	typedef long		LONG;
	typedef unsigned long	ULONG;
	typedef unsigned long long	ULONGLONG;
	typedef char*   	LPBYTE;
	typedef int32_t		INT;
	typedef uint16_t	WORD;
	typedef uint16_t	word;
	
	typedef void       	VOID;
	typedef VOID*      	PVOID;
	typedef VOID*      	LPVOID;
	typedef const VOID*	LPCVOID;

	typedef char        	CHAR;
	typedef CHAR*       	LPSTR;
	typedef const CHAR* 	LPCSTR;
	typedef char        	TCHAR;
	typedef TCHAR*      	LPTSTR;
	typedef const TCHAR*	LPCTSTR;

	typedef wchar_t		WCHAR;
	typedef WCHAR*		LPWSTR;
	typedef const WCHAR*	LPCWSTR;

	typedef uint32_t	COLORREF;
	typedef int16_t 	WPARAM;
	typedef int32_t 	LPARAM;

	typedef struct sPOINT {
	  UINT x;
	  UINT y;
	} POINT;

	typedef struct sRECT {
	  UINT left;
	  UINT top;
	  UINT right;
	  UINT bottom;
	} RECT;

	typedef RECT		*LPRECT;

	typedef struct sWINDOWPLACEMENT {
	  UINT length;
	  UINT flags;
	  UINT showCmd;
	  POINT ptMinPosition;
	  POINT ptMaxPosition;
	  RECT rcNormalPosition;
	} WINDOWPLACEMENT;
#endif /* __WXMSW__ */

#ifdef AMULE_DAEMON
#define AMULE_TIMER_CLASS CTimer
#define AMULE_TIMER_EVENT_CLASS wxEvent
class wxEvtHandler;

#include <wx/thread.h>

/*
 * replace wxTimer with my own on non-X builds
 */
class CTimer {
	wxEvtHandler *owner;
	int id;
	class CTimerThread : public wxThread {
		unsigned long m_period;
		bool m_oneShot;
		wxEvtHandler *m_owner;
		int m_id;
		
		void *Entry();

		public:
		CTimerThread(wxEvtHandler *owner, unsigned long period, bool oneShot, int id);
	};
	CTimerThread *thread;
	public:
	CTimer(wxEvtHandler *owner = 0, int timerid = -1);
	~CTimer();
	void SetOwner(wxEvtHandler *owner, int id = -1);
	bool Start(int millisecs, bool oneShot = false);
	bool IsRunning() const;
	void Stop();
};

#else
#define AMULE_TIMER_CLASS wxTimer
#define AMULE_TIMER_EVENT_CLASS wxTimerEvent
#endif

/*
 * Check version stuff
 */
#ifndef wxSUBRELEASE_NUMBER
	#define wxSUBRELEASE_NUMBER 0
#endif

#ifndef wxCHECK_VERSION_FULL

	#define wxCHECK_VERSION_FULL(major,minor,release,subrel) \
		(wxMAJOR_VERSION > (major) || \
		(wxMAJOR_VERSION == (major) && wxMINOR_VERSION > (minor)) || \
		(wxMAJOR_VERSION == (major) && wxMINOR_VERSION == (minor) && \
		 	wxRELEASE_NUMBER > (release)) || \
		 wxMAJOR_VERSION == (major) && wxMINOR_VERSION == (minor) && \
		 	wxRELEASE_NUMBER == (release) && wxSUBRELEASE_NUMBER >= (subrel))

#endif

#endif /* TYPES_H */


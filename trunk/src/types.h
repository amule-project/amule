// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef TYPES_H
#define TYPES_H

#include <wx/defs.h>
#include <inttypes.h>
// Backwards compatibility with emule.
// Note that the int* types are indeed unsigned.
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

#ifdef __WXMSW__
	#include <windef.h>
	#include <wingdi.h>
	#include <winuser.h>
	#include <wx/msw/winundef.h>    // Needed to be able to include mingw headers

	typedef uint32_t	in_addr_t;
	typedef uint8_t		byte;
	typedef uint32_t	uint;
	typedef uint16_t	WORD;
	typedef uint32_t	UINT;
#else 
	typedef uint32_t	UINT;
	typedef uint32_t*	UINT_PTR;
	typedef char*		LPCSTR;
	typedef char*		LPSTR;
	typedef char*		LPCTSTR;
	typedef uint8_t		byte;
	typedef uint32_t	DWORD;
	typedef int32_t		LONG;
	typedef uint64_t	ULONGLONG;
	typedef char*		LPBYTE;
	typedef int32_t		INT;
	typedef uint16_t	WORD;
	typedef uint16_t	word;
	typedef void		VOID;
	typedef void*		LPVOID;
	typedef void*		PVOID;
	typedef char		TCHAR;
	typedef uint32_t	COLORREF;
	typedef int16_t		WPARAM;
	typedef int32_t		LPARAM;

	struct POINT {
	  UINT x;
	  UINT y;
	};

	struct RECT {
	  UINT left;
	  UINT top;
	  UINT right;
	  UINT bottom;
	};

	typedef RECT*			LPRECT;

	struct WINDOWPLACEMENT {
	  UINT length;
	  UINT flags;
	  UINT showCmd;
	  POINT ptMinPosition;
	  POINT ptMaxPosition;
	  RECT rcNormalPosition;
	};
#endif // __WXMSW__

#endif // TYPES_H

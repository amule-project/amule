//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef TYPES_H
#define TYPES_H

#include <wx/string.h>		// Needed for wxString and wxEmptyString

#include <list>			// Needed for std::list
#include <vector>		// Needed for std::vector
#include <inttypes.h>		// Do_not_auto_remove (Old gccs)


// These are MSVC defines used in eMule. They should 
// not be used in aMule, instead, use this table to 
// find the type to use in order to get the desired 
// effect. 
//////////////////////////////////////////////////
// Name              // Type To Use In Amule    //
//////////////////////////////////////////////////
// BOOL              // bool                    //
// WORD              // uint16                  //
// INT               // int32                   //
// UINT              // uint32                  //
// UINT_PTR          // uint32*                 //
// PUINT             // uint32*                 //
// DWORD             // uint32                  //
// LONG              // long                    //
// ULONG             // unsigned long           //
// LONGLONG          // long long               //
// ULONGLONG         // unsigned long long      //
// LPBYTE            // char*                   //
// VOID              // void                    //
// PVOID             // void*                   //
// LPVOID            // void*                   //
// LPCVOID           // const void*             //
// CHAR              // char                    //
// LPSTR             // char*                   //
// LPCSTR            // const char*             //
// TCHAR             // char                    //
// LPTSTR            // char*                   //
// LPCTSTR           // const char*             //
// WCHAR             // wchar_t                 //
// LPWSTR            // wchar_t*                //
// LPCWSTR           // const wchar_t*          //
// WPARAM            // uint16                  //
// LPARAM            // uint32                  //
// POINT             // wxPoint                 //
//////////////////////////////////////////////////

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
typedef uint8_t		byte;



class CKnownFile;
class CUpDownClient;

//! Various common list-types.
//@{ 
typedef std::list<wxString> CStringList;
typedef std::list<CKnownFile*> CKnownFilePtrList;
typedef std::list<CUpDownClient*> CClientPtrList;
//@}

typedef std::vector<uint16> ArrayOfUInts16;

/* This is the Evil Void String For Returning On Const References From Hell */
static const wxString EmptyString = wxEmptyString;

#ifndef __cplusplus
	typedef int bool;
#endif


#ifdef __WXMSW__
	#include <windef.h> // Needed for RECT  // Do_not_auto_remove
	#include <wingdi.h> // Do_not_auto_remove
	#include <winuser.h> // Do_not_auto_remove
	#include <wx/msw/winundef.h>	/* Needed to be able to include mingw headers */  // Do_not_auto_remove

#else 

	typedef struct sRECT {
	  uint32 left;
	  uint32 top;
	  uint32 right;
	  uint32 bottom;
	} RECT;

#endif /* __WXMSW__ */

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
// File_checked_for_headers

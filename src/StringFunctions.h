// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Angel Vidal Veiga - Kry (kry@amule.org)
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

#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include "types.h"		// Needed for uint16 and uint32
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/strconv.h>

/****************************************************/ 
/******************* Inlines ************************/
/****************************************************/

// Unicode <-> char* conversion functions

/*
 * Please, DO NOT store pointers returned by unicode2char(), because they 
 * get free'ed as soon as the return value of cWX2MB gets out of scope.
 * If you need to store a pointer, use unicode2charbuf() instead, which has
 * a return type of wxCharBuffer, and then cast it to a char pointer, e.g.:
 * 
 * const wxCharBuffer buf = unicode2charbuf(aWxString);
 * const char *p = (const char *)buf;
 * 
 * --- Now you can freely use p                              ---
 * --- don't worry about memory allocation, memory will be   ---
 * --- free'ed when buf gets out of scope, i.e., upon return ---
 * 
 */
static wxCSConv aMuleConv(wxT("iso8859-1"));
#if wxUSE_UNICODE
	inline const char* unicode2char(wxString x) { return ((const char*) aMuleConv.cWX2MB(x));};
	inline const wxCharBuffer unicode2charbuf(wxString x) { return aMuleConv.cWX2MB(x); };
	inline const wxWCharBuffer char2unicode(const char* x) { return aMuleConv.cMB2WX(x); };
#else
	inline const char* unicode2char(wxString x) { return ((const char*) x); };
	inline const wxCharBuffer unicode2charbuf(wxString x) { return (const char*)x; };
	inline const wxCharBuffer char2unicode(const char* x) { return x; };
#endif

inline const char *aMuleConvToUTF8(const wxString &x) { 
	return wxConvUTF8.cWC2MB(wxString(x).wc_str(aMuleConv));
};

// Replaces "&" with "&&" in 'in' for use with text-labels
inline wxString MakeStringEscaped(wxString in) {
	in.Replace(wxT("&"),wxT("&&"));
	return in;
}

// Make a string be a folder
inline wxString MakeFoldername(wxString path) {
	#warning Who commented out this?
	/*
	if ( !path.IsEmpty() && ( path.Right(1) == wxT('/' )) ) {
		path.RemoveLast();
	}
	*/
	return path;
}

// Duplicates a string
inline char* nstrdup(const char* src)
{
	int len = (src ? strlen(src) : 0) + 1;
	char *res = new char[len];
	if ( src ) strcpy(res, src);
	res[len-1] = 0;
	return res;
}


// Replacements for atoi and atol that removes the need for converting
// a string to normal chars with unicode2char. The value returned is the
// value represented in the string or 0 if the conversion failed.
inline long StrToLong( const wxString& str ) {
	long value = 0;
	str.ToLong( &value );
	return value;
}

inline unsigned long StrToULong( const wxString& str ) {
	unsigned long value = 0;
	str.ToULong( &value );
	return value;
}

/****************************************************/ 
/***************** Non-inlines **********************/
/****************************************************/

/**
 * Truncates a filename to the specified length.
 *
 * @param filename The original filename.
 * @param length The max length of the resulting filename.
 * @param isFilePath If true, then the path will be truncated rather than the filename if possible.
 * @return The truncated filename.
 */
wxString TruncateFilename(const wxString& filename, size_t length, bool isFilePath = false);

// Makes sIn suitable for inclusion in an URL, by escaping all chars that could cause trouble.
wxString URLEncode(wxString sIn);
	
#endif // STRING_FUNCTIONS_H

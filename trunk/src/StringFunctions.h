//
// This file is part of the aMule Project.
//
// Copyright (c) 2004 Angel Vidal Veiga - Kry (kry@amule.org)
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
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

#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "StringFunctions.h"
#endif

#include "Types.h"		// Needed for uint16 and uint32
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/strconv.h>

// UTF8 types: No UTF8, BOM prefix, or Raw UTF8
enum EUtf8Str
{
	utf8strNone,
	utf8strOptBOM,
	utf8strRaw
};

/****************************************************/ 
/******************* Inlines ************************/
/****************************************************/

/** 
 * Functions to perform Unicode <-> (char *) and UTF-8 conversion
 * 
 * Please, DO NOT store pointers returned by unicode2char(), because they 
 * get free'ed as soon as the return value of cWX2MB gets out of scope.
 * If you need to store a pointer, use a buffer of type wxWX2MBbuf:
 * and then cast it to a char pointer, e.g.:
 * 
 * const wxWX2MBbuf buf(unicode2char(aWxString));
 * 
 * --- Now you can freely use buf as if it were a (const char *) ---
 * 
 * puts(buf);
 * printf("%s", (const char *)buf);
 *
 * The cast in printf is necessary because variable number of parameter
 * functions have no type for these parameters, so the automatic casting
 * of wxWX2MBbuf to (const char *) is not performed.
 * 
 * --- don't worry about memory allocation, memory will be       ---
 * --- free'ed when buf gets out of scope, i.e., upon return     ---
 * 
 * wxMB2WXbuf, wxWX2MBbuf are always the appropriate return type,
 * either (wxChar *) or (wxWCharBuffer)
 *
 * Use the simplified names Unicode2CharBuf and Char2UnicodeBuf, and
 * do not declare these names const or the compiler will complain about
 * a double const.
 */
#define Unicode2CharBuf	const wxWX2MBbuf
#define Char2UnicodeBuf const wxMB2WXbuf

static wxCSConv aMuleConv(wxT("iso8859-1"));

inline Unicode2CharBuf unicode2char(wxString    x) { return (const char *)aMuleConv.cWX2MB(x); }
inline Char2UnicodeBuf char2unicode(const char *x) { return               aMuleConv.cMB2WX(x); }

inline Unicode2CharBuf unicode2UTF8(wxString    x) { return (const char *)wxConvUTF8.cWX2MB(x); }
inline Char2UnicodeBuf UTF82unicode(const char *x) { return               wxConvUTF8.cMB2WX(x); }

inline const wxCharBuffer char2UTF8(const char *x) { return unicode2UTF8(char2unicode(x)); }
inline const wxCharBuffer UTF82char(const char *x) { return unicode2char(UTF82unicode(x)); }

//
// Replaces "&" with "&&" in 'in' for use with text-labels
//
inline wxString MakeStringEscaped(wxString in) {
	in.Replace(wxT("&"),wxT("&&"));
	return in;
}

// Make a string be a folder
inline wxString MakeFoldername(wxString path) {

	if ( !path.IsEmpty() && ( path.Right(1) == wxT('/' )) ) {
		path.RemoveLast();
	}

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

inline bool NeedUTF8String(wchar_t* pwsz)
{
	while (*pwsz != L'\0')
	{
		if (*pwsz >= 0x100)
			return true;
		pwsz++;
	}
	return false;
}

inline unsigned int GetRawSize(const wxString& rstr, EUtf8Str eEncode)
{
	unsigned int RealLen = 0;
	switch (eEncode) {
		case utf8strOptBOM:
			RealLen = 3;
		case utf8strRaw: {
			Unicode2CharBuf s(unicode2UTF8(rstr));
			if (s) {
				RealLen += strlen(s);
				break;
			} else {
				RealLen = 0;
			}
		}
		default: {
			Unicode2CharBuf s(unicode2char(rstr));
			if (s) {
				RealLen = strlen(s);
			}
		}
	}
	return RealLen;
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

wxString CleanupFilename(const wxString& filename, bool keepSpaces = true);

// Makes sIn suitable for inclusion in an URL, by escaping all chars that could cause trouble.
wxString URLEncode(wxString sIn);
	
#endif // STRING_FUNCTIONS_H

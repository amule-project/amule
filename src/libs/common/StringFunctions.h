//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal Veiga - Kry (kry@amule.org)
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef STRING_FUNCTIONS_H
#define STRING_FUNCTIONS_H

#include "../../Types.h"		// Needed for uint16 and uint32
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/strconv.h>
#include <wx/filename.h>

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
typedef const wxWX2MBbuf Unicode2CharBuf;
typedef const wxMB2WXbuf Char2UnicodeBuf;

static wxCSConv aMuleConv(wxT("iso8859-1"));

inline Unicode2CharBuf	unicode2char(const wxChar* x)	{ return aMuleConv.cWX2MB(x); }
inline Char2UnicodeBuf	char2unicode(const char* x)		{ return aMuleConv.cMB2WX(x); }

inline Unicode2CharBuf unicode2UTF8(const wxChar* x)	{ return wxConvUTF8.cWX2MB(x); }
inline Char2UnicodeBuf UTF82unicode(const char* x)		{ return wxConvUTF8.cMB2WX(x); }

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

inline unsigned long long StrToULongLong( const wxString& str ) {
	return atoll(unicode2char(str));
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

/**
 * Removes invalid chars from the filename.
 *
 * @param filename the filename to clean.
 * @param keepSpace If false, spaces are replaced with underscores.
 * @param fat32 If true, chars invalid on fat32 are also replaced.
 *
 * Note that fat32 is always considered to be true on wxMSW.
 */ 
wxString CleanupFilename(const wxString& filename, bool keepSpaces = true, bool fat32 = false);


/**
 * Strips all path separators from the specified end of a path.
 *
 * Note: type must be either leading or trailing.
 */
wxString StripSeparators(wxString path, wxString::stripType type);


/**
 * Joins two path with the operating system specific path-separator.
 *
 * If any of the parameters are empty, the other parameter is
 * returned unchanged.
 */
wxString JoinPaths(const wxString& path, const wxString& file);

// Makes sIn suitable for inclusion in an URL, by escaping all chars that could cause trouble.
wxString URLEncode(const wxString& sIn);


/**
 * Converts a hexadecimal number to a char.
 *
 * @param hex The hex-number, must be at most 2 digits long.
 * @return The resulting char or \0 if conversion failed.
 */
wxChar HexToDec( const wxString& hex );


/**
 * This function converts all valid HTML escape-codes to their corresponding chars.
 *
 * @param str The string to unescape.
 * @return The unescaped version of the input string.
 */
wxString UnescapeHTML( const wxString& str );


/**
 * Ensures that the url pass is valid by escaping various chars.
 */
wxString validateURI(const wxString& url);


/**
 * This class provides a simple and fast tokenizer.
 */
class CSimpleTokenizer
{
public:
	/**
	 * @param str The string to tokenize.
	 * @param delim The delimiter used to split the string.
	 */
	CSimpleTokenizer(const wxString& str, wxChar delim);

	/**
	 * Returns the next part of the string separated by the
	 * given delimiter. When the entire string has been
	 * tokenized, an empty string is returned. Note that
	 * empty tokens are also returned.
	 */
	wxString next();

	/**
	 * Returns the remaining part of the string.
	 *
	 * The remaining part is defined as being the part after
	 * the last encountered token, or an empty string if the
	 * entire string has been tokenized.
	 *
	 * If next() has yet to be called, the entire string will
	 * be returned.
	 */
	wxString remaining() const;

	/**
	 * Returns the number of tokens encountered so far.
	 */
	size_t tokenCount() const;

private:
	//! The string being tokenized.
	wxString m_string;
	
	//! The delimiter used to split the string.
	wxChar m_delim;
	
	//! A pointer to the current position in the string.
	const wxChar* m_ptr;

	//! The number of tokens encountered.
	size_t m_count;
};


#endif // STRING_FUNCTIONS_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
 * If you need to store a pointer, use a buffer of type wxCharBuffer:
 * and then cast it to a char pointer, e.g.:
 *
 * const wxCharBuffer buf(unicode2char(aWxString));
 *
 * --- Now you can freely use buf as if it were a (const char *) ---
 *
 * puts(buf);
 * printf("%s", (const char *)buf);
 *
 * The cast in printf is necessary because variable number of parameter
 * functions have no type for these parameters, so the automatic casting
 * of wxCharBuffer to (const char *) is not performed.
 *
 * --- don't worry about memory allocation, memory will be       ---
 * --- free'ed when buf gets out of scope, i.e., upon return     ---
 *
 * wxWCharBuffer, wxCharBuffer are always the appropriate return type,
 * either (wxChar *) or (wxWCharBuffer)
 *
 * Use the simplified names Unicode2CharBuf and Char2UnicodeBuf, and
 * do not declare these names const or the compiler will complain about
 * a double const.
 */
typedef const wxCharBuffer Unicode2CharBuf;
typedef const wxWCharBuffer Char2UnicodeBuf;

Unicode2CharBuf unicode2char(const wxChar* x);
Unicode2CharBuf unicode2char(const Char2UnicodeBuf& x);
inline Unicode2CharBuf unicode2char(const wxString& x)		{ return unicode2char(x.wc_str()); }
inline Char2UnicodeBuf char2unicode(const char* x)	{ return wxConvLocal.cMB2WX(x); }

inline Unicode2CharBuf unicode2UTF8(const wxChar* x)	{ return wxConvUTF8.cWX2MB(x); }
inline Unicode2CharBuf unicode2UTF8(const Char2UnicodeBuf& x)	{ return wxConvUTF8.cWX2MB(x); }
inline Unicode2CharBuf unicode2UTF8(const wxString& x)	{ return x.utf8_str(); }
inline Char2UnicodeBuf UTF82unicode(const char* x)	{ return wxConvUTF8.cMB2WX(x); }

inline const wxCharBuffer char2UTF8(const char *x)	{ return unicode2UTF8(char2unicode(x)); }
inline const wxCharBuffer UTF82char(const char *x)	{ return unicode2char(UTF82unicode(x)); }

inline Unicode2CharBuf filename2char(const wxChar* x)	{ return wxConvFile.cWC2MB(x); }
inline Unicode2CharBuf filename2char(const wxString& x)	{ return x.mb_str(wxConvFile); }
inline Char2UnicodeBuf char2filename(const char* x)	{ return wxConvFile.cMB2WC(x); }


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
	size_t len = (src ? strlen(src) : 0) + 1;
	char *res = new char[len];
	if ( src ) strcpy(res, src);
	res[len-1] = 0;
	return res;
}


// Replacements for atoi and atol that removes the need for converting
// a string to normal chars with unicode2char. The value returned is the
// value represented in the string or 0 if the conversion failed.
inline long StrToLong(const wxString& str)
{
	long value = 0;
	if (!str.ToLong(&value)) {	// value may be changed even if it failes according to wx docu
		value = 0;
	}
	return value;
}

inline unsigned long StrToULong(const wxString& str)
{
	unsigned long value = 0;
	if (!str.ToULong(&value)) {
		value = 0;
	}
	return value;
}

inline unsigned long long StrToULongLong(const wxString& str)
{
#if wxCHECK_VERSION(2, 9, 0)
	unsigned long long value = 0;
	if (!str.ToULongLong(&value)) {
		value = 0;
	}
	return value;

#else	// wx 2.8

	Unicode2CharBuf buf = unicode2char(str);
	if (!buf) {		// something went wrong
		return 0;
	}
#ifdef _MSC_VER
	return _atoi64(buf);
#else
	return atoll(buf);
#endif
#endif	// wx 2.8
}

inline size_t GetRawSize(const wxString& rstr, EUtf8Str eEncode)
{
	size_t RealLen = 0;
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
 * Compares two strings, while taking numerals into consideration.
 *
 * @return Returns -1 if a < b, 1 if a > b and 0 if a = b
 *
 * This function basically splits the two strings into a number of
 * fields, deliniated by whitespace, non-alphanumerical chars. The
 * numerals are then converted to integers, and the fields are
 * compared. This allows strings such as "a (2)" and "a (10)" to
 * be properly sorted for displaying.
 *
 * Currently does not handle floats (they are treated as to seperate
 * fields, nor negative numbers.
 */
int FuzzyStrCmp(const wxString& a, const wxString& b);

/**
 * As with FuzzyStrCmp, but case insensitive.
 */
int FuzzyStrCaseCmp(const wxString& a, const wxString& b);


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
// File_checked_for_headers

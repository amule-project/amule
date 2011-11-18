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

#include "StringFunctions.h"

#include <wx/filename.h>	// Needed for wxFileName
#include <wx/uri.h>		// Needed for wxURI

#include <cstring>		// Needed for std::strlen()

// Implementation of the non-inlines

//
// Conversion of wxString so it can be used by printf() in a console
// On some platforms (Windows) the console allows only "plain" characters,
// so try to convert as much as possible and replace the others with '?'.
// On other platforms (some Linux) wxConvLocal silently converts to UTF8
// so the console can show even Chinese chars.
//
Unicode2CharBuf unicode2char(const wxChar* s)
{
	// First try the straight way.
	Unicode2CharBuf buf1(wxConvLocal.cWX2MB(s));
	if ((const char *) buf1) {
		return buf1;
	}
	// Failed. Try to convert as much as possible.
	size_t len = wxStrlen(s);
	size_t maxlen = len * 4;		// Allow for an encoding of up to 4 byte per char.
	wxCharBuffer buf(maxlen + 1);	// This is wasteful, but the string is used temporary anyway.
	char * data = buf.data();
	for (size_t i = 0, pos = 0; i < len; i++) {
		size_t len_char = wxConvLocal.FromWChar(data + pos, maxlen - pos, s + i, 1);
		if (len_char != wxCONV_FAILED) {
			pos += len_char - 1;
		} else if (pos < maxlen) {
			data[pos++] = '?';
			data[pos] = 0;
		}
	}
	return buf;
}


static byte base16Chars[17] = "0123456789ABCDEF";

wxString URLEncode(const wxString& sIn)
{
	wxString sOut;
	unsigned char curChar;
	
	for ( unsigned int i = 0; i < sIn.Length(); ++i ) {
		curChar = sIn.GetChar( i );

		if ( isalnum( curChar ) ) {
	        sOut += curChar;
	    } else if( isspace ( curChar ) ) {
		    sOut += wxT("+");
		} else {
			sOut += wxT("%");
			sOut += base16Chars[ curChar >> 4];
			sOut += base16Chars[ curChar & 0xf];
		}

	}

	return sOut;
}


wxChar HexToDec( const wxString& hex )
{
	wxChar result = 0;
	wxString str = hex.Upper();
	
	for ( size_t i = 0; i < str.Len(); ++i ) {
		result *= 16;
		wxChar cur = str.GetChar(i);
		
		if ( isdigit( cur ) ) {
			result += cur - wxT('0');
		} else if ( cur >= wxT('A') && cur <= wxT('F') ) {
			result += cur - wxT('A') + 10;
		} else {
			return wxT('\0');
		}
	}

	return result;
}


wxString UnescapeHTML(const wxString& str)
{
	wxWritableCharBuffer buf = str.char_str(wxConvUTF8);

	// Work around wxWritableCharBuffer's operator[] not being writable
	char *buffer = (char *)buf;

	size_t len = std::strlen(buffer);
	size_t j = 0;
	for (size_t i = 0; i < len; ++i, ++j) {
		if (buffer[i] == '%' && (len > i + 2)) {
			wxChar unesc = HexToDec(str.Mid(i + 1, 2));
			if (unesc) {
				i += 2;
				buffer[j] = (char)unesc;
			} else {
				// If conversion failed, then we just add the escape-code
				// and continue past it like nothing happened.
				buffer[j] = buffer[i];
			}
		} else {
			buffer[j] = buffer[i];
		}
	}
	buffer[j] = '\0';

	// Try to interpret the result as UTF-8
	wxString result(buffer, wxConvUTF8);
	if (len > 0 && result.length() == 0) {
		// Fall back to ISO-8859-1
		result = wxString(buffer, wxConvISO8859_1);
	}

	return result;
}


wxString validateURI(const wxString& url)
{
	wxURI uri(url);
	
	return uri.BuildURI();
}


enum ECharType {
	ECTInteger,
	ECTText,
	ECTNone
};

inline wxString GetNextField(const wxString& str, size_t& cookie)
{
	// These are taken to seperate "fields"
	static const wxChar* s_delims = wxT("\t\n\x0b\x0c\r !\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~");
	
	wxString field;
	ECharType curType = ECTNone;
	for (; cookie < str.Length(); ++cookie) {
		wxChar c = str[cookie];

		if ((c >= wxT('0')) && (c <= wxT('9'))) {
			if (curType == ECTText) {
				break;
			}

			curType = ECTInteger;
			field += c;
		} else if (wxStrchr(s_delims, c)) {
			if (curType == ECTNone) {
				continue;
			} else {
				break;
			}
		} else {
			if (curType == ECTInteger) {
				break;
			}

			curType = ECTText;
			field += c;
		}
	}

	return field;
}


int FuzzyStrCmp(const wxString& a, const wxString& b)
{
	size_t aCookie = 0, bCookie = 0;
	wxString aField, bField;

	do {
		aField = GetNextField(a, aCookie);
		bField = GetNextField(b, bCookie);

		if (aField.IsNumber() && bField.IsNumber()) {
			unsigned long aInteger = StrToULong(aField);
			unsigned long bInteger = StrToULong(bField);
			
			if (aInteger < bInteger) {
				return -1;
			} else if (aInteger > bInteger) {
				return  1;
			}
		} else if (aField < bField) {
			return -1;
		} else if (aField > bField) {
			return  1;
		}
	} while (!aField.IsEmpty() && !bField.IsEmpty());

	return 0;
}


int FuzzyStrCaseCmp(const wxString& a, const wxString& b)
{
	return FuzzyStrCmp(a.Lower(), b.Lower());
}


	
CSimpleTokenizer::CSimpleTokenizer(const wxString& str, wxChar token)
	: m_string(str),
	  m_delim(token),
	  m_ptr(m_string.c_str()),
	  m_count(0)
{
}


wxString CSimpleTokenizer::next()
{
	const wxChar* start = m_ptr;
	const wxChar* end   = m_string.c_str() + m_string.Len() + 1;

	for (; m_ptr < end; ++m_ptr) {
		if (*m_ptr == m_delim) {
			m_count++;
			break;
		}
	}

	// Return the token
	return m_string.Mid(start - m_string.c_str(), m_ptr++ - start);
}


wxString CSimpleTokenizer::remaining() const
{
	return m_string.Mid(m_ptr - m_string.c_str());
}


size_t CSimpleTokenizer::tokenCount() const
{
	return m_count;
}

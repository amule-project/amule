//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2009 Angel Vidal Veiga - Kry (kry@amule.org)
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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


#define STRINGFUNCTIONS_CPP


#include "StringFunctions.h"
#include "Path.h"

#include <wx/filename.h>	// Needed for wxFileName
#include <wx/uri.h>		// Needed for wxURI

// Implementation of the non-inlines
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

wxString TruncateFilename(const CPath& filename, size_t length, bool isFilePath)
{
	wxString file = filename.GetPrintable();

	// Check if there's anything to do
	if (file.Length() <= length)
		return file;
	
	// If the filename is a path, then prefer to remove from the path, rather than the filename
	if ( isFilePath ) {
		wxString path = wxFileName(file).GetPath();
		file          = wxFileName(file).GetFullName();

		if ( path.Length() >= length ) {
			path.Clear();
		} else if ( file.Length() >= length ) {
			path.Clear();
		} else {
			// Minus 6 for "[...]" + separator
			int pathlen = (int)(length - file.Length() - 6);
			
			if ( pathlen > 0 ) {
				path = wxT("[...]") + path.Right( pathlen );
			} else {
				path.Clear();
			}
		}
		
		file = JoinPaths(path, file);
	}

	if ( file.Length() > length ) {
		if ( length > 5 ) {		
			file = file.Left( length - 5 ) + wxT("[...]");
		} else {
			file.Clear();
		}
	}
	

	return file;
}



wxString StripSeparators(wxString path, wxString::stripType type)
{
	wxASSERT((type == wxString::leading) || (type == wxString::trailing));
	const wxString seps = wxFileName::GetPathSeparators();

	while (!path.IsEmpty()) {
		size_t pos = ((type == wxString::leading) ? 0 : path.Length() - 1);

		if (seps.Contains(path.GetChar(pos))) {
			path.Remove(pos, 1);
		} else {
			break;
		}
	}
	
	return path;
}


wxString JoinPaths(const wxString& path, const wxString& file)
{
	if (path.IsEmpty()) {
		return file;
	} else if (file.IsEmpty()) {
		return path;
	} 

	return StripSeparators(path, wxString::trailing)
	   + wxFileName::GetPathSeparator()
	   + StripSeparators(file, wxString::leading);
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


wxString UnescapeHTML( const wxString& str )
{
	wxString result;
	result.Alloc( str.Len() );
	
	for ( size_t i = 0; i < str.Len(); ++i ) {
		if ( str.GetChar(i) == wxT('%') && ( i + 2 < str.Len() ) ) {
			wxChar unesc = HexToDec( str.Mid( i + 1, 2 ) );

			if ( unesc ) {
				i += 2;

				result += unesc;
			} else {
				// If conversion failed, then we just add the escape-code
				// and continue past it like nothing happened.
				result += str.at(i);
			}
		} else {
			result += str.at(i);
		}
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


// File_checked_for_headers

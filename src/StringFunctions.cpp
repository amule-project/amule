//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal Veiga - Kry (kry@amule.org)
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "StringFunctions.h"
#endif

#include <cctype>

#include <wx/filename.h>
#include <wx/url.h>

#include "StringFunctions.h"


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

wxString TruncateFilename(const wxString& filename, size_t length, bool isFilePath)
{
	// Check if there's anything to do
	if ( filename.Length() <= length )
		return filename;
	
	wxString file = filename;
	
	// If the filename is a path, then prefer to remove from the path, rather than the filename
	if ( isFilePath ) {
		wxString path = file.BeforeLast( wxFileName::GetPathSeparator() );
		file          = file.AfterLast( wxFileName::GetPathSeparator() );

		if ( path.Length() >= length ) {
			path.Clear();
		} else if ( file.Length() >= length ) {
			path.Clear();
		} else {
			// Minus 6 for "[...]" + seperator
			int pathlen = length - file.Length() - 6;
			
			if ( pathlen > 0 ) {
				path = wxT("[...]") + path.Right( pathlen );
			} else {
				path.Clear();
			}
		}
		
		if ( !path.IsEmpty() ) {
			file = path + wxFileName::GetPathSeparator() + file;
		}
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

// Strips specific chars to ensure legal filenames
wxString CleanupFilename(const wxString& filename, bool keepSpaces)
{
	wxString result;

	for ( unsigned int i = 0; i < filename.Length(); i++ ) {
		switch ( filename[ i ] ) {
			case wxT('/'):
#ifdef __WXMSW__
			case wxT('\"'):
			case wxT('*'):
			case wxT('<'):
			case wxT('>'):
			case wxT('?'):
			case wxT('|'):
			case wxT('\\'):
			case wxT(':'):
#endif
				continue;
			case wxT(' '):
				if ( !keepSpaces ) {
					result += wxT('_');
					continue;
				}
				
			default:
				// Many illegal for filenames in windows below the 32th char (which is space).
				if ( (wxUChar) filename[i] > 31 ) {
						result += filename[i];
				}
		}
	}

	return result;
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
#if wxCHECK_VERSION_FULL(2,5,3,2)
	wxURI uri(url);
	
	return uri.BuildURI();
#else
	wxString strURI = wxURL::ConvertToValidURI(url);
	
	// The following cause problems, so we escape them
	strURI.Replace(wxT("\""), wxT("%22")); 
	strURI.Replace(wxT("'"),  wxT("%27")); 
	strURI.Replace(wxT("`"),  wxT("%60")); 
	
	return strURI;
#endif
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



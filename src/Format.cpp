//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
#pragma implementation "Format.h"
#endif

#include "Format.h"


/**
 * Returns true if the char is a format-type.
 */
bool isTypeChar( wxChar c )
{
	switch ( c ) {
		case wxT('s'):		// String of characters
		case wxT('u'):		// Unsigned decimal integer
		case wxT('i'):		// Signed decimal integer
		case wxT('d'):		// Signed decimal integer
		case wxT('x'):		// Unsigned hexadecimal integer
		case wxT('c'):		// Character
		case wxT('e'):		// Scientific notation (mantise/exponent) using e character
		case wxT('f'):		// Decimal floating point
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
		case wxT('o'):		// Signed octal
		case wxT('E'):		// Scientific notation (mantise/exponent) using E character
		case wxT('g'):		// Use shorter %e or %f
		case wxT('G'):		// Use shorter %E or %f
		case wxT('p'):		// Not supported, still needs to be caught though
		case wxT('n'):		// Not supported, still needs to be caught though
			return true;
	}

	return false;
}


CFormat::Modifiers CFormat::getModifier( const wxString& str )
{
	switch ( str.GetChar( str.Len() - 2 ) ) {
		case wxT('h'):		// short int (integer types).
			return modShort;
		case wxT('l'):		// long int (interger types) or double (floating point types).
			if ( str.Len() > 3 && str.GetChar( str.Len() - 3 ) == wxT('l') ) {
				return modLongLong;
			} else {
				return modLong;
			}
		case wxT('L'):		// long double (floating point types).
			return modLongDouble;
		default:
			return modNone;
	}
}


CFormat::CFormat( const wxChar* str )
{
	m_index = 0;
	m_indexEnd = 0;
	SetString( str );
}


void CFormat::SetString( const wxChar* str )
{
	m_format = str;
	ResetString();

	// Initialize to the first format-string.
	SetCurrentField( wxEmptyString );
}


const wxString& CFormat::GetString() const
{
	wxASSERT( m_index == m_format.Len() );
	
	return m_result;
}


#if GCC_VERSION > 30202
CFormat::operator const wxString&() const
#else
CFormat::operator const wxString() const
#endif
{
	wxASSERT( m_index == m_format.Len() );

	return m_result;
}


void CFormat::ResetString()
{
	m_index = 0;
	m_indexEnd = 0;
	m_result.Clear();
	m_result.Alloc( m_format.Len() * 2 );	
}


wxString CFormat::GetCurrentField()
{
	wxASSERT( m_index != m_format.Len() );

	if ( m_index != m_format.Len() ) {
		m_indexEnd = m_index + 1;
		for ( ; m_indexEnd < m_format.Len(); m_indexEnd++ ) {
			if ( isTypeChar( m_format.GetChar( m_indexEnd ) ) ) {
				break;
			}
		}
		
		wxASSERT( m_indexEnd != m_format.Len() );

		m_indexEnd++;

		// Extract the field
		return wxString( m_format.c_str() + m_index, m_indexEnd - m_index );
	} else {
		// Return something that can have Last called on it, but 
		// wont be thought of as a valid format-string.
		return wxString( wxT('\0') );
	}
}


CFormat& CFormat::SetCurrentField(const wxString& str)
{
	size_t length = m_format.Length();
	
	wxASSERT( m_index != length );
	wxASSERT( m_index <= m_indexEnd );
	
	// Locate the next format-string
	const wxChar* start = m_format.c_str() + m_indexEnd;
	const wxChar* end = m_format.c_str() + length;
	const wxChar* ptr = start;

	for ( ; ptr < end; ++ptr ) {
		if ( *ptr == wxT('%') ) {
			wxASSERT( ptr + 1 < end );
			
			if ( *(ptr + 1) != wxT('%') ) {
				break;
			} else {
				m_result.Append( start, ptr - start + 1 );
				start = ptr += 2;
			}
		}
	}
		
	if ( !str.IsEmpty() ) {
		m_result += str;
	}
		
	if ( ptr > start ) {
		m_result.Append( start, ptr - start );
	}

	m_index = ptr - m_format.c_str();

	return *this;
}


CFormat& CFormat::operator%( const wxChar* val )
{
	wxString field = GetCurrentField();
	
	if ( field.Last() == wxT('s') ) {
		// Check if a max-length is specified
		if ( field.GetChar( 1 ) == wxT('.') ) {
			wxString size = field.Mid( 2, field.Len() - 3 );
			long lSize = 0;

			// Try to convert the length-field.
			if ( size.ToLong( &lSize ) ) {
				if ( size > 0 ) {
					if ( val ) {
						SetCurrentField( wxString( val, lSize ) );
					} else {
						SetCurrentField( wxT("(null)") );
					}
				} else {
					SetCurrentField( wxEmptyString );
				}
			} else {
				wxASSERT( false );
			}
		} else {
			if ( val ) {
				// No limit on size, just set the string
				SetCurrentField( val );
			} else {
				SetCurrentField( wxT("(null)") );
			}
		}
	} else {
		wxASSERT( false );
	}

	return *this;
}


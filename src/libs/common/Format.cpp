//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2007aMule Team ( admin@amule.org / http://www.amule.org )
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
		case wxT('F'):		// Decimal floating point
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
		case wxT('o'):		// Unsigned octal
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
	switch ( (wxChar)str[str.Len() - 2]) {
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
	if (!m_format.IsEmpty()) {
		SetCurrentField( wxEmptyString );
	}
}

bool CFormat::IsReady() const
{
	return (m_index == m_format.Len());
}


const wxString& CFormat::GetString() const
{
	MULE_VALIDATE_STATE(IsReady(), wxT("Called GetString() before all values were passed: ") + m_format);
	
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
	MULE_VALIDATE_STATE(m_index < m_format.Len(), wxT("Value passed to already completed string."));

	m_indexEnd = m_index + 1;
	for (; m_indexEnd < m_format.Len(); m_indexEnd++) {
		wxChar c = m_format.GetChar(m_indexEnd);

		MULE_VALIDATE_STATE(c != wxT('*'), wxT("'*' precision not supported."));
		
		if (isTypeChar(c)) {
			break;
		}
	}
	
	MULE_VALIDATE_STATE(m_indexEnd < m_format.Len(), wxT("Invalid format string, unknown type."));

	m_indexEnd++;

	// Extract the field
	return m_format.Mid(m_index, m_indexEnd - m_index);
}


CFormat& CFormat::SetCurrentField(const wxString& str)
{
	m_result += str;
	
	size_t length = m_format.Length();
	
	MULE_ASSERT_MSG(m_index != length, wxT("Invalid operation: SetCurrentField on completed string."));
	MULE_ASSERT(m_index <= m_indexEnd);
	
	// Locate the next format-string
	const wxChar* start = (const wxChar*)m_format.c_str() + m_indexEnd;
	const wxChar* end = (const wxChar*)m_format.c_str() + length;
	const wxChar* ptr = start;

	for ( ; ptr < end; ++ptr ) {
		if ( *ptr == wxT('%') ) {
			MULE_VALIDATE_STATE(ptr + 1 < end, wxT("Incomplete format-string found."));
			
			if ( *(ptr + 1) != wxT('%') ) {
				break;
			} else {
				m_result.Append( start, ptr - start + 1 );
				start = ptr + 2;
				++ptr;
			}
		}
	}
		
	if ( ptr > start ) {
		m_result.Append( start, ptr - start );
	}

	m_index = ptr - m_format.c_str();

	return *this;
}


wxString CFormat::GetIntegerField(const wxChar* fieldType)
{
	wxString field = GetCurrentField();
	wxChar type = field.Last();

	// Drop type and modifier
	while (isalpha(field.Last())) {
		field.RemoveLast();
	}
	
	// Set the correct integer type
	field += fieldType;

	switch (type) {
		case wxT('o'):		// Unsigned octal
		case wxT('x'):		// Unsigned hexadecimal integer
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
			// Override the default type
			field.Last() = type;
		
		case wxT('d'):		// Signed decimal integer
		case wxT('i'):		// Signed decimal integer
		case wxT('u'):		// Unsigned decimal integer
			return field;
		
		default:
			MULE_VALIDATE_PARAMS(false, wxT("Integer value passed to non-integer format string."));
	}
}


CFormat& CFormat::operator%(double value)
{
	wxString field = GetCurrentField();
	
	switch ( (wxChar)field.Last() ) {
		case wxT('e'):		// Scientific notation (mantise/exponent) using e character
		case wxT('E'):		// Scientific notation (mantise/exponent) using E character
		case wxT('f'):		// Decimal floating point
		case wxT('F'):		// Decimal floating point
		case wxT('g'):		// Use shorter %e or %f
		case wxT('G'):		// Use shorter %E or %f
			MULE_VALIDATE_PARAMS(getModifier(field) == modNone, wxT("Invalid modifier specified for floating-point format."));
			
			return SetCurrentField(wxString::Format(field, value));
		
		default:
			MULE_VALIDATE_PARAMS(false, wxT("Floating-point value passed to non-float format string.") );
	}
}


CFormat& CFormat::operator%(wxChar value)
{
	wxString field = GetCurrentField();
	MULE_VALIDATE_PARAMS(field.Last() == wxT('c'), wxT("Char value passed to non-char format string."));
	
	return SetCurrentField(wxString::Format(field, value));
}


CFormat& CFormat::operator%( const wxChar* val )
{
	wxString field = GetCurrentField();
	MULE_VALIDATE_PARAMS(field.Last() == wxT('s'), wxT("String value passed to non-string format string."));
	
	// Check if a max-length is specified
	if (field.GetChar(1) == wxT('.')) {
		wxString size = field.Mid( 2, field.Len() - 3 );
		long lSize = 0;

		// Try to convert the length-field.
		if ((size.IsEmpty() || size.ToLong(&lSize)) && (lSize >= 0)) {
			SetCurrentField(wxString(val).Left(lSize));
		} else {
			MULE_VALIDATE_STATE(false, wxT("Invalid value found in 'precision' field."));
		}
	} else if (field.GetChar(1) == wxT('s')) {
		// No limit on size, just set the string
		SetCurrentField( val );
	} else {
		MULE_VALIDATE_STATE(false, wxT("Malformed string format field."));
	}

	return *this;
}

// File_checked_for_headers

//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <ctype.h>

#if defined HAVE_STDINT_H
#       include <stdint.h>
#elif defined HAVE_INTTYPES_H
#	include <inttypes.h>
#endif


//! Known type-modifiers. 
enum Modifiers
{
	//! No modifier field.
	modNone,
	//! Argument is interpreted as short int (integer types).
	modShort,
	//! Argument is interpreted as long int (integer types).
	modLong,
	//! Two 'long' modifieres, arguments is interpreted as long long (integer types).
	modLongLong,
	//! Argument is interpreted as long double (floating point types). Not supported.
	modLongDouble
};


/**
 * Extracts modifiers from the argument.
 *
 * Note that this function will possibly return wrong results
 * for malformed format strings.
 */
Modifiers getModifier(const wxString& str)
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


/** Returns true if the char is a format-type. */
bool isTypeChar(wxChar c)
{
	switch (c) {
		case wxT('s'):		// String of characters
		case wxT('u'):		// Unsigned decimal integer
		case wxT('i'):		// Signed decimal integer
		case wxT('d'):		// Signed decimal integer
		case wxT('c'):		// Character
		case wxT('f'):		// Decimal floating point
		case wxT('F'):		// Decimal floating point
		case wxT('x'):		// Unsigned hexadecimal integer
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
		case wxT('o'):		// Unsigned octal
		case wxT('e'):		// Scientific notation (mantise/exponent) using e character
		case wxT('E'):		// Scientific notation (mantise/exponent) using E character
		case wxT('g'):		// Use shorter %e or %f
		case wxT('G'):		// Use shorter %E or %f
		case wxT('p'):		// Not supported, still needs to be caught though
		case wxT('n'):		// Not supported, still needs to be caught though
			return true;
	}

	return false;
}

/** Returns true if the char is a valid flag. */
bool isFlagChar(wxChar c)
{
	switch (c) {
		case wxT('+'):		// Include sign for integers
		case wxT('-'):		// Left-align output
		case wxT('#'):		// Alternate form, varies
		case wxT(' '):		// Pad with spaces
		case wxT('0'):		// Pad with zeros
			return true;
	}

	return false;
}

/** Returns true if the char is an integer (for width + precision). */
bool isIntChar(wxChar c)
{
	return ((c >= wxT('0')) && (c <= wxT('9')));
}


/** Returns true if the char is a valid length modifier. */
bool isLengthChar(wxChar c)
{
	switch (c) {
		case wxT('h'):		// Short ('hh') and char ('h')
		case wxT('l'):		// Long ('l') and long long ('ll')
		case wxT('L'):		// Double long
		case wxT('z'):		// size_t
		case wxT('j'):		// intmax_t
		case wxT('t'):		// ptrdiff_t
			return true;
	}

	// Catches widths, precisons and zero-padding
	return (c >= wxT('0')) && (c <= wxT('9'));
}


CFormat::CFormat(const wxChar* str)
{
	m_fieldStart = 0;
	m_fieldLength = 0;
	m_skipCount = 0;
	m_format = str;

	if (m_format.Length()) {
		SetCurrentField(wxEmptyString);
	}
}


bool CFormat::IsReady() const
{
	return (m_fieldStart == m_format.Length());
}


wxString CFormat::GetString() const
{
	if (IsReady()) {
		return m_result;
	} else {
		wxFAIL_MSG(wxT("Called GetString() before all values were passed: ") + m_format);

		// Return as much as possible ...
		return m_result + m_format.Mid(m_fieldStart);
	}
}


void CFormat::SetCurrentField(const wxString& value)
{
	wxCHECK_RET(m_fieldStart < m_format.Length(),
		wxT("Setting field in already completed string: ") + m_format);

	if (value.Length()) {
		m_result += value;
	}

	enum {
		PosNone = 0,
		PosStart,
		PosFlags,
		PosWidth,
		PosPrecision,
		PosLength,
		PosEnd
	} pos = PosNone;

	// Format strings are expected to follow the folllowing structure:
	// 	%[Flags][Width][.Precision][Length]<Type>
	for (size_t i = m_fieldStart + m_fieldLength; i < m_format.Length(); ++i) {
		const wxChar c = m_format[i];

		if (pos >= PosStart) {
			m_fieldLength++;

			if ((pos <= PosFlags) && isFlagChar(c)) {
				pos = PosFlags;
			} else if ((pos <= PosWidth) && isIntChar(c)) {
				pos = PosWidth;
			} else if ((pos < PosPrecision) && (c == wxT('.'))) {
				pos = PosPrecision;
			} else if ((pos == PosPrecision) && isIntChar(c)) {
				// Nothing to do ...
			} else if ((pos < PosLength) && isLengthChar(c)) {
				pos = PosLength;
			} else if ((pos == PosLength) && isLengthChar(c) && (c == m_format[i - 1])) {
				// Nothing to do ...
			} else if ((pos <= PosLength) && isTypeChar(c)) {
				pos = PosEnd;
				break;
			} else if ((pos <= PosLength) && (c == wxT('%'))) {
				// Append the %*% to the result
				m_result += wxT("%");
				
				pos = PosNone;
			} else {
				// Field is broken ...
				break;
			}
		} else if (c == wxT('%')) {
			const size_t offset = m_fieldStart + m_fieldLength;
			// If there was anything before this, then prepend it.
			if (offset < i) {
				m_result += m_format.Mid(offset, i - offset);
			}

			// Starting a new format string
			pos = PosStart;
			m_fieldStart = i;
			m_fieldLength = 1;
		} else {
			// Normal text, nothing to do ...
		}
	}

	if (pos == PosNone) {
		// No fields left
		m_result += m_format.Mid(m_fieldStart + m_fieldLength);
		
		m_fieldStart = m_fieldLength = m_format.Length();
	} else if (pos != PosEnd) {
		// A partial field was found ...
		wxFAIL_MSG(wxT("Invalid field in format string: ") + m_format);
		wxASSERT_MSG(m_fieldStart + m_fieldLength <= m_format.Length(),
			wxT("Invalid field-start/length in format string: ") + m_format);

		// Prepend the parsed part of the format-string
		m_result += m_format.Mid(m_fieldStart, m_fieldLength);

		// Return an empty string the next time GetCurrentField is called
		m_skipCount++;

		// Anything left to do?
		if (!IsReady()) {
			// Find the next format string
			SetCurrentField(wxEmptyString);
		}
	}
}


wxString CFormat::GetCurrentField()
{
	wxCHECK_MSG(m_fieldStart < m_format.Length(), wxEmptyString,
		wxT("Passing argument to already completed string: ") + m_format);
	wxASSERT_MSG(m_fieldStart + m_fieldLength <= m_format.Length(),
		wxT("Invalid field-start/length in format string: ") + m_format);

	if (m_skipCount) {
		// The current field was invalid, so we skip it.
		m_skipCount--;

		return wxEmptyString;
	}

	return m_format.Mid(m_fieldStart, m_fieldLength);
}


wxString CFormat::GetIntegerField(const wxChar* fieldType)
{
	const wxString field = GetCurrentField();
	if (field.IsEmpty()) {
		// Invalid or missing field ...
		return field;
	}

	// Drop type and length
	wxString newField = field;
	while (isalpha(newField.Last())) {
		newField.RemoveLast();
	}
	
	// Set the correct integer type
	newField += fieldType;

	switch ((wxChar)field.Last()) {
		case wxT('o'):		// Unsigned octal
		case wxT('x'):		// Unsigned hexadecimal integer
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
			// Override the default type
			newField.Last() = field.Last();
		
		case wxT('d'):		// Signed decimal integer
		case wxT('i'):		// Signed decimal integer
		case wxT('u'):		// Unsigned decimal integer
			return newField;
		
		default:
			wxFAIL_MSG(wxT("Integer value passed to non-integer format string: ") + m_format);
			SetCurrentField(field);
			return wxEmptyString;
	}
}


CFormat& CFormat::operator%(double value)
{
	wxString field = GetCurrentField();
	if (field.IsEmpty()) {
		return *this;
	}

	switch ( (wxChar)field.Last() ) {
		case wxT('e'):		// Scientific notation (mantise/exponent) using e character
		case wxT('E'):		// Scientific notation (mantise/exponent) using E character
		case wxT('f'):		// Decimal floating point
		case wxT('F'):		// Decimal floating point
		case wxT('g'):		// Use shorter %e or %f
		case wxT('G'):		// Use shorter %E or %f
			wxASSERT_MSG(getModifier(field) == modNone, wxT("Invalid modifier specified for floating-point format: ") + m_format);
			
			SetCurrentField(wxString::Format(field, value));
			break;
		
		default:
			wxFAIL_MSG(wxT("Floating-point value passed to non-float format string: ") + m_format);
			SetCurrentField(field);
	}

	return *this;
}


CFormat& CFormat::operator%(wxChar value)
{
	wxString field = GetCurrentField();
	
	if (field.IsEmpty()) {
		// We've already asserted in GetCurrentField.
	} else if (field.Last() != wxT('c')) {
		wxFAIL_MSG(wxT("Char value passed to non-char format string: ") + m_format);
		SetCurrentField(field);
	} else {
		SetCurrentField(wxString::Format(field, value));
	}

	return *this;
}


CFormat& CFormat::operator%(signed long long value)
{
	wxString field = GetIntegerField(wxLongLongFmtSpec  wxT("i"));
	if (!field.IsEmpty()) {
		SetCurrentField(wxString::Format(field, value));
	}

	return *this;
}


CFormat& CFormat::operator%(unsigned long long value)
{
	wxString field = GetIntegerField(wxLongLongFmtSpec  wxT("u"));
	if (!field.IsEmpty()) {
		SetCurrentField(wxString::Format(field, value));
	}

	return *this;
}


CFormat& CFormat::operator%(const wxString& val)
{
	wxString field = GetCurrentField();
	
	if (field.IsEmpty()) {
		// We've already asserted in GetCurrentField
	} else if (field.Last() != wxT('s')) {
		wxFAIL_MSG(wxT("String value passed to non-string format string:") + m_format);
		SetCurrentField(field);
	} else if (field.GetChar(1) == wxT('.')) {
		// A max-length is specified
		wxString size = field.Mid( 2, field.Len() - 3 );
		long lSize = 0;

		// Try to convert the length-field.
		if ((size.IsEmpty() || size.ToLong(&lSize)) && (lSize >= 0)) {
			SetCurrentField(val.Left(lSize));
		} else {
			wxFAIL_MSG(wxT("Invalid value found in 'precision' field: ") + m_format);
			SetCurrentField(field);
		}
	} else if (field.GetChar(1) == wxT('s')) {
		// No limit on size, just set the string
		SetCurrentField(val);
	} else {
		SetCurrentField(field);
		wxFAIL_MSG(wxT("Malformed string format field: ") + m_format);
	}

	return *this;
}

CFormat& CFormat::operator%(void * value)
{
	wxString field = GetCurrentField();
	
	if (field.IsEmpty()) {
		// We've already asserted in GetCurrentField.
	} else if (field.Last() != wxT('p')) {
		wxFAIL_MSG(wxT("Pointer value passed to non-pointer format string: ") + m_format);
		SetCurrentField(field);
	} else if (field != wxT("%p")) {
		wxFAIL_MSG(wxT("Modifiers are not allowed for pointer format string: ") + m_format);
		SetCurrentField(field);
	} else {
		// built-in Format for pointer is not optimal:
		// - Windows: uppercase, no leading 0x
		// - Linux:   leading zeros missing 
		// -> format it as hex
		if (sizeof (void *) == 8) { // 64 bit
			SetCurrentField(wxString::Format(wxT("0x%016x"), (uintptr_t) value));
		} else { // 32 bit
			SetCurrentField(wxString::Format(wxT("0x%08x"),  (uintptr_t) value));
		}
	}

	return *this;
}

// File_checked_for_headers

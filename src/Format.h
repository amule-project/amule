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

#ifndef FORMAT_H
#define FORMAT_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Format.h"
#endif

#include <wx/string.h>
#include <limits>

#include "MuleDebug.h"

#ifdef __GNUC__
    #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
    /* Assume that other compilers don't have this bug */
    #define GCC_VERSION	99999
#endif


/**
 * This class offers a typesafe alternative to wxString::Format.
 *
 * Unlike normal format and printf, this class makes useage of integer
 * types safe and transparent, such that any integer-type can be used
 * for any integer-format, provided that it represents a valid value
 * for that type.
 *
 * This means that the format strings for integer-types only represents
 * the upper and lower bound of the value displayed and doesn't set any
 * requirements for the actual type of the value provided.
 *
 * This is not done for floating-point types.
 *
 * CFormat lacks the following capabilities:
 *  * The "*" width-modifier, because only one argument is fed at a time.
 *  * The "p" type, could be implemented using void* or templates.
 *  * The "n" type, just unsafe, wont be implemented.
 *  * The Long Double type, which is extremly slow and shouldn't be used.
 *
 *  CFormat has been implemented against the description of printf found
 *  in the "man 3 printf" manual page.
 */
class CFormat
{
public:
	/**
	 * Constructor.
	 *
	 * @param str The format-string to be used.
	 */
	CFormat(const wxChar* str);

	
	/**
	 * Sets the format-string.
	 */
	void SetString(const wxChar* str);

	
	/**
	 * Returns true if the resulting string is ready for use.
	 *
	 * For a string to be ready to use, all format fields must have
	 * had a value assigned through the operator% functions.
	 */
	bool IsReady() const;


	/**
	 * Feeds an value into the format-string.
	 *
	 * Passing an type that isn't compatible with the current format
	 * field results in an illegal-argument exception. Passing any
	 * type to an CFormat with no free fields results in an illegal-
	 * state exception.
	 * 
	 * Special rules apply to integers, see above.
	 */
	// \{
	CFormat& operator%(wxChar value);
	CFormat& operator%(signed short value);
	CFormat& operator%(unsigned short value);
	CFormat& operator%(signed int value);
	CFormat& operator%(unsigned int value);
	CFormat& operator%(signed long value);
	CFormat& operator%(unsigned long value);
	CFormat& operator%(signed long long value);
	CFormat& operator%(unsigned long long value);	
	CFormat& operator%(double value);
	CFormat& operator%(const wxChar* value);
	// \}


	/**
	 * Returns the resulting string, should only be used when all arguments have been given.
	 */
	const wxString& GetString() const;
	
	/**
	 * Resets the resulting string, allowing the parser to be fed new values.
	 */
	void ResetString();

	
	/**
	 * Implicit conversion to wxString.
	 */
#if GCC_VERSION > 30300
	operator const wxString&() const	{ return GetString(); };
#else
	operator wxString() const			{ return GetString(); };
#endif
	 
private:
	/**
	 * Returns the current format-field.
	 */
	wxString	GetCurrentField();

	/**
	 * Replaces the current format-field with the specified string.
	 */
	CFormat&	SetCurrentField(const wxString&);
	

	//! Known type-modifiers. 
	enum Modifiers
	{
		//! No modifier field.
		modNone,
		//! Argument is interpreted as short int (integer types).
		modShort,
		//! Argument is interpreted as long int (interger types).
		modLong,
		//! Two 'long' modifieres, arguments is interpreted as long long (integer types).
		modLongLong,
		//! Argument is interpreted as long double (floating point types). Not supported.
		modLongDouble
	};

	
	/**
	 * Extracts modifiers from the argument.
	 *
	 * @param str A format string.
	 * @return The identified modifier.
	 *
	 * Note that this function will possibly return wrong results
	 * for malformed format strings.
	 */
	Modifiers getModifier(const wxString& str);
	
	
	/**
	 * Attempts to apply the current format-string to a integer-type value.
	 * 
	 * @param value Any integer type, from char to long long.
	 *
	 * This function does the value-verification needed to ensure that
	 * any integer-type can be used for any integer-format, provided that
	 * it represents a valid value in that type.
	 */
	template <typename ValueType>
	CFormat& FormatInteger(ValueType value);
	
	
	//! Index to the current format-field.
	unsigned int m_index;
	//! Index to past the end of the current format-field.
	unsigned int m_indexEnd;

	//! The format-string fed to the parser.
	wxString	m_format;
	//! The current result of arguments fed to the parser.
	wxString	m_result;
};





////////////////////////////////////////////////////////////////////////////////

//! Checks if a value can be represented in the TargetType.
template <typename TargetType, typename CurrentType>
inline bool CanRepresent(CurrentType value) 
{
	typedef std::numeric_limits<TargetType> TT;
	typedef std::numeric_limits<CurrentType> CT;

	// Check that the new type can contain the value
	if ((CurrentType)((TargetType)value) == value) {
		if (TT::is_signed != CT::is_signed) {
			// Check that the signed bit isn't set, since that would either
			// mean that the value is negative, or that it is too large to
			// fit in a signed variable.
			return value >> (sizeof(CurrentType) * 8 - 1) == 0;
		} else {
			return true;
		}
	} else {
		return false;
	}
}


template <typename ValueType>
inline CFormat& CFormat::FormatInteger(ValueType value)
{
	wxString field = GetCurrentField();

	switch (field.Last()) {
		case wxT('d'):		// Signed decimal integer
		case wxT('i'):		// Signed decimal integer
		{
			switch (getModifier(field)) {
				case modNone:
					MULE_VALIDATE_PARAMS(CanRepresent<signed int>(value), wxT("Integer value passed cannot be represented as an signed int."));
					
					return SetCurrentField(wxString::Format(field, (signed int)value));
				
				case modShort:
					MULE_VALIDATE_PARAMS(CanRepresent<signed short>(value), wxT("Integer value passed cannot be represented as an signed short."));
					
					return SetCurrentField(wxString::Format(field, (signed short)value));
				
				case modLong:
					MULE_VALIDATE_PARAMS(CanRepresent<signed long>(value), wxT("Integer value passed cannot be represented as an signed long."));
					
					return SetCurrentField(wxString::Format(field, (signed long)value));
				
				case modLongLong:
					MULE_VALIDATE_PARAMS(CanRepresent<signed long long>(value), wxT("Integer value passed cannot be represented as an signed long long."));
					
					return SetCurrentField(wxString::Format(field, (signed long long)value));

				default:
					MULE_VALIDATE_STATE(false, wxT("Invalid modifier specified for interger format."));
			}
		}

		
		case wxT('o'):		// Unsigned octal
		case wxT('u'):		// Unsigned decimal integer
		case wxT('x'):		// Unsigned hexadecimal integer
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
		{
			switch (getModifier(field)) {
				case modNone:
					MULE_VALIDATE_PARAMS(CanRepresent<unsigned int>(value), wxT("Integer value passed cannot be represented as an unsigned int."));
					
					return SetCurrentField( wxString::Format( field, (unsigned int)value ) );
				
				case modShort:
					MULE_VALIDATE_PARAMS(CanRepresent<unsigned short>(value), wxT("Integer value passed cannot be represented as an unsigned short."));
					
					return SetCurrentField(wxString::Format(field, (unsigned short)value));
				
				case modLong:
					MULE_VALIDATE_PARAMS(CanRepresent<unsigned long>(value), wxT("Integer value passed cannot be represented as an unsigned long."));
					
					return SetCurrentField(wxString::Format(field, (unsigned long)value));
				
				case modLongLong:
					MULE_VALIDATE_PARAMS(CanRepresent<unsigned long long>(value), wxT("Integer value passed cannot be represented as an unsigned long long."));
					
					return SetCurrentField(wxString::Format(field, (unsigned long long)value));

				default:
					MULE_VALIDATE_STATE(false, wxT("Invalid modifier specified for interger format."));
			}
		}
		
		case wxT('c'):		// Character
			MULE_VALIDATE_PARAMS(CanRepresent<wxChar>(value), wxT("Integer value passed cannot be represented as a wxChar.") );
			
			return SetCurrentField(wxString::Format(field, (wxChar)value));

		default:
			MULE_VALIDATE_PARAMS(false, wxT("Integer value passed to non-integer format string."));
	}
}


inline CFormat& CFormat::operator%(wxChar value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(signed short value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(unsigned short value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(signed int value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(unsigned int value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(signed long value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(unsigned long value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(signed long long value)
{
	return FormatInteger(value);
}


inline CFormat& CFormat::operator%(unsigned long long value)
{
	return FormatInteger(value);
}

#endif

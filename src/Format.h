//
// This file is part of the aMule Project.
//
// Copyright (c) 2005 aMule Team ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef FORMAT_H
#define FORMAT_H

#include <wx/string.h>
#include <limits>


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
 * The same is done for floating-point types.
 *
 * CFormat lacks the following capabilities:
 *  * The "*" width-modifier, because only one argument is fed at a time.
 *  * The "p" type, could be implemented using void* or templates.
 *  * The "n" type, just unsafe, wont be implemented.
 */
class CFormat
{
public:
	/**
	 * Constructor.
	 *
	 * @param str The format-string to be used.
	 */
	CFormat( const wxChar* str );

	
	/**
	 * Sets the format-string.
	 */
	void SetString( const wxChar* str );

	
	/**
	 * Returns true if the resulting string is ready for use.
	 */
	bool IsReady() const;


	/**
	 * Feeds an value into the format-string.
	 *
	 * If the given doesn't match the current format-string, then an assert
	 * will be raised and the format-string will be skipped. Special rules
	 * apply to integers, see above.
	 */
	// \{
	CFormat& operator%( wxChar value );
	CFormat& operator%( signed short value );
	CFormat& operator%( unsigned short value );
	CFormat& operator%( signed int value );
	CFormat& operator%( unsigned int value );
	CFormat& operator%( signed long value );
	CFormat& operator%( unsigned long value );
	CFormat& operator%( signed long long value );
	CFormat& operator%( unsigned long long value );	
	CFormat& operator%( float value );
	CFormat& operator%( double value );
	CFormat& operator%( long double value );
	CFormat& operator%( const wxChar* value );
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
	operator const wxString&() const;
	 
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
		//! Argument is interpreted as long int (interger types) or double (floating point types).
		modLong,
		//! Two 'long' modifieres, arguments is interpreted as long long (integer types).
		modLongLong,
		//! Argument is interpreted as long double (floating point types).
		modLongDouble,
	};

	/**
	 * Extracts modifiers from the 
	 */
	Modifiers getModifier( const wxString& str );
	
	
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
	CFormat& FormatInteger( ValueType value );
	
	
	/**
	 * Attempts to apply the current format-string to a floating-point value.
	 * 
	 * @param value Any floating-point value, from float to long double.
	 *
	 * This function does the value-verification needed to ensure that
	 * any floating-point type can be used for any fp-format, provided that
	 * it represents a valid value in that type.
	 */
	template <typename ValueType>
	CFormat& FormatFloat( ValueType value );
	
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




//! Constants used to validate values when casting to smaller types.
// \{
const wxChar MIN_WXCHAR			= std::numeric_limits<wxChar>::min();
const wxChar MAX_WXCHAR			= std::numeric_limits<wxChar>::max();
const signed short MIN_SHORT	= std::numeric_limits<signed short>::min();
const signed short MAX_SHORT	= std::numeric_limits<signed short>::max();
const signed int MIN_INT		= std::numeric_limits<signed int>::min();
const signed int MAX_INT		= std::numeric_limits<signed int>::max();
const signed long MIN_LONG		= std::numeric_limits<signed long>::min();
const signed long MAX_LONG		= std::numeric_limits<signed long>::max();
const float MIN_FLOAT			= std::numeric_limits<float>::min();
const float MAX_FLOAT			= std::numeric_limits<float>::max();
const double MIN_DOUBLE			= std::numeric_limits<double>::min();
const double MAX_DOUBLE			= std::numeric_limits<double>::max();
//\}





template <typename ValueType>
inline CFormat& CFormat::FormatInteger( ValueType value )
{
	wxString field = GetCurrentField();

	switch ( field.Last() ) {
		case wxT('d'):		// Signed decimal integer
		case wxT('i'):		// Signed decimal integer
		case wxT('o'):		// Signed octal
		case wxT('u'):		// Unsigned decimal integer
		case wxT('x'):		// Unsigned hexadecimal integer
		case wxT('X'):		// Unsigned hexadecimal integer (capital letters)
		{
			switch ( getModifier( field ) ) {
				case modNone:
					if ( value <= MAX_INT && value >= MIN_INT ) {
						return SetCurrentField( wxString::Format( field, (signed int)value ) );
					} else {
						wxASSERT( false );
					}
					break;
				case modShort:
					if ( value <= MAX_SHORT && value >= MIN_SHORT ) {
						return SetCurrentField( wxString::Format( field, (signed short)value ) );
					} else {
						wxASSERT( false );
					}
					break;
				case modLong:
					if ( value <= MAX_LONG && value >= MIN_LONG ) {
						return SetCurrentField( wxString::Format( field, (signed long)value ) );
					} else {
						wxASSERT( false );
					}
					break;
				case modLongLong:
					// No need to check, can contain all other value-types
					return SetCurrentField( wxString::Format( field, (signed long long)value ) );
					break;

				default:
					wxASSERT( false );
			}
			
			break;
		}
		
		case wxT('c'):		// Character
			if ( value <= MAX_WXCHAR && value >= MIN_WXCHAR ) {
				return SetCurrentField( wxString( (wxChar)value ) );
			} else {
				wxASSERT( false );
			}

		default:
			wxASSERT( false );
	}

	return SetCurrentField( field );
}


template <typename ValueType>
inline CFormat& CFormat::FormatFloat( ValueType value )
{
	wxString field = GetCurrentField();

	switch ( field.Last() ) {
		case wxT('e'):		// Scientific notation (mantise/exponent) using e character
		case wxT('E'):		// Scientific notation (mantise/exponent) using E character
		case wxT('f'):		// Decimal floating point
		case wxT('g'):		// Use shorter %e or %f
		case wxT('G'):		// Use shorter %E or %f
			switch ( getModifier( field ) ) {
				case modNone:
					if ( value <= MAX_FLOAT && value >= MIN_FLOAT ) {
						return SetCurrentField( wxString::Format( field, (float)value ) );
					} else {
						wxASSERT( false );
					}
					break;
				case modLong:
					if ( value <= MAX_DOUBLE && value >= MIN_DOUBLE ) {
						return SetCurrentField( wxString::Format( field, (double)value ) );
					} else {
						wxASSERT( false );
					}
					break;
				case modLongDouble:
					// No need to check, can contain all other value-types
					SetCurrentField( wxString::Format( field, (long double)value ) );
					
				default:
					wxASSERT( false );
			}
		
			break;

		default:
			wxASSERT( false );				
	}

	return SetCurrentField( field );
}


inline CFormat& CFormat::operator%( wxChar value )
{
	return FormatInteger( value );
}


inline CFormat& CFormat::operator%( signed short value )
{
	return FormatInteger( value );
}


inline CFormat& CFormat::operator%( unsigned short value )
{
	return FormatInteger( (signed short)value );
}



inline CFormat& CFormat::operator%( signed int value )
{
	return FormatInteger( value );
}


inline CFormat& CFormat::operator%( unsigned int value )
{
	return FormatInteger( (signed int)value );
}


inline CFormat& CFormat::operator%( signed long value )
{
	return FormatInteger( value );
}


inline CFormat& CFormat::operator%( unsigned long value )
{
	return FormatInteger( (signed long)value );
}


inline CFormat& CFormat::operator%( signed long long value )
{
	return FormatInteger( value );
}


inline CFormat& CFormat::operator%( unsigned long long value )
{
	return FormatInteger( (signed long long)value );
}


inline CFormat& CFormat::operator%( float value )
{
	return FormatFloat( value );
}


inline CFormat& CFormat::operator%( double value )
{
	return FormatFloat( value );
}


inline CFormat& CFormat::operator%( long double value )
{
	return FormatFloat( value );
}

#endif

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
 * Unlike normal format and printf, this class does not care about the
 * type of integer values passed to it, and will handle a value correctly,
 * even if the format-string and the actual type dissagree. Therefore, it
 * is suggested that only %i and %u be used for integers, for the sake of
 * clarity, though %i alone is enough. 
 *
 * The other integer type-fields are supported, but will have no inpact on
 * how the value is represented. The only exception to this is the 'o', 'x'
 * and 'X' fields, which will always be considered to be unsigned!
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
	CFormat&	SetCurrentField(const wxString& value);

	/**
	 * Returns the next field modified to fit the given integer type.
	 *
	 * @param fieldType A modifier and type for an integer value.
	 * @return The resulting format-string.
	 *
	 * This function is used to generate the integer-type independant
	 * fields, by modifying the existing format-string to fit the type
	 * of the integer value that has been passed to it.
	 */
	wxString	GetIntegerField(const wxChar* fieldType);
	

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


inline CFormat& CFormat::operator%(signed short value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("hi")), value));
}


inline CFormat& CFormat::operator%(unsigned short value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("hu")), value));
}


inline CFormat& CFormat::operator%(signed int value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("i")), value));
}


inline CFormat& CFormat::operator%(unsigned int value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("u")), value));
}


inline CFormat& CFormat::operator%(signed long value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("li")), value));
}


inline CFormat& CFormat::operator%(unsigned long value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("lu")), value));
}


inline CFormat& CFormat::operator%(signed long long value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("lli")), value));
}


inline CFormat& CFormat::operator%(unsigned long long value)
{
	return SetCurrentField(wxString::Format(GetIntegerField(wxT("llu")), value));
}

#endif

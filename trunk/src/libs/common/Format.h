//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef FORMAT_H
#define FORMAT_H


#include "MuleDebug.h"

#ifdef __GNUC__
    #define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
    /* Assume that other compilers don't have this bug */
    #define GCC_VERSION	99999
#endif


/**
 * Classes that implement this interface are usable as
 * arguments for %s format strings. This is needed because
 * CFormat objects are typically created as temporary objects,
 * which makes external declarations of operator% impossible,
 * due to the fact that a non-const reference to a temporary
 * object is prohibited, as in:
 * 	CFormat& operator%(CFormat& fmt, ...)
 *
 * With this approch, it is possible to use CFormat as usual:
 * 	CFormat(...) % <some CPrintable object>;
 */
class CPrintable
{
public:
	/** Must return a "pretty" string representation of the object. */
	virtual wxString GetPrintableString() const = 0;

protected:
	virtual ~CPrintable() {}
};




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
	 * Constructor.
	 * This form is required to construct from a plain char * 
	 * with wx 2.9
	 *
	 * @param str The format-string to be used.
	 */
	CFormat(const wxString& str);

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
	 * field results in field being skipped, and an exception is raised.
	 * Passing any type to an CFormat with no free fields results an
	 * assertion, and the argument being ignored.
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
	CFormat& operator%(const wxString& value);
	CFormat& operator%(const CPrintable& value);	
	CFormat& operator%(void * value);	
	// \}


	/**
	 * Returns the resulting string, should only be used when all arguments have been given.
	 */
	wxString GetString() const;
	
	/**
	 * Implicit conversion to wxString.
	 */
	operator wxString() const		{ return GetString(); };
	 
private:
	/**
	 * Sets the value of the current field, and locates
	 * the next format field in the string.
	 */
	void		SetCurrentField(const wxString& value);

	/**
 	 * Returns the current format-field, or an empty 
 	 * string if no field was found.
	 */
	wxString	GetCurrentField();

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

	//! Index to the current format field.
	size_t 	m_fieldStart;
	//! Length of the current format field.
	size_t	m_fieldLength;
	//! The number of fields to skip in GetCurrentField
	size_t	m_skipCount;

	//! The format-string fed to the parser.
	wxString	m_format;
	//! The current result of arguments fed to the parser.
	wxString	m_result;
};



////////////////////////////////////////////////////////////////////////////////

inline CFormat& CFormat::operator%(signed short value)
{
	return *this % (signed long long)value;
}


inline CFormat& CFormat::operator%(unsigned short value)
{
	return *this % (unsigned long long)value;
}


inline CFormat& CFormat::operator%(signed int value)
{
	return *this % (signed long long)value;
}


inline CFormat& CFormat::operator%(unsigned int value)
{
	return *this % (unsigned long long)value;
}


inline CFormat& CFormat::operator%(signed long value)
{
	return *this % (signed long long)value;
}


inline CFormat& CFormat::operator%(unsigned long value)
{
	return *this % (unsigned long long)value;
}


inline CFormat& CFormat::operator%(const wxChar* val)
{
	return *this % wxString(val);
}

inline CFormat& CFormat::operator%(const CPrintable& value)
{
	return *this % value.GetPrintableString();
}

#if wxCHECK_VERSION(2, 9, 0)
#define WXLONGLONGFMTSPEC wxT(wxLongLongFmtSpec)
#else
#define WXLONGLONGFMTSPEC wxLongLongFmtSpec
#endif

#endif
// File_checked_for_headers

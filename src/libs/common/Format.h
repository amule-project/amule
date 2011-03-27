//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <list>
#include "MuleDebug.h"


/**
 * This class offers a typesafe alternative to wxString::Format.
 *
 * %CFormat has been implemented against the description of printf found
 * in the "man 3 printf" manual page.
 *
 * %CFormat lacks the following capabilities:
 *  - The @c "*" width-modifier, because only one argument is fed at a time.
 *  - The @c "n" type, just unsafe and won't be implemented.
 *  - The @c "C" and @c "S" types, which are considered obsolete.
 *  - The Long Double type, which is extremly slow and shouldn't be used.
 *
 * Support for the C99 @c a, @c A conversions and the non-standard @c ', @c I
 * flags depend on the underlying C library. Do not use them.
 *
 * Supports the glibc-specific @c m conversion on all platforms, where there's
 * a way to get the error description. If the underlying C library has a
 * thread-safe way to get the error description, then this conversion is
 * thread-safe, too.
 *
 * Deviations from printf(3):
 *
 * %CFormat tries hard to format the passed POD-type according to the
 * conversion type. Basic type conversions may take place to accomplish this
 * goal. This results in formats accepting a variety of types, namely:
 *  - @c c, @c i, @c d, @c u, @c o, @c x, @c X accept @c wxChar and all integer
 *    types,
 *  - @c a, @c A, @c e, @c E, @c f, @c F, @c g, @c G accept @c wxChar, integer
 *    and floating-point types,
 *  - @c p accepts only pointers,
 *  - @c s accepts all the above mentioned types in addition to @c wxString and
 *    @c wxChar* types.
 *
 * The only exception from this rule is integer (@c d, @c i, @c u) conversion.
 * It will always use the correct conversion (@c i or @c u) depending on the
 * signedness of the passed argument.
 *
 * @c 's' conversions are inspired by the <em>"we're converting to string,
 * anyway"</em> mood. Thus they use a 'default' conversion for each accepted
 * type: @c 'c' for @c wxChar, @c 'i' and @c 'u' for signed and unsigned
 * integers, respectively, @c 'g' for floating-point numbers and @c 'p' for
 * pointers.
 *
 * Other relaxations / differences from printf(3):
 *  - Length modifiers are read and validated, but always ignored.
 *  - As a consequence, invalid combinations of length modifiers and conversion
 *    types are silently ignored (i.e. for example the invalid @c '%%qs'
 *    format-specifier is silently treated as @c '%%s').
 *  - @c 'p' conversion ignores all modifiers except the argument index reference.
 *  - You can mix positional and indexed argument references.
 *  - With indexed argument references we allow to leave gaps in the indices.
 */
class CFormat
{
      private:
	/**
	 * Structure to hold a format specifier.
	 */
	struct FormatSpecifier {
		unsigned	argIndex;	//!< Argument index. (Position, unless specified otherwise.)
		wxChar		flag;		//!< The optional flag character.
		unsigned	width;		//!< The optional field width.
		signed 		precision;	//!< The optional precision value.
		// length is not stored
		wxChar		type;		//!< The conversion type.
		size_t		startPos;	//!< Position of the first character of the format-specifier in the format-string.
		size_t		endPos;		//!< Position of the last character of the format-specifier in the format-string.
		wxString	result;		//!< Result of the conversion. Initialized to the format-specifier.
	};

public:
	/**
	 * Constructor.
	 *
	 * @param str The format-string to be used.
	 */
	CFormat(const wxChar* str)	{ Init(str); }

	/**
	 * Constructor.
	 *
	 * This form is required to construct from a plain char * 
	 * with wx 2.9
	 *
	 * @param str The format-string to be used.
	 */
	CFormat(const wxString& str)	{ Init(str); }

	/**
	 * Feeds a value into the format-string.
	 *
	 * Passing a type that isn't compatible with the current format
	 * field results in the field being skipped, and an exception raised.
	 *
	 * Passing any type to a CFormat with no free fields results in the
	 * argument being ignored.
	 *
	 * Specialize this member template to teach CFormat how to handle
	 * other types.
	 */
	template<typename _Tp> CFormat& operator%(_Tp value);

	// Overload hack to map all pointer types to void*
	template<typename _Tp> CFormat& operator%(_Tp* value)	{ return this->operator%<void*>(value); }

	// explicit overloads to avoid pass-by-value even in debug builds.
	CFormat& operator%(const wxString& value)		{ return this->operator%<const wxString&>(value); }
	CFormat& operator%(const CFormat& value)		{ return this->operator%<const wxString&>(value); }

	/**
	 * Returns the resulting string.
	 */
	wxString GetString() const;
	
	/**
	 * Implicit conversion to wxString.
	 */
	operator wxString() const		{ return GetString(); };
	 
private:
	/**
	 * Initialize internal structures.
	 *
	 * Initializes member variables and parses the given format string.
	 */
	void	Init(const wxString& str);

	//! Type holding format specifiers.
	typedef std::list<FormatSpecifier>	FormatList;

	//! Retrieve the modifiers for the given format specifier.
	wxString GetModifiers(FormatList::const_iterator it) const;

	//! Do one argument conversion.
	template<typename _Tp>
	void	ProcessArgument(FormatList::iterator it, _Tp value);

	//! List of the valid format-specifiers found in the format string.
	FormatList	m_formats;

	//! Number of the previous argument.
	unsigned	m_argIndex;

	//! The format-string fed to the parser.
	wxString	m_formatString;
};

// type mappings
template<> inline CFormat& CFormat::operator%(char value)		{ return *this % (wxChar)value; }
template<> inline CFormat& CFormat::operator%(signed char value)	{ return *this % (wxChar)value; }
template<> inline CFormat& CFormat::operator%(unsigned char value)	{ return *this % (wxChar)value; }
template<> inline CFormat& CFormat::operator%(bool value)		{ return *this % (signed long long)value; }
template<> inline CFormat& CFormat::operator%(signed short value)	{ return *this % (signed long long)value; }
template<> inline CFormat& CFormat::operator%(unsigned short value)	{ return *this % (unsigned long long)value; }
template<> inline CFormat& CFormat::operator%(signed int value)		{ return *this % (signed long long)value; }
template<> inline CFormat& CFormat::operator%(unsigned int value)	{ return *this % (unsigned long long)value; }
template<> inline CFormat& CFormat::operator%(signed long value)	{ return *this % (signed long long)value; }
template<> inline CFormat& CFormat::operator%(unsigned long value)	{ return *this % (unsigned long long)value; }
template<> inline CFormat& CFormat::operator%(float value)		{ return *this % (double)value; }
template<> inline CFormat& CFormat::operator%(const wxChar* value)	{ return this->operator%<const wxString&>(wxString(value)); }


#if wxCHECK_VERSION(2, 9, 0)
#define WXLONGLONGFMTSPEC wxT(wxLongLongFmtSpec)
#else
#define WXLONGLONGFMTSPEC wxLongLongFmtSpec
#endif

#endif
// File_checked_for_headers

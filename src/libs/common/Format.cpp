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

#include "Format.h"

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#if defined HAVE_STDINT_H
#	include <stdint.h>
#elif defined HAVE_INTTYPES_H
#	include <inttypes.h>
#endif

#include <wx/intl.h>			// Needed for _()

#include <errno.h>			// Needed for errno and EINVAL
#include "strerror_r.h"			// Needed for mule_strerror_r()

/** Returns true if the char is a format-type. */
inline bool isTypeChar(wxChar c)
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
		case wxT('G'):		// Use shorter %E or %F
		case wxT('p'):		// Pointer
		case wxT('n'):		// Not supported, still needs to be caught though
		case wxT('a'):		// (C99; not in SUSv2) Double in hexadecimal notation.
		case wxT('A'):		// (C99; not in SUSv2) Double in hexadecimal notation (capital letters).
		case wxT('C'):		// (Not in C99, but in SUSv2.)  Synonym for lc.  Don't use. Not supported.
		case wxT('S'):		// (Not in C99, but in SUSv2.)  Synonym for ls.  Don't use. Not supported.
		case wxT('m'):		// (Glibc extension.)  Print output of strerror(errno).  No argument is required.
//		case wxT('%'):		// A `%' is written.  No argument is converted.  The complete conversion specification is `%%'.
			return true;
	}

	return false;
}

/** Returns true if the char is a valid flag. */
inline bool isFlagChar(wxChar c)
{
	switch (c) {
		// C standard flags
		case wxT('+'):		// Include sign for integers
		case wxT('-'):		// Left-align output
		case wxT('#'):		// Alternate form, varies
		case wxT(' '):		// Pad with spaces
		case wxT('0'):		// Pad with zeros
		// SUSv2
		case wxT('\''):		// For decimal conversion (i, d, u, f, F, g, G) the output is to be grouped with thousands' grouping characters if the locale information indicates any.
		// glibc 2.2
		case wxT('I'):		// For decimal integer conversion (i, d, u) the output uses the locale's alternative output digits, if any.
			return true;
	}

	return false;
}

/** Returns true if the char is a valid length modifier. */
inline bool isLengthChar(wxChar c)
{
	switch (c) {
		case wxT('h'):		// Short ('hh') and char ('h')
		case wxT('l'):		// Long ('l') and long long ('ll')
		case wxT('L'):		// Double long
		case wxT('z'):		// size_t
		case wxT('j'):		// intmax_t
		case wxT('t'):		// ptrdiff_t
		case wxT('q'):		// quad. Synonym for 'll'. Don't use.
			return true;
	}

	return false;
}

/** Returns true if the argument is a valid length modifier */
inline bool isValidLength(const wxString& str)
{
	return ((str == wxT("hh")) || (str == wxT("h")) ||
		(str == wxT("l")) || (str == wxT("ll")) ||
		(str == wxT("L")) || (str == wxT("z")) ||
		(str == wxT("j")) || (str == wxT("t")) ||
		(str == wxT("q")));
}


enum eStringParserStates {
	esNonFormat = 0,	// Not in a format string
	esFormatStart,		// Start of a format string
	esFormat,		// Inside a format string
	esFormatEnd,		// Finished reading a format string
	esInvalidFormat		// Invalid (incomplete) format specifier
};

/**
 * State machine to extract format specifiers from the string
 *
 * All format strings will be extracted, regardless whether they are valid or
 * not. Also '%%' is considered to be special format string which requires no
 * arguments, thus it will be extracted too (that is because it has to be
 * converted to '%').
 */
static eStringParserStates stringParser[][3] = {
			/* %-sign, 		type-char, 	other */
/* esNonFormat */	{ esFormatStart,	esNonFormat,	esNonFormat	},
/* esFormatStart */	{ esFormatEnd,		esFormatEnd,	esFormat	},
/* esFormat */		{ esInvalidFormat,	esFormatEnd,	esFormat	},
/* esFormatEnd */	{ esFormatStart,	esNonFormat,	esNonFormat	},
/* esInvalidFormat */	{ esFormatEnd,		esFormatEnd,	esFormat	}
};

enum eFormatParserStates {
	efStart = 0,	// Format string start
	efArgIndex,	// Argument index
	efArgIndexEnd,	// End of the argument index ('$' sign)
	efFlagChar,	// Flag character
	efWidth,	// Width field
	efPrecStart,	// Precision field start ('.' character)
	efPrecision,	// Precision field
	efLength,	// Length field
	// The following two are terminal states, they terminate processing
	efType,		// Type character
	efError		// Invalid format specifier
};

/**
 * State machine to parse format specifiers
 *
 * Format specifiers are expected to follow the following structure:
 * 	%[argIndex$][Flags][Width][.Precision][Length]<Type>
 */
static eFormatParserStates formatParser[][7] = {
			/* [1-9],	'0',		flagChar,	'.',		lengthChar,	typeChar,	'$' */
/* efStart */		{ efArgIndex,	efFlagChar,	efFlagChar,	efPrecStart,	efLength,	efType,		efError, },
/* efArgIndex */	{ efArgIndex,	efArgIndex,	efError,	efPrecStart,	efLength,	efType,		efArgIndexEnd, },
/* efArgIndexEnd */	{ efWidth,	efFlagChar,	efFlagChar,	efPrecStart,	efLength,	efType,		efError, },
/* efFlagChar */	{ efWidth,	efError,	efError,	efPrecStart,	efLength,	efType,		efError, },
/* efWidth */		{ efWidth,	efWidth,	efError,	efPrecStart,	efLength,	efType,		efError, },
/* efPrecStart */	{ efPrecision,	efPrecision,	efError,	efError,	efLength,	efType,		efError, },
/* efPrecision */	{ efPrecision,	efPrecision,	efError,	efError,	efLength,	efType,		efError, },
/* efLength */		{ efError,	efError,	efError,	efError,	efLength,	efType,		efError, }
};

// Forward-declare the specialization for const wxString&, needed by the parser
template<> void CFormat::ProcessArgument(FormatList::iterator, const wxString&);

void CFormat::Init(const wxString& str)
{
	m_formatString = str;
	m_argIndex = 0;

	// Extract format-string-like substrings from the input
	{
		size_t formatStart;
		eStringParserStates state = esNonFormat;
		for (size_t pos = 0; pos < str.length(); ++pos) {
			if (str[pos] == wxT('%')) {
				state = stringParser[state][0];
			} else if (isTypeChar(str[pos])) {
				state = stringParser[state][1];
			} else {
				state = stringParser[state][2];
			}
			switch (state) {
				case esInvalidFormat:
					wxFAIL_MSG(wxT("Invalid format specifier: ") + str.Mid(formatStart, pos - formatStart + 1));
				case esFormatStart:
					formatStart = pos;
					break;
				case esFormatEnd:
					{
						FormatSpecifier fs;
						fs.startPos = formatStart;
						fs.endPos = pos;
						fs.result = str.Mid(formatStart, pos - formatStart + 1);
						m_formats.push_back(fs);
					}
				default:
					break;
			}
		}
		wxASSERT_MSG((state == esFormatEnd) || (state == esNonFormat), wxT("Incomplete format specifier: ") + str.Mid(formatStart));
	}

	// Parse the extracted format specifiers, removing invalid ones
	unsigned formatCount = 0;
	for (FormatList::iterator it = m_formats.begin(); it != m_formats.end();) {
		if (it->result == wxT("%%")) {
			it->argIndex = 0;
			it->result = wxT("%");
			++it;
		} else {
			it->argIndex = ++formatCount;
			it->flag = '\0';
			it->width = 0;
			it->precision = -1;
			it->type = '\0';
			unsigned num = 0;
			wxString lengthModifier;
			bool isPrecision = false;
			eFormatParserStates state = efStart;
			for (size_t pos = 1; pos < it->result.length(); ++pos) {
				wxChar c = it->result[pos];
				if ((c >= wxT('1')) && (c <= wxT('9'))) {
					state = formatParser[state][0];
				} else if (c == wxT('0')) {
					state = formatParser[state][1];
				} else if (isFlagChar(c)) {
					state = formatParser[state][2];
				} else if (c == wxT('.')) {
					state = formatParser[state][3];
				} else if (isLengthChar(c)) {
					state = formatParser[state][4];
				} else if (isTypeChar(c)) {
					state = formatParser[state][5];
				} else if (c == wxT('$')) {
					state = formatParser[state][6];
				} else {
					state = efError;
				}
				if ((c >= wxT('0')) && (c <= wxT('9'))) {
					num *= 10;
					num += (c - wxT('0'));
				}
				switch (state) {
					case efArgIndexEnd:
						it->argIndex = num;
						num = 0;
						break;
					case efFlagChar:
						it->flag = c;
						break;
					case efPrecStart:
						it->width = num;
						num = 0;
						isPrecision = true;
						break;
					case efLength:
						if (isPrecision) {
							it->precision = num;
						} else if (num > 0) {
							it->width = num;
						}
						num = 0;
						lengthModifier += c;
						if (!isValidLength(lengthModifier)) {
							state = efError;
						}
						break;
					case efType:
						if (isPrecision) {
							it->precision = num;
						} else if (num > 0) {
							it->width = num;
						}
						if (c == wxT('m')) {
							it->argIndex = 0;
							it->type = wxT('s');
							int errnum = errno;
#if defined(HAVE_STRERROR) || defined(HAVE_STRERROR_R)
							unsigned buflen = 256;
							bool done = false;
							do {
								errno = 0;
								char* buf = new char[buflen];
								*buf = '\0';
								int result = mule_strerror_r(errnum, buf, buflen);
								if ((result == 0) || (buflen > 1024)) {
									ProcessArgument<const wxString&>(it, wxString(buf, wxConvLocal));
								} else if (errno == EINVAL) {
									if (*buf == '\0') {
										ProcessArgument<const wxString&>(it, wxString::Format(_("Unknown error %d"), errnum));
									} else {
										ProcessArgument<const wxString&>(it, wxString(buf, wxConvLocal));
									}
								} else if (errno != ERANGE) {
									ProcessArgument<const wxString&>(it, wxString::Format(_("Unable to get error description for error %d"), errnum));
								} else {
									buflen <<= 1;
								}
								delete [] buf;
								done = ((result == 0) || (errno != ERANGE));
							} while (!done);
#else
							wxFAIL_MSG(wxString::Format(wxT("Unable to get error description for error %d."), errnum));
							ProcessArgument<const wxString&>(it, wxString::Format(_("Unable to get error description for error %d"), errnum));
#endif
						} else {
							it->type = c;
						}
					default:
						break;
				}
				wxCHECK2_MSG(state != efError, break, wxT("Invalid format specifier: ") + it->result);
				if (state == efType) {
					// Needed by the '%m' conversion, which takes place immediately,
					// overwriting it->result
					break;
				}
			}
			if (state == efError) {
				it = m_formats.erase(it);
				--formatCount;
			} else {
				++it;
			}
		}
	}
}

wxString CFormat::GetString() const
{
	wxString result;
	FormatList::const_iterator it = m_formats.begin();
	if (it == m_formats.end()) {
		result = m_formatString;
	} else {
		unsigned lastEnd = 0;
		for (; it != m_formats.end(); ++it) {
			result += m_formatString.Mid(lastEnd, it->startPos - lastEnd);
			result += it->result;
			lastEnd = it->endPos + 1;
		}
		result += m_formatString.Mid(lastEnd);
	}
	return result;
}

wxString CFormat::GetModifiers(FormatList::const_iterator it) const
{
	wxString result = wxT("%");
	if (it->flag != wxT('\0')) {
		result += it->flag;
	}
	if (it->width > 0) {
		result += wxString::Format(wxT("%u"), it->width);
	}
	if (it->precision >= 0) {
		result += wxString::Format(wxT(".%u"), it->precision);
	}
	return result;
}

// Forward-declare the specialization for unsigned long long
template<> void CFormat::ProcessArgument(FormatList::iterator, unsigned long long);

// Processing a double-precision floating-point argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, double value)
{
	switch (it->type) {
		case wxT('a'):
		case wxT('A'):
		case wxT('e'):
		case wxT('E'):
		case wxT('f'):
		case wxT('F'):
		case wxT('g'):
		case wxT('G'):
			break;
		case wxT('s'):
			it->type = wxT('g');
			break;
		default:
			wxFAIL_MSG(wxT("Floating-point value passed for non-floating-point format field: ") + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + it->type, value);
}

// Processing a wxChar argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, wxChar value)
{
	switch (it->type) {
		case wxT('c'):
			break;
		case wxT('s'):
			it->type = wxT('c');
			break;
		case wxT('u'):
		case wxT('d'):
		case wxT('i'):
		case wxT('o'):
		case wxT('x'):
		case wxT('X'):
			ProcessArgument(it, (unsigned long long)value);
			return;
		case wxT('a'):
		case wxT('A'):
		case wxT('e'):
		case wxT('E'):
		case wxT('f'):
		case wxT('F'):
		case wxT('g'):
		case wxT('G'):
			ProcessArgument(it, (double)value);
			return;
		default:
			wxFAIL_MSG(wxT("Character value passed to non-character format field: ") + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + it->type, value);
}

// Processing a signed long long argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, signed long long value)
{
	switch (it->type) {
		case wxT('c'):
			ProcessArgument(it, (wxChar)value);
			return;
		case wxT('i'):
			break;
		case wxT('o'):
		case wxT('x'):
		case wxT('X'):
			ProcessArgument(it, (unsigned long long)value);
			return;
		case wxT('u'):
		case wxT('d'):
		case wxT('s'):
			it->type = wxT('i');
			break;
		case wxT('a'):
		case wxT('A'):
		case wxT('e'):
		case wxT('E'):
		case wxT('f'):
		case wxT('F'):
		case wxT('g'):
		case wxT('G'):
			ProcessArgument(it, (double)value);
			return;
		default:
			wxFAIL_MSG(wxT("Integer value passed for non-integer format field: ") + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + WXLONGLONGFMTSPEC + it->type, value);
}

// Processing an unsigned long long argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, unsigned long long value)
{
	switch (it->type) {
		case wxT('c'):
			ProcessArgument(it, (wxChar)value);
			return;
		case wxT('u'):
		case wxT('o'):
		case wxT('x'):
		case wxT('X'):
			break;
		case wxT('i'):
		case wxT('d'):
		case wxT('s'):
			it->type = wxT('u');
			break;
		case wxT('a'):
		case wxT('A'):
		case wxT('e'):
		case wxT('E'):
		case wxT('f'):
		case wxT('F'):
		case wxT('g'):
		case wxT('G'):
			ProcessArgument(it, (double)value);
			return;
		default:
			wxFAIL_MSG(wxT("Integer value passed for non-integer format field: ") + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + WXLONGLONGFMTSPEC + it->type, value);
}

// Processing a wxString argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, const wxString& value)
{
	if (it->type != wxT('s')) {
		wxFAIL_MSG(wxT("String value passed for non-string format field: ") + it->result);
	} else {
		if (it->precision >= 0) {
			it->result = value.Left(it->precision);
		} else {
			it->result = value;
		}
		if ((it->width > 0) && (it->result.length() < it->width)) {
			if (it->flag == wxT('-')) {
				it->result += wxString(it->width - it->result.length(), wxT(' '));
			} else {
				it->result = wxString(it->width -it->result.length(), wxT(' ')) + it->result;
			}
		}
	}
}

// Processing pointer arguments
template<>
void CFormat::ProcessArgument(FormatList::iterator it, void * value)
{
	if ((it->type == wxT('p')) || (it->type == wxT('s'))) {
		// Modifiers (if any) are ignored for pointer conversions
		// built-in Format for pointer is not consistent:
		// - Windows: uppercase, no leading 0x
		// - Linux:   leading zeros missing
		// -> format it as hex
		if (sizeof(void*) == 8) {
			// 64 bit
			it->result = wxString::Format(wxT("0x%016x"), (uintptr_t)value);
		} else {
			// 32 bit
			it->result = wxString::Format(wxT("0x%08x"), (uintptr_t)value);
		}
	} else {
		wxFAIL_MSG(wxT("Pointer value passed for non-pointer format field: ") + it->result);
	}
}


// Generic argument processor template
template<typename _Tp>
CFormat& CFormat::operator%(_Tp value)
{
	m_argIndex++;
	for (FormatList::iterator it = m_formats.begin(); it != m_formats.end(); ++it) {
		if (it->argIndex == m_argIndex) {
			if ((it->type != wxT('n')) && (it->type != wxT('C')) && (it->type != wxT('S'))) {
				ProcessArgument<_Tp>(it, value);
			} else {
				wxFAIL_MSG(wxT("Not supported conversion type in format field: ") + it->result);
			}
		}
	}
	return *this;
}

// explicit instatiation for the types we handle
template CFormat& CFormat::operator%<double>(double);
template CFormat& CFormat::operator%<wxChar>(wxChar);
template CFormat& CFormat::operator%<signed long long>(signed long long);
template CFormat& CFormat::operator%<unsigned long long>(unsigned long long);
template CFormat& CFormat::operator%<const wxString&>(const wxString&);
template CFormat& CFormat::operator%<void *>(void *);

// File_checked_for_headers

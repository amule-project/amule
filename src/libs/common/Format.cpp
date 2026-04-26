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

#include "config.h"

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
		case 's':		// String of characters
		case 'u':		// Unsigned decimal integer
		case 'i':		// Signed decimal integer
		case 'd':		// Signed decimal integer
		case 'c':		// Character
		case 'f':		// Decimal floating point
		case 'F':		// Decimal floating point
		case 'x':		// Unsigned hexadecimal integer
		case 'X':		// Unsigned hexadecimal integer (capital letters)
		case 'o':		// Unsigned octal
		case 'e':		// Scientific notation (mantise/exponent) using e character
		case 'E':		// Scientific notation (mantise/exponent) using E character
		case 'g':		// Use shorter %e or %f
		case 'G':		// Use shorter %E or %F
		case 'p':		// Pointer
		case 'n':		// Not supported, still needs to be caught though
		case 'a':		// (C99; not in SUSv2) Double in hexadecimal notation.
		case 'A':		// (C99; not in SUSv2) Double in hexadecimal notation (capital letters).
		case 'C':		// (Not in C99, but in SUSv2.)  Synonym for lc.  Don't use. Not supported.
		case 'S':		// (Not in C99, but in SUSv2.)  Synonym for ls.  Don't use. Not supported.
		case 'm':		// (Glibc extension.)  Print output of strerror(errno).  No argument is required.
//		case '%':		// A `%' is written.  No argument is converted.  The complete conversion specification is `%%'.
			return true;
	}

	return false;
}

/** Returns true if the char is a valid flag. */
inline bool isFlagChar(wxChar c)
{
	switch (c) {
		// C standard flags
		case '+':		// Include sign for integers
		case '-':		// Left-align output
		case '#':		// Alternate form, varies
		case ' ':		// Pad with spaces
		case '0':		// Pad with zeros
		// SUSv2
		case '\'':		// For decimal conversion (i, d, u, f, F, g, G) the output is to be grouped with thousands' grouping characters if the locale information indicates any.
		// glibc 2.2
		case 'I':		// For decimal integer conversion (i, d, u) the output uses the locale's alternative output digits, if any.
			return true;
	}

	return false;
}

/** Returns true if the char is a valid length modifier. */
inline bool isLengthChar(wxChar c)
{
	switch (c) {
		case 'h':		// Short ('hh') and char ('h')
		case 'l':		// Long ('l') and long long ('ll')
		case 'L':		// Double long
		case 'z':		// size_t
		case 'j':		// intmax_t
		case 't':		// ptrdiff_t
		case 'q':		// quad. Synonym for 'll'. Don't use.
			return true;
	}

	return false;
}

/** Returns true if the argument is a valid length modifier */
inline bool isValidLength(const wxString& str)
{
	return ((str == "hh") || (str == "h") ||
		(str == "l") || (str == "ll") ||
		(str == "L") || (str == "z") ||
		(str == "j") || (str == "t") ||
		(str == "q"));
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
			/* %-sign,		type-char,	other */
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
 *	%[argIndex$][Flags][Width][.Precision][Length]<Type>
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
		size_t formatStart = 0;
		eStringParserStates state = esNonFormat;
		for (size_t pos = 0; pos < str.length(); ++pos) {
			if (str[pos] == '%') {
				state = stringParser[state][0];
			} else if (isTypeChar(str[pos])) {
				state = stringParser[state][1];
			} else {
				state = stringParser[state][2];
			}
			switch (state) {
				case esInvalidFormat:
					wxFAIL_MSG("Invalid format specifier: " + str.Mid(formatStart, pos - formatStart + 1));
				/* fall through */
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
		wxASSERT_MSG((state == esFormatEnd) || (state == esNonFormat), "Incomplete format specifier: " + str.Mid(formatStart));
	}

	// Parse the extracted format specifiers, removing invalid ones
	unsigned formatCount = 0;
	for (FormatList::iterator it = m_formats.begin(); it != m_formats.end();) {
		if (it->result == "%%") {
			it->argIndex = 0;
			it->result = "%";
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
				if ((c >= '1') && (c <= '9')) {
					state = formatParser[state][0];
				} else if (c == '0') {
					state = formatParser[state][1];
				} else if (isFlagChar(c)) {
					state = formatParser[state][2];
				} else if (c == '.') {
					state = formatParser[state][3];
				} else if (isLengthChar(c)) {
					state = formatParser[state][4];
				} else if (isTypeChar(c)) {
					state = formatParser[state][5];
				} else if (c == '$') {
					state = formatParser[state][6];
				} else {
					state = efError;
				}
				if ((c >= '0') && (c <= '9')) {
					num *= 10;
					num += (c - '0');
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
						if (c == 'm') {
							it->argIndex = 0;
							it->type = 's';
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
									ProcessArgument<const wxString&>(it, wxString(buf, wxConvLibc));
								} else if (errno == EINVAL) {
									if (*buf == '\0') {
										ProcessArgument<const wxString&>(it, wxString::Format(_("Unknown error %d"), errnum));
									} else {
										ProcessArgument<const wxString&>(it, wxString(buf, wxConvLibc));
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
							wxFAIL_MSG(wxString::Format("Unable to get error description for error %d.", errnum));
							ProcessArgument<const wxString&>(it, wxString::Format(_("Unable to get error description for error %d"), errnum));
#endif
						} else {
							it->type = c;
						}
					default:
						break;
				}
				wxCHECK2_MSG(state != efError, break, "Invalid format specifier: " + it->result);
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
	wxString result = "%";
	if (it->flag != '\0') {
		result += it->flag;
	}
	if (it->width > 0) {
		result += wxString::Format("%u", it->width);
	}
	if (it->precision >= 0) {
		result += wxString::Format(".%u", it->precision);
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
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			break;
		case 's':
			it->type = 'g';
			break;
		default:
			wxFAIL_MSG("Floating-point value passed for non-floating-point format field: " + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + it->type, value);
}

// Processing a wxChar argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, wxChar value)
{
	switch (it->type) {
		case 'c':
			break;
		case 's':
			it->type = 'c';
			break;
		case 'u':
		case 'd':
		case 'i':
		case 'o':
		case 'x':
		case 'X':
			ProcessArgument(it, (unsigned long long)value);
			return;
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			ProcessArgument(it, (double)value);
			return;
		default:
			wxFAIL_MSG("Character value passed to non-character format field: " + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + it->type, value);
}

// Processing a signed long long argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, signed long long value)
{
	switch (it->type) {
		case 'c':
			ProcessArgument(it, (wxChar)value);
			return;
		case 'i':
			break;
		case 'o':
		case 'x':
		case 'X':
			ProcessArgument(it, (unsigned long long)value);
			return;
		case 'u':
		case 'd':
		case 's':
			it->type = 'i';
			break;
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			ProcessArgument(it, (double)value);
			return;
		default:
			wxFAIL_MSG("Integer value passed for non-integer format field: " + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + WXLONGLONGFMTSPEC + it->type, value);
}

// Processing an unsigned long long argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, unsigned long long value)
{
	switch (it->type) {
		case 'c':
			ProcessArgument(it, (wxChar)value);
			return;
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			break;
		case 'i':
		case 'd':
		case 's':
			it->type = 'u';
			break;
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
			ProcessArgument(it, (double)value);
			return;
		default:
			wxFAIL_MSG("Integer value passed for non-integer format field: " + it->result);
			return;
	}
	it->result = wxString::Format(GetModifiers(it) + WXLONGLONGFMTSPEC + it->type, value);
}

// Processing a wxString argument
template<>
void CFormat::ProcessArgument(FormatList::iterator it, const wxString& value)
{
	if (it->type != 's') {
		wxFAIL_MSG("String value passed for non-string format field: " + it->result);
	} else {
		if (it->precision >= 0) {
			it->result = value.Left(it->precision);
		} else {
			it->result = value;
		}
		if ((it->width > 0) && (it->result.length() < it->width)) {
			if (it->flag == '-') {
				it->result += wxString(it->width - it->result.length(), ' ');
			} else {
				it->result = wxString(it->width -it->result.length(), ' ') + it->result;
			}
		}
	}
}

// Processing pointer arguments
template<>
void CFormat::ProcessArgument(FormatList::iterator it, void * value)
{
	if ((it->type == 'p') || (it->type == 's')) {
		// Modifiers (if any) are ignored for pointer conversions
		// built-in Format for pointer is not consistent:
		// - Windows: uppercase, no leading 0x
		// - Linux:   leading zeros missing
		// -> format it as hex
		if (sizeof(void*) == 8) {
			// 64 bit
			it->result = wxString::Format(wxString("0x%016") + WXLONGLONGFMTSPEC + "x", (uintptr_t)value);
		} else {
			// 32 bit
			it->result = wxString::Format("0x%08x", (uintptr_t)value);
		}
	} else {
		wxFAIL_MSG("Pointer value passed for non-pointer format field: " + it->result);
	}
}


// Generic argument processor template
template<typename _Tp>
CFormat& CFormat::operator%(_Tp value)
{
	m_argIndex++;
	for (FormatList::iterator it = m_formats.begin(); it != m_formats.end(); ++it) {
		if (it->argIndex == m_argIndex) {
			if ((it->type != 'n') && (it->type != 'C') && (it->type != 'S')) {
				ProcessArgument<_Tp>(it, value);
			} else {
				wxFAIL_MSG("Not supported conversion type in format field: " + it->result);
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

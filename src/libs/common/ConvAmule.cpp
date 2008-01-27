//
// This file is part of the aMule Project.
//
// Copyright (c) 2007-2008 Marcelo Roberto Jimenez - Phoenix (phoenix@amule.org)
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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


#define CONVAMULE_CPP


#include "ConvAmule.h"

/*
 * ConvAmuleBrokenFileNames
 *
 * This class has two important functions:
 * 1 - MB2WC(), that performs multibyte to wide character (UNICODE) conversions;
 * 2 - WC2MB(), that performs UNICODE to multibyte sequences conversions.
 *
 * The tricky part is that the following expression MUST BE TRUE:
 *
 * WC2MB(MC2WB(x)) == x
 *
 * i.e., the above operation must be the identity operator, or in other words,
 * this operator is an invariant. Failure to satisfy it will result in a file
 * name beeing read from a directory, but if the same name (now stored as an
 * UNICODE string) is used to open the file, it will result in a failure.
 *
 * Trying to cope with two encodings at the same time is the source of our
 * problems. Either we should be able to store the original file name string
 * or encoding, so that we have enough information to satisfy the invariance
 * relation, or we could not use UNICODE strings to store the file names. Or
 * maybe we are just doomed.
 */


/*
 * Constructor
 */
ConvAmuleBrokenFileNames::ConvAmuleBrokenFileNames(
	const wxChar *charset)
:
wxMBConvUTF8(MAP_INVALID_UTF8_NOT),
m_charset(charset),
m_conv(charset)
{
}


/*
 * Copy constructor
 */
ConvAmuleBrokenFileNames::ConvAmuleBrokenFileNames(
	const ConvAmuleBrokenFileNames& conv)
:
wxMBConvUTF8(MAP_INVALID_UTF8_NOT),
m_charset(conv.m_charset),
m_conv(conv.m_charset)
{
}


/*
 * Destructor
 */
ConvAmuleBrokenFileNames::~ConvAmuleBrokenFileNames()
{
}


/*
 * ConvAmuleBrokenFileNames::MB2WC()
 *
 * This function is used to convert multibyte sequences to UNICODE.
 * In particular, it will be used to convert file names read from the system
 * to UNICODE.
 *
 * The present strategy is to try to convert the name assuming it is a valid
 * UTF-8 sequence. This is supposed to be 99% of the cases. But some systems
 * use non-UTF-8 encodings, and even on UTF-8 configured systems, an
 * application is free to save a file name using any valid character sequence
 * that it whishes, as long as it does not contain some special characters like
 * "*", "?", "/", etc.
 *
 * So, if the conversion to UNICODE fails, this means that the name is not
 * UTF-8 encoded, so we use ISO-8859-* (a reasonable default value should be
 * ISO-8859-1) and this way the conversion never fails.
 *
 * This is the only way to guarantee that we can open any file in the file
 * system, even if it is not an UTF-8 valid sequence. But it does not guarantee
 * that a file name that was search in a directory will be able to be opened
 * later if you store the UNICODE string (read above for the invariance relation
 * that must be satisfied for that).
 */
size_t ConvAmuleBrokenFileNames::MB2WC(
	wchar_t *out,
	const char *in,
	size_t outLen) const
{
// Uncomment here to allow only UTF-8. Doing so will imply the impossibility of
// working with some ISO-8859-* encoded file names.
#if 0
	size_t ret = wxMBConvUTF8::MB2WC(out, in, outLen);
	
	return ret;
#else
	size_t ret = wxMBConvUTF8::MB2WC(out, in, outLen);
	if (ret == wxCONV_FAILED) {
		ret = m_conv.MB2WC(out, in, outLen);
	}
	
	return ret;
#endif
}


/*
 * ConvAmuleBrokenFileNames::WC2MB()
 *
 * This function is used to convert UNICODE to multibyte sequences.
 * In previous versions, we had the following code:
 *
	size_t ret = m_conv.WC2MB(out, in, outLen);
	if (ret == wxCONV_FAILED) {
		ret = wxMBConvUTF8::WC2MB(out, in, outLen);
	}
	
	return ret;
 *
 * The strategy here is to assume that the system encoding is non-UTF-8, and
 * first attempt to convert UNICODE to ISO-8859-*. If the conversion is not
 * possible, then we perform a conversion to UTF-8, which is always possible.
 *
 * This behaviour is not good, as it will create ISO-8859-* encoded file names
 * in a UTF-8 encoded system. It also fails to satisfy the invariance relation
 * stated at the top of this file, which means that some file names will be
 * read from a directory, but an attempt to open these files using the stored
 * UNICODE string will fail.
 *
 * The most sane option seems to be to create only UTF-8 encoded file names
 * because it will consistently create file names in UTF-8, which should be
 * what the majority of systems expect from an application today.
 */
size_t ConvAmuleBrokenFileNames::WC2MB(
	char *out,
	const wchar_t *in,
	size_t outLen) const
{
// Uncomment here to allow only UTF-8
#if 0
	size_t ret = wxMBConvUTF8::WC2MB(out, in, outLen);
	
	return ret;
#else
	size_t ret = m_conv.WC2MB(out, in, outLen);
	if (ret == wxCONV_FAILED) {
		ret = wxMBConvUTF8::WC2MB(out, in, outLen);
	}
	
	return ret;
#endif
}


size_t ConvAmuleBrokenFileNames::GetMBNulLen() const
{
	// cast needed to call a private function
	return m_conv.GetMBNulLen();
}


wxMBConv *ConvAmuleBrokenFileNames::Clone() const
{
	return new ConvAmuleBrokenFileNames(*this);
}


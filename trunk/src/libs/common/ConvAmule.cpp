//
// This file is part of the aMule Project.
//
// Copyright (c) 2007 Marcelo Roberto Jimenez - Phoenix (phoenix@amule.org)
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#if !wxCHECK_VERSION(2,8,0)
#define wxCONV_FAILED ((size_t)-1)
#endif


// When converting file names, we will always first try to create an ANSI
// file name, even if that means an extended ANSI file name. Only if it is
// not possible to do that, we fall back to  UTF-8 file names. This is unicode
// safe and is the only way to guarantee that we can open any file in the file
// system, even if it is not an UTF-8 valid sequence.


ConvAmuleBrokenFileNames::ConvAmuleBrokenFileNames(
	const wxChar *charset)
:
wxMBConvUTF8(MAP_INVALID_UTF8_NOT),
m_charset(charset),
m_conv(charset)
{
}


ConvAmuleBrokenFileNames::ConvAmuleBrokenFileNames(
	const ConvAmuleBrokenFileNames& conv)
:
wxMBConvUTF8(MAP_INVALID_UTF8_NOT),
m_charset(conv.m_charset),
m_conv(conv.m_charset)
{
}


ConvAmuleBrokenFileNames::~ConvAmuleBrokenFileNames()
{
}


size_t ConvAmuleBrokenFileNames::MB2WC(
	wchar_t *out,
	const char *in,
	size_t outLen) const
{
	size_t ret;
	
	ret = wxMBConvUTF8::MB2WC(out, in, outLen);
	if (ret == wxCONV_FAILED) {
		ret = m_conv.MB2WC(out, in, outLen);
	}
	
	return ret;
}


size_t ConvAmuleBrokenFileNames::WC2MB(
	char *out,
	const wchar_t *in,
	size_t outLen) const
{
	size_t ret;
	
	ret = m_conv.WC2MB(out, in, outLen);
	if (ret == wxCONV_FAILED) {
		ret = wxMBConvUTF8::WC2MB(out, in, outLen);
	}
	
	return ret;
}


#if wxCHECK_VERSION(2,8,0)
size_t ConvAmuleBrokenFileNames::GetMBNulLen() const
{
	// cast needed to call a private function
	return m_conv.GetMBNulLen();
}
#endif


wxMBConv *ConvAmuleBrokenFileNames::Clone() const
{
	return new ConvAmuleBrokenFileNames(*this);
}


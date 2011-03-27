//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef FILEVIEW_PRINT_H
#define FILEVIEW_PRINT_H

#include <common/DataFileVersion.h>
#include <common/StringFunctions.h>
#include "../../kademlia/utils/UInt128.h"
#include "../../MD4Hash.h"
#include "../../Tag.h"

#include <cstdio>
#include <time.h>
#include <iostream>

#if defined __GNUC__ && defined __GNUC_MINOR__
#	if __GNUC__ > 3 || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 2))
#		define __attribute_always_inline__	__attribute__((__always_inline__))
#	else
#		define __attrbiute_always_inline__
#	endif
#else
#	define __attribute_always_inline__
#endif

using std::cout;
using std::cerr;
using std::endl;

enum SDMODE {
	SD_DISPLAY,
	SD_SAFE,
	SD_UTF8,
	SD_NONE
};

extern SDMODE g_stringDecodeMode;

inline void SetStringsMode(SDMODE mode)	{ g_stringDecodeMode = mode; }
inline SDMODE GetStringsMode()	{ return g_stringDecodeMode; }

wxString MakePrintableString(const wxString& str);

#if !wxCHECK_VERSION(2, 9, 0)
inline __attribute_always_inline__ std::ostream& operator<<(std::ostream& x, const wxString& y)		{ return x << (const char *)unicode2char(y); }
#endif
inline std::ostream& operator<<(std::ostream& x, const Kademlia::CUInt128& y)	{ return x << y.ToHexString(); }
inline std::ostream& operator<<(std::ostream& x, const CMD4Hash& y)		{ return x << y.Encode(); }

inline wxString hex(uint8_t value)	{ return wxString::Format(wxT("0x%02x"), value); }
inline wxString hex(uint16_t value)	{ return wxString::Format(wxT("0x%04x"), value); }
inline wxString hex(uint32_t value)	{ return wxString::Format(wxT("0x%08x"), value); }

inline void PrintByteArray(const void *buf, unsigned int size)
{
// #if wxCHECK_VERSION(2, 8, 4)
// 	cout << MakePrintableString(wxString::From8BitData(static_cast<const char *>(buf), size));
// #else
	for (unsigned int i = 0; i < size; i++) {
		cout << wxString::Format(wxT("%02X "), static_cast<const unsigned char *>(buf)[i]);
	}
// #endif
}


class CTimeT
{
      public:
	explicit CTimeT(time_t t) { m_time = t; }
	operator time_t() const { return m_time; }
      private:
	time_t m_time;
};

class CKadIP
{
      public:
	explicit CKadIP(uint32_t ip) { m_ip = ip; }
	operator uint32_t() const { return m_ip; }
      private:
	uint32_t m_ip;
};

class CeD2kIP
{
      public:
	explicit CeD2kIP(uint32_t ip) { m_ip = ip; }
	operator uint32_t() const { return m_ip; }
      private:
	uint32_t m_ip;
};

class CServerTag : public CTag
{
      public:
	CServerTag(const CTag& tag)
		: CTag(tag)
	{}
};

class CFriendTag : public CTag
{
      public:
	CFriendTag(const CTag& tag)
		: CTag(tag)
	{}
};

std::ostream& operator<<(std::ostream& x, const CTimeT& y);
std::ostream& operator<<(std::ostream& x, const CKadIP& ip);
std::ostream& operator<<(std::ostream& x, const CeD2kIP& ip);
std::ostream& operator<<(std::ostream& x, const CTag& tag);
std::ostream& operator<<(std::ostream& x, const CServerTag& tag);
std::ostream& operator<<(std::ostream& x, const CFriendTag& tag);

#endif /* FILEVIEW_PRINT_H */

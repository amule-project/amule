//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2009 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2004-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <common/StringFunctions.h>
#include "../../kademlia/utils/UInt128.h"
#include "../../MD4Hash.h"

#include <cstdio>
#include <time.h>

class CTag;


wxString MakePrintableString(const wxString& str);


template<typename _Tp> void Print(const _Tp& value);
template<typename _Tp> void PrintHex(_Tp value);

void inline DoPrint(const wxString& str)	{ std::printf("%s", (const char *)unicode2char(str)); }

template<> inline void PrintHex<uint8_t>(uint8_t value)		{ DoPrint(wxString::Format(wxT("0x%02x"), value)); }
template<> inline void PrintHex<uint16_t>(uint16_t value)	{ DoPrint(wxString::Format(wxT("0x%04x"), value)); }
template<> inline void PrintHex<uint32_t>(uint32_t value)	{ DoPrint(wxString::Format(wxT("0x%08x"), value)); }

template<> inline void Print<bool>(const bool& value)					{ DoPrint(value ? wxT("true") : wxT("false")); }
template<> inline void Print<uint8_t>(const uint8_t& value)				{ DoPrint(wxString::Format(wxT("%u"), value)); }
template<> inline void Print<uint16_t>(const uint16_t& value)				{ DoPrint(wxString::Format(wxT("%u"), value)); }
template<> inline void Print<uint32_t>(const uint32_t& value)				{ DoPrint(wxString::Format(wxT("%u"), value)); }
template<> inline void Print<uint64_t>(const uint64_t& value)				{ DoPrint(wxString::Format(wxT("%") wxLongLongFmtSpec wxT("u"), value)); }
template<> inline void Print<float>(const float& value)					{ DoPrint(wxString::Format(wxT("%g"), value)); }
template<> inline void Print<double>(const double& value)				{ DoPrint(wxString::Format(wxT("%g"), value)); }
template<> inline void Print<wxString>(const wxString& value)				{ DoPrint(MakePrintableString(value)); }
template<> inline void Print<Kademlia::CUInt128>(const Kademlia::CUInt128& value)	{ DoPrint(value.ToHexString()); }
template<> inline void Print<CMD4Hash>(const CMD4Hash& value)				{ DoPrint(value.Encode()); }

// Transparent class for Kad IPs
class CKadIP
{
      public:
	CKadIP(uint32_t ip) { m_ip = ip; }
	operator uint32_t() const { return m_ip; }
      private:
	uint32_t m_ip;
};

template<> void Print<time_t>(const time_t& time);
template<> void Print<CKadIP>(const CKadIP& ip);
template<> void Print<CTag>(const CTag& tag);


// Some formatting functions

inline wxString Uint32toStringIP(uint32_t ip)
{
	return wxString::Format(wxT("%u.%u.%u.%u"), (uint8_t)ip, (uint8_t)(ip>>8), (uint8_t)(ip>>16), (uint8_t)(ip>>24));	
}

inline wxString Uint32_16toStringIP_Port(uint32_t ip, uint16_t port)
{
	return wxString::Format(wxT("%u.%u.%u.%u:%u"), (uint8_t)ip, (uint8_t)(ip>>8), (uint8_t)(ip>>16), (uint8_t)(ip>>24), port);	
}

#endif /* FILEVIEW_PRINT_H */

//
// This file is part of the aMule Project.
//
// Copyright (c) 2015 aMule Team ( admin@amule.org / http://www.amule.org )
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


// Implementation of these functions cannot be in ECTag.cpp, because here we
// use non-inline member functions of CUInt128.  This way these functions are
// only needed if we actually use the CUInt128 class, and will not cause
// linker errors for apps like aMuleCmd, which don't use CUInt128.
//
// The other working possibility would be to make all these functions inline
// members of CECTag, but that would clutter the header too much.


#include "ECTag.h"
#include "ECSpecialTags.h"
#include "../../../kademlia/utils/UInt128.h"


CECTag::CECTag(ec_tagname_t tagname, const CUInt128& data)
	: m_tagName(tagname),
	  m_dataType(EC_TAGTYPE_UINT128),
	  m_dataLen(16)
{
	NewData();
	data.ToByteArray(reinterpret_cast<uint8_t *>(m_tagData));
}

void CECTag::AddTag(ec_tagname_t name, const CUInt128& data, CValueMap *valuemap)
{
	if (valuemap) {
		valuemap->CreateTag(name, data, this);
	} else {
		AddTag(CECTag(name, data));
	}
}

CUInt128 CECTag::GetInt128Data() const
{
	if (m_dataType != EC_TAGTYPE_UINT128) {
		EC_ASSERT(m_dataType == EC_TAGTYPE_UNKNOWN);
		return CUInt128(false);
	}

	EC_ASSERT(m_tagData != NULL);

	if (m_tagData) {
		return CUInt128(reinterpret_cast<const uint8_t *>(m_tagData));
	} else {
		return CUInt128(false);
	}
}

CUInt128 CECTag::AssignIfExist(ec_tagname_t tagname, CUInt128 *target) const
{
	if (AssignIfExist(tagname, *target)) {
		return *target;
	} else {
		return CUInt128(false);
	}
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, CUInt128 &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetInt128Data();
		return true;
	}
	return false;
}

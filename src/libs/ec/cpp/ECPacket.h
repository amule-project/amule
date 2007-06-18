//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef ECPACKET_H
#define ECPACKET_H

#include "ECTag.h"

// Define this to keep partial packets
// (those that had an error upon reception/creation)
#undef KEEP_PARTIAL_PACKETS

class CECSocket;

/**
 * High level EC packet handler class
 */
class CECPacket : protected CECEmptyTag {
	friend class CECSocket;
	public:
		CECPacket(ec_opcode_t opCode, EC_DETAIL_LEVEL detail_level = EC_DETAIL_FULL)
		: CECEmptyTag(0), m_opCode(opCode)
		{
			// since EC_DETAIL_FULL is default - no point transmit it
			if ( detail_level != EC_DETAIL_FULL ) {
				AddTag(CECTag(EC_TAG_DETAIL_LEVEL, (uint64)detail_level));
			}
		}
		
		CECTag::AddTag;
		CECTag::GetTagByIndex;
		CECTag::GetTagByIndexSafe;
		CECTag::GetTagByName;
		CECTag::GetTagByNameSafe;
		CECTag::GetTagCount;

		ec_opcode_t	GetOpCode(void) const { return m_opCode; }
		uint32_t		GetPacketLength(void) const { return CECTag::GetTagLen(); }
		EC_DETAIL_LEVEL GetDetailLevel() const
		{
			const CECTag *tag = GetTagByName(EC_TAG_DETAIL_LEVEL);
			return (tag) ? (EC_DETAIL_LEVEL)tag->GetInt() : EC_DETAIL_FULL;
		}
		
	private:
		CECPacket(const CECSocket& socket)
			: CECEmptyTag(socket)
			{}

		bool ReadFromSocket(CECSocket& socket);
		bool WritePacket(CECSocket& socket) const;
		ec_opcode_t	m_opCode;
};

#endif /* ECPACKET_H */
// File_checked_for_headers

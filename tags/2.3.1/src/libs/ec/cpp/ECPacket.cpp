//
// This file is part of the aMule Project.
//
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

#include "ECPacket.h"	// Needed for ECPacket
#include "ECSocket.h"	// Needed for CECSocket

/**********************************************************
 *							  *
 *	CECPacket class					  *
 *							  *
 **********************************************************/

bool CECPacket::ReadFromSocket(CECSocket& socket)
{
	return socket.ReadNumber(&m_opCode, sizeof(ec_opcode_t))
		&& ReadChildren(socket);
}


bool CECPacket::WritePacket(CECSocket& socket) const
{
	if (!socket.WriteNumber(&m_opCode, sizeof(ec_opcode_t))) return false;
	if (!WriteChildren(socket)) return false;
	return true;
}

#ifdef __DEBUG__
#include <common/Format.h>  // Needed for CFormat
void CECPacket::DebugPrint(bool incoming, uint32 trueSize) const
{
	wxString GetDebugNameECOpCodes(uint8 arg);

	if (ECLogIsEnabled()) {
		uint32 size = GetPacketLength() + sizeof(ec_opcode_t) + 2;	// full length incl. header

		if (trueSize == 0 || size == trueSize) {
			DoECLogLine(CFormat(wxT("%s %s %d")) % (incoming ? wxT("<") : wxT(">")) 
				% GetDebugNameECOpCodes(m_opCode) % size);
		} else {
			DoECLogLine(CFormat(wxT("%s %s %d (compressed: %d)")) % (incoming ? wxT("<") : wxT(">")) 
				% GetDebugNameECOpCodes(m_opCode) % size % trueSize);
		}
		CECTag::DebugPrint(1, false);
	}
}
#else
void CECPacket::DebugPrint(bool, uint32) const {}
#endif

/*!
 * \fn CECPacket::CECPacket(ec_opcode_t opCode, EC_DETAIL_LEVEL detail_level)
 *
 * \brief Creates a new packet with given OPCODE.
 */

/*!
 * \fn ec_opcode_t CECPacket::GetOpCode(void) const
 *
 * \brief Returns OPCODE.
 *
 * \return The OpCode of the packet.
 */

/*!
 * \fn uint32 CECPacket::GetPacketLength(void) const
 *
 * \brief Returns the length of the packet.
 *
 * \return The length of the packet.
 */
// File_checked_for_headers

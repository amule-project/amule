//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
	if (m_state == bsName) {
		if (!socket.ReadNumber(&m_opCode, sizeof(ec_opcode_t))) {
			return false;
		} else {
			m_state = bsChildCnt;
		}
	}
	if (m_state == bsChildCnt || m_state == bsChildren) {
		if (!ReadChildren(socket)) {
			return false;
		}
	}
	m_state = bsFinished;
	return true;
}


bool CECPacket::WritePacket(CECSocket& socket) const
{
	if (!socket.WriteNumber(&m_opCode, sizeof(ec_opcode_t))) return false;
	if (!WriteChildren(socket)) return false;
	return true;
}

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


//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef ED2KSEARCHPACKETBUILDER_H
#define ED2KSEARCHPACKETBUILDER_H

#include <wx/string.h>
#include <memory>
#include <cstdint>

namespace search {

class SearchParams;

/**
 * ED2K Search Packet Builder
 * 
 * This class handles the creation of ED2K search packets,
 * including parameter encoding and packet formatting.
 */
class ED2KSearchPacketBuilder
{
public:
    /**
     * Creates a search packet for ED2K search.
     * 
     * @param params Search parameters
     * @param supports64bit Whether the server supports 64-bit file sizes
     * @param[out] packetData The created packet data
     * @param[out] packetSize Size of the packet
     * @return true if packet was created successfully
     */
    static bool CreateSearchPacket(const SearchParams& params, bool supports64bit,
				 uint8_t*& packetData, uint32_t& packetSize);

    /**
     * Cleans up packet data created by CreateSearchPacket.
     * 
     * @param packetData Packet data to free
     */
    static void FreeSearchPacket(uint8_t* packetData);

private:
    // Helper methods for packet construction
    static bool EncodeSearchParams(const SearchParams& params, bool supports64bit,
			       uint8_t*& packetData, uint32_t& packetSize);
};

} // namespace search

#endif // ED2KSEARCHPACKETBUILDER_H


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

#ifndef ED2KSEARCHHELPER_H
#define ED2KSEARCHHELPER_H

#include <wx/string.h>
#include <memory>
#include "SearchModel.h"

namespace search {

class SearchParams;

/**
 * Helper class for ED2K search operations.
 * 
 * This class provides utility functions for ED2K searches,
 * including packet creation and search validation.
 */
class ED2KSearchHelper
{
public:
	/**
	 * Validates if ED2K search can be performed.
	 * 
	 * @return true if ED2K is connected and ready for search
	 */
	static bool CanPerformSearch();

	/**
	 * Checks if the current server supports 64-bit file sizes.
	 * 
	 * @return true if server supports large files
	 */
	static bool SupportsLargeFiles();

	/**
	 * Creates a search packet for ED2K search.
	 * 
	 * @param params Search parameters
	 * @param isLocalSearch Whether this is a local search
	 * @param[out] packetData The created packet data
	 * @param[out] packetSize Size of the packet
	 * @return true if packet was created successfully
	 */
	static bool CreateSearchPacket(const SearchParams& params, ModernSearchType searchType,
				      uint8_t*& packetData, uint32_t& packetSize);

	/**
	 * Sends a search packet to the ED2K server.
	 * 
	 * @param packetData Packet data to send
	 * @param packetSize Size of the packet
	 * @param isLocalSearch Whether this is a local search
	 * @return true if packet was sent successfully
	 */
	static bool SendSearchPacket(const uint8_t* packetData, uint32_t packetSize,
				     ModernSearchType searchType);

	/**
	 * Cleans up packet data created by CreateSearchPacket.
	 * 
	 * @param packetData Packet data to free
	 */
	static void FreeSearchPacket(uint8_t* packetData);
};

} // namespace search

#endif // ED2KSEARCHHELPER_H


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

#ifndef KADSEARCHHELPER_H
#define KADSEARCHHELPER_H

#include <wx/string.h>
#include <memory>

namespace search {

class SearchParams;

/**
 * Helper class for Kad search operations.
 * 
 * This class provides utility functions for Kad searches,
 * including packet creation and search validation.
 */
class KadSearchHelper
{
public:
	/**
	 * Validates if Kad search can be performed.
	 * 
	 * @return true if Kad is running and ready for search
	 */
	static bool CanPerformSearch();

	/**
	 * Extracts the keyword for Kad search from the search string.
	 * 
	 * @param searchString The search string
	 * @param[out] keyword The extracted keyword
	 * @return true if keyword was extracted successfully
	 */
	static bool ExtractKeyword(const wxString& searchString, wxString& keyword);

	/**
	 * Creates a search packet for Kad search.
	 * 
	 * @param params Search parameters
	 * @param[out] packetData The created packet data
	 * @param[out] packetSize Size of the packet
	 * @return true if packet was created successfully
	 */
	static bool CreateSearchPacket(const SearchParams& params,
				      uint8_t*& packetData, uint32_t& packetSize);

	/**
	 * Starts a Kad search with the given packet.
	 * 
	 * @param packetData Packet data to send
	 * @param packetSize Size of the packet
	 * @param keyword The search keyword
	 * @param[out] searchId The assigned search ID
	 * @return true if search was started successfully
	 */
	static bool StartSearch(const uint8_t* packetData, uint32_t packetSize,
			       const wxString& keyword, uint32_t& searchId);

	/**
	 * Stops a Kad search.
	 * 
	 * @param searchId The search ID to stop
	 * @return true if search was stopped successfully
	 */
	static bool StopSearch(uint32_t searchId);

	/**
	 * Cleans up packet data created by CreateSearchPacket.
	 * 
	 * @param packetData Packet data to free
	 */
	static void FreeSearchPacket(uint8_t* packetData);
};

} // namespace search

#endif // KADSEARCHHELPER_H

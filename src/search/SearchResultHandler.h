
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

#ifndef SEARCHRESULTHANDLER_H
#define SEARCHRESULTHANDLER_H

#include <vector>
#include <cstdint>

// Forward declarations
class CSearchFile;

namespace search {

/**
 * SearchResultHandler - Interface for handling search results
 * 
 * This interface allows SearchList to forward search results
 * to the appropriate controller for processing.
 */
class SearchResultHandler
{
public:
    virtual ~SearchResultHandler() = default;

    /**
     * Handle a single search result
     * 
     * @param searchId The ID of the search
     * @param result The search result to handle
     */
    virtual void handleResult(uint32_t searchId, CSearchFile* result) = 0;

    /**
     * Handle multiple search results
     * 
     * @param searchId The ID of the search
     * @param results The search results to handle
     */
    virtual void handleResults(uint32_t searchId, const std::vector<CSearchFile*>& results) = 0;

    /**
     * Check if this handler is for a specific search
     * 
     * @param searchId The search ID to check
     * @return true if this handler handles the search
     */
    virtual bool handlesSearch(uint32_t searchId) const = 0;

    /**
     * Update the search ID (used during retry)
     * 
     * @param newSearchId The new search ID
     */
    virtual void updateSearchId(uint32_t newSearchId) = 0;
};

} // namespace search

#endif // SEARCHRESULTHANDLER_H

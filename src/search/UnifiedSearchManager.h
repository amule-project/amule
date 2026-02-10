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

#ifndef UNIFIED_SEARCH_MANAGER_H
#define UNIFIED_SEARCH_MANAGER_H

#include "SearchModel.h"
#include <wx/string.h>
#include <memory>
#include <cstdint>
#include <functional>

// Forward declarations
class CSearchFile;
namespace search {
class SearchController;
}

namespace search {

/**
 * UnifiedSearchManager - Common abstraction layer for all search types
 *
 * This class provides a unified interface for managing searches across
 * different networks (ED2K Local, ED2K Global, Kad). It ensures consistent
 * behavior and lifecycle management for all search types.
 *
 * Key design principles:
 * 1. Single source of truth for search state (SearchModel)
 * 2. Unified lifecycle management (start, stop, retry, completion)
 * 3. Consistent result handling and routing
 * 4. Type-specific initialization while maintaining common behavior
 *
 * Search Lifecycle:
 * 1. Initialize - Validate prerequisites and parameters
 * 2. Start - Execute search (network-specific)
 * 3. Progress - Receive results and update state
 * 4. Complete - Mark search as finished or retry
 */
class UnifiedSearchManager {
public:
    /**
     * Constructor
     */
    UnifiedSearchManager();

    /**
     * Destructor
     */
    ~UnifiedSearchManager();

    // Delete copy constructor and copy assignment operator
    UnifiedSearchManager(const UnifiedSearchManager&) = delete;
    UnifiedSearchManager& operator=(const UnifiedSearchManager&) = delete;

    /**
     * Start a new search
     *
     * @param params Search parameters
     * @param error Output parameter for error message
     * @return Search ID if successful, 0 otherwise
     */
    uint32_t startSearch(const SearchParams& params, wxString& error);

    /**
     * Stop an ongoing search
     *
     * @param searchId Search ID to stop
     */
    void stopSearch(uint32_t searchId);

    /**
     * Request more results for a search (global ED2K only)
     *
     * @param searchId Search ID
     * @param error Output parameter for error message
     * @return true if request was successful
     */
    bool requestMoreResults(uint32_t searchId, wxString& error);

    /**
     * Get search state
     *
     * @param searchId Search ID
     * @return Current search state
     */
    SearchState getSearchState(uint32_t searchId) const;

    /**
     * Get search results
     *
     * @param searchId Search ID
     * @return Vector of search results
     */
    std::vector<CSearchFile*> getResults(uint32_t searchId) const;

    /**
     * Get result count
     *
     * @param searchId Search ID
     * @return Number of results
     */
    size_t getResultCount(uint32_t searchId) const;

    /**
     * Check if a search is active
     *
     * @param searchId Search ID
     * @return true if search is active
     */
    bool isSearchActive(uint32_t searchId) const;

    /**
     * Handle search results (called from result router)
     *
     * @param searchId Search ID
     * @param results Vector of results
     */
    void handleResults(uint32_t searchId, const std::vector<CSearchFile*>& results);

    /**
     * Mark search as complete
     *
     * @param searchId Search ID
     * @param hasResults Whether search has results
     */
    void markSearchComplete(uint32_t searchId, bool hasResults);

    /**
     * Set search completion callback
     *
     * @param callback Function to call when search completes
     */
    void setSearchCompletedCallback(std::function<void(uint32_t, bool)> callback);

private:
    /**
     * Create appropriate search controller based on search type
     *
     * @param params Search parameters
     * @return Search controller instance
     */
    std::unique_ptr<SearchController> createSearchController(const SearchParams& params);

    /**
     * Validate search prerequisites
     *
     * @param params Search parameters
     * @param error Output parameter for error message
     * @return true if prerequisites are valid
     */
    bool validatePrerequisites(const SearchParams& params, wxString& error) const;

    /**
     * Cleanup completed search
     *
     * @param searchId Search ID
     */
    void cleanupSearch(uint32_t searchId);

    // Map of search ID to controller
    std::unordered_map<uint32_t, std::unique_ptr<SearchController>> m_controllers;

    // Search completion callback
    std::function<void(uint32_t, bool)> m_onSearchCompleted;

    // Mutex for thread safety
    mutable wxMutex m_mutex;
};

} // namespace search

#endif // UNIFIED_SEARCH_MANAGER_H

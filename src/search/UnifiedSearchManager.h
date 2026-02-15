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
#include "SearchTimeoutManager.h"
#include <wx/string.h>
#include <memory>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>
#include <list>

// Forward declarations
class CSearchFile;
class CMD4Hash;
class CMemFile;
class CUpDownClient;
class CTag;
typedef std::vector<CSearchFile*> CSearchResultList;
typedef std::list<CTag*> TagPtrList;
namespace Kademlia {
class CUInt128;
}
namespace search {
class SearchController;
class SearchControllerBase;
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
     * Get singleton instance
     */
    static UnifiedSearchManager& Instance();

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
     * Stop all active searches
     */
    void stopAllSearches();

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
     * Get shown result count (filtered)
     *
     * @param searchId Search ID
     * @return Number of shown results
     */
    size_t getShownResultCount(uint32_t searchId) const;

    /**
     * Get hidden result count (filtered)
     *
     * @param searchId Search ID
     * @return Number of hidden results
     */
    size_t getHiddenResultCount(uint32_t searchId) const;

    /**
     * Get result by index
     *
     * @param searchId Search ID
     * @param index Result index
     * @return Result pointer, or nullptr if not found
     */
    CSearchFile* getResultByIndex(uint32_t searchId, size_t index) const;

    /**
     * Get result by hash
     *
     * @param searchId Search ID
     * @param hash File hash
     * @return Result pointer, or nullptr if not found
     */
    CSearchFile* getResultByHash(uint32_t searchId, const CMD4Hash& hash) const;

    /**
     * Get search model for a search ID
     *
     * @param searchId Search ID
     * @return Search model pointer, or nullptr if not found
     */
    SearchModel* getSearchModel(uint32_t searchId) const;

    /**
     * Filter results for a search
     *
     * @param searchId Search ID
     * @param filter Filter string
     * @param invert Invert filter
     * @param knownOnly Show only known files
     */
    void filterResults(uint32_t searchId, const wxString& filter, bool invert, bool knownOnly);

    /**
     * Clear filters for a search
     *
     * @param searchId Search ID
     */
    void clearFilters(uint32_t searchId);

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

    /**
     * Get the timeout manager
     *
     * @return Reference to the timeout manager singleton
     */
    SearchTimeoutManager& getTimeoutManager() { return SearchTimeoutManager::Instance(); }

    /**
     * Get all active search IDs
     *
     * @return Vector of active search IDs
     */
    std::vector<uint32_t> getActiveSearchIds() const;

    /**
     * Get search progress (0-100)
     *
     * @param searchId Search ID
     * @return Progress percentage
     */
    uint32_t getSearchProgress(uint32_t searchId) const;

    /**
     * Remove results for a search
     *
     * @param searchId Search ID (0xffffffff for all)
     */
    void removeResults(uint32_t searchId);

    /**
     * Add a file to download by hash
     *
     * @param hash File hash
     * @param category Download category
     */
    void addFileToDownloadByHash(const CMD4Hash& hash, uint8_t category);

    /**
     * Set Kad search as finished
     */
    void setKadSearchFinished();

    /**
     * Get search results for a search ID
     *
     * @param searchId Search ID (0xffffffff for all results)
     * @return Reference to result list
     */
    const CSearchResultList& getSearchResults(uint32_t searchId) const;

    /**
     * Get a search file by parent ID
     *
     * @param parentId Parent file ID
     * @return Search file pointer, or nullptr if not found
     */
    CSearchFile* getSearchFileById(uint32_t parentId) const;

    /**
     * Update search file by hash (e.g., when download status changes)
     *
     * @param hash File hash to update
     */
    void updateSearchFileByHash(const CMD4Hash& hash);

    /**
     * Process a search answer from a server
     *
     * @param packet Packet data
     * @param size Packet size
     * @param optUTF8 Whether UTF8 is used
     * @param serverIP Server IP
     * @param serverPort Server port
     */
    void processSearchAnswer(const uint8_t* packet, uint32_t size, bool optUTF8,
                             uint32_t serverIP, uint16_t serverPort);

    /**
     * Process a UDP search answer
     *
     * @param packet Memory file containing the data
     * @param optUTF8 Whether UTF8 is used
     * @param serverIP Server IP
     * @param serverPort Server port
     */
    void processUDPSearchAnswer(const CMemFile& packet, bool optUTF8,
                                uint32_t serverIP, uint16_t serverPort);

    /**
     * Process a Kad search keyword result
     *
     * @param searchID Search ID
     * @param fileID File ID (Kad UInt128)
     * @param name File name
     * @param size File size
     * @param type File type
     * @param kadPublishInfo Publish info
     * @param taglist Tag list
     */
    void processKadSearchKeyword(uint32_t searchID, const Kademlia::CUInt128* fileID,
                                 const wxString& name, uint64_t size,
                                 const wxString& type, uint32_t kadPublishInfo,
                                 const TagPtrList& taglist);

    /**
     * End a local search
     */
    void localSearchEnd();

    /**
     * Process a shared file list (from another client)
     *
     * @param packet Packet data
     * @param size Packet size
     * @param sender Sending client
     * @param moreResultsAvailable Output parameter for more results flag
     * @param directory Directory
     */
    void processSharedFileList(const uint8_t* packet, uint32_t size,
                               CUpDownClient* sender, bool* moreResultsAvailable,
                               const wxString& directory);

    /**
     * Add a search result to the list
     *
     * @param result Search result to add
     * @param bClientResponse Whether this is a client response
     */
    void addToList(CSearchFile* result, bool bClientResponse = false);
    /**
     * Map Kad search ID to internal search ID
     *
     * @param kadSearchId Kad search ID
     * @param searchId Internal search ID
     */
    void mapKadSearchId(uint32_t kadSearchId, uint32_t searchId);

    /**
     * Remove Kad search ID mapping
     *
     * @param kadSearchId Kad search ID
     */
    void removeKadSearchIdMapping(uint32_t kadSearchId);

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

    // Timeout manager is now a singleton accessed via SearchTimeoutManager::Instance()

    // Mutex for thread safety
    mutable wxMutex m_mutex;
};

} // namespace search

#endif // UNIFIED_SEARCH_MANAGER_H

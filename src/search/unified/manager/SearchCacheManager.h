//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team
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

#ifndef SEARCH_CACHE_MANAGER_H
#define SEARCH_CACHE_MANAGER_H

#include "../core/SearchTypes.h"
#include "../core/SearchId.h"
#include "../core/SearchParams.h"
#include <unordered_map>
#include <string>
#include <chrono>
#include <vector>

namespace search {

/**
 * Cached search information
 */
struct CachedSearch {
    SearchId searchId;
    SearchParams params;
    SearchState state;
    std::chrono::system_clock::time_point timestamp;
    size_t resultCount;
    bool isActive;

    CachedSearch()
        : state(SearchState::IDLE)
        , timestamp(std::chrono::system_clock::now())
        , resultCount(0)
        , isActive(false)
    {}
};

/**
 * Search cache manager for reusing existing searches
 * Improves user experience by avoiding duplicate searches
 */
class SearchCacheManager {
public:
    /**
     * Constructor
     */
    SearchCacheManager();

    /**
     * Destructor
     */
    ~SearchCacheManager();

    /**
     * Check if a search with identical parameters already exists
     * @param params Search parameters to check
     * @param[out] existingSearchId ID of existing search if found
     * @return true if an identical search exists and is active
     */
    bool FindExistingSearch(const SearchParams& params, SearchId& existingSearchId);

    /**
     * Register a new search in the cache
     * @param searchId ID of the new search
     * @param params Search parameters
     */
    void RegisterSearch(SearchId searchId, const SearchParams& params);

    /**
     * Update search state and result count
     * @param searchId Search ID to update
     * @param state New search state
     * @param resultCount Current result count
     */
    void UpdateSearch(SearchId searchId, SearchState state, size_t resultCount);

    /**
     * Remove a search from the cache
     * @param searchId Search ID to remove
     */
    void RemoveSearch(SearchId searchId);

    /**
     * Clean up old inactive searches from cache
     * @param maxAge Maximum age in seconds before cleanup
     */
    void CleanupOldSearches(int maxAge = 3600);  // 1 hour default

    /**
     * Get all active searches
     * @return Vector of active cached searches
     */
    std::vector<CachedSearch> GetActiveSearches() const;

    /**
     * Get search information by ID
     * @param searchId Search ID to query
     * @return Cached search info, or nullptr if not found
     */
    const CachedSearch* GetSearchInfo(SearchId searchId) const;

    /**
     * Generate cache key from search parameters
     * @param params Search parameters
     * @return Cache key string
     */
    std::string GenerateCacheKey(const SearchParams& params) const;

    /**
     * Check if two search parameters are identical
     * @param params1 First search parameters
     * @param params2 Second search parameters
     * @return true if parameters are identical
     */
    bool AreParametersIdentical(const SearchParams& params1,
                                const SearchParams& params2) const;

private:
    // Cache storage
    std::unordered_map<std::string, CachedSearch> m_cache;
    std::unordered_map<SearchId, std::string> m_searchIdToKey;

    // Configuration
    struct Config {
        int maxCacheSize{100};
        int maxAgeSeconds{3600};  // 1 hour
        bool caseSensitive{false};
    } m_config;
};

} // namespace search

#endif // SEARCH_CACHE_MANAGER_H

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

#ifndef ISEARCH_ENGINE_H
#define ISEARCH_ENGINE_H

#include "../core/SearchTypes.h"
#include "../core/SearchId.h"
#include "../core/SearchParams.h"
#include "../core/SearchResult.h"
#include "../core/SearchCommand.h"
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace search {

/**
 * Abstract interface for all search engines (local, global, Kad)
 * All implementations must be thread-safe for single-threaded use
 * (no internal locking needed as all calls happen in search thread)
 */
class ISearchEngine {
public:
    virtual ~ISearchEngine() = default;

    /**
     * Start a new search with given parameters
     * @param params Search parameters
     * @return Search ID for tracking
     */
    virtual SearchId StartSearch(const SearchParams& params) = 0;

    /**
     * Stop an active search
     * @param searchId Search to stop
     */
    virtual void StopSearch(SearchId searchId) = 0;

    /**
     * Pause an active search (can be resumed)
     * @param searchId Search to pause
     */
    virtual void PauseSearch(SearchId searchId) = 0;

    /**
     * Resume a paused search
     * @param searchId Search to resume
     */
    virtual void ResumeSearch(SearchId searchId) = 0;

    /**
     * Request more results for an active search
     * @param searchId Search to request more results for
     */
    virtual void RequestMoreResults(SearchId searchId) = 0;

    /**
     * Get current state of a search
     * @param searchId Search to query
     * @return Current search state
     */
    virtual SearchState GetSearchState(SearchId searchId) const = 0;

    /**
     * Get search parameters
     * @param searchId Search to query
     * @return Search parameters
     */
    virtual SearchParams GetSearchParams(SearchId searchId) const = 0;

    /**
     * Get current results for a search
     * @param searchId Search to query
     * @param maxResults Maximum results to return (0 = all)
     * @return Vector of search results
     */
    virtual std::vector<SearchResult> GetResults(SearchId searchId, size_t maxResults = 0) const = 0;

    /**
     * Get result count for a search
     * @param searchId Search to query
     * @return Number of results
     */
    virtual size_t GetResultCount(SearchId searchId) const = 0;

    /**
     * Process a command (called by UnifiedSearchManager)
     * @param command Command to process
     */
    virtual void ProcessCommand(const SearchCommand& command) = 0;

    /**
     * Perform periodic maintenance (called by UnifiedSearchManager)
     * @param currentTime Current time
     */
    virtual void ProcessMaintenance(std::chrono::system_clock::time_point currentTime) = 0;

    /**
     * Shutdown the engine and cleanup resources
     */
    virtual void Shutdown() = 0;

    /**
     * Get the search type this engine handles
     * @return Search type
     */
    virtual SearchType GetSearchType() const = 0;

    /**
     * Get statistics for this engine
     */
    struct EngineStatistics {
        size_t totalSearches;
        size_t activeSearches;
        size_t completedSearches;
        size_t failedSearches;
        size_t totalResults;
        std::chrono::system_clock::time_point startTime;
    };

    virtual EngineStatistics GetStatistics() const = 0;
};

} // namespace search

#endif // ISEARCH_ENGINE_H

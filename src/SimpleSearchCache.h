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

#ifndef SIMPLE_SEARCH_CACHE_H
#define SIMPLE_SEARCH_CACHE_H

#include <wx/string.h>
#include <wx/thread.h>
#include <map>
#include <cstdint>

// Forward declarations
class CSearchList;

// Include SearchList.h to access CSearchParams
#include "SearchList.h"

/**
 * Cached search information for legacy SearchDlg
 */
struct LegacyCachedSearch {
    uint32_t searchId;
    CSearchList::CSearchParams params;
    bool isActive;
    wxLongLong timestamp;

    LegacyCachedSearch()
        : searchId(0)
        , isActive(false)
        , timestamp(wxGetLocalTimeMillis())
    {}
};

/**
 * Simple search cache manager for legacy SearchDlg
 * Handles duplicate search detection without requiring full UnifiedSearchManager
 */
class SimpleSearchCache {
public:
    /**
     * Constructor
     */
    SimpleSearchCache();

    /**
     * Destructor
     */
    ~SimpleSearchCache();

    /**
     * Check if a search with identical parameters already exists
     * @param searchType The search type (Local, Global, Kad)
     * @param searchString The search string/keyword
     * @param params The search parameters
     * @param[out] existingSearchId ID of existing search if found
     * @return true if an identical search exists and is active
     */
    bool FindExistingSearch(SearchType searchType, const wxString& searchString,
                           const CSearchList::CSearchParams& params, uint32_t& existingSearchId);

    /**
     * Register a new search in the cache
     * @param searchId ID of the new search
     * @param searchType The search type
     * @param searchString The search string
     * @param params The search parameters
     */
    void RegisterSearch(uint32_t searchId, SearchType searchType,
                       const wxString& searchString, const CSearchList::CSearchParams& params);

    /**
     * Update search state
     * @param searchId Search ID to update
     * @param isActive Whether the search is still active
     */
    void UpdateSearch(uint32_t searchId, bool isActive);

    /**
     * Remove a search from the cache
     * @param searchId Search ID to remove
     */
    void RemoveSearch(uint32_t searchId);

    /**
     * Clean up old inactive searches from cache
     * @param maxAgeSeconds Maximum age in seconds before cleanup
     */
    void CleanupOldSearches(int maxAgeSeconds = 3600);  // 1 hour default

    /**
     * Generate cache key from search parameters
     * @param searchType The search type
     * @param searchString The search string
     * @param params The search parameters
     * @return Cache key string
     */
    wxString GenerateCacheKey(SearchType searchType, const wxString& searchString,
                             const CSearchList::CSearchParams& params) const;

    /**
     * Check if two search parameters are identical
     * @param searchType1 First search type
     * @param searchString1 First search string
     * @param params1 First search parameters
     * @param searchType2 Second search type
     * @param searchString2 Second search string
     * @param params2 Second search parameters
     * @return true if parameters are identical
     */
    bool AreParametersIdentical(SearchType searchType1, const wxString& searchString1,
                                const CSearchList::CSearchParams& params1,
                                SearchType searchType2, const wxString& searchString2,
                                const CSearchList::CSearchParams& params2) const;

    /**
     * Enable or disable the cache
     * @param enabled true to enable, false to disable
     */
    void SetEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * Check if cache is enabled
     * @return true if enabled
     */
    bool IsEnabled() const { return m_enabled; }

private:
    // Cache storage
    std::map<wxString, LegacyCachedSearch> m_cache;
    std::map<uint32_t, wxString> m_searchIdToKey;

    // Mutex for thread-safe access
    mutable wxMutex m_mutex;

    // Configuration
    bool m_enabled;
    int m_maxAgeSeconds;
    bool m_caseSensitive;
};

#endif // SIMPLE_SEARCH_CACHE_H

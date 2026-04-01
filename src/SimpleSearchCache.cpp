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

#include "SimpleSearchCache.h"
#include <algorithm>

SimpleSearchCache::SimpleSearchCache()
    : m_enabled(true)
    , m_maxAgeSeconds(3600)  // 1 hour
    , m_caseSensitive(false)
{
}

SimpleSearchCache::~SimpleSearchCache()
{
}

bool SimpleSearchCache::FindExistingSearch(SearchType searchType, const wxString& searchString,
                                           const CSearchList::CSearchParams& params, uint32_t& existingSearchId)
{
    wxMutexLocker lock(m_mutex);

    if (!m_enabled) {
        return false;
    }

    wxString key = GenerateCacheKey(searchType, searchString, params);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        const LegacyCachedSearch& cached = it->second;
        existingSearchId = cached.searchId;
        return true;
    }

    return false;
}

void SimpleSearchCache::RegisterSearch(uint32_t searchId, SearchType searchType,
                                       const wxString& searchString, const CSearchList::CSearchParams& params)
{
    wxMutexLocker lock(m_mutex);

    if (!m_enabled) {
        return;
    }

    wxString key = GenerateCacheKey(searchType, searchString, params);

    LegacyCachedSearch cached;
    cached.searchId = searchId;
    cached.params = params;
    cached.isActive = true;
    cached.timestamp = wxGetLocalTimeMillis();

    m_cache[key] = cached;
    m_searchIdToKey[searchId] = key;
}

void SimpleSearchCache::UpdateSearch(uint32_t searchId, bool isActive)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_searchIdToKey.find(searchId);
    if (it != m_searchIdToKey.end()) {
        auto cacheIt = m_cache.find(it->second);
        if (cacheIt != m_cache.end()) {
            cacheIt->second.isActive = isActive;
        }
    }
}

void SimpleSearchCache::RemoveSearch(uint32_t searchId)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_searchIdToKey.find(searchId);
    if (it != m_searchIdToKey.end()) {
        m_cache.erase(it->second);
        m_searchIdToKey.erase(it);
    }
}

void SimpleSearchCache::CleanupOldSearches(int maxAgeSeconds)
{
    wxMutexLocker lock(m_mutex);

    wxLongLong now = wxGetLocalTimeMillis();
    wxLongLong maxAge = maxAgeSeconds * 1000;  // Convert to milliseconds

    std::vector<wxString> keysToRemove;

    for (const auto& [key, cached] : m_cache) {
        wxLongLong age = now - cached.timestamp;

        // Remove if:
        // 1. Older than maxAge
        // 2. Inactive
        if (age > maxAge || !cached.isActive) {
            keysToRemove.push_back(key);
        }
    }

    // Remove marked keys
    for (const auto& key : keysToRemove) {
        uint32_t searchId = m_cache[key].searchId;
        m_searchIdToKey.erase(searchId);
        m_cache.erase(key);

    }
}

wxString SimpleSearchCache::GenerateCacheKey(SearchType searchType, const wxString& searchString,
                                             const CSearchList::CSearchParams& params) const
{
    wxString key;

    // Include search type
    key << static_cast<int>(searchType) << wxT("|");

    // Include query (normalized)
    wxString query = searchString;
    if (!m_caseSensitive) {
        query.MakeLower();
    }
    key << query << wxT("|");

    // Include filters
    if (params.minSize > 0) {
        key << wxT("min:") << params.minSize << wxT("|");
    }
    if (params.maxSize > 0) {
        key << wxT("max:") << params.maxSize << wxT("|");
    }

    // Include file type
    if (!params.typeText.IsEmpty()) {
        wxString typeText = params.typeText;
        if (!m_caseSensitive) {
            typeText.MakeLower();
        }
        key << wxT("type:") << typeText << wxT("|");
    }

    return key;
}

bool SimpleSearchCache::AreParametersIdentical(SearchType searchType1, const wxString& searchString1,
                                                const CSearchList::CSearchParams& params1,
                                                SearchType searchType2, const wxString& searchString2,
                                                const CSearchList::CSearchParams& params2) const
{
    // Compare search type
    if (searchType1 != searchType2) {
        return false;
    }

    // Compare query
    wxString query1 = searchString1;
    wxString query2 = searchString2;

    if (!m_caseSensitive) {
        query1.MakeLower();
        query2.MakeLower();
    }

    if (query1 != query2) {
        return false;
    }

    // Compare min file size
    if (params1.minSize != params2.minSize) {
        return false;
    }

    // Compare max file size
    if (params1.maxSize != params2.maxSize) {
        return false;
    }

    // Compare file type
    wxString type1 = params1.typeText;
    wxString type2 = params2.typeText;

    if (!m_caseSensitive) {
        type1.MakeLower();
        type2.MakeLower();
    }

    if (type1 != type2) {
        return false;
    }

    return true;
}

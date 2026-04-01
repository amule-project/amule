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

#include "SearchCacheManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace search {

SearchCacheManager::SearchCacheManager()
{
    std::cout << "[SearchCacheManager] Initialized" << std::endl;
}

SearchCacheManager::~SearchCacheManager()
{
    std::cout << "[SearchCacheManager] Shutdown" << std::endl;
}

bool SearchCacheManager::FindExistingSearch(const SearchParams& params, SearchId& existingSearchId)
{
    // Generate cache key from parameters
    std::string key = GenerateCacheKey(params);

    // Check if search exists in cache
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        const CachedSearch& cached = it->second;

        // Check if search is still active (not completed/failed/cancelled)
        if (cached.isActive &&
            (cached.state == SearchState::RUNNING ||
             cached.state == SearchState::PAUSED)) {
            existingSearchId = cached.searchId;
            std::cout << "[SearchCacheManager] Found existing search: "
                      << existingSearchId.ToString()
                      << " for query: " << params.query << std::endl;
            return true;
        }
    }

    return false;
}

void SearchCacheManager::RegisterSearch(SearchId searchId, const SearchParams& params)
{
    std::string key = GenerateCacheKey(params);

    CachedSearch cached;
    cached.searchId = searchId;
    cached.params = params;
    cached.state = SearchState::STARTING;
    cached.timestamp = std::chrono::system_clock::now();
    cached.resultCount = 0;
    cached.isActive = true;

    m_cache[key] = cached;
    m_searchIdToKey[searchId] = key;

    std::cout << "[SearchCacheManager] Registered search: " << searchId.ToString()
              << " with key: " << key << std::endl;

    // Cleanup old searches if cache is full
    if (m_cache.size() > static_cast<size_t>(m_config.maxCacheSize)) {
        CleanupOldSearches();
    }
}

void SearchCacheManager::UpdateSearch(SearchId searchId, SearchState state, size_t resultCount)
{
    auto it = m_searchIdToKey.find(searchId);
    if (it != m_searchIdToKey.end()) {
        auto cacheIt = m_cache.find(it->second);
        if (cacheIt != m_cache.end()) {
            cacheIt->second.state = state;
            cacheIt->second.resultCount = resultCount;

            // Mark as inactive if search is completed/failed/cancelled
            if (state == SearchState::COMPLETED ||
                state == SearchState::FAILED ||
                state == SearchState::CANCELLED) {
                cacheIt->second.isActive = false;
            }
        }
    }
}

void SearchCacheManager::RemoveSearch(SearchId searchId)
{
    auto it = m_searchIdToKey.find(searchId);
    if (it != m_searchIdToKey.end()) {
        m_cache.erase(it->second);
        m_searchIdToKey.erase(it);

        std::cout << "[SearchCacheManager] Removed search: " << searchId.ToString() << std::endl;
    }
}

void SearchCacheManager::CleanupOldSearches(int maxAge)
{
    auto now = std::chrono::system_clock::now();
    std::vector<std::string> keysToRemove;

    for (const auto& [key, cached] : m_cache) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - cached.timestamp).count();

        // Remove if:
        // 1. Older than maxAge
        // 2. Inactive (completed/failed/cancelled)
        if (age > maxAge || !cached.isActive) {
            keysToRemove.push_back(key);
        }
    }

    // Remove marked keys
    for (const auto& key : keysToRemove) {
        SearchId searchId = m_cache[key].searchId;
        m_searchIdToKey.erase(searchId);
        m_cache.erase(key);

        std::cout << "[SearchCacheManager] Cleaned up search: " << searchId.ToString() << std::endl;
    }

    if (!keysToRemove.empty()) {
        std::cout << "[SearchCacheManager] Cleaned up " << keysToRemove.size()
                  << " old searches" << std::endl;
    }
}

std::vector<CachedSearch> SearchCacheManager::GetActiveSearches() const
{
    std::vector<CachedSearch> activeSearches;

    for (const auto& [key, cached] : m_cache) {
        if (cached.isActive) {
            activeSearches.push_back(cached);
        }
    }

    return activeSearches;
}

const CachedSearch* SearchCacheManager::GetSearchInfo(SearchId searchId) const
{
    auto it = m_searchIdToKey.find(searchId);
    if (it != m_searchIdToKey.end()) {
        auto cacheIt = m_cache.find(it->second);
        if (cacheIt != m_cache.end()) {
            return &cacheIt->second;
        }
    }
    return nullptr;
}

std::string SearchCacheManager::GenerateCacheKey(const SearchParams& params) const
{
    std::ostringstream key;

    // Include search type
    key << static_cast<int>(params.type) << "|";

    // Include query (normalized)
    std::string query = params.query;
    if (!m_config.caseSensitive) {
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);
    }
    key << query << "|";

    // Include filters
    if (params.minFileSize) {
        key << "min:" << params.minFileSize.value() << "|";
    }
    if (params.maxFileSize) {
        key << "max:" << params.maxFileSize.value() << "|";
    }

    // Include file types
    if (!params.fileTypes.empty()) {
        key << "types:";
        for (size_t i = 0; i < params.fileTypes.size(); ++i) {
            if (i > 0) key << ",";
            key << params.fileTypes[i];
        }
        key << "|";
    }

    return key.str();
}

bool SearchCacheManager::AreParametersIdentical(const SearchParams& params1,
                                                const SearchParams& params2) const
{
    // Compare search type
    if (params1.type != params2.type) {
        return false;
    }

    // Compare query
    std::string query1 = params1.query;
    std::string query2 = params2.query;

    if (!m_config.caseSensitive) {
        std::transform(query1.begin(), query1.end(), query1.begin(), ::tolower);
        std::transform(query2.begin(), query2.end(), query2.begin(), ::tolower);
    }

    if (query1 != query2) {
        return false;
    }

    // Compare min file size
    if (params1.minFileSize != params2.minFileSize) {
        return false;
    }

    // Compare max file size
    if (params1.maxFileSize != params2.maxFileSize) {
        return false;
    }

    // Compare file types (order-independent)
    if (params1.fileTypes.size() != params2.fileTypes.size()) {
        return false;
    }

    std::vector<std::string> types1 = params1.fileTypes;
    std::vector<std::string> types2 = params2.fileTypes;

    if (!m_config.caseSensitive) {
        for (auto& type : types1) {
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        }
        for (auto& type : types2) {
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
        }
    }

    std::sort(types1.begin(), types1.end());
    std::sort(types2.begin(), types2.end());

    if (types1 != types2) {
        return false;
    }

    return true;
}

} // namespace search

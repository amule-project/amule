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

#include "GlobalSearchEngine.h"

#include <iostream>

namespace search {

GlobalSearchEngine::GlobalSearchEngine()
{
    m_statistics.startTime = std::chrono::system_clock::now();
    std::cout << "[GlobalSearchEngine] Initialized (stub)" << std::endl;
}

GlobalSearchEngine::~GlobalSearchEngine()
{
    Shutdown();
}

SearchId GlobalSearchEngine::StartSearch(const SearchParams& params)
{
    SearchId searchId = SearchId::Generate();

    std::cout << "[GlobalSearchEngine] Starting search " << searchId.ToString()
              << " for query: " << params.query << " (stub implementation)" << std::endl;

    // Create search data
    SearchData search;
    search.params = params;
    search.state = SearchState::RUNNING;
    search.startTime = std::chrono::system_clock::now();
    search.lastUpdateTime = search.startTime;

    // Store search data
    m_searches[searchId] = search;

    // Update statistics
    m_statistics.totalSearches++;
    m_statistics.activeSearches++;

    // TODO: Implement actual server communication
    // For now, mark as completed immediately
    search.state = SearchState::COMPLETED;
    m_statistics.completedSearches++;
    m_statistics.activeSearches--;

    return searchId;
}

void GlobalSearchEngine::StopSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        it->second.state = SearchState::CANCELLED;
        m_statistics.activeSearches--;
        std::cout << "[GlobalSearchEngine] Stopped search " << searchId.ToString() << std::endl;
    }
}

void GlobalSearchEngine::PauseSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end() && it->second.state == SearchState::RUNNING) {
        it->second.state = SearchState::PAUSED;
        std::cout << "[GlobalSearchEngine] Paused search " << searchId.ToString() << std::endl;
    }
}

void GlobalSearchEngine::ResumeSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end() && it->second.state == SearchState::PAUSED) {
        it->second.state = SearchState::RUNNING;
        std::cout << "[GlobalSearchEngine] Resumed search " << searchId.ToString() << std::endl;
    }
}

void GlobalSearchEngine::RequestMoreResults(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        std::cout << "[GlobalSearchEngine] Request more results for search "
                  << searchId.ToString() << " (stub)" << std::endl;
    }
}

SearchState GlobalSearchEngine::GetSearchState(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        return it->second.state;
    }
    return SearchState::IDLE;
}

SearchParams GlobalSearchEngine::GetSearchParams(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        return it->second.params;
    }
    return SearchParams{};
}

std::vector<SearchResult> GlobalSearchEngine::GetResults(SearchId searchId, size_t maxResults) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        if (maxResults == 0 || maxResults >= it->second.results.size()) {
            return it->second.results;
        } else {
            return std::vector<SearchResult>(
                it->second.results.begin(),
                it->second.results.begin() + maxResults
            );
        }
    }
    return {};
}

size_t GlobalSearchEngine::GetResultCount(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it != m_searches.end()) {
        return it->second.results.size();
    }
    return 0;
}

void GlobalSearchEngine::ProcessCommand(const SearchCommand& command)
{
    // Global search engine doesn't need special command processing
}

void GlobalSearchEngine::ProcessMaintenance(std::chrono::system_clock::time_point currentTime)
{
    // Clean up old searches (older than 1 hour)
    auto cutoff = currentTime - std::chrono::hours(1);
    for (auto it = m_searches.begin(); it != m_searches.end(); ) {
        if (it->second.lastUpdateTime < cutoff &&
            (it->second.state == SearchState::COMPLETED ||
             it->second.state == SearchState::FAILED ||
             it->second.state == SearchState::CANCELLED)) {
            it = m_searches.erase(it);
        } else {
            ++it;
        }
    }
}

void GlobalSearchEngine::Shutdown()
{
    std::cout << "[GlobalSearchEngine] Shutting down" << std::endl;
    m_searches.clear();
}

GlobalSearchEngine::EngineStatistics GlobalSearchEngine::GetStatistics() const
{
    return m_statistics;
}

} // namespace search

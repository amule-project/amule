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

#ifndef GLOBAL_SEARCH_ENGINE_H
#define GLOBAL_SEARCH_ENGINE_H

#include "../ISearchEngine.h"
#include "../../core/SearchTypes.h"
#include "../../core/SearchId.h"
#include "../../core/SearchParams.h"
#include "../../core/SearchResult.h"
#include "../../core/SearchCommand.h"

#include <unordered_map>
#include <vector>
#include <chrono>

namespace search {

/**
 * Global search engine implementation (stub)
 * Searches through ED2K servers
 * TODO: Implement full server communication
 */
class GlobalSearchEngine : public ISearchEngine {
public:
    GlobalSearchEngine();
    virtual ~GlobalSearchEngine();

    // ISearchEngine implementation
    SearchId StartSearch(const SearchParams& params) override;
    void StopSearch(SearchId searchId) override;
    void PauseSearch(SearchId searchId) override;
    void ResumeSearch(SearchId searchId) override;
    void RequestMoreResults(SearchId searchId) override;
    SearchState GetSearchState(SearchId searchId) const override;
    SearchParams GetSearchParams(SearchId searchId) const override;
    std::vector<SearchResult> GetResults(SearchId searchId, size_t maxResults = 0) const override;
    size_t GetResultCount(SearchId searchId) const override;
    void ProcessCommand(const SearchCommand& command) override;
    void ProcessMaintenance(std::chrono::system_clock::time_point currentTime) override;
    void Shutdown() override;
    SearchType GetSearchType() const override { return SearchType::GLOBAL; }
    EngineStatistics GetStatistics() const override;

 private:
    // Search state (only accessed in search thread)
    struct SearchData {
        SearchParams params;
        SearchState state;
        std::vector<SearchResult> results;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point lastUpdateTime;

        SearchData()
            : state(SearchState::IDLE)
            , startTime(std::chrono::system_clock::now())
            , lastUpdateTime(std::chrono::system_clock::now())
        {}
    };

    // Search data storage
    std::unordered_map<SearchId, SearchData> m_searches;

    // Statistics
    EngineStatistics m_statistics;
};

} // namespace search

#endif // GLOBAL_SEARCH_ENGINE_H

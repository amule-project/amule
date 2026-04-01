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
#include <memory>
#include <queue>
#include <string>

namespace search {

/**
 * Server information for global search
 */
struct ServerInfo {
    uint32_t ip;
    uint16_t port;
    std::string name;
    uint32_t userCount;
    uint32_t fileCount;
    bool isConnected;
    bool isPreferred;
    std::chrono::system_clock::time_point lastUsed;
    uint32_t failedAttempts;

    ServerInfo()
        : ip(0)
        , port(0)
        , userCount(0)
        , fileCount(0)
        , isConnected(false)
        , isPreferred(false)
        , lastUsed(std::chrono::system_clock::now())
        , failedAttempts(0)
    {}
};

/**
 * Search request sent to a server
 */
struct ServerSearchRequest {
    SearchId searchId;
    uint32_t serverIp;
    uint16_t serverPort;
    std::vector<uint8_t> requestData;
    std::chrono::system_clock::time_point sentTime;
    int retryCount;
    bool completed;

    ServerSearchRequest()
        : serverIp(0)
        , serverPort(0)
        , sentTime(std::chrono::system_clock::now())
        , retryCount(0)
        , completed(false)
    {}
};

/**
 * Global search engine implementation
 * Searches through ED2K servers with result aggregation
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

    // Server management
    void AddServer(const ServerInfo& server);
    void RemoveServer(uint32_t ip, uint16_t port);
    void UpdateServerStatus(uint32_t ip, uint16_t port, uint32_t userCount, uint32_t fileCount);
    std::vector<ServerInfo> GetServers() const;
    bool IsServerConnected(uint32_t ip, uint16_t port) const;

    // Result aggregation
    void HandleSearchResult(SearchId searchId, const SearchResult& result);
    void SetOnSearchResult(std::function<void(SearchId, const SearchResult&)> callback);

private:
    // Search state (only accessed in search thread)
    struct SearchData {
        SearchParams params;
        SearchState state;
        std::vector<SearchResult> results;
        std::unordered_map<std::string, SearchResult> resultDedup;  // For duplicate detection
        std::vector<ServerSearchRequest> activeRequests;
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point lastUpdateTime;
        uint32_t serversQueried;
        uint32_t serversResponded;

        SearchData()
            : state(SearchState::IDLE)
            , startTime(std::chrono::system_clock::now())
            , lastUpdateTime(std::chrono::system_clock::now())
            , serversQueried(0)
            , serversResponded(0)
        {}
    };

    // Configuration
    struct Config {
        std::chrono::milliseconds requestTimeout{30000};  // 30 seconds
        int maxRetries{3};
        size_t maxResultsPerSearch{500};
        size_t maxServersPerSearch{10};
        bool enableResultDeduplication{true};
        bool enableServerPrioritization{true};
    };

    // Helper methods
    std::vector<uint8_t> BuildSearchRequest(const SearchParams& params);
    void SendSearchToServers(SearchId searchId, const SearchParams& params);
    ServerInfo* FindServer(uint32_t ip, uint16_t port);
    void SelectServersForSearch(SearchId searchId, size_t maxServers);
    bool IsDuplicateResult(const std::string& fileHash, SearchId searchId) const;
    void CleanupOldSearches(std::chrono::system_clock::time_point currentTime);
    void RetryFailedRequests(SearchId searchId);

    // Result callback
    std::function<void(SearchId, const SearchResult&)> m_onSearchResult;

    // Search data storage
    std::unordered_map<SearchId, SearchData> m_searches;

    // Server list
    std::unordered_map<uint64_t, ServerInfo> m_servers;  // Key: (ip << 16) | port

    // Statistics
    EngineStatistics m_statistics;

    // Configuration
    Config m_config;

    // Shutdown flag
    bool m_shutdown;
};

} // namespace search

#endif // GLOBAL_SEARCH_ENGINE_H

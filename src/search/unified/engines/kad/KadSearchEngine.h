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

#ifndef KAD_SEARCH_ENGINE_H
#define KAD_SEARCH_ENGINE_H

#include "../ISearchEngine.h"
#include "../../core/SearchTypes.h"
#include "../../core/SearchId.h"
#include "../../core/SearchParams.h"
#include "../../core/SearchResult.h"
#include "../../core/SearchCommand.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <string>
#include <queue>

namespace search {

/**
 * Kademlia contact information
 */
struct KadContact {
    std::string nodeId;  // 128-bit node ID as hex string
    uint32_t ip;
    uint16_t port;
    uint32_t lastSeen;
    uint32_t failedRequests;
    bool isResponsive;

    KadContact()
        : ip(0)
        , port(0)
        , lastSeen(0)
        , failedRequests(0)
        , isResponsive(false)
    {}
};

/**
 * Kademlia search request sent to a node
 */
struct KadSearchRequest {
    SearchId searchId;
    std::string targetNodeId;  // Target node ID
    std::string contactNodeId;  // Contact node ID
    uint32_t contactIp;
    uint16_t contactPort;
    std::vector<uint8_t> requestData;
    std::chrono::system_clock::time_point sentTime;
    int retryCount;
    bool completed;

    KadSearchRequest()
        : contactIp(0)
        , contactPort(0)
        , sentTime(std::chrono::system_clock::now())
        , retryCount(0)
        , completed(false)
    {}
};

/**
 * Kad search engine implementation
 * Searches through Kademlia DHT network with keyword-based lookup
 */
class KadSearchEngine : public ISearchEngine {
public:
    KadSearchEngine();
    virtual ~KadSearchEngine();

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
    SearchType GetSearchType() const override { return SearchType::KADEMLIA; }
    EngineStatistics GetStatistics() const override;

    // Kademlia-specific methods
    void AddContact(const KadContact& contact);
    void RemoveContact(const std::string& nodeId);
    void UpdateContactStatus(const std::string& nodeId, bool responsive);
    std::vector<KadContact> GetContacts() const;
    bool IsKadConnected() const;

    // Result handling
    void HandleKadResponse(SearchId searchId, const std::string& nodeId,
                          const std::vector<SearchResult>& results);
    void HandleKadResult(SearchId searchId, const SearchResult& result);
    void SetOnKadResult(std::function<void(SearchId, const SearchResult&)> callback);

private:
    // Search state (only accessed in search thread)
    struct SearchData {
        SearchParams params;
        SearchState state;
        std::vector<SearchResult> results;
        std::unordered_map<std::string, SearchResult> resultDedup;  // For duplicate detection
        std::vector<KadSearchRequest> activeRequests;
        std::unordered_set<std::string> contactedNodes;  // Nodes we've already queried
        std::unordered_set<std::string> respondedNodes;   // Nodes that responded
        std::queue<KadContact> pendingContacts;  // Contacts to query
        std::chrono::system_clock::time_point startTime;
        std::chrono::system_clock::time_point lastUpdateTime;
        std::chrono::system_clock::time_point lastJumpStart;
        uint32_t totalRequests;
        uint32_t totalResponses;
        uint32_t jumpStartCount;
        bool jumpStarted;

        SearchData()
            : state(SearchState::IDLE)
            , startTime(std::chrono::system_clock::now())
            , lastUpdateTime(std::chrono::system_clock::now())
            , lastJumpStart(std::chrono::system_clock::now())
            , totalRequests(0)
            , totalResponses(0)
            , jumpStartCount(0)
            , jumpStarted(false)
        {}
    };

    // Configuration
    struct Config {
        std::chrono::milliseconds requestTimeout{30000};  // 30 seconds
        int maxRetries{3};
        size_t maxResultsPerSearch{500};
        size_t maxConcurrentRequests{10};
        size_t maxContactsPerSearch{50};
        std::chrono::milliseconds jumpStartInterval{5000};  // 5 seconds
        int maxJumpStarts{5};
        bool enableResultDeduplication{true};
        bool enableKeywordHashing{true};
    };

    // Helper methods
    std::string ComputeKeywordHash(const std::string& keyword);
    std::vector<std::string> ExtractKeywords(const std::string& query);
    std::vector<uint8_t> BuildKadSearchPacket(const SearchParams& params,
                                              const std::string& keywordHash);
    void SendSearchToNodes(SearchId searchId, const SearchParams& params);
    void SelectNodesForSearch(SearchId searchId, size_t maxNodes);
    void JumpStart(SearchId searchId);
    bool IsDuplicateResult(const std::string& fileHash, SearchId searchId) const;
    void CleanupOldSearches(std::chrono::system_clock::time_point currentTime);
    void RetryFailedRequests(SearchId searchId);

    // Contact management
    KadContact* FindContact(const std::string& nodeId);
    void SortContactsByDistance(const std::string& targetNodeId,
                                std::vector<KadContact>& contacts);

    // Utility
    uint32_t ComputeXorDistance(const std::string& id1, const std::string& id2);

    // Result callback
    std::function<void(SearchId, const SearchResult&)> m_onKadResult;

    // Search data storage
    std::unordered_map<SearchId, SearchData> m_searches;

    // Contact list (Kademlia routing table)
    std::unordered_map<std::string, KadContact> m_contacts;  // Key: node ID

    // Statistics
    EngineStatistics m_statistics;

    // Configuration
    Config m_config;

    // Shutdown flag
    bool m_shutdown;

    // Kad connection status
    bool m_kadConnected;
};

} // namespace search

#endif // KAD_SEARCH_ENGINE_H

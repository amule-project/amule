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
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

namespace search {

GlobalSearchEngine::GlobalSearchEngine()
    : m_shutdown(false)
{
    m_statistics.totalSearches = 0;
    m_statistics.activeSearches = 0;
    m_statistics.completedSearches = 0;
    m_statistics.failedSearches = 0;
    m_statistics.totalResults = 0;
    m_statistics.averageSearchTime = 0.0;

    std::cout << "[GlobalSearchEngine] Initialized" << std::endl;
}

GlobalSearchEngine::~GlobalSearchEngine()
{
    Shutdown();
}

SearchId GlobalSearchEngine::StartSearch(const SearchParams& params)
{
    if (m_shutdown) {
        std::cerr << "[GlobalSearchEngine] Engine is shutting down" << std::endl;
        return SearchId::Invalid();
    }

    if (!params.IsValid()) {
        std::cerr << "[GlobalSearchEngine] Invalid search parameters" << std::endl;
        return SearchId::Invalid();
    }

    if (params.type != SearchType::GLOBAL) {
        std::cerr << "[GlobalSearchEngine] Wrong search type for global engine" << std::endl;
        return SearchId::Invalid();
    }

    SearchId searchId = SearchId::Generate();
    SearchData data;
    data.params = params;
    data.state = SearchState::STARTING;

    m_searches[searchId] = std::move(data);
    m_statistics.totalSearches++;
    m_statistics.activeSearches++;

    std::cout << "[GlobalSearchEngine] Starting global search " << searchId.ToString()
              << " for query: " << params.query << std::endl;

    // Send search to servers
    SendSearchToServers(searchId, params);

    // Update state
    m_searches[searchId].state = SearchState::RUNNING;
    m_searches[searchId].startTime = std::chrono::system_clock::now();

    return searchId;
}

void GlobalSearchEngine::StopSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[GlobalSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    std::cout << "[GlobalSearchEngine] Stopping search " << searchId.ToString() << std::endl;

    // Cancel all active requests
    for (auto& request : it->second.activeRequests) {
        request.completed = true;
    }

    // Update state
    it->second.state = SearchState::CANCELLED;
    m_statistics.activeSearches--;
    m_statistics.completedSearches++;

    // Calculate search time
    auto endTime = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - it->second.startTime).count();
    UpdateAverageSearchTime(duration);
}

void GlobalSearchEngine::PauseSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[GlobalSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    if (it->second.state == SearchState::RUNNING) {
        std::cout << "[GlobalSearchEngine] Pausing search " << searchId.ToString() << std::endl;
        it->second.state = SearchState::PAUSED;
    }
}

void GlobalSearchEngine::ResumeSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[GlobalSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    if (it->second.state == SearchState::PAUSED) {
        std::cout << "[GlobalSearchEngine] Resuming search " << searchId.ToString() << std::endl;
        it->second.state = SearchState::RUNNING;

        // Re-send search to servers
        SendSearchToServers(searchId, it->second.params);
    }
}

void GlobalSearchEngine::RequestMoreResults(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[GlobalSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    std::cout << "[GlobalSearchEngine] Requesting more results for search "
              << searchId.ToString() << std::endl;

    // Send OP_QUERY_MORE_RESULT to connected servers
    // In a full implementation, this would send the "more results" packet
    // to all servers that were queried for this search

    // For now, we'll simulate by querying additional servers
    if (it->second.state == SearchState::RUNNING ||
        it->second.state == SearchState::COMPLETED) {
        it->second.state = SearchState::RUNNING;
        SelectServersForSearch(searchId, m_config.maxServersPerSearch);
    }
}

SearchState GlobalSearchEngine::GetSearchState(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return SearchState::IDLE;
    }
    return it->second.state;
}

SearchParams GlobalSearchEngine::GetSearchParams(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return SearchParams();
    }
    return it->second.params;
}

std::vector<SearchResult> GlobalSearchEngine::GetResults(SearchId searchId, size_t maxResults) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return {};
    }

    const auto& results = it->second.results;
    if (maxResults == 0 || maxResults >= results.size()) {
        return results;
    }

    return std::vector<SearchResult>(results.begin(), results.begin() + maxResults);
}

size_t GlobalSearchEngine::GetResultCount(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return 0;
    }
    return it->second.results.size();
}

void GlobalSearchEngine::ProcessCommand(const SearchCommand& command)
{
    // Commands are processed through the public methods
    switch (command.type) {
        case SearchCommand::Type::STOP_SEARCH:
            StopSearch(command.searchId);
            break;

        case SearchCommand::Type::PAUSE_SEARCH:
            PauseSearch(command.searchId);
            break;

        case SearchCommand::Type::RESUME_SEARCH:
            ResumeSearch(command.searchId);
            break;

        case SearchCommand::Type::REQUEST_MORE_RESULTS:
            RequestMoreResults(command.searchId);
            break;

        default:
            std::cerr << "[GlobalSearchEngine] Unknown command type: "
                      << static_cast<int>(command.type) << std::endl;
            break;
    }
}

void GlobalSearchEngine::ProcessMaintenance(std::chrono::system_clock::time_point currentTime)
{
    if (m_shutdown) {
        return;
    }

    // Check for timed out searches
    for (auto& [searchId, data] : m_searches) {
        if (data.state == SearchState::RUNNING) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - data.startTime).count();

            if (elapsed > data.params.timeout.count()) {
                std::cout << "[GlobalSearchEngine] Search " << searchId.ToString()
                          << " timed out after " << elapsed << "ms" << std::endl;

                data.state = SearchState::COMPLETED;
                m_statistics.activeSearches--;
                m_statistics.completedSearches++;
                UpdateAverageSearchTime(elapsed);
            }
        }

        // Retry failed requests
        RetryFailedRequests(searchId);
    }

    // Cleanup old completed searches
    CleanupOldSearches(currentTime);
}

void GlobalSearchEngine::Shutdown()
{
    if (m_shutdown) {
        return;
    }

    std::cout << "[GlobalSearchEngine] Shutting down..." << std::endl;
    m_shutdown = true;

    // Stop all active searches
    for (auto& [searchId, data] : m_searches) {
        if (data.state == SearchState::RUNNING ||
            data.state == SearchState::STARTING ||
            data.state == SearchState::PAUSED) {
            data.state = SearchState::CANCELLED;
        }
    }

    m_statistics.activeSearches = 0;
    std::cout << "[GlobalSearchEngine] Shutdown complete" << std::endl;
}

EngineStatistics GlobalSearchEngine::GetStatistics() const
{
    return m_statistics;
}

void GlobalSearchEngine::AddServer(const ServerInfo& server)
{
    uint64_t key = (static_cast<uint64_t>(server.ip) << 16) | server.port;
    m_servers[key] = server;

    std::cout << "[GlobalSearchEngine] Added server " << server.name
              << " (" << (server.ip >> 24) << "."
              << ((server.ip >> 16) & 0xFF) << "."
              << ((server.ip >> 8) & 0xFF) << "."
              << (server.ip & 0xFF) << ":" << server.port << ")" << std::endl;
}

void GlobalSearchEngine::RemoveServer(uint32_t ip, uint16_t port)
{
    uint64_t key = (static_cast<uint64_t>(ip) << 16) | port;
    m_servers.erase(key);

    std::cout << "[GlobalSearchEngine] Removed server " << (ip >> 24) << "."
              << ((ip >> 16) & 0xFF) << "."
              << ((ip >> 8) & 0xFF) << "."
              << (ip & 0xFF) << ":" << port << std::endl;
}

void GlobalSearchEngine::UpdateServerStatus(uint32_t ip, uint16_t port,
                                            uint32_t userCount, uint32_t fileCount)
{
    auto it = m_servers.find((static_cast<uint64_t>(ip) << 16) | port);
    if (it != m_servers.end()) {
        it->second.userCount = userCount;
        it->second.fileCount = fileCount;
        it->second.isConnected = true;
        it->second.lastUsed = std::chrono::system_clock::now();
    }
}

std::vector<ServerInfo> GlobalSearchEngine::GetServers() const
{
    std::vector<ServerInfo> servers;
    servers.reserve(m_servers.size());

    for (const auto& [key, server] : m_servers) {
        servers.push_back(server);
    }

    // Sort by preference and user count
    std::sort(servers.begin(), servers.end(),
        [](const ServerInfo& a, const ServerInfo& b) {
            if (a.isPreferred != b.isPreferred) {
                return a.isPreferred > b.isPreferred;
            }
            return a.userCount > b.userCount;
        });

    return servers;
}

bool GlobalSearchEngine::IsServerConnected(uint32_t ip, uint16_t port) const
{
    auto it = m_servers.find((static_cast<uint64_t>(ip) << 16) | port);
    return it != m_servers.end() && it->second.isConnected;
}

void GlobalSearchEngine::HandleSearchResult(SearchId searchId, const SearchResult& result)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[GlobalSearchEngine] Search not found for result: "
                  << searchId.ToString() << std::endl;
        return;
    }

    // Check for duplicate results
    if (m_config.enableResultDeduplication &&
        IsDuplicateResult(result.fileHash, searchId)) {
        std::cout << "[GlobalSearchEngine] Duplicate result filtered: "
                  << result.fileName << std::endl;
        return;
    }

    // Apply filters
    if (result.fileSize < it->second.params.minFileSize.value_or(0) ||
        result.fileSize > it->second.params.maxFileSize.value_or(UINT64_MAX)) {
        return;
    }

    if (!it->second.params.fileTypes.empty()) {
        std::string ext = result.fileName.substr(result.fileName.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        bool matches = false;
        for (const auto& type : it->second.params.fileTypes) {
            if (ext == type) {
                matches = true;
                break;
            }
        }
        if (!matches) {
            return;
        }
    }

    // Add result
    it->second.results.push_back(result);
    it->second.resultDedup[result.fileHash] = result;
    it->second.lastUpdateTime = std::chrono::system_clock::now();
    m_statistics.totalResults++;

    // Check if we've reached max results
    if (it->second.results.size() >= m_config.maxResultsPerSearch) {
        std::cout << "[GlobalSearchEngine] Search " << searchId.ToString()
                  << " reached max results (" << m_config.maxResultsPerSearch << ")" << std::endl;
        it->second.state = SearchState::COMPLETED;
        m_statistics.activeSearches--;
        m_statistics.completedSearches++;
    }

    // Notify callback
    if (m_onSearchResult) {
        m_onSearchResult(searchId, result);
    }

    std::cout << "[GlobalSearchEngine] Result received for search " << searchId.ToString()
              << ": " << result.fileName << " (" << result.fileSize << " bytes)" << std::endl;
}

void GlobalSearchEngine::SetOnSearchResult(std::function<void(SearchId, const SearchResult&)> callback)
{
    m_onSearchResult = std::move(callback);
}

std::vector<uint8_t> GlobalSearchEngine::BuildSearchRequest(const SearchParams& params)
{
    std::vector<uint8_t> request;

    // In a full implementation, this would build the ED2K search packet
    // Format: OP_SEARCHREQUEST (0x16) <Query_Tree>

    // For now, we'll create a simplified version
    // The actual ED2K protocol uses a complex tag-based query format

    // Placeholder: Create a simple binary representation
    // This would be replaced with actual ED2K packet construction

    std::string query = params.query;
    request.insert(request.end(), query.begin(), query.end());

    return request;
}

void GlobalSearchEngine::SendSearchToServers(SearchId searchId, const SearchParams& params)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return;
    }

    SelectServersForSearch(searchId, m_config.maxServersPerSearch);

    // Build search request
    std::vector<uint8_t> requestData = BuildSearchRequest(params);

    // Send to selected servers
    for (auto& request : it->second.activeRequests) {
        if (!request.completed) {
            request.requestData = requestData;
            request.sentTime = std::chrono::system_clock::now();

            // In a full implementation, this would send the packet via the network layer
            // For now, we'll simulate by generating some demo results

            std::cout << "[GlobalSearchEngine] Sent search request to server "
                      << (request.serverIp >> 24) << "."
                      << ((request.serverIp >> 16) & 0xFF) << "."
                      << ((request.serverIp >> 8) & 0xFF) << "."
                      << (request.serverIp & 0xFF) << ":" << request.serverPort << std::endl;

            it->second.serversQueried++;
        }
    }

    // Mark as completed if no servers available
    if (it->second.activeRequests.empty()) {
        std::cout << "[GlobalSearchEngine] No servers available for search "
                  << searchId.ToString() << std::endl;
        it->second.state = SearchState::FAILED;
        m_statistics.activeSearches--;
        m_statistics.failedSearches++;
    }
}

ServerInfo* GlobalSearchEngine::FindServer(uint32_t ip, uint16_t port)
{
    auto it = m_servers.find((static_cast<uint64_t>(ip) << 16) | port);
    if (it != m_servers.end()) {
        return &it->second;
    }
    return nullptr;
}

void GlobalSearchEngine::SelectServersForSearch(SearchId searchId, size_t maxServers)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return;
    }

    // Get sorted server list
    std::vector<ServerInfo> servers = GetServers();

    // Filter connected servers
    std::vector<ServerInfo> connectedServers;
    for (const auto& server : servers) {
        if (server.isConnected && server.failedAttempts < 3) {
            connectedServers.push_back(server);
        }
    }

    // Select top servers
    size_t count = std::min(maxServers, connectedServers.size());

    // Create search requests
    for (size_t i = 0; i < count; ++i) {
        ServerSearchRequest request;
        request.searchId = searchId;
        request.serverIp = connectedServers[i].ip;
        request.serverPort = connectedServers[i].port;
        request.completed = false;
        request.retryCount = 0;

        it->second.activeRequests.push_back(request);
    }

    std::cout << "[GlobalSearchEngine] Selected " << count << " servers for search "
              << searchId.ToString() << std::endl;
}

bool GlobalSearchEngine::IsDuplicateResult(const std::string& fileHash, SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return false;
    }
    return it->second.resultDedup.find(fileHash) != it->second.resultDedup.end();
}

void GlobalSearchEngine::CleanupOldSearches(std::chrono::system_clock::time_point currentTime)
{
    auto it = m_searches.begin();
    while (it != m_searches.end()) {
        const auto& [searchId, data] = *it;

        if (data.state == SearchState::COMPLETED ||
            data.state == SearchState::FAILED ||
            data.state == SearchState::CANCELLED) {
            auto age = std::chrono::duration_cast<std::chrono::minutes>(
                currentTime - data.lastUpdateTime).count();

            // Remove searches older than 1 hour
            if (age > 60) {
                std::cout << "[GlobalSearchEngine] Cleaning up old search "
                          << searchId.ToString() << std::endl;
                it = m_searches.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void GlobalSearchEngine::RetryFailedRequests(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end() || it->second.state != SearchState::RUNNING) {
        return;
    }

    auto now = std::chrono::system_clock::now();

    for (auto& request : it->second.activeRequests) {
        if (!request.completed) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - request.sentTime).count();

            // Check for timeout
            if (elapsed > m_config.requestTimeout.count()) {
                if (request.retryCount < m_config.maxRetries) {
                    std::cout << "[GlobalSearchEngine] Retrying request to server "
                              << (request.serverIp >> 24) << "."
                              << ((request.serverIp >> 16) & 0xFF) << "."
                              << ((request.serverIp >> 8) & 0xFF) << "."
                              << (request.serverIp & 0xFF) << ":" << request.serverPort
                              << " (attempt " << (request.retryCount + 1) << ")" << std::endl;

                    request.retryCount++;
                    request.sentTime = now;

                    // Increment server failure count
                    auto server = FindServer(request.serverIp, request.serverPort);
                    if (server) {
                        server->failedAttempts++;
                    }
                } else {
                    std::cout << "[GlobalSearchEngine] Request to server failed after "
                              << m_config.maxRetries << " retries" << std::endl;
                    request.completed = true;
                    it->second.serversResponded++;
                }
            }
        }
    }
}

void GlobalSearchEngine::UpdateAverageSearchTime(int64_t durationMs)
{
    if (m_statistics.completedSearches == 0) {
        m_statistics.averageSearchTime = durationMs;
    } else {
        // Running average
        m_statistics.averageSearchTime =
            (m_statistics.averageSearchTime * (m_statistics.completedSearches - 1) + durationMs) /
            m_statistics.completedSearches;
    }
}

} // namespace search

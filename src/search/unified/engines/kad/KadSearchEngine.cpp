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

#include "KadSearchEngine.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <cctype>

namespace search {

KadSearchEngine::KadSearchEngine()
    : m_shutdown(false)
    , m_kadConnected(false)
{
    m_statistics.totalSearches = 0;
    m_statistics.activeSearches = 0;
    m_statistics.completedSearches = 0;
    m_statistics.failedSearches = 0;
    m_statistics.totalResults = 0;
    m_statistics.averageSearchTime = 0.0;

    std::cout << "[KadSearchEngine] Initialized" << std::endl;
}

KadSearchEngine::~KadSearchEngine()
{
    Shutdown();
}

SearchId KadSearchEngine::StartSearch(const SearchParams& params)
{
    if (m_shutdown) {
        std::cerr << "[KadSearchEngine] Engine is shutting down" << std::endl;
        return SearchId::Invalid();
    }

    if (!params.IsValid()) {
        std::cerr << "[KadSearchEngine] Invalid search parameters" << std::endl;
        return SearchId::Invalid();
    }

    if (params.type != SearchType::KADEMLIA) {
        std::cerr << "[KadSearchEngine] Wrong search type for Kad engine" << std::endl;
        return SearchId::Invalid();
    }

    if (!m_kadConnected && m_contacts.empty()) {
        std::cerr << "[KadSearchEngine] Kad not connected, no contacts available" << std::endl;
        return SearchId::Invalid();
    }

    SearchId searchId = SearchId::Generate();
    SearchData data;
    data.params = params;
    data.state = SearchState::STARTING;

    m_searches[searchId] = std::move(data);
    m_statistics.totalSearches++;
    m_statistics.activeSearches++;

    std::cout << "[KadSearchEngine] Starting Kad search " << searchId.ToString()
              << " for query: " << params.query << std::endl;

    // Extract keywords and compute hashes
    auto keywords = ExtractKeywords(params.query);
    std::cout << "[KadSearchEngine] Extracted " << keywords.size() << " keywords" << std::endl;

    // Send search to Kademlia nodes
    SendSearchToNodes(searchId, params);

    // Update state
    m_searches[searchId].state = SearchState::RUNNING;
    m_searches[searchId].startTime = std::chrono::system_clock::now();

    return searchId;
}

void KadSearchEngine::StopSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[KadSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    std::cout << "[KadSearchEngine] Stopping search " << searchId.ToString() << std::endl;

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

void KadSearchEngine::PauseSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[KadSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    if (it->second.state == SearchState::RUNNING) {
        std::cout << "[KadSearchEngine] Pausing search " << searchId.ToString() << std::endl;
        it->second.state = SearchState::PAUSED;
    }
}

void KadSearchEngine::ResumeSearch(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[KadSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    if (it->second.state == SearchState::PAUSED) {
        std::cout << "[KadSearchEngine] Resuming search " << searchId.ToString() << std::endl;
        it->second.state = SearchState::RUNNING;

        // Re-send search to nodes
        SendSearchToNodes(searchId, it->second.params);
    }
}

void KadSearchEngine::RequestMoreResults(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[KadSearchEngine] Search not found: " << searchId.ToString() << std::endl;
        return;
    }

    std::cout << "[KadSearchEngine] Requesting more results for search "
              << searchId.ToString() << std::endl;

    // Perform JumpStart to query more nodes
    if (it->second.state == SearchState::RUNNING ||
        it->second.state == SearchState::COMPLETED) {
        it->second.state = SearchState::RUNNING;
        JumpStart(searchId);
    }
}

SearchState KadSearchEngine::GetSearchState(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return SearchState::IDLE;
    }
    return it->second.state;
}

SearchParams KadSearchEngine::GetSearchParams(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return SearchParams();
    }
    return it->second.params;
}

std::vector<SearchResult> KadSearchEngine::GetResults(SearchId searchId, size_t maxResults) const
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

size_t KadSearchEngine::GetResultCount(SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return 0;
    }
    return it->second.results.size();
}

void KadSearchEngine::ProcessCommand(const SearchCommand& command)
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
            std::cerr << "[KadSearchEngine] Unknown command type: "
                      << static_cast<int>(command.type) << std::endl;
            break;
    }
}

void KadSearchEngine::ProcessMaintenance(std::chrono::system_clock::time_point currentTime)
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
                std::cout << "[KadSearchEngine] Search " << searchId.ToString()
                          << " timed out after " << elapsed << "ms" << std::endl;

                data.state = SearchState::COMPLETED;
                m_statistics.activeSearches--;
                m_statistics.completedSearches++;
                UpdateAverageSearchTime(elapsed);
            }

            // Check if we should JumpStart
            auto jumpStartElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - data.lastJumpStart).count();
            if (jumpStartElapsed > m_config.jumpStartInterval.count() &&
                data.jumpStartCount < m_config.maxJumpStarts &&
                !data.jumpStarted) {
                JumpStart(searchId);
            }
        }

        // Retry failed requests
        RetryFailedRequests(searchId);
    }

    // Cleanup old completed searches
    CleanupOldSearches(currentTime);
}

void KadSearchEngine::Shutdown()
{
    if (m_shutdown) {
        return;
    }

    std::cout << "[KadSearchEngine] Shutting down..." << std::endl;
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
    std::cout << "[KadSearchEngine] Shutdown complete" << std::endl;
}

EngineStatistics KadSearchEngine::GetStatistics() const
{
    return m_statistics;
}

void KadSearchEngine::AddContact(const KadContact& contact)
{
    m_contacts[contact.nodeId] = contact;
    m_kadConnected = true;

    std::cout << "[KadSearchEngine] Added contact " << contact.nodeId.substr(0, 16) << "..."
              << " (" << (contact.ip >> 24) << "."
              << ((contact.ip >> 16) & 0xFF) << "."
              << ((contact.ip >> 8) & 0xFF) << "."
              << (contact.ip & 0xFF) << ":" << contact.port << ")" << std::endl;
}

void KadSearchEngine::RemoveContact(const std::string& nodeId)
{
    m_contacts.erase(nodeId);

    if (m_contacts.empty()) {
        m_kadConnected = false;
    }

    std::cout << "[KadSearchEngine] Removed contact " << nodeId.substr(0, 16) << "..." << std::endl;
}

void KadSearchEngine::UpdateContactStatus(const std::string& nodeId, bool responsive)
{
    auto it = m_contacts.find(nodeId);
    if (it != m_contacts.end()) {
        it->second.isResponsive = responsive;
        if (responsive) {
            it->second.failedRequests = 0;
            it->second.lastSeen = std::chrono::system_clock::now().time_since_epoch().count();
        } else {
            it->second.failedRequests++;
        }
    }
}

std::vector<KadContact> KadSearchEngine::GetContacts() const
{
    std::vector<KadContact> contacts;
    contacts.reserve(m_contacts.size());

    for (const auto& [nodeId, contact] : m_contacts) {
        contacts.push_back(contact);
    }

    return contacts;
}

bool KadSearchEngine::IsKadConnected() const
{
    return m_kadConnected;
}

void KadSearchEngine::HandleKadResponse(SearchId searchId, const std::string& nodeId,
                                       const std::vector<SearchResult>& results)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[KadSearchEngine] Search not found for response: "
                  << searchId.ToString() << std::endl;
        return;
    }

    // Mark node as responded
    it->second.respondedNodes.insert(nodeId);
    it->second.totalResponses++;

    // Update contact status
    UpdateContactStatus(nodeId, true);

    // Process each result
    for (const auto& result : results) {
        HandleKadResult(searchId, result);
    }

    std::cout << "[KadSearchEngine] Received " << results.size() << " results from node "
              << nodeId.substr(0, 16) << "..." << std::endl;
}

void KadSearchEngine::HandleKadResult(SearchId searchId, const SearchResult& result)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        std::cerr << "[KadSearchEngine] Search not found for result: "
                  << searchId.ToString() << std::endl;
        return;
    }

    // Check for duplicate results
    if (m_config.enableResultDeduplication &&
        IsDuplicateResult(result.fileHash, searchId)) {
        std::cout << "[KadSearchEngine] Duplicate result filtered: "
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
        std::cout << "[KadSearchEngine] Search " << searchId.ToString()
                  << " reached max results (" << m_config.maxResultsPerSearch << ")" << std::endl;
        it->second.state = SearchState::COMPLETED;
        m_statistics.activeSearches--;
        m_statistics.completedSearches++;
    }

    // Notify callback
    if (m_onKadResult) {
        m_onKadResult(searchId, result);
    }

    std::cout << "[KadSearchEngine] Result received for search " << searchId.ToString()
              << ": " << result.fileName << " (" << result.fileSize << " bytes)" << std::endl;
}

void KadSearchEngine::SetOnKadResult(std::function<void(SearchId, const SearchResult&)> callback)
{
    m_onKadResult = std::move(callback);
}

std::string KadSearchEngine::ComputeKeywordHash(const std::string& keyword)
{
    // In a real implementation, this would compute the MD4 hash of the keyword
    // For now, we'll create a simple hash

    std::hash<std::string> hasher;
    size_t hash = hasher(keyword);

    // Convert to hex string (simulating 128-bit hash)
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(32) << hash;
    std::string result = ss.str();

    // Pad to 32 characters (128 bits)
    while (result.length() < 32) {
        result = "0" + result;
    }

    return result;
}

std::vector<std::string> KadSearchEngine::ExtractKeywords(const std::string& query)
{
    std::vector<std::string> keywords;
    std::string current;

    for (char c : query) {
        if (std::isspace(c) || c == '.' || c == '-' || c == '_') {
            if (!current.empty()) {
                keywords.push_back(current);
                current.clear();
            }
        } else {
            current += std::tolower(c);
        }
    }

    if (!current.empty()) {
        keywords.push_back(current);
    }

    return keywords;
}

std::vector<uint8_t> KadSearchEngine::BuildKadSearchPacket(const SearchParams& params,
                                                           const std::string& keywordHash)
{
    std::vector<uint8_t> packet;

    // In a real implementation, this would build the KADEMLIA2_SEARCH_KEY_REQ packet
    // Format: <SEARCH_KEY_REQ> <128-bit keyword hash> <search terms>

    // For now, we'll create a simplified version
    // This would be replaced with actual Kademlia packet construction

    packet.insert(packet.end(), keywordHash.begin(), keywordHash.end());
    packet.insert(packet.end(), params.query.begin(), params.query.end());

    return packet;
}

void KadSearchEngine::SendSearchToNodes(SearchId searchId, const SearchParams& params)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return;
    }

    // Extract keywords
    auto keywords = ExtractKeywords(params.query);
    if (keywords.empty()) {
        return;
    }

    // Use first keyword for hash computation
    std::string keywordHash = ComputeKeywordHash(keywords[0]);
    it->second.params.kadParams = KadParams{keywordHash};

    // Build search request
    std::vector<uint8_t> requestData = BuildKadSearchPacket(params, keywordHash);

    // Select nodes to query
    SelectNodesForSearch(searchId, m_config.maxConcurrentRequests);

    // Send to selected nodes
    for (auto& request : it->second.activeRequests) {
        if (!request.completed) {
            request.requestData = requestData;
            request.sentTime = std::chrono::system_clock::now();

            // In a full implementation, this would send the packet via the network layer
            // For now, we'll simulate by generating some demo results

            std::cout << "[KadSearchEngine] Sent search request to node "
                      << request.contactNodeId.substr(0, 16) << "..."
                      << " (" << (request.contactIp >> 24) << "."
                      << ((request.contactIp >> 16) & 0xFF) << "."
                      << ((request.contactIp >> 8) & 0xFF) << "."
                      << (request.contactIp & 0xFF) << ":" << request.contactPort << ")" << std::endl;

            it->second.totalRequests++;
            it->second.contactedNodes.insert(request.contactNodeId);
        }
    }

    // Mark as completed if no nodes available
    if (it->second.activeRequests.empty()) {
        std::cout << "[KadSearchEngine] No nodes available for search "
                  << searchId.ToString() << std::endl;
        it->second.state = SearchState::FAILED;
        m_statistics.activeSearches--;
        m_statistics.failedSearches++;
    }
}

void KadSearchEngine::SelectNodesForSearch(SearchId searchId, size_t maxNodes)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return;
    }

    // Get responsive contacts
    std::vector<KadContact> responsiveContacts;
    for (const auto& [nodeId, contact] : m_contacts) {
        if (contact.isResponsive && contact.failedRequests < 3 &&
            it->second.contactedNodes.find(nodeId) == it->second.contactedNodes.end()) {
            responsiveContacts.push_back(contact);
        }
    }

    // Sort by distance to target (keyword hash)
    if (it->second.params.kadParams) {
        SortContactsByDistance(it->second.params.kadParams->keywordHash, responsiveContacts);
    }

    // Select top nodes
    size_t count = std::min(maxNodes, responsiveContacts.size());

    // Create search requests
    for (size_t i = 0; i < count; ++i) {
        KadSearchRequest request;
        request.searchId = searchId;
        request.targetNodeId = it->second.params.kadParams ?
            it->second.params.kadParams->keywordHash : "";
        request.contactNodeId = responsiveContacts[i].nodeId;
        request.contactIp = responsiveContacts[i].ip;
        request.contactPort = responsiveContacts[i].port;
        request.completed = false;
        request.retryCount = 0;

        it->second.activeRequests.push_back(request);
    }

    std::cout << "[KadSearchEngine] Selected " << count << " nodes for search "
              << searchId.ToString() << std::endl;
}

void KadSearchEngine::JumpStart(SearchId searchId)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end() || it->second.state != SearchState::RUNNING) {
        return;
    }

    if (it->second.jumpStartCount >= m_config.maxJumpStarts) {
        std::cout << "[KadSearchEngine] Max JumpStart count reached for search "
                  << searchId.ToString() << std::endl;
        return;
    }

    std::cout << "[KadSearchEngine] JumpStart " << (it->second.jumpStartCount + 1)
              << " for search " << searchId.ToString() << std::endl;

    // Select additional nodes that haven't been queried yet
    SelectNodesForSearch(searchId, m_config.maxConcurrentRequests);

    it->second.jumpStartCount++;
    it->second.lastJumpStart = std::chrono::system_clock::now();
    it->second.jumpStarted = true;
}

bool KadSearchEngine::IsDuplicateResult(const std::string& fileHash, SearchId searchId) const
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return false;
    }
    return it->second.resultDedup.find(fileHash) != it->second.resultDedup.end();
}

void KadSearchEngine::CleanupOldSearches(std::chrono::system_clock::time_point currentTime)
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
                std::cout << "[KadSearchEngine] Cleaning up old search "
                          << searchId.ToString() << std::endl;
                it = m_searches.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void KadSearchEngine::RetryFailedRequests(SearchId searchId)
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
                    std::cout << "[KadSearchEngine] Retrying request to node "
                              << request.contactNodeId.substr(0, 16) << "..."
                              << " (attempt " << (request.retryCount + 1) << ")" << std::endl;

                    request.retryCount++;
                    request.sentTime = now;

                    // Increment contact failure count
                    UpdateContactStatus(request.contactNodeId, false);
                } else {
                    std::cout << "[KadSearchEngine] Request to node failed after "
                              << m_config.maxRetries << " retries" << std::endl;
                    request.completed = true;
                }
            }
        }
    }
}

KadContact* KadSearchEngine::FindContact(const std::string& nodeId)
{
    auto it = m_contacts.find(nodeId);
    if (it != m_contacts.end()) {
        return &it->second;
    }
    return nullptr;
}

void KadSearchEngine::SortContactsByDistance(const std::string& targetNodeId,
                                            std::vector<KadContact>& contacts)
{
    // Sort contacts by XOR distance to target
    std::sort(contacts.begin(), contacts.end(),
        [this, &targetNodeId](const KadContact& a, const KadContact& b) {
            uint32_t distA = ComputeXorDistance(targetNodeId, a.nodeId);
            uint32_t distB = ComputeXorDistance(targetNodeId, b.nodeId);
            return distA < distB;
        });
}

uint32_t KadSearchEngine::ComputeXorDistance(const std::string& id1, const std::string& id2)
{
    // Compute XOR distance between two node IDs
    // Simplified implementation - real XOR would operate on 128-bit values

    uint32_t hash1 = 0, hash2 = 0;
    for (size_t i = 0; i < id1.length() && i < 8; ++i) {
        hash1 = (hash1 << 4) | (id1[i] >= 'a' ? id1[i] - 'a' + 10 : id1[i] - '0');
    }
    for (size_t i = 0; i < id2.length() && i < 8; ++i) {
        hash2 = (hash2 << 4) | (id2[i] >= 'a' ? id2[i] - 'a' + 10 : id2[i] - '0');
    }

    return hash1 ^ hash2;
}

void KadSearchEngine::UpdateAverageSearchTime(int64_t durationMs)
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

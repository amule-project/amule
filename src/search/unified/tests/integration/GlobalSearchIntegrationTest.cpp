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

#include <gtest/gtest.h>
#include "search/unified/engines/global/GlobalSearchEngine.h"
#include "search/unified/manager/UnifiedSearchManager.h"
#include <thread>
#include <chrono>

using namespace search;

class GlobalSearchIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<UnifiedSearchManager>();
        manager->Start();

        // Set up result callback
        resultsReceived = 0;
        manager->SetEventCallback([this](const SearchEvent& event) {
            if (event.type == SearchEvent::Type::RESULTS_RECEIVED) {
                resultsReceived += event.results.size();
                lastResults = event.results;
            }
        });

        // Get the global search engine
        // In a real implementation, we'd access it through the manager
        globalEngine = std::make_unique<GlobalSearchEngine>();

        // Add test servers
        SetupTestServers();
    }

    void TearDown() override {
        globalEngine->Shutdown();
        globalEngine.reset();
        manager->Shutdown();
        manager.reset();
    }

    void SetupTestServers() {
        ServerInfo server1;
        server1.ip = 0x7F000001;  // 127.0.0.1
        server1.port = 4661;
        server1.name = "TestServer1";
        server1.isConnected = true;
        server1.userCount = 5000;
        server1.fileCount = 100000;
        globalEngine->AddServer(server1);

        ServerInfo server2;
        server2.ip = 0x7F000002;  // 127.0.0.2
        server2.port = 4662;
        server2.name = "TestServer2";
        server2.isConnected = true;
        server2.userCount = 3000;
        server2.fileCount = 80000;
        globalEngine->AddServer(server2);

        ServerInfo server3;
        server3.ip = 0x7F000003;  // 127.0.0.3
        server3.port = 4663;
        server3.name = "TestServer3";
        server3.isConnected = true;
        server3.userCount = 7000;
        server3.fileCount = 150000;
        globalEngine->AddServer(server3);
    }

    std::unique_ptr<UnifiedSearchManager> manager;
    std::unique_ptr<GlobalSearchEngine> globalEngine;
    size_t resultsReceived;
    std::vector<SearchResult> lastResults;

    SearchParams CreateGlobalParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::GLOBAL;
        params.query = query;
        params.maxResults = 100;
        params.timeout = std::chrono::seconds(30);
        return params;
    }
};

// Test 1: Start and stop global search
TEST_F(GlobalSearchIntegrationTest, StartStopGlobalSearch) {
    SearchParams params = CreateGlobalParams("music");
    SearchCommand cmd = SearchCommand::StartSearch(params);

    ASSERT_TRUE(manager->SendCommand(cmd));
    EXPECT_TRUE(cmd.searchId.IsValid());

    // Wait a bit for search to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the search
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    ASSERT_TRUE(manager->SendCommand(stopCmd));

    // Wait for stop to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Check search state
    SearchCommand stateCmd = SearchCommand::GetSearchState(cmd.searchId);
    stateCmd.responseCallback = [&](const std::vector<uint8_t>& response) {
        if (!response.empty()) {
            // Should be cancelled
        }
    };
    manager->SendCommand(stateCmd);
}

// Test 2: Multiple concurrent global searches
TEST_F(GlobalSearchIntegrationTest, MultipleConcurrentGlobalSearches) {
    std::vector<SearchId> searchIds;

    // Start multiple searches
    for (int i = 0; i < 5; ++i) {
        SearchParams params = CreateGlobalParams("query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        ASSERT_TRUE(manager->SendCommand(cmd));
        searchIds.push_back(cmd.searchId);
    }

    // Wait for searches to process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop all searches
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }

    // Verify statistics
    auto stats = manager->GetStatistics();
    EXPECT_GE(stats.totalSearches, 5);
}

// Test 3: Global search with filters
TEST_F(GlobalSearchIntegrationTest, GlobalSearchWithFilters) {
    SearchParams params = CreateGlobalParams("music");
    params.minFileSize = 1024 * 1024;  // 1 MB
    params.maxFileSize = 100 * 1024 * 1024;  // 100 MB
    params.fileTypes = {"mp3", "flac", "ogg"};

    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Add some test results (simulating server responses)
    SearchResult result1;
    result1.searchId = cmd.searchId;
    result1.sourceType = SearchType::GLOBAL;
    result1.fileName = "song.mp3";
    result1.fileSize = 5 * 1024 * 1024;
    result1.fileHash = "hash1";
    result1.availability = 10;

    globalEngine->HandleSearchResult(cmd.searchId, result1);

    // Result too small
    SearchResult result2;
    result2.searchId = cmd.searchId;
    result2.sourceType = SearchType::GLOBAL;
    result2.fileName = "tiny.mp3";
    result2.fileSize = 512 * 1024;
    result2.fileHash = "hash2";
    result2.availability = 5;

    globalEngine->HandleSearchResult(cmd.searchId, result2);

    // Result too large
    SearchResult result3;
    result3.searchId = cmd.searchId;
    result3.sourceType = SearchType::GLOBAL;
    result3.fileName = "huge.flac";
    result3.fileSize = 200 * 1024 * 1024;
    result3.fileHash = "hash3";
    result3.availability = 15;

    globalEngine->HandleSearchResult(cmd.searchId, result3);

    // Check result count (should only have result1)
    size_t resultCount = globalEngine->GetResultCount(cmd.searchId);
    EXPECT_EQ(resultCount, 1);

    // Cleanup
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    manager->SendCommand(stopCmd);
}

// Test 4: Result deduplication across servers
TEST_F(GlobalSearchIntegrationTest, ResultDeduplicationAcrossServers) {
    SearchParams params = CreateGlobalParams("music");
    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Simulate same result from different servers
    SearchResult result;
    result.searchId = cmd.searchId;
    result.sourceType = SearchType::GLOBAL;
    result.fileName = "song.mp3";
    result.fileSize = 5 * 1024 * 1024;
    result.fileHash = "same_hash";  // Same hash
    result.availability = 10;

    // Add result multiple times (simulating from different servers)
    globalEngine->HandleSearchResult(cmd.searchId, result);
    globalEngine->HandleSearchResult(cmd.searchId, result);
    globalEngine->HandleSearchResult(cmd.searchId, result);

    // Should only have one result due to deduplication
    size_t resultCount = globalEngine->GetResultCount(cmd.searchId);
    EXPECT_EQ(resultCount, 1);

    // Cleanup
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    manager->SendCommand(stopCmd);
}

// Test 5: Server selection and prioritization
TEST_F(GlobalSearchIntegrationTest, ServerSelectionAndPrioritization) {
    // Add a preferred server
    ServerInfo preferredServer;
    preferredServer.ip = 0x7F000004;
    preferredServer.port = 4664;
    preferredServer.name = "PreferredServer";
    preferredServer.isConnected = true;
    preferredServer.userCount = 1000;
    preferredServer.fileCount = 50000;
    preferredServer.isPreferred = true;
    globalEngine->AddServer(preferredServer);

    SearchParams params = CreateGlobalParams("music");
    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify search is running
    EXPECT_EQ(globalEngine->GetSearchState(cmd.searchId), SearchState::RUNNING);

    // Cleanup
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    manager->SendCommand(stopCmd);
}

// Test 6: Pause and resume global search
TEST_F(GlobalSearchIntegrationTest, PauseResumeGlobalSearch) {
    SearchParams params = CreateGlobalParams("music");
    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    std::this_thread->sleep_for(std::chrono::milliseconds(100));

    // Pause the search
    SearchCommand pauseCmd = SearchCommand::PauseSearch(cmd.searchId);
    ASSERT_TRUE(manager->SendCommand(pauseCmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify paused state
    EXPECT_EQ(globalEngine->GetSearchState(cmd.searchId), SearchState::PAUSED);

    // Resume the search
    SearchCommand resumeCmd = SearchCommand::ResumeSearch(cmd.searchId);
    ASSERT_TRUE(manager->SendCommand(resumeCmd));

    std::this_thread->sleep_for(std::chrono::milliseconds(100));

    // Verify running state
    EXPECT_EQ(globalEngine->GetSearchState(cmd.searchId), SearchState::RUNNING);

    // Cleanup
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    manager->SendCommand(stopCmd);
}

// Test 7: Request more results
TEST_F(GlobalSearchIntegrationTest, RequestMoreResults) {
    SearchParams params = CreateGlobalParams("music");
    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Add some initial results
    for (int i = 0; i < 10; ++i) {
        SearchResult result;
        result.searchId = cmd.searchId;
        result.sourceType = SearchType::GLOBAL;
        result.fileName = "song" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        globalEngine->HandleSearchResult(cmd.searchId, result);
    }

    size_t initialCount = globalEngine->GetResultCount(cmd.searchId);
    EXPECT_EQ(initialCount, 10);

    // Request more results
    SearchCommand moreCmd = SearchCommand::RequestMoreResults(cmd.searchId);
    ASSERT_TRUE(manager->SendCommand(moreCmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Add more results
    for (int i = 10; i < 20; ++i) {
        SearchResult result;
        result.searchId = cmd.searchId;
        result.sourceType = SearchType::GLOBAL;
        result.fileName = "song" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        globalEngine->HandleSearchResult(cmd.searchId, result);
    }

    size_t finalCount = globalEngine->GetResultCount(cmd.searchId);
    EXPECT_EQ(finalCount, 20);

    // Cleanup
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    manager->SendCommand(stopCmd);
}

// Test 8: Get search results via manager
TEST_F(GlobalSearchIntegrationTest, GetSearchResultsViaManager) {
    SearchParams params = CreateGlobalParams("music");
    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Add some results
    for (int i = 0; i < 5; ++i) {
        SearchResult result;
        result.searchId = cmd.searchId;
        result.sourceType = SearchType::GLOBAL;
        result.fileName = "song" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        globalEngine->HandleSearchResult(cmd.searchId, result);
    }

    // Get results via manager
    SearchCommand getCmd = SearchCommand::GetResults(cmd.searchId, 3);
    std::vector<SearchResult> retrievedResults;
    bool gotResponse = false;

    getCmd.responseCallback = [&](const std::vector<uint8_t>& response) {
        // Deserialize results
        size_t pos = 0;
        while (pos < response.size()) {
            if (pos + sizeof(uint32_t) > response.size()) break;
            uint32_t resultLen = *reinterpret_cast<const uint32_t*>(&response[pos]);
            pos += sizeof(uint32_t);
            if (pos + resultLen > response.size()) break;
            std::vector<uint8_t> resultData(response.begin() + pos,
                                              response.begin() + pos + resultLen);
            retrievedResults.push_back(SearchResult::Deserialize(resultData));
            pos += resultLen;
        }
        gotResponse = true;
    };

    manager->SendCommand(getCmd);

    // Wait for response
    auto start = std::chrono::steady_clock::now();
    while (!gotResponse && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(5000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(gotResponse);
    EXPECT_EQ(retrievedResults.size(), 3);

    // Cleanup
    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
    manager->SendCommand(stopCmd);
}

// Test 9: Server status updates
TEST_F(GlobalSearchIntegrationTest, ServerStatusUpdates) {
    auto servers = globalEngine->GetServers();
    ASSERT_GE(servers.size(), 3);

    // Update server status
    globalEngine->UpdateServerStatus(0x7F000001, 4661, 6000, 120000);
    globalEngine->UpdateServerStatus(0x7F000002, 4662, 4000, 90000);

    // Verify updates
    auto updatedServers = globalEngine->GetServers();

    bool found1 = false, found2 = false;
    for (const auto& server : updatedServers) {
        if (server.ip == 0x7F000001 && server.port == 4661) {
            EXPECT_EQ(server.userCount, 6000);
            EXPECT_EQ(server.fileCount, 120000);
            found1 = true;
        }
        if (server.ip == 0x7F000002 && server.port == 4662) {
            EXPECT_EQ(server.userCount, 4000);
            EXPECT_EQ(server.fileCount, 90000);
            found2 = true;
        }
    }

    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

// Test 10: High volume global searches
TEST_F(GlobalSearchIntegrationTest, HighVolumeGlobalSearches) {
    std::vector<SearchId> searchIds;

    // Start many searches
    for (int i = 0; i < 20; ++i) {
        SearchParams params = CreateGlobalParams("query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        ASSERT_TRUE(manager->SendCommand(cmd));
        searchIds.push_back(cmd.searchId);
    }

    // Wait for searches to process
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Stop all searches
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }

    // Verify statistics
    auto stats = manager->GetStatistics();
    EXPECT_GE(stats.totalSearches, 20);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

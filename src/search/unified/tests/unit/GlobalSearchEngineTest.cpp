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
#include "search/unified/core/SearchParams.h"
#include "search/unified/core/SearchResult.h"
#include <thread>
#include <chrono>

using namespace search;

class GlobalSearchEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<GlobalSearchEngine>();
    }

    void TearDown() override {
        engine->Shutdown();
        engine.reset();
    }

    std::unique_ptr<GlobalSearchEngine> engine;

    // Helper to create a valid search params
    SearchParams CreateValidParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::GLOBAL;
        params.query = query;
        params.maxResults = 100;
        params.timeout = std::chrono::seconds(30);
        return params;
    }

    // Helper to create a server
    ServerInfo CreateServer(uint32_t ip, uint16_t port, const std::string& name) {
        ServerInfo server;
        server.ip = ip;
        server.port = port;
        server.name = name;
        server.isConnected = true;
        server.userCount = 1000;
        server.fileCount = 100000;
        return server;
    }

    // Helper to create a search result
    SearchResult CreateResult(const std::string& fileName, uint64_t size) {
        SearchResult result;
        result.fileName = fileName;
        result.fileSize = size;
        result.fileHash = fileName;  // Use filename as hash for testing
        result.availability = 10;
        return result;
    }
};

// Test 1: Engine initialization
TEST_F(GlobalSearchEngineTest, InitializeEngine) {
    ASSERT_NE(engine, nullptr);
    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 0);
    EXPECT_EQ(stats.activeSearches, 0);
    EXPECT_EQ(stats.completedSearches, 0);
    EXPECT_EQ(stats.failedSearches, 0);
    EXPECT_EQ(stats.totalResults, 0);
}

// Test 2: Start search with valid parameters
TEST_F(GlobalSearchEngineTest, StartSearchValidParams) {
    SearchParams params = CreateValidParams("test query");

    SearchId searchId = engine->StartSearch(params);

    EXPECT_TRUE(searchId.IsValid());
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::RUNNING);

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 1);
    EXPECT_EQ(stats.activeSearches, 1);
}

// Test 3: Start search with invalid parameters
TEST_F(GlobalSearchEngineTest, StartSearchInvalidParams) {
    SearchParams params;
    params.type = SearchType::GLOBAL;
    params.query = "";  // Empty query

    SearchId searchId = engine->StartSearch(params);

    EXPECT_FALSE(searchId.IsValid());

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 0);
}

// Test 4: Start search with wrong type
TEST_F(GlobalSearchEngineTest, StartSearchWrongType) {
    SearchParams params = CreateValidParams("test query");
    params.type = SearchType::LOCAL;  // Wrong type for global engine

    SearchId searchId = engine->StartSearch(params);

    EXPECT_FALSE(searchId.IsValid());
}

// Test 5: Stop running search
TEST_F(GlobalSearchEngineTest, StopSearch) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    engine->StopSearch(searchId);

    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::CANCELLED);

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.activeSearches, 0);
    EXPECT_EQ(stats.completedSearches, 1);
}

// Test 6: Pause and resume search
TEST_F(GlobalSearchEngineTest, PauseResumeSearch) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    engine->PauseSearch(searchId);
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::PAUSED);

    engine->ResumeSearch(searchId);
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::RUNNING);
}

// Test 7: Handle search results
TEST_F(GlobalSearchEngineTest, HandleSearchResults) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add a result
    SearchResult result1 = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleSearchResult(searchId, result1);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Add another result
    SearchResult result2 = CreateResult("file2.mp3", 2 * 1024 * 1024);
    engine->HandleSearchResult(searchId, result2);

    EXPECT_EQ(engine->GetResultCount(searchId), 2);
}

// Test 8: Duplicate result filtering
TEST_F(GlobalSearchEngineTest, DuplicateResultFiltering) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add a result
    SearchResult result1 = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleSearchResult(searchId, result1);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Add duplicate result (same hash)
    SearchResult result2 = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleSearchResult(searchId, result2);

    // Should still be 1 due to duplicate filtering
    EXPECT_EQ(engine->GetResultCount(searchId), 1);
}

// Test 9: File size filtering
TEST_F(GlobalSearchEngineTest, FileSizeFiltering) {
    SearchParams params = CreateValidParams("test query");
    params.minFileSize = 1024 * 1024;  // 1 MB
    params.maxFileSize = 10 * 1024 * 1024;  // 10 MB

    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add result within range
    SearchResult result1 = CreateResult("file1.mp3", 5 * 1024 * 1024);
    engine->HandleSearchResult(searchId, result1);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Add result below range
    SearchResult result2 = CreateResult("file2.mp3", 512 * 1024);
    engine->HandleSearchResult(searchId, result2);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);  // Should be filtered

    // Add result above range
    SearchResult result3 = CreateResult("file3.mp3", 20 * 1024 * 1024);
    engine->HandleSearchResult(searchId, result3);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);  // Should be filtered
}

// Test 10: Get results with limit
TEST_F(GlobalSearchEngineTest, GetResultsWithLimit) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add multiple results
    for (int i = 0; i < 10; ++i) {
        SearchResult result = CreateResult("file" + std::to_string(i) + ".mp3",
                                            (i + 1) * 1024 * 1024);
        engine->HandleSearchResult(searchId, result);
    }

    EXPECT_EQ(engine->GetResultCount(searchId), 10);

    // Get limited results
    auto results = engine->GetResults(searchId, 5);
    EXPECT_EQ(results.size(), 5);
}

// Test 11: Server management
TEST_F(GlobalSearchEngineTest, ServerManagement) {
    // Add servers
    ServerInfo server1 = CreateServer(0x01020304, 4661, "Server1");
    ServerInfo server2 = CreateServer(0x05060708, 4662, "Server2");

    engine->AddServer(server1);
    engine->AddServer(server2);

    auto servers = engine->GetServers();
    EXPECT_EQ(servers.size(), 2);

    // Check server is connected
    EXPECT_TRUE(engine->IsServerConnected(0x01020304, 4661));

    // Remove server
    engine->RemoveServer(0x01020304, 4661);

    servers = engine->GetServers();
    EXPECT_EQ(servers.size(), 1);

    EXPECT_FALSE(engine->IsServerConnected(0x01020304, 4661));
}

// Test 12: Server status update
TEST_F(GlobalSearchEngineTest, UpdateServerStatus) {
    ServerInfo server = CreateServer(0x01020304, 4661, "Server1");
    engine->AddServer(server);

    // Update status
    engine->UpdateServerStatus(0x01020304, 4661, 5000, 500000);

    auto servers = engine->GetServers();
    ASSERT_EQ(servers.size(), 1);
    EXPECT_EQ(servers[0].userCount, 5000);
    EXPECT_EQ(servers[0].fileCount, 500000);
}

// Test 13: Multiple searches
TEST_F(GlobalSearchEngineTest, MultipleSearches) {
    SearchParams params1 = CreateValidParams("query1");
    SearchParams params2 = CreateValidParams("query2");

    SearchId searchId1 = engine->StartSearch(params1);
    SearchId searchId2 = engine->StartSearch(params2);

    ASSERT_TRUE(searchId1.IsValid());
    ASSERT_TRUE(searchId2.IsValid());

    EXPECT_EQ(engine->GetSearchState(searchId1), SearchState::RUNNING);
    EXPECT_EQ(engine->GetSearchState(searchId2), SearchState::RUNNING);

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.activeSearches, 2);
}

// Test 14: Request more results
TEST_F(GlobalSearchEngineTest, RequestMoreResults) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add some results
    SearchResult result = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleSearchResult(searchId, result);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Request more results
    engine->RequestMoreResults(searchId);

    // State should remain running (or become running if completed)
    EXPECT_TRUE(engine->GetSearchState(searchId) == SearchState::RUNNING);
}

// Test 15: Get search parameters
TEST_F(GlobalSearchEngineTest, GetSearchParams) {
    SearchParams params = CreateValidParams("test query");
    params.minFileSize = 1024 * 1024;
    params.maxFileSize = 10 * 1024 * 1024;

    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    SearchParams retrievedParams = engine->GetSearchParams(searchId);

    EXPECT_EQ(retrievedParams.query, "test query");
    EXPECT_EQ(retrievedParams.type, SearchType::GLOBAL);
    EXPECT_EQ(retrievedParams.minFileSize, 1024 * 1024);
    EXPECT_EQ(retrievedParams.maxFileSize, 10 * 1024 * 1024);
}

// Test 16: Result callback
TEST_F(GlobalSearchEngineTest, ResultCallback) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    bool callbackCalled = false;
    SearchResult receivedResult;

    engine->SetOnSearchResult([&](SearchId id, const SearchResult& result) {
        callbackCalled = true;
        receivedResult = result;
    });

    // Add a result
    SearchResult result = CreateResult("test.mp3", 1024 * 1024);
    engine->HandleSearchResult(searchId, result);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedResult.fileName, "test.mp3");
}

// Test 17: Statistics update
TEST_F(GlobalSearchEngineTest, StatisticsUpdate) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add results
    for (int i = 0; i < 5; ++i) {
        SearchResult result = CreateResult("file" + std::to_string(i) + ".mp3",
                                            (i + 1) * 1024 * 1024);
        engine->HandleSearchResult(searchId, result);
    }

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 1);
    EXPECT_EQ(stats.activeSearches, 1);
    EXPECT_EQ(stats.totalResults, 5);

    // Stop search
    engine->StopSearch(searchId);

    stats = engine->GetStatistics();
    EXPECT_EQ(stats.activeSearches, 0);
    EXPECT_EQ(stats.completedSearches, 1);
}

// Test 18: Maintenance cleanup
TEST_F(GlobalSearchEngineTest, MaintenanceCleanup) {
    SearchParams params = CreateValidParams("test query");
    params.timeout = std::chrono::milliseconds(100);  // Short timeout

    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Run maintenance
    engine->ProcessMaintenance(std::chrono::system_clock::now());

    // Search should be completed due to timeout
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::COMPLETED);
}

// Test 19: Shutdown
TEST_F(GlobalSearchEngineTest, Shutdown) {
    SearchParams params = CreateValidParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Shutdown engine
    engine->Shutdown();

    // Try to start new search after shutdown
    SearchParams params2 = CreateValidParams("query2");
    SearchId searchId2 = engine->StartSearch(params2);

    EXPECT_FALSE(searchId2.IsValid());
}

// Test 20: Server prioritization
TEST_F(GlobalSearchEngineTest, ServerPrioritization) {
    // Add servers with different characteristics
    ServerInfo server1 = CreateServer(0x01020304, 4661, "Server1");
    server1.userCount = 1000;
    server1.isPreferred = false;

    ServerInfo server2 = CreateServer(0x05060708, 4662, "Server2");
    server2.userCount = 5000;
    server2.isPreferred = true;

    ServerInfo server3 = CreateServer(0x090A0B0C, 4663, "Server3");
    server3.userCount = 3000;
    server3.isPreferred = false;

    engine->AddServer(server1);
    engine->AddServer(server2);
    engine->AddServer(server3);

    auto servers = engine->GetServers();

    // Preferred server should be first
    EXPECT_EQ(servers[0].name, "Server2");

    // Then sorted by user count
    EXPECT_EQ(servers[1].name, "Server3");
    EXPECT_EQ(servers[2].name, "Server1");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

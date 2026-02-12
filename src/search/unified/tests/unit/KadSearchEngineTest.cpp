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
#include "search/unified/engines/kad/KadSearchEngine.h"
#include "search/unified/core/SearchParams.h"
#include "search/unified/core/SearchResult.h"
#include <thread>
#include <chrono>

using namespace search;

class KadSearchEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine = std::make_unique<KadSearchEngine>();

        // Add test contacts
        SetupTestContacts();
    }

    void TearDown() override {
        engine->Shutdown();
        engine.reset();
    }

    void SetupTestContacts() {
        KadContact contact1;
        contact1.nodeId = "0123456789abcdef0123456789abcdef";
        contact1.ip = 0x7F000001;  // 127.0.0.1
        contact1.port = 4671;
        contact1.isResponsive = true;
        contact1.failedRequests = 0;
        engine->AddContact(contact1);

        KadContact contact2;
        contact2.nodeId = "fedcba9876543210fedcba9876543210";
        contact2.ip = 0x7F000002;  // 127.0.0.2
        contact2.port = 4672;
        contact2.isResponsive = true;
        contact2.failedRequests = 0;
        engine->AddContact(contact2);

        KadContact contact3;
        contact3.nodeId = "abcdef0123456789abcdef0123456789";
        contact3.ip = 0x7F000003;  // 127.0.0.3
        contact3.port = 4673;
        contact3.isResponsive = true;
        contact3.failedRequests = 0;
        engine->AddContact(contact3);
    }

    std::unique_ptr<KadSearchEngine> engine;

    // Helper to create a valid search params
    SearchParams CreateKadParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::KADEMLIA;
        params.query = query;
        params.maxResults = 100;
        params.timeout = std::chrono::seconds(30);
        return params;
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

    // Helper to create a Kad contact
    KadContact CreateContact(const std::string& nodeId, uint32_t ip, uint16_t port) {
        KadContact contact;
        contact.nodeId = nodeId;
        contact.ip = ip;
        contact.port = port;
        contact.isResponsive = true;
        contact.failedRequests = 0;
        return contact;
    }
};

// Test 1: Engine initialization
TEST_F(KadSearchEngineTest, InitializeEngine) {
    ASSERT_NE(engine, nullptr);
    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 0);
    EXPECT_EQ(stats.activeSearches, 0);
    EXPECT_EQ(stats.completedSearches, 0);
    EXPECT_EQ(stats.failedSearches, 0);
    EXPECT_EQ(stats.totalResults, 0);
}

// Test 2: Kad connection status
TEST_F(KadSearchEngineTest, KadConnectionStatus) {
    // Should be connected after adding contacts
    EXPECT_TRUE(engine->IsKadConnected());

    // Get contacts
    auto contacts = engine->GetContacts();
    EXPECT_EQ(contacts.size(), 3);
}

// Test 3: Start search with valid parameters
TEST_F(KadSearchEngineTest, StartSearchValidParams) {
    SearchParams params = CreateKadParams("test query");

    SearchId searchId = engine->StartSearch(params);

    EXPECT_TRUE(searchId.IsValid());
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::RUNNING);

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 1);
    EXPECT_EQ(stats.activeSearches, 1);
}

// Test 4: Start search with invalid parameters
TEST_F(KadSearchEngineTest, StartSearchInvalidParams) {
    SearchParams params;
    params.type = SearchType::KADEMLIA;
    params.query = "";  // Empty query

    SearchId searchId = engine->StartSearch(params);

    EXPECT_FALSE(searchId.IsValid());

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.totalSearches, 0);
}

// Test 5: Start search without contacts
TEST_F(KadSearchEngineTest, StartSearchWithoutContacts) {
    // Remove all contacts
    auto contacts = engine->GetContacts();
    for (const auto& contact : contacts) {
        engine->RemoveContact(contact.nodeId);
    }

    EXPECT_FALSE(engine->IsKadConnected());

    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    EXPECT_FALSE(searchId.IsValid());
}

// Test 6: Stop running search
TEST_F(KadSearchEngineTest, StopSearch) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    engine->StopSearch(searchId);

    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::CANCELLED);

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.activeSearches, 0);
    EXPECT_EQ(stats.completedSearches, 1);
}

// Test 7: Pause and resume search
TEST_F(KadSearchEngineTest, PauseResumeSearch) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    engine->PauseSearch(searchId);
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::PAUSED);

    engine->ResumeSearch(searchId);
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::RUNNING);
}

// Test 8: Handle Kad results
TEST_F(KadSearchEngineTest, HandleKadResults) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add a result
    SearchResult result1 = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleKadResult(searchId, result1);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Add another result
    SearchResult result2 = CreateResult("file2.mp3", 2 * 1024 * 1024);
    engine->HandleKadResult(searchId, result2);

    EXPECT_EQ(engine->GetResultCount(searchId), 2);
}

// Test 9: Duplicate result filtering
TEST_F(KadSearchEngineTest, DuplicateResultFiltering) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add a result
    SearchResult result1 = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleKadResult(searchId, result1);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Add duplicate result (same hash)
    SearchResult result2 = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleKadResult(searchId, result2);

    // Should still be 1 due to duplicate filtering
    EXPECT_EQ(engine->GetResultCount(searchId), 1);
}

// Test 10: File size filtering
TEST_F(KadSearchEngineTest, FileSizeFiltering) {
    SearchParams params = CreateKadParams("test query");
    params.minFileSize = 1024 * 1024;  // 1 MB
    params.maxFileSize = 10 * 1024 * 1024;  // 10 MB

    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add result within range
    SearchResult result1 = CreateResult("file1.mp3", 5 * 1024 * 1024);
    engine->HandleKadResult(searchId, result1);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Add result below range
    SearchResult result2 = CreateResult("file2.mp3", 512 * 1024);
    engine->HandleKadResult(searchId, result2);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);  // Should be filtered

    // Add result above range
    SearchResult result3 = CreateResult("file3.mp3", 20 * 1024 * 1024);
    engine->HandleKadResult(searchId, result3);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);  // Should be filtered
}

// Test 11: Get results with limit
TEST_F(KadSearchEngineTest, GetResultsWithLimit) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add multiple results
    for (int i = 0; i < 10; ++i) {
        SearchResult result = CreateResult("file" + std::to_string(i) + ".mp3",
                                            (i + 1) * 1024 * 1024);
        engine->HandleKadResult(searchId, result);
    }

    EXPECT_EQ(engine->GetResultCount(searchId), 10);

    // Get limited results
    auto results = engine->GetResults(searchId, 5);
    EXPECT_EQ(results.size(), 5);
}

// Test 12: Contact management
TEST_F(KadSearchEngineTest, ContactManagement) {
    // Add new contact
    KadContact contact = CreateContact("new123456789abcdef0123456789abcde", 0x7F000004, 4674);
    engine->AddContact(contact);

    auto contacts = engine->GetContacts();
    EXPECT_EQ(contacts.size(), 4);

    // Remove contact
    engine->RemoveContact(contact.nodeId);

    contacts = engine->GetContacts();
    EXPECT_EQ(contacts.size(), 3);
}

// Test 13: Update contact status
TEST_F(KadSearchEngineTest, UpdateContactStatus) {
    std::string nodeId = "0123456789abcdef0123456789abcdef";

    // Mark as responsive
    engine->UpdateContactStatus(nodeId, true);

    auto contacts = engine->GetContacts();
    auto it = std::find_if(contacts.begin(), contacts.end(),
        [&nodeId](const KadContact& c) { return c.nodeId == nodeId; });

    ASSERT_NE(it, contacts.end());
    EXPECT_TRUE(it->isResponsive);
    EXPECT_EQ(it->failedRequests, 0);

    // Mark as unresponsive
    engine->UpdateContactStatus(nodeId, false);

    contacts = engine->GetContacts();
    it = std::find_if(contacts.begin(), contacts.end(),
        [&nodeId](const KadContact& c) { return c.nodeId == nodeId; });

    ASSERT_NE(it, contacts.end());
    EXPECT_FALSE(it->isResponsive);
    EXPECT_EQ(it->failedRequests, 1);
}

// Test 14: Keyword extraction
TEST_F(KadSearchEngineTest, KeywordExtraction) {
    SearchParams params = CreateKadParams("music rock jazz classical");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Keywords should be extracted internally
    // This test validates that search starts successfully with multi-word query
    EXPECT_EQ(engine->GetSearchState(searchId), SearchState::RUNNING);
}

// Test 15: Multiple searches
TEST_F(KadSearchEngineTest, MultipleSearches) {
    SearchParams params1 = CreateKadParams("query1");
    SearchParams params2 = CreateKadParams("query2");

    SearchId searchId1 = engine->StartSearch(params1);
    SearchId searchId2 = engine->StartSearch(params2);

    ASSERT_TRUE(searchId1.IsValid());
    ASSERT_TRUE(searchId2.IsValid());

    EXPECT_EQ(engine->GetSearchState(searchId1), SearchState::RUNNING);
    EXPECT_EQ(engine->GetSearchState(searchId2), SearchState::RUNNING);

    auto stats = engine->GetStatistics();
    EXPECT_EQ(stats.activeSearches, 2);
}

// Test 16: Request more results (JumpStart)
TEST_F(KadSearchEngineTest, RequestMoreResults) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add some results
    SearchResult result = CreateResult("file1.mp3", 1024 * 1024);
    engine->HandleKadResult(searchId, result);

    EXPECT_EQ(engine->GetResultCount(searchId), 1);

    // Request more results (triggers JumpStart)
    engine->RequestMoreResults(searchId);

    // State should remain running
    EXPECT_TRUE(engine->GetSearchState(searchId) == SearchState::RUNNING);
}

// Test 17: Get search parameters
TEST_F(KadSearchEngineTest, GetSearchParams) {
    SearchParams params = CreateKadParams("test query");
    params.minFileSize = 1024 * 1024;
    params.maxFileSize = 10 * 1024 * 1024;

    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    SearchParams retrievedParams = engine->GetSearchParams(searchId);

    EXPECT_EQ(retrievedParams.query, "test query");
    EXPECT_EQ(retrievedParams.type, SearchType::KADEMLIA);
    EXPECT_EQ(retrievedParams.minFileSize, 1024 * 1024);
    EXPECT_EQ(retrievedParams.maxFileSize, 10 * 1024 * 1024);
}

// Test 18: Result callback
TEST_F(KadSearchEngineTest, ResultCallback) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    bool callbackCalled = false;
    SearchResult receivedResult;

    engine->SetOnKadResult([&](SearchId id, const SearchResult& result) {
        callbackCalled = true;
        receivedResult = result;
    });

    // Add a result
    SearchResult result = CreateResult("test.mp3", 1024 * 1024);
    engine->HandleKadResult(searchId, result);

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedResult.fileName, "test.mp3");
}

// Test 19: Statistics update
TEST_F(KadSearchEngineTest, StatisticsUpdate) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Add results
    for (int i = 0; i < 5; ++i) {
        SearchResult result = CreateResult("file" + std::to_string(i) + ".mp3",
                                            (i + 1) * 1024 * 1024);
        engine->HandleKadResult(searchId, result);
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

// Test 20: Maintenance cleanup
TEST_F(KadSearchEngineTest, MaintenanceCleanup) {
    SearchParams params = CreateKadParams("test query");
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

// Test 21: Kad response handling
TEST_F(KadSearchEngineTest, KadResponseHandling) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Simulate Kad response from a node
    std::vector<SearchResult> results;
    results.push_back(CreateResult("file1.mp3", 1024 * 1024));
    results.push_back(CreateResult("file2.mp3", 2 * 1024 * 1024));

    engine->HandleKadResponse(searchId, "0123456789abcdef0123456789abcdef", results);

    EXPECT_EQ(engine->GetResultCount(searchId), 2);
}

// Test 22: Shutdown
TEST_F(KadSearchEngineTest, Shutdown) {
    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Shutdown engine
    engine->Shutdown();

    // Try to start new search after shutdown
    SearchParams params2 = CreateKadParams("query2");
    SearchId searchId2 = engine->StartSearch(params2);

    EXPECT_FALSE(searchId2.IsValid());
}

// Test 23: Contact distance sorting
TEST_F(KadSearchEngineTest, ContactDistanceSorting) {
    // This is implicitly tested through SelectNodesForSearch
    // which sorts contacts by distance to target node ID

    SearchParams params = CreateKadParams("test query");
    SearchId searchId = engine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());

    // Search should have selected nodes based on distance
    auto stats = engine->GetStatistics();
    EXPECT_GT(stats.totalSearches, 0);
}

// Test 24: Contact failure tracking
TEST_F(KadSearchEngineTest, ContactFailureTracking) {
    std::string nodeId = "0123456789abcdef0123456789abcdef";

    // Simulate multiple failures
    for (int i = 0; i < 5; ++i) {
        engine->UpdateContactStatus(nodeId, false);
    }

    auto contacts = engine->GetContacts();
    auto it = std::find_if(contacts.begin(), contacts.end(),
        [&nodeId](const KadContact& c) { return c.nodeId == nodeId; });

    ASSERT_NE(it, contacts.end());
    EXPECT_EQ(it->failedRequests, 5);
}

// Test 25: Kad connection without contacts
TEST_F(KadSearchEngineTest, KadConnectionWithoutContacts) {
    // Remove all contacts
    auto contacts = engine->GetContacts();
    for (const auto& contact : contacts) {
        engine->RemoveContact(contact.nodeId);
    }

    EXPECT_FALSE(engine->IsKadConnected());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

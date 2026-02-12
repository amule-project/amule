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
#include "search/unified/manager/UnifiedSearchManager.h"
#include <thread>
#include <chrono>

using namespace search;

class KadSearchIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        kadEngine = std::make_unique<KadSearchEngine>();

        // Add test contacts
        SetupTestContacts();

        // Set up result callback
        resultsReceived = 0;

        kadEngine->SetOnKadResult([this](SearchId id, const SearchResult& result) {
            resultsReceived++;
            lastResults.push_back(result);
        });
    }

    void TearDown() override {
        kadEngine->Shutdown();
        kadEngine.reset();
    }

    void SetupTestContacts() {
        KadContact contact1;
        contact1.nodeId = "0123456789abcdef0123456789abcdef";
        contact1.ip = 0x7F000001;
        contact1.port = 4671;
        contact1.isResponsive = true;
        contact1.failedRequests = 0;
        kadEngine->AddContact(contact1);

        KadContact contact2;
        contact2.nodeId = "fedcba9876543210fedcba9876543210";
        contact2.ip = 0x7F000002;
        contact2.port = 4672;
        contact2.isResponsive = true;
        contact2.failedRequests = 0;
        kadEngine->AddContact(contact2);

        KadContact contact3;
        contact3.nodeId = "abcdef0123456789abcdef0123456789";
        contact3.ip = 0x7F000003;
        contact3.port = 4673;
        contact3.isResponsive = true;
        contact3.failedRequests = 0;
        kadEngine->AddContact(contact3);
    }

    std::unique_ptr<KadSearchEngine> kadEngine;
    size_t resultsReceived;
    std::vector<SearchResult> lastResults;

    SearchParams CreateKadParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::KADEMLIA;
        params.query = query;
        params.maxResults = 100;
        params.timeout = std::chrono::seconds(30);
        return params;
    }

    SearchResult CreateResult(const std::string& fileName, uint64_t size) {
        SearchResult result;
        result.fileName = fileName;
        result.fileSize = size;
        result.fileHash = fileName;
        result.availability = 10;
        return result;
    }
};

// Test 1: Start and stop Kad search
TEST_F(KadSearchIntegrationTest, StartStopKadSearch) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    ASSERT_TRUE(searchId.IsValid());
    EXPECT_EQ(kadEngine->GetSearchState(searchId), SearchState::RUNNING);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    kadEngine->StopSearch(searchId);
    EXPECT_EQ(kadEngine->GetSearchState(searchId), SearchState::CANCELLED);
}

// Test 2: Multiple concurrent Kad searches
TEST_F(KadSearchIntegrationTest, MultipleConcurrentKadSearches) {
    std::vector<SearchId> searchIds;

    for (int i = 0; i < 5; ++i) {
        SearchParams params = CreateKadParams("query" + std::to_string(i));
        SearchId searchId = kadEngine->StartSearch(params);
        searchIds.push_back(searchId);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    for (const auto& searchId : searchIds) {
        kadEngine->StopSearch(searchId);
    }

    auto stats = kadEngine->GetStatistics();
    EXPECT_GE(stats.totalSearches, 5);
}

// Test 3: Kad search with filters
TEST_F(KadSearchIntegrationTest, KadSearchWithFilters) {
    SearchParams params = CreateKadParams("music");
    params.minFileSize = 1024 * 1024;
    params.maxFileSize = 100 * 1024 * 1024;
    params.fileTypes = {"mp3", "flac", "ogg"};

    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    SearchResult result1;
    result1.fileName = "song.mp3";
    result1.fileSize = 5 * 1024 * 1024;
    result1.fileHash = "hash1";
    result1.availability = 10;

    kadEngine->HandleKadResult(searchId, result1);

    SearchResult result2;
    result2.fileName = "tiny.mp3";
    result2.fileSize = 512 * 1024;
    result2.fileHash = "hash2";
    result2.availability = 5;

    kadEngine->HandleKadResult(searchId, result2);

    SearchResult result3;
    result3.fileName = "huge.flac";
    result3.fileSize = 200 * 1024 * 1024;
    result3.fileHash = "hash3";
    result3.availability = 15;

    kadEngine->HandleKadResult(searchId, result3);

    size_t resultCount = kadEngine->GetResultCount(searchId);
    EXPECT_EQ(resultCount, 1);

    kadEngine->StopSearch(searchId);
}

// Test 4: Result deduplication across nodes
TEST_F(KadSearchIntegrationTest, ResultDeduplicationAcrossNodes) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    SearchResult result;
    result.fileName = "song.mp3";
    result.fileSize = 5 * 1024 * 1024;
    result.fileHash = "same_hash";
    result.availability = 10;

    kadEngine->HandleKadResult(searchId, result);
    kadEngine->HandleKadResult(searchId, result);
    kadEngine->HandleKadResult(searchId, result);

    size_t resultCount = kadEngine->GetResultCount(searchId);
    EXPECT_EQ(resultCount, 1);

    kadEngine->StopSearch(searchId);
}

// Test 5: Node selection based on distance
TEST_F(KadSearchIntegrationTest, NodeSelectionBasedOnDistance) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(kadEngine->GetSearchState(searchId), SearchState::RUNNING);

    kadEngine->StopSearch(searchId);
}

// Test 6: Pause and resume Kad search
TEST_F(KadSearchIntegrationTest, PauseResumeKadSearch) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    kadEngine->PauseSearch(searchId);
    EXPECT_EQ(kadEngine->GetSearchState(searchId), SearchState::PAUSED);

    kadEngine->ResumeSearch(searchId);
    EXPECT_EQ(kadEngine->GetSearchState(searchId), SearchState::RUNNING);

    kadEngine->StopSearch(searchId);
}

// Test 7: Request more results (JumpStart)
TEST_F(KadSearchIntegrationTest, RequestMoreResults) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (int i = 0; i < 10; ++i) {
        SearchResult result;
        result.fileName = "song" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        kadEngine->HandleKadResult(searchId, result);
    }

    size_t initialCount = kadEngine->GetResultCount(searchId);
    EXPECT_EQ(initialCount, 10);

    kadEngine->RequestMoreResults(searchId);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (int i = 10; i < 20; ++i) {
        SearchResult result;
        result.fileName = "song" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        kadEngine->HandleKadResult(searchId, result);
    }

    size_t finalCount = kadEngine->GetResultCount(searchId);
    EXPECT_EQ(finalCount, 20);

    kadEngine->StopSearch(searchId);
}

// Test 8: Kad response handling with multiple results
TEST_F(KadSearchIntegrationTest, KadResponseHandlingWithMultipleResults) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<SearchResult> results;
    for (int i = 0; i < 5; ++i) {
        SearchResult result = CreateResult("file" + std::to_string(i) + ".mp3",
                                            (i + 1) * 1024 * 1024);
        results.push_back(result);
    }

    kadEngine->HandleKadResponse(searchId, "0123456789abcdef0123456789abcdef", results);

    EXPECT_EQ(kadEngine->GetResultCount(searchId), 5);

    kadEngine->StopSearch(searchId);
}

// Test 9: Contact status updates during search
TEST_F(KadSearchIntegrationTest, ContactStatusUpdatesDuringSearch) {
    SearchParams params = CreateKadParams("music");
    SearchId searchId = kadEngine->StartSearch(params);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Simulate responsive node
    kadEngine->HandleKadResponse(searchId, "0123456789abcdef0123456789abcdef",
                                  {CreateResult("file.mp3", 1024 * 1024)});

    auto contacts = kadEngine->GetContacts();
    auto it = std::find_if(contacts.begin(), contacts.end(),
        [](const KadContact& c) { return c.nodeId == "0123456789abcdef0123456789abcdef"; });

    ASSERT_NE(it, contacts.end());
    EXPECT_EQ(it->failedRequests, 0);

    kadEngine->StopSearch(searchId);
}

// Test 10: High volume Kad searches
TEST_F(KadSearchIntegrationTest, HighVolumeKadSearches) {
    std::vector<SearchId> searchIds;

    for (int i = 0; i < 20; ++i) {
        SearchParams params = CreateKadParams("query" + std::to_string(i));
        SearchId searchId = kadEngine->StartSearch(params);
        searchIds.push_back(searchId);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    for (const auto& searchId : searchIds) {
        kadEngine->StopSearch(searchId);
    }

    auto stats = kadEngine->GetStatistics();
    EXPECT_GE(stats.totalSearches, 20);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

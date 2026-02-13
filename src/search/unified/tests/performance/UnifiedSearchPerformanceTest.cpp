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
#include "search/unified/manager/UnifiedSearchManager.h"
#include "search/unified/engines/local/LocalSearchEngine.h"
#include "search/unified/engines/global/GlobalSearchEngine.h"
#include "search/unified/engines/kad/KadSearchEngine.h"
#include <thread>
#include <chrono>
#include <vector>
#include <random>

using namespace search;

/**
 * Performance benchmarking tests for unified search architecture
 * These tests measure performance characteristics under various loads
 */
class UnifiedSearchPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<UnifiedSearchManager>();
        manager->Start();

        // Set up result callback for benchmarking
        resultsReceived = 0;
        totalLatency = 0;

        manager->SetEventCallback([this](const SearchEvent& event) {
            if (event.type == SearchEvent::Type::RESULTS_RECEIVED) {
                resultsReceived += event.results.size();
            }
        });

        // Setup test data
        SetupTestData();
    }

    void TearDown() override {
        manager->Shutdown();
        manager.reset();
    }

    void SetupTestData() {
        // Setup Kad contacts for testing
        kadEngine = std::make_unique<KadSearchEngine>();
        for (int i = 0; i < 50; ++i) {
            KadContact contact;
            contact.nodeId = GenerateRandomNodeId();
            contact.ip = 0x7F000001 + (i % 255);
            contact.port = 4671 + i;
            contact.isResponsive = true;
            kadEngine->AddContact(contact);
        }

        // Setup servers for testing
        globalEngine = std::make_unique<GlobalSearchEngine>();
        for (int i = 0; i < 20; ++i) {
            ServerInfo server;
            server.ip = 0x7F000001 + (i % 255);
            server.port = 4661 + i;
            server.name = "TestServer" + std::to_string(i);
            server.isConnected = true;
            server.userCount = 1000 + (i * 100);
            server.fileCount = 100000 + (i * 10000);
            globalEngine->AddServer(server);
        }
    }

    std::string GenerateRandomNodeId() {
        std::string nodeId;
        const char hexChars[] = "0123456789abcdef";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        for (int i = 0; i < 32; ++i) {
            nodeId += hexChars[dis(gen)];
        }
        return nodeId;
    }

    SearchParams CreateLocalParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = query;
        params.maxResults = 500;
        params.timeout = std::chrono::seconds(30);
        return params;
    }

    SearchParams CreateGlobalParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::GLOBAL;
        params.query = query;
        params.maxResults = 500;
        params.timeout = std::chrono::seconds(30);
        return params;
    }

    SearchParams CreateKadParams(const std::string& query) {
        SearchParams params;
        params.type = SearchType::KADEMLIA;
        params.query = query;
        params.maxResults = 500;
        params.timeout = std::chrono::seconds(30);
        return params;
    }

    std::unique_ptr<UnifiedSearchManager> manager;
    std::unique_ptr<KadSearchEngine> kadEngine;
    std::unique_ptr<GlobalSearchEngine> globalEngine;
    size_t resultsReceived;
    int64_t totalLatency;
};

// Test 1: Single search latency
TEST_F(UnifiedSearchPerformanceTest, SingleSearchLatency) {
    SearchParams params = CreateLocalParams("test query");
    SearchCommand cmd = SearchCommand::StartSearch(params);

    auto start = std::chrono::high_resolution_clock::now();
    ASSERT_TRUE(manager->SendCommand(cmd));
    auto end = std::chrono::high_resolution_clock::now();

    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "Single search latency: " << latency << " microseconds" << std::endl;

    // Should complete within 100ms
    EXPECT_LT(latency, 100000);
}

// Test 2: Concurrent search throughput
TEST_F(UnifiedSearchPerformanceTest, ConcurrentSearchThroughput) {
    const int numSearches = 100;
    std::vector<SearchId> searchIds;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numSearches; ++i) {
        SearchParams params = CreateLocalParams("query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        ASSERT_TRUE(manager->SendCommand(cmd));
        searchIds.push_back(cmd.searchId);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Started " << numSearches << " searches in " << duration << " ms" << std::endl;
    std::cout << "Throughput: " << (numSearches * 1000.0 / duration) << " searches/second" << std::endl;

    // Should be able to start 100 searches within 1 second
    EXPECT_LT(duration, 1000);
}

// Test 3: Memory usage during high load
TEST_F(UnifiedSearchPerformanceTest, MemoryUsageHighLoad) {
    const int numSearches = 1000;
    std::vector<SearchId> searchIds;

    // Record initial memory (this is a simplified check)
    size_t initialSearches = manager->GetStatistics().totalSearches;

    for (int i = 0; i < numSearches; ++i) {
        SearchParams params = CreateLocalParams("query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        manager->SendCommand(cmd);
        searchIds.push_back(cmd.searchId);
    }

    auto stats = manager->GetStatistics();
    std::cout << "Total searches: " << stats.totalSearches << std::endl;
    std::cout << "Active searches: " << stats.activeSearches << std::endl;

    EXPECT_EQ(stats.totalSearches, initialSearches + numSearches);

    // Cleanup
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }
}

// Test 4: Result processing latency
TEST_F(UnifiedSearchPerformanceTest, ResultProcessingLatency) {
    SearchParams params = CreateLocalParams("test query");
    SearchCommand cmd = SearchCommand::StartSearch(params);
    ASSERT_TRUE(manager->SendCommand(cmd));

    // Simulate receiving many results
    const int numResults = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numResults; ++i) {
        SearchResult result;
        result.fileName = "file" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        // Results would be processed internally
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Processed " << numResults << " results in " << duration << " ms" << std::endl;
    std::cout << "Result processing rate: " << (numResults * 1000.0 / duration) << " results/second" << std::endl;

    // Should process 10000 results within 1 second
    EXPECT_LT(duration, 1000);
}

// Test 5: Command queue throughput
TEST_F(UnifiedSearchPerformanceTest, CommandQueueThroughput) {
    const int numCommands = 10000;
    std::vector<SearchCommand> commands;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numCommands; ++i) {
        SearchCommand cmd;
        cmd.type = SearchCommand::Type::GET_SEARCH_STATE;
        cmd.searchId = SearchId{static_cast<uint64_t>(i + 1)};
        commands.push_back(cmd);
        manager->SendCommand(cmd);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Queued " << numCommands << " commands in " << duration << " ms" << std::endl;
    std::cout << "Command throughput: " << (numCommands * 1000.0 / duration) << " commands/second" << std::endl;

    // Should queue 10000 commands within 1 second
    EXPECT_LT(duration, 1000);
}

// Test 6: Serialization performance
TEST_F(UnifiedSearchPerformanceTest, SerializationPerformance) {
    const int numResults = 10000;
    std::vector<SearchResult> results;

    // Create test results
    for (int i = 0; i < numResults; ++i) {
        SearchResult result;
        result.fileName = "file" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        results.push_back(result);
    }

    // Measure serialization time
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& result : results) {
        auto serialized = result.Serialize();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Serialized " << numResults << " results in " << duration << " ms" << std::endl;
    std::cout << "Serialization rate: " << (numResults * 1000.0 / duration) << " results/second" << std::endl;

    // Should serialize 10000 results within 1 second
    EXPECT_LT(duration, 1000);
}

// Test 7: Deserialization performance
TEST_F(UnifiedSearchPerformanceTest, DeserializationPerformance) {
    const int numResults = 10000;
    std::vector<std::vector<uint8_t>> serializedResults;

    // Create and serialize test results
    for (int i = 0; i < numResults; ++i) {
        SearchResult result;
        result.fileName = "file" + std::to_string(i) + ".mp3";
        result.fileSize = (i + 1) * 1024 * 1024;
        result.fileHash = "hash" + std::to_string(i);
        result.availability = 10;
        serializedResults.push_back(result.Serialize());
    }

    // Measure deserialization time
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& serialized : serializedResults) {
        SearchResult result = SearchResult::Deserialize(serialized);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Deserialized " << numResults << " results in " << duration << " ms" << std::endl;
    std::cout << "Deserialization rate: " << (numResults * 1000.0 / duration) << " results/second" << std::endl;

    // Should deserialize 10000 results within 1 second
    EXPECT_LT(duration, 1000);
}

// Test 8: Search ID generation performance
TEST_F(UnifiedSearchPerformanceTest, SearchIdGenerationPerformance) {
    const int numIds = 1000000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numIds; ++i) {
        SearchId id = SearchId::Generate();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Generated " << numIds << " search IDs in " << duration << " ms" << std::endl;
    std::cout << "ID generation rate: " << (numIds * 1000.0 / duration) << " IDs/second" << std::endl;

    // Should generate 1M IDs within 1 second
    EXPECT_LT(duration, 1000);
}

// Test 9: Search statistics performance
TEST_F(UnifiedSearchPerformanceTest, SearchStatisticsPerformance) {
    const int numSearches = 100;
    std::vector<SearchId> searchIds;

    // Start searches
    for (int i = 0; i < numSearches; ++i) {
        SearchParams params = CreateLocalParams("query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        manager->SendCommand(cmd);
        searchIds.push_back(cmd.searchId);
    }

    // Measure statistics retrieval time
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 1000; ++i) {
        auto stats = manager->GetStatistics();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Retrieved statistics 1000 times in " << duration << " ms" << std::endl;
    std::cout << "Statistics retrieval rate: " << (1000 * 1000.0 / duration) << " retrievals/second" << std::endl;

    // Should retrieve statistics 1000 times within 100ms
    EXPECT_LT(duration, 100);

    // Cleanup
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }
}

// Test 10: Mixed search types performance
TEST_F(UnifiedSearchPerformanceTest, MixedSearchTypesPerformance) {
    const int numSearches = 30; // 10 of each type
    std::vector<SearchId> searchIds;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numSearches; ++i) {
        SearchParams params;
        if (i % 3 == 0) {
            params = CreateLocalParams("local query " + std::to_string(i));
        } else if (i % 3 == 1) {
            params = CreateGlobalParams("global query " + std::to_string(i));
        } else {
            params = CreateKadParams("kad query " + std::to_string(i));
        }

        SearchCommand cmd = SearchCommand::StartSearch(params);
        manager->SendCommand(cmd);
        searchIds.push_back(cmd.searchId);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Started " << numSearches << " mixed searches in " << duration << " ms" << std::endl;

    // Should start 30 mixed searches within 500ms
    EXPECT_LT(duration, 500);

    // Cleanup
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

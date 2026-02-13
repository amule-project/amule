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
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <random>

using namespace search;

/**
 * Load testing framework for unified search architecture
 * These tests measure system behavior under sustained high load
 */
class UnifiedSearchLoadTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<UnifiedSearchManager>();
        manager->Start();

        resultsReceived = 0;
        errorsReceived = 0;

        manager->SetEventCallback([this](const SearchEvent& event) {
            if (event.type == SearchEvent::Type::RESULTS_RECEIVED) {
                resultsReceived += event.results.size();
            } else if (event.type == SearchEvent::Type::ERROR_OCCURRED) {
                errorsReceived++;
            }
        });
    }

    void TearDown() override {
        manager->Shutdown();
        manager.reset();
    }

    SearchParams CreateParams(SearchType type, const std::string& query) {
        SearchParams params;
        params.type = type;
        params.query = query;
        params.maxResults = 500;
        params.timeout = std::chrono::seconds(30);
        return params;
    }

    std::unique_ptr<UnifiedSearchManager> manager;
    std::atomic<size_t> resultsReceived;
    std::atomic<size_t> errorsReceived;
};

// Test 1: Sustained load test - 100 concurrent searches
TEST_F(UnifiedSearchLoadTest, SustainedLoad100Concurrent) {
    const int numSearches = 100;
    const int durationSeconds = 10;
    std::vector<SearchId> searchIds;

    std::cout << "Starting sustained load test with " << numSearches << " concurrent searches..." << std::endl;

    // Start initial searches
    for (int i = 0; i < numSearches; ++i) {
        SearchParams params = CreateParams(SearchType::LOCAL, "query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        ASSERT_TRUE(manager->SendCommand(cmd));
        searchIds.push_back(cmd.searchId);
    }

    // Maintain load for duration
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(durationSeconds);

    int cycle = 0;
    while (std::chrono::steady_clock::now() < endTime) {
        // Stop some searches and start new ones
        for (int i = 0; i < 10; ++i) {
            size_t idx = (cycle * 10 + i) % searchIds.size();
            SearchCommand stopCmd = SearchCommand::StopSearch(searchIds[idx]);
            manager->SendCommand(stopCmd);

            SearchParams params = CreateParams(SearchType::LOCAL, "query" + std::to_string(cycle * 10 + i));
            SearchCommand startCmd = SearchCommand::StartSearch(params);
            ASSERT_TRUE(manager->SendCommand(startCmd));
            searchIds[idx] = startCmd.searchId;
        }

        cycle++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop all searches
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }

    auto stats = manager->GetStatistics();
    std::cout << "Load test completed:" << std::endl;
    std::cout << "  Total searches: " << stats.totalSearches << std::endl;
    std::cout << "  Errors received: " << errorsReceived << std::endl;
    std::cout << "  Results received: " << resultsReceived << std::endl;

    // Should have minimal errors
    EXPECT_LT(errorsReceived, 10);
}

// Test 2: Burst load test - rapid search creation
TEST_F(UnifiedSearchLoadTest, BurstLoadTest) {
    const int numBursts = 10;
    const int searchesPerBurst = 50;
    std::vector<SearchId> allSearchIds;

    std::cout << "Starting burst load test with " << numBursts << " bursts of " << searchesPerBurst << " searches..." << std::endl;

    for (int burst = 0; burst < numBursts; ++burst) {
        std::cout << "Starting burst " << (burst + 1) << "..." << std::endl;

        // Start burst of searches
        for (int i = 0; i < searchesPerBurst; ++i) {
            SearchParams params = CreateParams(SearchType::LOCAL, "burst" + std::to_string(burst) + "_query" + std::to_string(i));
            SearchCommand cmd = SearchCommand::StartSearch(params);
            ASSERT_TRUE(manager->SendCommand(cmd));
            allSearchIds.push_back(cmd.searchId);
        }

        // Wait briefly between bursts
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop all searches
    for (const auto& searchId : allSearchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }

    auto stats = manager->GetStatistics();
    std::cout << "Burst test completed:" << std::endl;
    std::cout << "  Total searches: " << stats.totalSearches << std::endl;
    std::cout << "  Expected: " << (numBursts * searchesPerBurst) << std::endl;

    EXPECT_EQ(stats.totalSearches, numBursts * searchesPerBurst);
}

// Test 3: Memory leak detection during load
TEST_F(UnifiedSearchLoadTest, MemoryLeakDetection) {
    const int numCycles = 100;
    const int searchesPerCycle = 10;

    std::cout << "Starting memory leak detection test..." << std::endl;

    for (int cycle = 0; cycle < numCycles; ++cycle) {
        std::vector<SearchId> searchIds;

        // Start searches
        for (int i = 0; i < searchesPerCycle; ++i) {
            SearchParams params = CreateParams(SearchType::LOCAL, "cycle" + std::to_string(cycle) + "_query" + std::to_string(i));
            SearchCommand cmd = SearchCommand::StartSearch(params);
            manager->SendCommand(cmd);
            searchIds.push_back(cmd.searchId);
        }

        // Stop searches
        for (const auto& searchId : searchIds) {
            SearchCommand cmd = SearchCommand::StopSearch(searchId);
            manager->SendCommand(cmd);
        }

        // Run maintenance to cleanup
        manager->ProcessMaintenance(std::chrono::system_clock::now());

        // Check that active searches are cleaned up
        auto stats = manager->GetStatistics();
        if (cycle % 10 == 0) {
            std::cout << "Cycle " << cycle << ": Active searches = " << stats.activeSearches << std::endl;
        }
    }

    // Final cleanup
    manager->ProcessMaintenance(std::chrono::system_clock::now());
    auto finalStats = manager->GetStatistics();

    std::cout << "Memory leak test completed:" << std::endl;
    std::cout << "  Total searches: " << finalStats.totalSearches << std::endl;
    std::cout << "  Active searches: " << finalStats.activeSearches << std::endl;
    std::cout << "  Completed searches: " << finalStats.completedSearches << std::endl;

    // All searches should be cleaned up
    EXPECT_EQ(finalStats.activeSearches, 0);
}

// Test 4: Concurrent access from multiple threads
TEST_F(UnifiedSearchLoadTest, MultiThreadedAccess) {
    const int numThreads = 10;
    const int searchesPerThread = 20;
    std::vector<std::thread> threads;
    std::vector<std::vector<SearchId>> threadSearchIds(numThreads);

    std::cout << "Starting multi-threaded access test with " << numThreads << " threads..." << std::endl;

    // Launch threads
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, searchesPerThread, &threadSearchIds]() {
            for (int i = 0; i < searchesPerThread; ++i) {
                SearchParams params = CreateParams(SearchType::LOCAL, "thread" + std::to_string(t) + "_query" + std::to_string(i));
                SearchCommand cmd = SearchCommand::StartSearch(params);
                if (manager->SendCommand(cmd)) {
                    threadSearchIds[t].push_back(cmd.searchId);

                    // Simulate some operations
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                    // Get statistics
                    auto stats = manager->GetStatistics();

                    // Stop search
                    SearchCommand stopCmd = SearchCommand::StopSearch(cmd.searchId);
                    manager->SendCommand(stopCmd);
                }
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    auto stats = manager->GetStatistics();
    std::cout << "Multi-threaded test completed:" << std::endl;
    std::cout << "  Total searches: " << stats.totalSearches << std::endl;
    std::cout << "  Expected: " << (numThreads * searchesPerThread) << std::endl;

    EXPECT_EQ(stats.totalSearches, numThreads * searchesPerThread);
}

// Test 5: Long-running stability test
TEST_F(UnifiedSearchLoadTest, LongRunningStability) {
    const int durationMinutes = 2;  // 2 minutes for testing
    const int concurrentSearches = 20;
    std::vector<SearchId> searchIds;

    std::cout << "Starting long-running stability test for " << durationMinutes << " minutes..." << std::endl;

    // Start initial searches
    for (int i = 0; i < concurrentSearches; ++i) {
        SearchParams params = CreateParams(SearchType::LOCAL, "stability_query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);
        ASSERT_TRUE(manager->SendCommand(cmd));
        searchIds.push_back(cmd.searchId);
    }

    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::minutes(durationMinutes);

    int cycle = 0;
    while (std::chrono::steady_clock::now() < endTime) {
        // Rotate searches
        size_t idx = cycle % searchIds.size();
        SearchCommand stopCmd = SearchCommand::StopSearch(searchIds[idx]);
        manager->SendCommand(stopCmd);

        SearchParams params = CreateParams(SearchType::LOCAL, "stability_query" + std::to_string(cycle));
        SearchCommand startCmd = SearchCommand::StartSearch(params);
        ASSERT_TRUE(manager->SendCommand(startCmd));
        searchIds[idx] = startCmd.searchId;

        cycle++;

        // Check for errors
        if (errorsReceived > 0) {
            std::cout << "Error detected at cycle " << cycle << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stop all searches
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }

    auto stats = manager->GetStatistics();
    std::cout << "Stability test completed:" << std::endl;
    std::cout << "  Total searches: " << stats.totalSearches << std::endl;
    std::cout << "  Errors received: " << errorsReceived << std::endl;
    std::cout << "  Cycles completed: " << cycle << std::endl;

    // Should have minimal errors
    EXPECT_LT(errorsReceived, 5);
}

// Test 6: Extreme load test - 1000 concurrent searches
TEST_F(UnifiedSearchLoadTest, ExtremeLoadTest) {
    const int numSearches = 1000;
    std::vector<SearchId> searchIds;

    std::cout << "Starting extreme load test with " << numSearches << " concurrent searches..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // Start all searches
    for (int i = 0; i < numSearches; ++i) {
        SearchParams params = CreateParams(SearchType::LOCAL, "extreme_query" + std::to_string(i));
        SearchCommand cmd = SearchCommand::StartSearch(params);

        if (!manager->SendCommand(cmd)) {
            std::cout << "Failed to start search " << i << std::endl;
        } else {
            searchIds.push_back(cmd.searchId);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Started " << searchIds.size() << " searches in " << duration << " ms" << std::endl;

    // Wait briefly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop all searches
    for (const auto& searchId : searchIds) {
        SearchCommand cmd = SearchCommand::StopSearch(searchId);
        manager->SendCommand(cmd);
    }

    auto stats = manager->GetStatistics();
    std::cout << "Extreme load test completed:" << std::endl;
    std::cout << "  Searches started: " << searchIds.size() << std::endl;
    std::cout << "  Errors received: " << errorsReceived << std::endl;

    // Should start most searches successfully
    EXPECT_GT(searchIds.size(), numSearches * 0.9);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

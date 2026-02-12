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
#include "../../manager/UnifiedSearchManager.h"
#include "../../core/SearchTypes.h"
#include "../../core/SearchParams.h"
#include "../../core/SearchCommand.h"
#include "../../core/SearchEvent.h"
#include <thread>
#include <chrono>

using namespace search;

/**
 * Integration tests for multiple concurrent searches
 * Tests that searches of different types don't interfere with each other
 */
class MultiSearchIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<UnifiedSearchManager>();
        manager->Start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void TearDown() override {
        if (manager) {
            manager->Shutdown();
        }
    }

    std::unique_ptr<UnifiedSearchManager> manager;

    // Helper to wait for event
    bool WaitForEvent(std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (eventReceived) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return false;
    }

    bool eventReceived = false;
    SearchEvent lastEvent;
};

TEST_F(MultiSearchIntegrationTest, MultipleLocalSearches) {
    const int numSearches = 10;
    std::vector<SearchId> searchIds;
    int startedCount = 0;
    int completedCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completedCount++;
        }
    });

    // Start multiple local searches
    for (int i = 0; i < numSearches; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    // Wait for all to complete
    auto start = std::chrono::steady_clock::now();
    while (completedCount < numSearches && 
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(5000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(startedCount, numSearches);
    EXPECT_EQ(completedCount, numSearches);
    EXPECT_EQ(searchIds.size(), numSearches);

    // Verify all IDs are unique
    std::set<uint64_t> uniqueIds;
    for (const auto& id : searchIds) {
        EXPECT_TRUE(uniqueIds.insert(id.value).second);
    }
}

TEST_F(MultiSearchIntegrationTest, MixedSearchTypes) {
    std::vector<SearchId> searchIds;
    int startedCount = 0;
    int completedCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completedCount++;
        }
    });

    // Start searches of different types
    // Local searches
    for (int i = 0; i < 3; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "local" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    // Global searches (stubs)
    for (int i = 0; i < 2; ++i) {
        SearchParams params;
        params.type = SearchType::GLOBAL;
        params.query = "global" + std::to_string(i);
        params.maxResults = 10;
        params.globalParams = SearchParams::GlobalParams{};
        params.globalParams->serverIp = 0x7F000001;
        params.globalParams->serverPort = 4662;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    // Kad searches (stubs)
    for (int i = 0; i < 2; ++i) {
        SearchParams params;
        params.type = SearchType::KADEMLIA;
        params.query = "kad" + std::to_string(i);
        params.maxResults = 10;
        params.kadParams = SearchParams::KadParams{};
        params.kadParams->keywordHash = "HASH" + std::to_string(i);
        params.kadParams->maxNodes = 500;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    // Wait for all to complete
    auto start = std::chrono::steady_clock::now();
    while (completedCount < 7 && 
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(5000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(startedCount, 7);
    EXPECT_EQ(completedCount, 7);
    EXPECT_EQ(searchIds.size(), 7);
}

TEST_F(MultiSearchIntegrationTest, StopSpecificSearch) {
    std::vector<SearchId> searchIds;
    int startedCount = 0;
    int completedCount = 0;
    int cancelledCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_CANCELLED) {
            cancelledCount++;
        }
    });

    // Start 5 searches
    for (int i = 0; i < 5; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop the second search
    if (searchIds.size() >= 2) {
        SearchCommand stopCmd = SearchCommand::StopSearch(searchIds[1]);
        EXPECT_TRUE(manager->SendCommand(stopCmd));
    }

    // Wait for remaining to complete
    auto start = std::chrono::steady_clock::now();
    while ((completedCount < 4 || cancelledCount < 1) && 
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(3000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(startedCount, 5);
    EXPECT_GE(completedCount, 4);
    EXPECT_EQ(cancelledCount, 1);
}

TEST_F(MultiSearchIntegrationTest, GetResultsForSpecificSearch) {
    std::vector<SearchId> searchIds;
    int startedCount = 0;
    int completedCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completedCount++;
        }
    });

    // Start 3 searches
    for (int i = 0; i < 3; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    // Wait for all to complete
    auto start = std::chrono::steady_clock::now();
    while (completedCount < 3 && 
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(3000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(startedCount, 3);
    EXPECT_EQ(completedCount, 3);
    EXPECT_EQ(searchIds.size(), 3);

    // Get results for the second search
    if (searchIds.size() >= 2) {
        std::vector<uint8_t> response;
        bool gotResponse = false;

        SearchCommand getCmd = SearchCommand::GetResults(searchIds[1], 10);
        getCmd.responseCallback = [&](const std::vector<uint8_t>& resp) {
            response = resp;
            gotResponse = true;
        };

        EXPECT_TRUE(manager->SendCommand(getCmd));

        start = std::chrono::steady_clock::now();
        while (!gotResponse && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1000)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        EXPECT_TRUE(gotResponse);
    }
}

TEST_F(MultiSearchIntegrationTest, HighVolumeSearches) {
    const int numSearches = 50;
    std::vector<SearchId> searchIds;
    int startedCount = 0;
    int completedCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completedCount++;
        }
    });

    // Start many searches rapidly
    for (int i = 0; i < numSearches; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 5;  // Limit results to reduce load

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    // Wait for all to complete
    auto start = std::chrono::steady_clock::now();
    while (completedCount < numSearches && 
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(10000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(startedCount, numSearches);
    EXPECT_EQ(completedCount, numSearches);
    EXPECT_EQ(searchIds.size(), numSearches);

    // Verify all IDs are unique
    std::set<uint64_t> uniqueIds;
    for (const auto& id : searchIds) {
        EXPECT_TRUE(uniqueIds.insert(id.value).second);
    }
    EXPECT_EQ(uniqueIds.size(), numSearches);

    // Check statistics
    auto stats = manager->GetStatistics();
    EXPECT_GE(stats.totalSearches, numSearches);
    EXPECT_GE(stats.completedSearches, numSearches);
}

TEST_F(MultiSearchIntegrationTest, SearchLifecycle) {
    std::vector<SearchId> searchIds;
    int startedCount = 0;
    int completedCount = 0;
    int cancelledCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completedCount++;
        } else if (event.type == SearchEvent::Type::SEARCH_CANCELLED) {
            cancelledCount++;
        }
    });

    // Start 5 searches
    for (int i = 0; i < 5; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop first 2 searches
    if (searchIds.size() >= 2) {
        SearchCommand stopCmd1 = SearchCommand::StopSearch(searchIds[0]);
        SearchCommand stopCmd2 = SearchCommand::StopSearch(searchIds[1]);
        EXPECT_TRUE(manager->SendCommand(stopCmd1));
        EXPECT_TRUE(manager->SendCommand(stopCmd2));
    }

    // Wait for remaining to complete
    auto start = std::chrono::steady_clock::now();
    while ((completedCount < 3 || cancelledCount < 2) && 
           std::chrono::steady_clock::now() - start < std::chrono::milliseconds(3000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(startedCount, 5);
    EXPECT_GE(completedCount, 3);
    EXPECT_EQ(cancelledCount, 2);
}

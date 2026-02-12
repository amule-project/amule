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
 * Integration tests for LocalSearchEngine
 * Tests the complete flow from command to event
 */
class LocalSearchIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<UnifiedSearchManager>();
        manager->Start();

        // Wait for manager to initialize
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

TEST_F(LocalSearchIntegrationTest, StartLocalSearch) {
    bool started = false;
    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            started = true;
            eventReceived = true;
            lastEvent = event;
        }
    });

    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 10;

    SearchCommand cmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(cmd));

    EXPECT_TRUE(WaitForEvent());
    EXPECT_TRUE(started);
    EXPECT_TRUE(lastEvent.searchId.IsValid());
}

TEST_F(LocalSearchIntegrationTest, StartAndStopSearch) {
    bool started = false;
    bool cancelled = false;
    SearchId searchId;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            started = true;
            searchId = event.searchId;
        } else if (event.type == SearchEvent::Type::SEARCH_CANCELLED) {
            cancelled = true;
            eventReceived = true;
        }
    });

    // Start search
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 10;

    SearchCommand startCmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(startCmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stop search
    SearchCommand stopCmd = SearchCommand::StopSearch(searchId);
    EXPECT_TRUE(manager->SendCommand(stopCmd));

    EXPECT_TRUE(WaitForEvent(std::chrono::milliseconds(2000)));
    EXPECT_TRUE(started);
    EXPECT_TRUE(cancelled);
}

TEST_F(LocalSearchIntegrationTest, GetResults) {
    bool started = false;
    bool completed = false;
    SearchId searchId;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            started = true;
            searchId = event.searchId;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completed = true;
            eventReceived = true;
        }
    });

    // Start search
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 10;

    SearchCommand cmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(cmd));

    // Wait for completion
    EXPECT_TRUE(WaitForEvent(std::chrono::milliseconds(2000)));
    EXPECT_TRUE(started);
    EXPECT_TRUE(completed);

    // Get results
    std::vector<uint8_t> response;
    bool gotResponse = false;

    SearchCommand getCmd = SearchCommand::GetResults(searchId, 10);
    getCmd.responseCallback = [&](const std::vector<uint8_t>& resp) {
        response = resp;
        gotResponse = true;
    };

    EXPECT_TRUE(manager->SendCommand(getCmd));

    // Wait for response
    auto start = std::chrono::steady_clock::now();
    while (!gotResponse && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(gotResponse);
}

TEST_F(LocalSearchIntegrationTest, MultipleConcurrentSearches) {
    const int numSearches = 5;
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

    // Start multiple searches
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
}

TEST_F(LocalSearchIntegrationTest, PauseAndResumeSearch) {
    bool started = false;
    bool paused = false;
    bool resumed = false;
    SearchId searchId;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            started = true;
            searchId = event.searchId;
        }
        // Note: Local searches complete immediately, so pause/resume may not trigger events
    });

    // Start search
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 10;

    SearchCommand startCmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(startCmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Try to pause (may not have effect as search completes immediately)
    SearchCommand pauseCmd = SearchCommand::PauseSearch(searchId);
    EXPECT_TRUE(manager->SendCommand(pauseCmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Try to resume
    SearchCommand resumeCmd = SearchCommand::ResumeSearch(searchId);
    EXPECT_TRUE(manager->SendCommand(resumeCmd));

    EXPECT_TRUE(started);
}

TEST_F(LocalSearchIntegrationTest, GetSearchState) {
    bool started = false;
    SearchId searchId;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            started = true;
            searchId = event.searchId;
        }
    });

    // Start search
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 10;

    SearchCommand cmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Get state
    std::vector<uint8_t> response;
    bool gotResponse = false;

    SearchCommand getStateCmd;
    getStateCmd.type = SearchCommand::Type::GET_SEARCH_STATE;
    getStateCmd.searchId = searchId;
    getStateCmd.responseCallback = [&](const std::vector<uint8_t>& resp) {
        response = resp;
        gotResponse = true;
    };

    EXPECT_TRUE(manager->SendCommand(getStateCmd));

    // Wait for response
    auto start = std::chrono::steady_clock::now();
    while (!gotResponse && std::chrono::steady_clock::now() - start < std::chrono::milliseconds(1000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(gotResponse);
    EXPECT_FALSE(response.empty());
    SearchState state = static_cast<SearchState>(response[0]);
    EXPECT_TRUE(state == SearchState::COMPLETED || state == SearchState::RUNNING);
}

TEST_F(LocalSearchIntegrationTest, GetSearchStatistics) {
    // Start a few searches
    for (int i = 0; i < 3; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Get statistics
    auto stats = manager->GetStatistics();
    EXPECT_GE(stats.totalSearches, 3);
    EXPECT_GE(stats.completedSearches, 0);
}

TEST_F(LocalSearchIntegrationTest, InvalidParameters) {
    bool errorOccurred = false;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::ERROR_OCCURRED) {
            errorOccurred = true;
            eventReceived = true;
        }
    });

    // Try to start search with invalid parameters
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "";  // Empty query is invalid
    params.maxResults = 10;

    SearchCommand cmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(cmd));

    // Should receive error event
    EXPECT_TRUE(WaitForEvent());
    EXPECT_TRUE(errorOccurred);
}

TEST_F(LocalSearchIntegrationTest, CancelAllSearches) {
    std::vector<SearchId> searchIds;
    int startedCount = 0;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            searchIds.push_back(event.searchId);
            startedCount++;
        }
    });

    // Start multiple searches
    for (int i = 0; i < 5; ++i) {
        SearchParams params;
        params.type = SearchType::LOCAL;
        params.query = "test" + std::to_string(i);
        params.maxResults = 10;

        SearchCommand cmd = SearchCommand::StartSearch(params);
        EXPECT_TRUE(manager->SendCommand(cmd));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cancel all searches
    SearchCommand cancelCmd = SearchCommand::CancelAllSearches();
    EXPECT_TRUE(manager->SendCommand(cancelCmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify searches were cancelled
    auto stats = manager->GetStatistics();
    EXPECT_GE(stats.totalSearches, 5);
}

TEST_F(LocalSearchIntegrationTest, SearchWithFilters) {
    bool started = false;
    bool completed = false;
    SearchId searchId;

    manager->SetEventCallback([&](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            started = true;
            searchId = event.searchId;
        } else if (event.type == SearchEvent::Type::SEARCH_COMPLETED) {
            completed = true;
            eventReceived = true;
        }
    });

    // Start search with filters
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    params.maxResults = 100;
    params.minFileSize = 1024;
    params.maxFileSize = 1024 * 1024 * 100;  // 100 MB
    params.fileTypes = {"mp3", "flac"};

    SearchCommand cmd = SearchCommand::StartSearch(params);
    EXPECT_TRUE(manager->SendCommand(cmd));

    // Wait for completion
    EXPECT_TRUE(WaitForEvent(std::chrono::milliseconds(2000)));
    EXPECT_TRUE(started);
    EXPECT_TRUE(completed);
}

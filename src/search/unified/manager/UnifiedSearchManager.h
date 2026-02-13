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

#ifndef UNIFIED_SEARCH_MANAGER_H
#define UNIFIED_SEARCH_MANAGER_H

#include "../core/SearchTypes.h"
#include "../core/SearchId.h"
#include "../core/SearchParams.h"
#include "../core/SearchResult.h"
#include "../core/SearchCommand.h"
#include "../core/SearchEvent.h"
#include "../engines/ISearchEngine.h"
#include "SearchCacheManager.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>

namespace search {

/**
 * Central manager for all search operations
 * Runs in its own thread, coordinating all search engines
 * All search operations are single-threaded within this manager
 */
class UnifiedSearchManager {
public:
    struct Config {
        std::chrono::milliseconds maintenanceInterval{1000};
        std::chrono::milliseconds commandTimeout{5000};
        size_t maxConcurrentSearches{10};
        size_t maxResultsPerSearch{500};
        bool enableSearchCache{true};  // Enable search reuse
    };

    explicit UnifiedSearchManager(const Config& config = Config{});
    ~UnifiedSearchManager();

    // Non-copyable, non-movable
    UnifiedSearchManager(const UnifiedSearchManager&) = delete;
    UnifiedSearchManager& operator=(const UnifiedSearchManager&) = delete;
    UnifiedSearchManager(UnifiedSearchManager&&) = delete;
    UnifiedSearchManager& operator=(UnifiedSearchManager&&) = delete;

    /**
     * Start the search manager and its worker thread
     */
    void Start();

    /**
     * Shutdown the search manager and wait for completion
     */
    void Shutdown();

    /**
     * Check if the manager is running
     */
    bool IsRunning() const;

    /**
     * Send a command to the search thread
     * Thread-safe: can be called from any thread
     * @param command Command to send
     * @return true if command was queued successfully
     */
    bool SendCommand(const SearchCommand& command);

    /**
     * Register a callback for search events
     * Callback will be called in the UI thread
     * @param callback Function to call with events
     */
    void SetEventCallback(std::function<void(const SearchEvent&)> callback);

    /**
     * Get current statistics
     */
    struct Statistics {
        size_t activeSearches;
        size_t completedSearches;
        size_t failedSearches;
        size_t totalResults;
        std::chrono::system_clock::time_point startTime;
    };

    Statistics GetStatistics() const;

    /**
     * Get configuration
     */
    const Config& GetConfig() const { return m_config; }

 private:
    // Worker thread function
    void WorkerThread();

    // Process a single command
    void ProcessCommand(const SearchCommand& command);

    // Send event to UI thread
    void SendEvent(const SearchEvent& event);

    // Periodic maintenance
    void PerformMaintenance();

    // Engine management
    void InitializeEngines();
    void CleanupEngines();

    // Engine selection
    ISearchEngine* SelectEngine(SearchType type);

    // Configuration
    Config m_config;

    // Thread management
    std::thread m_workerThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdownRequested{false};

    // Command queue (thread-safe)
    mutable std::mutex m_commandQueueMutex;
    std::condition_variable m_commandQueueCV;
    std::queue<SearchCommand> m_commandQueue;

    // Event callback (called in UI thread)
    std::function<void(const SearchEvent&)> m_eventCallback;
    mutable std::mutex m_eventCallbackMutex;

    // Search engines (only accessed in worker thread)
    std::unique_ptr<ISearchEngine> m_localSearchEngine;
    std::unique_ptr<ISearchEngine> m_globalSearchEngine;
    std::unique_ptr<ISearchEngine> m_kadSearchEngine;

    // Search cache manager
    std::unique_ptr<SearchCacheManager> m_cacheManager;

    // Search tracking (only accessed in worker thread)
    std::unordered_map<SearchId, SearchState> m_searchStates;
    std::unordered_map<SearchId, SearchParams> m_searchParams;
    std::unordered_map<SearchId, std::vector<SearchResult>> m_searchResults;

    // Statistics (only accessed in worker thread)
    Statistics m_statistics;
};

} // namespace search

#endif // UNIFIED_SEARCH_MANAGER_H

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

#include "UnifiedSearchManager.h"
#include "../engines/local/LocalSearchEngine.h"
#include "../engines/global/GlobalSearchEngine.h"
#include "../engines/kad/KadSearchEngine.h"

#include <iostream>
#include <sstream>

namespace search {

UnifiedSearchManager::UnifiedSearchManager(const Config& config)
    : m_config(config)
{
    m_statistics.startTime = std::chrono::system_clock::now();
}

UnifiedSearchManager::~UnifiedSearchManager()
{
    Shutdown();
}

void UnifiedSearchManager::Start()
{
    if (m_running.load()) {
        return; // Already running
    }

    m_running.store(true);
    m_shutdownRequested.store(false);

    // Start worker thread
    m_workerThread = std::thread(&UnifiedSearchManager::WorkerThread, this);

    std::cout << "[UnifiedSearchManager] Started" << std::endl;
}

void UnifiedSearchManager::Shutdown()
{
    if (!m_running.load()) {
        return; // Not running
    }

    // Signal shutdown
    m_shutdownRequested.store(true);
    m_commandQueueCV.notify_all();

    // Wait for worker thread to finish
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    m_running.store(false);

    std::cout << "[UnifiedSearchManager] Shutdown complete" << std::endl;
}

bool UnifiedSearchManager::IsRunning() const
{
    return m_running.load();
}

bool UnifiedSearchManager::SendCommand(const SearchCommand& command)
{
    std::lock_guard<std::mutex> lock(m_commandQueueMutex);

    if (!m_running.load() || m_shutdownRequested.load()) {
        return false;
    }

    m_commandQueue.push(command);
    m_commandQueueCV.notify_one();

    return true;
}

void UnifiedSearchManager::SetEventCallback(std::function<void(const SearchEvent&)> callback)
{
    std::lock_guard<std::mutex> lock(m_eventCallbackMutex);
    m_eventCallback = std::move(callback);
}

UnifiedSearchManager::Statistics UnifiedSearchManager::GetStatistics() const
{
    // Return a copy (safe since we're not modifying)
    return m_statistics;
}

void UnifiedSearchManager::WorkerThread()
{
    std::cout << "[UnifiedSearchManager] Worker thread started" << std::endl;

    // Initialize search engines in worker thread
    InitializeEngines();

    while (!m_shutdownRequested.load()) {
        // Wait for commands or maintenance interval
        std::unique_lock<std::mutex> lock(m_commandQueueMutex);

        if (m_commandQueue.empty()) {
            // Wait for command or timeout for maintenance
            m_commandQueueCV.wait_for(lock, m_config.maintenanceInterval, [this]() {
                return !m_commandQueue.empty() || m_shutdownRequested.load();
            });
        }

        if (m_shutdownRequested.load()) {
            break;
        }

        // Process all available commands
        while (!m_commandQueue.empty()) {
            SearchCommand command = m_commandQueue.front();
            m_commandQueue.pop();
            lock.unlock();

            // Process command outside lock
            ProcessCommand(command);

            lock.lock();
        }

        lock.unlock();

        // Perform periodic maintenance
        PerformMaintenance();
    }

    // Cleanup engines
    CleanupEngines();

    std::cout << "[UnifiedSearchManager] Worker thread stopped" << std::endl;
}

void UnifiedSearchManager::ProcessCommand(const SearchCommand& command)
{
    try {
        switch (command.type) {
            case SearchCommand::Type::START_SEARCH: {
                // Check if identical search already exists (if cache is enabled)
                if (m_cacheManager && m_config.enableSearchCache) {
                    SearchId existingSearchId;
                    if (m_cacheManager->FindExistingSearch(command.params, existingSearchId)) {
                        std::cout << "[UnifiedSearchManager] Reusing existing search: "
                                  << existingSearchId.ToString() << std::endl;

                        // Request more results from existing search
                        ISearchEngine* engine = SelectEngine(command.params.type);
                        if (engine) {
                            engine->RequestMoreResults(existingSearchId);
                            
                            // Update command searchId to existing searchId
                            const_cast<SearchCommand&>(command).searchId = existingSearchId;
                        }

                        SendEvent(SearchEvent::SearchStarted(existingSearchId));
                        SendEvent(SearchEvent::ProgressUpdate(
                            existingSearchId,
                            100,
                            "Requesting more results from existing search"
                        ));
                        return;
                    }
                }

                ISearchEngine* engine = SelectEngine(command.params.type);
                if (!engine) {
                    std::ostringstream oss;
                    oss << "No engine available for search type: "
                        << SearchTypeToString(command.params.type);
                    SendEvent(SearchEvent::ErrorOccurred(command.searchId, oss.str()));
                    return;
                }

                if (!command.params.IsValid()) {
                    SendEvent(SearchEvent::ErrorOccurred(command.searchId, "Invalid search parameters"));
                    return;
                }

                SearchId searchId = engine->StartSearch(command.params);

                // Register search in cache
                if (m_cacheManager && m_config.enableSearchCache) {
                    m_cacheManager->RegisterSearch(searchId, command.params);
                }

                // Track search state
                m_searchStates[searchId] = SearchState::STARTING;
                m_searchParams[searchId] = command.params;
                m_searchResults[searchId] = {};

                SendEvent(SearchEvent::SearchStarted(searchId));
                break;
            }

            case SearchCommand::Type::STOP_SEARCH: {
                // Find the engine for this search
                ISearchEngine* engine = nullptr;
                SearchType searchType = SearchType::LOCAL; // Default

                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end()) {
                    auto paramsIt = m_searchParams.find(command.searchId);
                    if (paramsIt != m_searchParams.end()) {
                        searchType = paramsIt->second.type;
                        engine = SelectEngine(searchType);
                    }
                }

                if (engine) {
                    engine->StopSearch(command.searchId);
                    m_searchStates[command.searchId] = SearchState::CANCELLED;
                    SendEvent(SearchEvent::SearchCancelled(command.searchId));
                }
                break;
            }

            case SearchCommand::Type::PAUSE_SEARCH: {
                ISearchEngine* engine = nullptr;
                SearchType searchType = SearchType::LOCAL;

                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end()) {
                    auto paramsIt = m_searchParams.find(command.searchId);
                    if (paramsIt != m_searchParams.end()) {
                        searchType = paramsIt->second.type;
                        engine = SelectEngine(searchType);
                    }
                }

                if (engine) {
                    engine->PauseSearch(command.searchId);
                    m_searchStates[command.searchId] = SearchState::PAUSED;
                }
                break;
            }

            case SearchCommand::Type::RESUME_SEARCH: {
                ISearchEngine* engine = nullptr;
                SearchType searchType = SearchType::LOCAL;

                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end()) {
                    auto paramsIt = m_searchParams.find(command.searchId);
                    if (paramsIt != m_searchParams.end()) {
                        searchType = paramsIt->second.type;
                        engine = SelectEngine(searchType);
                    }
                }

                if (engine) {
                    engine->ResumeSearch(command.searchId);
                    m_searchStates[command.searchId] = SearchState::RUNNING;
                }
                break;
            }

            case SearchCommand::Type::REQUEST_MORE_RESULTS: {
                ISearchEngine* engine = nullptr;
                SearchType searchType = SearchType::LOCAL;

                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end()) {
                    auto paramsIt = m_searchParams.find(command.searchId);
                    if (paramsIt != m_searchParams.end()) {
                        searchType = paramsIt->second.type;
                        engine = SelectEngine(searchType);
                    }
                }

                if (engine) {
                    engine->RequestMoreResults(command.searchId);
                }
                break;
            }

            case SearchCommand::Type::GET_RESULTS: {
                ISearchEngine* engine = nullptr;
                SearchType searchType = SearchType::LOCAL;

                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end()) {
                    auto paramsIt = m_searchParams.find(command.searchId);
                    if (paramsIt != m_searchParams.end()) {
                        searchType = paramsIt->second.type;
                        engine = SelectEngine(searchType);
                    }
                }

                if (engine && command.responseCallback) {
                    std::vector<SearchResult> results = engine->GetResults(command.searchId, command.maxResults);

                    // Serialize and send response
                    std::vector<uint8_t> response;
                    for (const auto& result : results) {
                        auto serialized = result.Serialize();
                        response.insert(response.end(), serialized.begin(), serialized.end());
                    }
                    command.responseCallback(response);
                }
                break;
            }

            case SearchCommand::Type::GET_RESULT_COUNT: {
                ISearchEngine* engine = nullptr;
                SearchType searchType = SearchType::LOCAL;

                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end()) {
                    auto paramsIt = m_searchParams.find(command.searchId);
                    if (paramsIt != m_searchParams.end()) {
                        searchType = paramsIt->second.type;
                        engine = SelectEngine(searchType);
                    }
                }

                if (engine && command.responseCallback) {
                    size_t count = engine->GetResultCount(command.searchId);

                    // Serialize count
                    std::vector<uint8_t> response;
                    response.insert(response.end(), reinterpret_cast<uint8_t*>(&count), reinterpret_cast<uint8_t*>(&count) + sizeof(count));
                    command.responseCallback(response);
                }
                break;
            }

            case SearchCommand::Type::GET_SEARCH_STATE: {
                auto stateIt = m_searchStates.find(command.searchId);
                if (stateIt != m_searchStates.end() && command.responseCallback) {
                    uint8_t state = static_cast<uint8_t>(stateIt->second);
                    std::vector<uint8_t> response;
                    response.push_back(state);
                    command.responseCallback(response);
                }
                break;
            }

            case SearchCommand::Type::GET_SEARCH_PARAMS: {
                auto paramsIt = m_searchParams.find(command.searchId);
                if (paramsIt != m_searchParams.end() && command.responseCallback) {
                    auto serialized = paramsIt->second.Serialize();
                    command.responseCallback(serialized);
                }
                break;
            }

            case SearchCommand::Type::CANCEL_ALL_SEARCHES: {
                // Cancel all searches in all engines
                if (m_localSearchEngine) {
                    m_localSearchEngine->Shutdown();
                    InitializeEngines(); // Re-initialize
                }
                if (m_globalSearchEngine) {
                    m_globalSearchEngine->Shutdown();
                    InitializeEngines(); // Re-initialize
                }
                if (m_kadSearchEngine) {
                    m_kadSearchEngine->Shutdown();
                    InitializeEngines(); // Re-initialize
                }

                // Clear search tracking
                m_searchStates.clear();
                m_searchParams.clear();
                m_searchResults.clear();
                break;
            }

            default:
                std::cerr << "[UnifiedSearchManager] Unknown command type: "
                          << static_cast<int>(command.type) << std::endl;
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "[UnifiedSearchManager] Error processing command: " << e.what() << std::endl;
        SendEvent(SearchEvent::ErrorOccurred(command.searchId, e.what()));
    }
}

void UnifiedSearchManager::SendEvent(const SearchEvent& event)
{
    std::lock_guard<std::mutex> lock(m_eventCallbackMutex);

    if (m_eventCallback) {
        // Post event to UI thread via wxWidgets
        // We need to serialize the event and pass it via wxCommandEvent
        auto serialized = event.Serialize();

        // Create a custom wx event to carry our search event
        wxCommandEvent* evt = new wxCommandEvent(wxEVT_SEARCH_EVENT);
        evt->SetInt(static_cast<int>(event.type));
        evt->SetString(wxString::FromUTF8(event.errorMessage.c_str()));

        // Store serialized data in the event's client data
        evt->SetClientObject(new wxClientDataContainer(serialized));

        // Post to main window
        wxQueueEvent(wxTheApp->GetTopWindow(), evt);
    }
}

void UnifiedSearchManager::PerformMaintenance()
{
    auto currentTime = std::chrono::system_clock::now();

    // Call maintenance on all engines
    if (m_localSearchEngine) {
        m_localSearchEngine->ProcessMaintenance(currentTime);
    }
    if (m_globalSearchEngine) {
        m_globalSearchEngine->ProcessMaintenance(currentTime);
    }
    if (m_kadSearchEngine) {
        m_kadSearchEngine->ProcessMaintenance(currentTime);
    }

    // Update statistics
    m_statistics.activeSearches = m_searchStates.size();
    m_statistics.completedSearches = std::count_if(
        m_searchStates.begin(), m_searchStates.end(),
        [](const auto& pair) { return pair.second == SearchState::COMPLETED; }
    );
    m_statistics.failedSearches = std::count_if(
        m_searchStates.begin(), m_searchStates.end(),
        [](const auto& pair) { return pair.second == SearchState::FAILED; }
    );
    m_statistics.totalResults = 0;
    for (const auto& [searchId, results] : m_searchResults) {
        m_statistics.totalResults += results.size();
    }
}

void UnifiedSearchManager::InitializeEngines()
{
    std::cout << "[UnifiedSearchManager] Initializing search engines" << std::endl;

    // Create search engines
    m_localSearchEngine = std::make_unique<LocalSearchEngine>();
    m_globalSearchEngine = std::make_unique<GlobalSearchEngine>();
    m_kadSearchEngine = std::make_unique<KadSearchEngine>();

    // Create cache manager if enabled
    if (m_config.enableSearchCache) {
        m_cacheManager = std::make_unique<SearchCacheManager>();
        std::cout << "[UnifiedSearchManager] Search cache manager initialized" << std::endl;
    }

    std::cout << "[UnifiedSearchManager] Search engines initialized" << std::endl;
}

void UnifiedSearchManager::CleanupEngines()
{
    std::cout << "[UnifiedSearchManager] Cleaning up search engines" << std::endl;

    // Shutdown engines
    if (m_localSearchEngine) {
        m_localSearchEngine->Shutdown();
        m_localSearchEngine.reset();
    }
    if (m_globalSearchEngine) {
        m_globalSearchEngine->Shutdown();
        m_globalSearchEngine.reset();
    }
    if (m_kadSearchEngine) {
        m_kadSearchEngine->Shutdown();
        m_kadSearchEngine.reset();
    }

    // Clear search tracking
    m_searchStates.clear();
    m_searchParams.clear();
    m_searchResults.clear();

    std::cout << "[UnifiedSearchManager] Search engines cleaned up" << std::endl;
}

ISearchEngine* UnifiedSearchManager::SelectEngine(SearchType type)
{
    switch (type) {
        case SearchType::LOCAL:
            return m_localSearchEngine.get();
        case SearchType::GLOBAL:
            return m_globalSearchEngine.get();
        case SearchType::KADEMLIA:
            return m_kadSearchEngine.get();
        default:
            return nullptr;
    }
}

} // namespace search

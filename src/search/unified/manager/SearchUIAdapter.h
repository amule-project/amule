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

#ifndef SEARCH_UI_ADAPTER_H
#define SEARCH_UI_ADAPTER_H

#include "../core/SearchTypes.h"
#include "../core/SearchId.h"
#include "../core/SearchParams.h"
#include "../core/SearchResult.h"
#include "../core/SearchEvent.h"
#include "../FeatureFlags.h"

#include <wx/wx.h>
#include <functional>
#include <memory>

namespace search {

/**
 * Adapter layer for integrating UnifiedSearchManager with wxWidgets UI
 * Handles event routing and provides convenient API for UI components
 */
class SearchUIAdapter : public wxEvtHandler {
public:
    /**
     * Callback types for UI events
     */
    using SearchStartedCallback = std::function<void(SearchId)>;
    using SearchCompletedCallback = std::function<void(SearchId)>;
    using SearchFailedCallback = std::function<void(SearchId, const wxString&)>;
    using SearchCancelledCallback = std::function<void(SearchId)>;
    using ResultsReceivedCallback = std::function<void(SearchId, const std::vector<SearchResult>&)>;
    using ProgressCallback = std::function<void(SearchId, int, const wxString&)>;
    using ErrorCallback = std::function<void(const wxString&)>;

    /**
     * Constructor
     * @param parent Parent window for event handling
     */
    explicit SearchUIAdapter(wxWindow* parent = nullptr);

    /**
     * Destructor
     */
    virtual ~SearchUIAdapter();

    /**
     * Initialize the adapter
     * @return true if initialized successfully
     */
    bool Initialize();

    /**
     * Shutdown the adapter
     */
    void Shutdown();

    /**
     * Check if adapter is initialized
     */
    bool IsInitialized() const { return m_initialized; }

    /**
     * Start a search
     * @param params Search parameters
     * @return Search ID, or invalid ID if failed
     */
    SearchId StartSearch(const SearchParams& params);

    /**
     * Stop a search
     * @param searchId Search to stop
     */
    void StopSearch(SearchId searchId);

    /**
     * Pause a search
     * @param searchId Search to pause
     */
    void PauseSearch(SearchId searchId);

    /**
     * Resume a search
     * @param searchId Search to resume
     */
    void ResumeSearch(SearchId searchId);

    /**
     * Request more results for a search
     * @param searchId Search to request more results for
     */
    void RequestMoreResults(SearchId searchId);

    /**
     * Get results for a search
     * @param searchId Search to query
     * @param maxResults Maximum results to return (0 = all)
     * @return Vector of search results
     */
    std::vector<SearchResult> GetResults(SearchId searchId, size_t maxResults = 0);

    /**
     * Get result count for a search
     * @param searchId Search to query
     * @return Number of results
     */
    size_t GetResultCount(SearchId searchId);

    /**
     * Cancel all active searches
     */
    void CancelAllSearches();

    // Callback setters
    void SetOnSearchStarted(SearchStartedCallback callback) { m_onSearchStarted = std::move(callback); }
    void SetOnSearchCompleted(SearchCompletedCallback callback) { m_onSearchCompleted = std::move(callback); }
    void SetOnSearchFailed(SearchFailedCallback callback) { m_onSearchFailed = std::move(callback); }
    void SetOnSearchCancelled(SearchCancelledCallback callback) { m_onSearchCancelled = std::move(callback); }
    void SetOnResultsReceived(ResultsReceivedCallback callback) { m_onResultsReceived = std::move(callback); }
    void SetOnProgress(ProgressCallback callback) { m_onProgress = std::move(callback); }
    void SetOnError(ErrorCallback callback) { m_onError = std::move(callback); }

private:
    /**
     * Handle wxCommandEvent from UnifiedSearchManager
     * @param event The command event
     */
    void OnSearchEvent(wxCommandEvent& event);

    /**
     * Process a search event
     * @param event The search event to process
     */
    void ProcessEvent(const SearchEvent& event);

    // UI callbacks
    SearchStartedCallback m_onSearchStarted;
    SearchCompletedCallback m_onSearchCompleted;
    SearchFailedCallback m_onSearchFailed;
    SearchCancelledCallback m_onSearchCancelled;
    ResultsReceivedCallback m_onResultsReceived;
    ProgressCallback m_onProgress;
    ErrorCallback m_onError;

    // Search manager (managed by adapter)
    class UnifiedSearchManager* m_manager;

    // Initialization state
    bool m_initialized;

    // Feature flag check
    bool IsUnifiedSearchEnabled() const;
};

} // namespace search

#endif // SEARCH_UI_ADAPTER_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
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

#ifndef SEARCHTIMEOUTMANAGER_H
#define SEARCHTIMEOUTMANAGER_H

#include <wx/timer.h>
#include <wx/datetime.h>
#include <map>
#include <functional>
#include <cstdint>
#include <wx/string.h>

/**
 * SearchTimeoutManager - Manages timeouts for searches to prevent them from getting stuck
 *
 * This class provides:
 * - Per-search timeout tracking
 * - Automatic timeout detection and recovery
 * - Heartbeat mechanism to detect stalled searches
 * - Configurable timeout values per search type
 */
class SearchTimeoutManager : public wxEvtHandler {
public:
    /**
     * Search type enumeration
     * Note: Must match SearchTimeoutType enum in SearchList.h
     */
    enum SearchType {
        LocalSearch = 0,   // TimeoutLocalSearch
        GlobalSearch,      // TimeoutGlobalSearch
        KadSearch          // TimeoutKadSearch
    };

    /**
     * Timeout callback type
     * Parameters:
     *   - searchId: The search ID that timed out
     *   - type: The search type
     *   - reason: The reason for timeout
     */
    using TimeoutCallback = std::function<void(uint32_t searchId, SearchType type, const wxString& reason)>;

    /**
     * Constructor
     */
    SearchTimeoutManager();

    /**
     * Destructor
     */
    virtual ~SearchTimeoutManager();

    /**
     * Get singleton instance
     */
    static SearchTimeoutManager& Instance();

    // Configuration
    /**
     * Set timeout for local searches (in milliseconds)
     * Default: 30000ms (30 seconds)
     */
    void setLocalSearchTimeout(int timeoutMs);

    /**
     * Get timeout for local searches
     */
    int getLocalSearchTimeout() const;

    /**
     * Set timeout for global searches (in milliseconds)
     * Default: 120000ms (2 minutes)
     */
    void setGlobalSearchTimeout(int timeoutMs);

    /**
     * Get timeout for global searches
     */
    int getGlobalSearchTimeout() const;

    /**
     * Set timeout for Kad searches (in milliseconds)
     * Default: 180000ms (3 minutes)
     */
    void setKadSearchTimeout(int timeoutMs);

    /**
     * Get timeout for Kad searches
     */
    int getKadSearchTimeout() const;

    /**
     * Set heartbeat interval (in milliseconds)
     * Default: 10000ms (10 seconds)
     */
    void setHeartbeatInterval(int intervalMs);

    /**
     * Get heartbeat interval
     */
    int getHeartbeatInterval() const;

    // Search lifecycle management
    /**
     * Register a search for timeout monitoring
     *
     * @param searchId The search ID
     * @param type The search type
     * @return true if registered successfully
     */
    bool registerSearch(uint32_t searchId, SearchType type);

    /**
     * Unregister a search from timeout monitoring
     *
     * @param searchId The search ID
     */
    void unregisterSearch(uint32_t searchId);

    /**
     * Update heartbeat for a search (call when search makes progress)
     *
     * @param searchId The search ID
     * @return true if heartbeat updated successfully
     */
    bool updateHeartbeat(uint32_t searchId);

    /**
     * Check if a search is registered
     *
     * @param searchId The search ID
     * @return true if search is registered
     */
    bool isSearchRegistered(uint32_t searchId) const;

    /**
     * Get search type
     *
     * @param searchId The search ID
     * @return The search type, or -1 if not found
     */
    SearchType getSearchType(uint32_t searchId) const;

    /**
     * Get elapsed time for a search
     *
     * @param searchId The search ID
     * @return Elapsed time in milliseconds, or -1 if not found
     */
    int64_t getElapsedTime(uint32_t searchId) const;

    /**
     * Get remaining time for a search
     *
     * @param searchId The search ID
     * @return Remaining time in milliseconds, or -1 if not found
     */
    int64_t getRemainingTime(uint32_t searchId) const;

    // Callback management
    /**
     * Set timeout callback
     *
     * @param callback The callback function to call when a search times out
     */
    void setTimeoutCallback(TimeoutCallback callback);

    // Manual timeout checking
    /**
     * Check for timed out searches manually
     * This is called automatically by the heartbeat timer, but can be called manually if needed
     */
    void checkTimeouts();

    // Statistics
    /**
     * Get number of registered searches
     */
    size_t getRegisteredSearchCount() const;

    /**
     * Get total number of timeouts
     */
    size_t getTotalTimeouts() const;

    /**
     * Reset timeout statistics
     */
    void resetStatistics();

private:
    // Event handlers
    void OnHeartbeatTimer(wxTimerEvent& event);

    /**
     * Check if a specific search has timed out
     *
     * @param searchId The search ID
     * @return true if search has timed out
     */
    bool isSearchTimedOut(uint32_t searchId) const;

    /**
     * Get timeout value for a search type
     *
     * @param type The search type
     * @return Timeout in milliseconds
     */
    int getTimeoutForType(SearchType type) const;

    /**
     * Search state structure
     */
    struct SearchState {
        uint32_t searchId;
        SearchType type;
        wxDateTime startTime;
        wxDateTime lastHeartbeat;
        bool isActive;

        SearchState()
            : searchId(0)
            , type(LocalSearch)
            , isActive(false)
        {
        }
    };

    // Configuration
    int m_localSearchTimeout;
    int m_globalSearchTimeout;
    int m_kadSearchTimeout;
    int m_heartbeatInterval;

    // Search tracking
    std::map<uint32_t, SearchState> m_searchStates;

    // Timers
    wxTimer m_heartbeatTimer;

    // Callbacks
    TimeoutCallback m_timeoutCallback;

    // Statistics
    size_t m_totalTimeouts;

    DECLARE_EVENT_TABLE()
};

#endif // SEARCHTIMEOUTMANAGER_H

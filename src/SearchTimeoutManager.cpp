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

#include "SearchTimeoutManager.h"
#include "Logger.h"
#include <common/Format.h>
#include <wx/log.h>

// Default configuration values
static const int DEFAULT_LOCAL_SEARCH_TIMEOUT = 30000;      // 30 seconds
static const int DEFAULT_GLOBAL_SEARCH_TIMEOUT = 120000;     // 2 minutes
static const int DEFAULT_KAD_SEARCH_TIMEOUT = 180000;        // 3 minutes
static const int DEFAULT_HEARTBEAT_INTERVAL = 10000;         // 10 seconds

BEGIN_EVENT_TABLE(SearchTimeoutManager, wxEvtHandler)
    EVT_TIMER(wxID_ANY, SearchTimeoutManager::OnHeartbeatTimer)
END_EVENT_TABLE()

SearchTimeoutManager::SearchTimeoutManager()
    : m_localSearchTimeout(DEFAULT_LOCAL_SEARCH_TIMEOUT)
    , m_globalSearchTimeout(DEFAULT_GLOBAL_SEARCH_TIMEOUT)
    , m_kadSearchTimeout(DEFAULT_KAD_SEARCH_TIMEOUT)
    , m_heartbeatInterval(DEFAULT_HEARTBEAT_INTERVAL)
    , m_heartbeatTimer(this)
    , m_totalTimeouts(0)
{
    // Start heartbeat timer
    m_heartbeatTimer.Start(m_heartbeatInterval);
    AddDebugLogLineC(logSearch, wxT("SearchTimeoutManager initialized"));
}

SearchTimeoutManager::~SearchTimeoutManager()
{
    // Stop heartbeat timer
    m_heartbeatTimer.Stop();

    // Clear all search states
    m_searchStates.clear();

    AddDebugLogLineC(logSearch, wxT("SearchTimeoutManager destroyed"));
}

void SearchTimeoutManager::setLocalSearchTimeout(int timeoutMs)
{
    wxCHECK_RET(timeoutMs > 0, wxT("Local search timeout must be positive"));
    m_localSearchTimeout = timeoutMs;
}

int SearchTimeoutManager::getLocalSearchTimeout() const
{
    return m_localSearchTimeout;
}

void SearchTimeoutManager::setGlobalSearchTimeout(int timeoutMs)
{
    wxCHECK_RET(timeoutMs > 0, wxT("Global search timeout must be positive"));
    m_globalSearchTimeout = timeoutMs;
}

int SearchTimeoutManager::getGlobalSearchTimeout() const
{
    return m_globalSearchTimeout;
}

void SearchTimeoutManager::setKadSearchTimeout(int timeoutMs)
{
    wxCHECK_RET(timeoutMs > 0, wxT("Kad search timeout must be positive"));
    m_kadSearchTimeout = timeoutMs;
}

int SearchTimeoutManager::getKadSearchTimeout() const
{
    return m_kadSearchTimeout;
}

void SearchTimeoutManager::setHeartbeatInterval(int intervalMs)
{
    wxCHECK_RET(intervalMs > 0, wxT("Heartbeat interval must be positive"));
    m_heartbeatInterval = intervalMs;

    // Restart timer with new interval
    if (m_heartbeatTimer.IsRunning()) {
        m_heartbeatTimer.Stop();
        m_heartbeatTimer.Start(m_heartbeatInterval);
    }
}

int SearchTimeoutManager::getHeartbeatInterval() const
{
    return m_heartbeatInterval;
}

bool SearchTimeoutManager::registerSearch(uint32_t searchId, SearchType type)
{
    if (searchId == 0) {
        AddDebugLogLineC(logSearch, wxT("SearchTimeoutManager: Invalid search ID (0)"));
        return false;
    }

    // Check if search is already registered
    if (m_searchStates.find(searchId) != m_searchStates.end()) {
        AddDebugLogLineC(logSearch,
            CFormat(wxT("SearchTimeoutManager: Search %u already registered, updating type"))
            % searchId);
        // Update the type and reset times
        SearchState& state = m_searchStates[searchId];
        state.type = type;
        state.startTime = wxDateTime::Now();
        state.lastHeartbeat = wxDateTime::Now();
        state.isActive = true;
        return true;
    }

    // Create new search state
    SearchState state;
    state.searchId = searchId;
    state.type = type;
    state.startTime = wxDateTime::Now();
    state.lastHeartbeat = wxDateTime::Now();
    state.isActive = true;

    m_searchStates[searchId] = state;

    wxString typeStr;
    switch (type) {
        case LocalSearch: typeStr = wxT("Local"); break;
        case GlobalSearch: typeStr = wxT("Global"); break;
        case KadSearch: typeStr = wxT("Kad"); break;
    }

    AddDebugLogLineC(logSearch,
        CFormat(wxT("SearchTimeoutManager: Registered search %u (type=%s, timeout=%dms)"))
        % searchId % typeStr % getTimeoutForType(type));

    return true;
}

void SearchTimeoutManager::unregisterSearch(uint32_t searchId)
{
    auto it = m_searchStates.find(searchId);
    if (it != m_searchStates.end()) {
        AddDebugLogLineC(logSearch,
            CFormat(wxT("SearchTimeoutManager: Unregistered search %u"))
            % searchId);
        m_searchStates.erase(it);
    }
}

bool SearchTimeoutManager::updateHeartbeat(uint32_t searchId)
{
    auto it = m_searchStates.find(searchId);
    if (it == m_searchStates.end()) {
        AddDebugLogLineC(logSearch,
            CFormat(wxT("SearchTimeoutManager: Cannot update heartbeat for unknown search %u"))
            % searchId);
        return false;
    }

    it->second.lastHeartbeat = wxDateTime::Now();

    AddDebugLogLineC(logSearch,
        CFormat(wxT("SearchTimeoutManager: Updated heartbeat for search %u"))
        % searchId);

    return true;
}

bool SearchTimeoutManager::isSearchRegistered(uint32_t searchId) const
{
    return m_searchStates.find(searchId) != m_searchStates.end();
}

SearchTimeoutManager::SearchType SearchTimeoutManager::getSearchType(uint32_t searchId) const
{
    auto it = m_searchStates.find(searchId);
    if (it != m_searchStates.end()) {
        return it->second.type;
    }
    return static_cast<SearchType>(-1);
}

int64_t SearchTimeoutManager::getElapsedTime(uint32_t searchId) const
{
    auto it = m_searchStates.find(searchId);
    if (it == m_searchStates.end()) {
        return -1;
    }

    wxTimeSpan elapsed = wxDateTime::Now() - it->second.startTime;
    return elapsed.GetMilliseconds().ToLong();
}

int64_t SearchTimeoutManager::getRemainingTime(uint32_t searchId) const
{
    auto it = m_searchStates.find(searchId);
    if (it == m_searchStates.end()) {
        return -1;
    }

    int timeout = getTimeoutForType(it->second.type);
    int64_t elapsed = getElapsedTime(searchId);

    if (elapsed < 0) {
        return -1;
    }

    return timeout - elapsed;
}

void SearchTimeoutManager::setTimeoutCallback(TimeoutCallback callback)
{
    m_timeoutCallback = callback;
}

void SearchTimeoutManager::checkTimeouts()
{
    wxDateTime now = wxDateTime::Now();

    // Check all registered searches for timeout
    std::vector<std::pair<uint32_t, SearchType>> timedOutSearchesWithType;
    
    for (auto it = m_searchStates.begin(); it != m_searchStates.end(); ) {
        const SearchState& state = it->second;

        if (!state.isActive) {
            // Skip inactive searches, but remove them from tracking
            AddDebugLogLineC(logSearch,
                CFormat(wxT("SearchTimeoutManager: Removing inactive search %u"))
                % state.searchId);
            it = m_searchStates.erase(it);
            continue;
        }

        if (isSearchTimedOut(state.searchId)) {
            // Search has timed out - save type before erasing
            timedOutSearchesWithType.push_back({state.searchId, state.type});
            m_totalTimeouts++;

            wxString typeStr;
            switch (state.type) {
                case LocalSearch: typeStr = wxT("Local"); break;
                case GlobalSearch: typeStr = wxT("Global"); break;
                case KadSearch: typeStr = wxT("Kad"); break;
            }

            wxTimeSpan elapsed = now - state.startTime;
            AddDebugLogLineC(logSearch,
                CFormat(wxT("SearchTimeoutManager: Search %u (%s) timed out after %ld ms (timeout=%dms)"))
                % state.searchId % typeStr % elapsed.GetMilliseconds().ToLong() % getTimeoutForType(state.type));

            // Mark as inactive and remove from tracking
            it->second.isActive = false;
            it = m_searchStates.erase(it);
        } else {
            ++it;
        }
    }

    // Trigger timeout callbacks for all timed out searches
    for (const auto& [searchId, type] : timedOutSearchesWithType) {
        if (m_timeoutCallback) {
            wxString reason;
            switch (type) {
                case LocalSearch:
                    reason = wxT("Local search timed out - no response from server");
                    break;
                case GlobalSearch:
                    reason = wxT("Global search timed out - no results from servers");
                    break;
                case KadSearch:
                    reason = wxT("Kad search timed out - no results from Kad network");
                    break;
                default:
                    reason = wxT("Search timed out");
                    break;
            }

            m_timeoutCallback(searchId, type, reason);
        }
    }
}

size_t SearchTimeoutManager::getRegisteredSearchCount() const
{
    return m_searchStates.size();
}

size_t SearchTimeoutManager::getTotalTimeouts() const
{
    return m_totalTimeouts;
}

void SearchTimeoutManager::resetStatistics()
{
    m_totalTimeouts = 0;
    AddDebugLogLineC(logSearch, wxT("SearchTimeoutManager: Statistics reset"));
}

void SearchTimeoutManager::OnHeartbeatTimer(wxTimerEvent& event)
{
    // Check for timed out searches
    checkTimeouts();

    // Log current status
    if (!m_searchStates.empty()) {
        AddDebugLogLineC(logSearch,
            CFormat(wxT("SearchTimeoutManager: Heartbeat - %zu active searches, %zu total timeouts"))
            % m_searchStates.size() % m_totalTimeouts);
    }
}

bool SearchTimeoutManager::isSearchTimedOut(uint32_t searchId) const
{
    auto it = m_searchStates.find(searchId);
    if (it == m_searchStates.end()) {
        return false;
    }

    const SearchState& state = it->second;
    if (!state.isActive) {
        return false;
    }

    int timeout = getTimeoutForType(state.type);
    wxTimeSpan elapsed = wxDateTime::Now() - state.startTime;

    return elapsed.GetMilliseconds().ToLong() >= timeout;
}

int SearchTimeoutManager::getTimeoutForType(SearchType type) const
{
    switch (type) {
        case LocalSearch:
            return m_localSearchTimeout;
        case GlobalSearch:
            return m_globalSearchTimeout;
        case KadSearch:
            return m_kadSearchTimeout;
        default:
            return DEFAULT_LOCAL_SEARCH_TIMEOUT;
    }
}

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

#ifndef PERSEARCHSTATE_H
#define PERSEARCHSTATE_H

#include <memory>
#include <set>
#include <wx/string.h>
#include <wx/thread.h>
#include "../Types.h"
#include "../Packet.h"
#include "../Timer.h"
#include "../ObservableQueue.h"

// Forward declarations
class CServer;
class CSearchList;

/**
 * PerSearchState - Manages state for a single search
 *
 * This class encapsulates all state that was previously stored globally
 * in CSearchList, allowing multiple searches to run concurrently without
 * interfering with each other.
 */
class PerSearchState {
public:
    /**
     * Constructor
     *
     * @param searchId The unique ID for this search
     * @param searchType The type of search (Local, Global, Kad)
     * @param params Search parameters
     */
    PerSearchState(uint32_t searchId, uint8_t searchType, const wxString& searchString);
    
    /**
     * Destructor
     */
    ~PerSearchState();
    
    // Delete copy constructor and copy assignment operator
    PerSearchState(const PerSearchState&) = delete;
    PerSearchState& operator=(const PerSearchState&) = delete;
    
    // Allow move constructor and move assignment operator
    PerSearchState(PerSearchState&&) = default;
    PerSearchState& operator=(PerSearchState&&) = default;
    
    /**
     * Get the search ID
     */
    uint32_t getSearchId() const { return m_searchId; }
    
    /**
     * Get the search type
     */
    uint8_t getSearchType() const { return m_searchType; }
    
    /**
     * Get the search string
     */
    wxString getSearchString() const { return m_searchString; }
    
    /**
     * Set the search packet for this search
     *
     * @param packet The search packet
     * @param is64bit Whether the packet uses 64-bit values
     */
    void setSearchPacket(std::unique_ptr<CPacket> packet, bool is64bit);
    
    /**
     * Get the search packet
     */
    CPacket* getSearchPacket() const { return m_searchPacket.get(); }
    
    /**
     * Check if the search packet uses 64-bit values
     */
    bool is64bitPacket() const { return m_64bitSearchPacket; }
    
    /**
     * Clear the search packet
     */
    void clearSearchPacket();
    
    /**
     * Add a server to the queried servers set
     *
     * @param serverId The server ID (IP address)
     */
    void addQueriedServer(uint32_t serverId);
    
    /**
     * Check if a server has been queried
     *
     * @param serverId The server ID to check
     * @return true if the server has been queried, false otherwise
     */
    bool hasQueriedServer(uint32_t serverId) const;
    
    /**
     * Get the number of queried servers
     */
    size_t getQueriedServerCount() const;
    
    /**
     * Clear the queried servers set
     */
    void clearQueriedServers();
    
    /**
     * Set more results mode
     *
     * @param enabled Whether more results mode is enabled
     * @param maxServers Maximum number of servers to query in more results mode
     */
    void setMoreResultsMode(bool enabled, int maxServers = 0);
    
    /**
     * Check if more results mode is enabled
     */
    bool isMoreResultsMode() const { return m_moreResultsMode; }
    
    /**
     * Get the maximum number of servers for more results mode
     */
    int getMoreResultsMaxServers() const { return m_moreResultsMaxServers; }
    
    /**
     * Set Kad search finished state
     *
     * @param finished Whether the Kad search is finished
     */
    void setKadSearchFinished(bool finished);
    
    /**
     * Check if Kad search is finished
     */
    bool isKadSearchFinished() const { return m_KadSearchFinished; }
    
    /**
     * Set Kad search retry count
     *
     * @param retryCount The retry count
     */
    void setKadSearchRetryCount(int retryCount);
    
    /**
     * Get Kad search retry count
     */
    int getKadSearchRetryCount() const { return m_KadSearchRetryCount; }
    
    /**
     * Increment Kad search retry count
     */
    void incrementKadSearchRetryCount();

    /**
     * Set the Kad keyword for this search
     *
     * @param keyword The Kad keyword
     */
    void setKadKeyword(const wxString& keyword);

    /**
     * Get the Kad keyword for this search
     *
     * @return The Kad keyword
     */
    wxString getKadKeyword() const { return m_kadKeyword; }

    /**
     * Set the search active state
     *
     * @param active Whether the search is active
     */
    void setSearchActive(bool active);
    
    /**
     * Check if the search is active
     *
     * @return true if the search is active, false otherwise
     */
    bool isSearchActive() const { return m_searchActive; }
    
    /**
     * Lock the state for thread-safe access
     */
    void lock() const { m_mutex.Lock(); }

    /**
     * Unlock the state
     */
    void unlock() const { m_mutex.Unlock(); }

    // Server queue management for global searches
    /**
     * Set the server queue for this search
     * @param queue The server queue to use
     */
    void setServerQueue(std::unique_ptr<CQueueObserver<CServer*>> queue);

    /**
     * Get the server queue for this search
     * @return The server queue, or nullptr if not set
     */
    CQueueObserver<CServer*>* getServerQueue() const;

    /**
     * Check if the server queue is active
     * @return true if the queue is active
     */
    bool isServerQueueActive() const;

    /**
     * Get the number of remaining servers in the queue
     * @return Number of remaining servers
     */
    size_t getRemainingServerCount() const;

    /**
     * Get the next server from the queue
     * @return The next server, or nullptr if no more servers
     */
    CServer* getNextServer();

    // Timer management for global searches
    /**
     * Set the timer for this search
     * @param timer The timer to use
     */
    void setTimer(std::unique_ptr<CTimer> timer);

    /**
     * Get the timer for this search
     * @return The timer, or nullptr if not set
     */
    CTimer* getTimer() const;

    /**
     * Check if the timer is running
     * @return true if the timer is running
     */
    bool isTimerRunning() const;

    /**
     * Start the timer
     * @param millisecs Timer frequency in milliseconds
     * @param oneShot Whether to run only once
     * @return true if started successfully
     */
    bool startTimer(int millisecs, bool oneShot = false);

    /**
     * Stop the timer
     */
    void stopTimer();

    // Progress tracking
    /**
     * Get the search progress percentage
     * @param totalServers Total number of servers for progress calculation
     * @return Progress percentage (0-100)
     */
    uint32_t getProgress(size_t totalServers) const;

    /**
     * Get the search progress percentage (auto-detect total servers)
     * @return Progress percentage (0-100)
     */
    uint32_t getProgress() const;

    /**
     * Set the total server count for progress calculation
     * @param totalServers Total number of servers
     */
    void setTotalServerCount(size_t totalServers);

    /**
     * Get the total server count
     * @return Total server count
     */
    size_t getTotalServerCount() const;

    // Owner reference
    /**
     * Set the owning CSearchList (for callback purposes)
     * @param owner The owning CSearchList
     */
    void setSearchList(CSearchList* owner);

    /**
     * Get the owning CSearchList
     * @return The owning CSearchList, or nullptr if not set
     */
    CSearchList* getSearchList() const;

private:
    // Search identification
    uint32_t m_searchId;
    uint8_t m_searchType;
    wxString m_searchString;

    // Search packet (for global searches)
    std::unique_ptr<CPacket> m_searchPacket;
    bool m_64bitSearchPacket;

    // Server tracking
    std::set<uint32_t> m_queriedServers;

    // More results mode
    bool m_moreResultsMode;
    int m_moreResultsMaxServers;

    // Kad search state
    bool m_KadSearchFinished;
    int m_KadSearchRetryCount;
    wxString m_kadKeyword;  // Per-search Kad keyword

    // Search active state
    bool m_searchActive;

    // NEW: Per-search server queue for global searches
    std::unique_ptr<CQueueObserver<CServer*>> m_serverQueue;

    // NEW: Per-search timer for global searches
    std::unique_ptr<CTimer> m_timer;

    // NEW: Total server count for progress tracking
    size_t m_totalServerCount;

    // NEW: Owner reference for callbacks
    CSearchList* m_owner;

    // Mutex for thread-safe access
    mutable wxMutex m_mutex;
};

#endif // PERSEARCHSTATE_H
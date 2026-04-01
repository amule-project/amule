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

#include "PerSearchState.h"
#include "../Logger.h"
#include <common/Format.h>
#include "../ServerList.h"  // For global server count
#include "../SearchList.h"  // For SearchType enum

PerSearchState::PerSearchState(uint32_t searchId, uint8_t searchType, const wxString& searchString)
    : m_searchId(searchId)
    , m_searchType(searchType)
    , m_searchString(searchString)
    , m_searchPacket(nullptr)
    , m_64bitSearchPacket(false)
    , m_moreResultsMode(false)
    , m_moreResultsMaxServers(0)
    , m_KadSearchFinished(false)
    , m_KadSearchRetryCount(0)
    , m_searchActive(true)  // Search is active when created
    , m_serverQueue(nullptr)
    , m_timer(nullptr)
    , m_totalServerCount(0)
    , m_owner(nullptr)
{
    AddDebugLogLineC(logSearch, CFormat(wxT("PerSearchState created: ID=%u, Type=%u, String='%s'")) % searchId % searchType % searchString);
}

PerSearchState::~PerSearchState()
{
    AddDebugLogLineC(logSearch, CFormat(wxT("PerSearchState destroyed: ID=%u")) % m_searchId);
}

void PerSearchState::setSearchPacket(std::unique_ptr<CPacket> packet, bool is64bit)
{
    wxMutexLocker lock(m_mutex);
    m_searchPacket = std::move(packet);
    m_64bitSearchPacket = is64bit;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Search packet set for ID=%u: size=%u, 64bit=%d")) % m_searchId % (m_searchPacket ? m_searchPacket->GetPacketSize() : 0) % is64bit);
}

void PerSearchState::clearSearchPacket()
{
    wxMutexLocker lock(m_mutex);
    m_searchPacket.reset();
    m_64bitSearchPacket = false;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Search packet cleared for ID=%u")) % m_searchId);
}

void PerSearchState::addQueriedServer(uint32_t serverId)
{
    wxMutexLocker lock(m_mutex);
    m_queriedServers.insert(serverId);
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Server added to queried set for ID=%u: server=%u")) % m_searchId % serverId);
}

bool PerSearchState::hasQueriedServer(uint32_t serverId) const
{
    wxMutexLocker lock(m_mutex);
    return m_queriedServers.find(serverId) != m_queriedServers.end();
}

size_t PerSearchState::getQueriedServerCount() const
{
    wxMutexLocker lock(m_mutex);
    return m_queriedServers.size();
}

void PerSearchState::clearQueriedServers()
{
    wxMutexLocker lock(m_mutex);
    m_queriedServers.clear();
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Queried servers cleared for ID=%u")) % m_searchId);
}

void PerSearchState::setMoreResultsMode(bool enabled, int maxServers)
{
    wxMutexLocker lock(m_mutex);
    m_moreResultsMode = enabled;
    m_moreResultsMaxServers = maxServers;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("More results mode set for ID=%u: enabled=%d, maxServers=%d")) % m_searchId % enabled % maxServers);
}

void PerSearchState::setKadSearchFinished(bool finished)
{
    wxMutexLocker lock(m_mutex);
    m_KadSearchFinished = finished;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Kad search finished state set for ID=%u: finished=%d")) % m_searchId % finished);
}

void PerSearchState::setKadSearchRetryCount(int retryCount)
{
    wxMutexLocker lock(m_mutex);
    m_KadSearchRetryCount = retryCount;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("Kad search retry count set for ID=%u: retryCount=%d")) % m_searchId % retryCount);
}

void PerSearchState::incrementKadSearchRetryCount()
{
    wxMutexLocker lock(m_mutex);
    m_KadSearchRetryCount++;

    AddDebugLogLineC(logSearch, CFormat(wxT("Kad search retry count incremented for ID=%u: newCount=%d")) % m_searchId % m_KadSearchRetryCount);
}

void PerSearchState::setKadKeyword(const wxString& keyword)
{
    wxMutexLocker lock(m_mutex);
    m_kadKeyword = keyword;

    AddDebugLogLineC(logSearch, CFormat(wxT("Kad keyword set for ID=%u: keyword='%s'")) % m_searchId % keyword);
}

void PerSearchState::setSearchActive(bool active)
{
    wxMutexLocker lock(m_mutex);
    m_searchActive = active;

    AddDebugLogLineC(logSearch, CFormat(wxT("Search active state set for ID=%u: active=%d")) % m_searchId % active);
}

// Server queue management
void PerSearchState::setServerQueue(std::unique_ptr<CQueueObserver<CServer*>> queue)
{
    wxMutexLocker lock(m_mutex);
    m_serverQueue = std::move(queue);

    AddDebugLogLineC(logSearch, CFormat(wxT("Server queue set for ID=%u: queue=%p")) % m_searchId % m_serverQueue.get());
}

CQueueObserver<CServer*>* PerSearchState::getServerQueue() const
{
    wxMutexLocker lock(m_mutex);
    return m_serverQueue.get();
}

bool PerSearchState::isServerQueueActive() const
{
    wxMutexLocker lock(m_mutex);
    return m_serverQueue && m_serverQueue->IsActive();
}

size_t PerSearchState::getRemainingServerCount() const
{
    wxMutexLocker lock(m_mutex);
    return m_serverQueue ? m_serverQueue->GetRemaining() : 0;
}

CServer* PerSearchState::getNextServer()
{
    wxMutexLocker lock(m_mutex);
    if (!m_serverQueue) {
        return nullptr;
    }
    return m_serverQueue->GetNext();
}

// Timer management
void PerSearchState::setTimer(std::unique_ptr<CTimer> timer)
{
    wxMutexLocker lock(m_mutex);
    m_timer = std::move(timer);

    AddDebugLogLineC(logSearch, CFormat(wxT("Timer set for ID=%u: timer=%p")) % m_searchId % m_timer.get());
}

CTimer* PerSearchState::getTimer() const
{
    wxMutexLocker lock(m_mutex);
    return m_timer.get();
}

bool PerSearchState::isTimerRunning() const
{
    wxMutexLocker lock(m_mutex);
    return m_timer && m_timer->IsRunning();
}

bool PerSearchState::startTimer(int millisecs, bool oneShot)
{
    wxMutexLocker lock(m_mutex);
    if (!m_timer) {
        AddDebugLogLineC(logSearch, CFormat(wxT("Failed to start timer for ID=%u: no timer set")) % m_searchId);
        return false;
    }

    bool result = m_timer->Start(millisecs, oneShot);

    AddDebugLogLineC(logSearch, CFormat(wxT("Timer %s for ID=%u: interval=%dms, oneShot=%d")) % (result ? wxT("started") : wxT("failed to start")) % m_searchId % millisecs % oneShot);

    return result;
}

void PerSearchState::stopTimer()
{
    wxMutexLocker lock(m_mutex);
    if (m_timer && m_timer->IsRunning()) {
        m_timer->Stop();
        AddDebugLogLineC(logSearch, CFormat(wxT("Timer stopped for ID=%u")) % m_searchId);
    }
}

// Progress tracking
uint32_t PerSearchState::getProgress(size_t totalServers) const
{
    wxMutexLocker lock(m_mutex);

    if (m_searchType == KadSearch) {
        // We cannot measure the progress of Kad searches
        return m_KadSearchFinished ? 0xfffe : 0;
    }

    if (m_searchType == LocalSearch) {
        // Local search is complete when started
        return 0xffff;
    }

    if (m_searchType == GlobalSearch) {
        // Calculate progress based on queried servers
        if (totalServers == 0) {
            return 0;
        }

        size_t queriedCount = m_queriedServers.size();
        uint32_t progress = 100 - (queriedCount * 100) / totalServers;

        AddDebugLogLineC(logSearch, CFormat(wxT("Progress for ID=%u: queried=%zu/%zu, progress=%u")) % m_searchId % queriedCount % totalServers % progress);

        return progress;
    }

    return 0;
}

uint32_t PerSearchState::getProgress() const
{
    wxMutexLocker lock(m_mutex);
    return getProgress(m_totalServerCount);
}

void PerSearchState::setTotalServerCount(size_t totalServers)
{
    wxMutexLocker lock(m_mutex);
    m_totalServerCount = totalServers;

    AddDebugLogLineC(logSearch, CFormat(wxT("Total server count set for ID=%u: count=%zu")) % m_searchId % totalServers);
}

size_t PerSearchState::getTotalServerCount() const
{
    wxMutexLocker lock(m_mutex);
    return m_totalServerCount;
}

// Owner reference
void PerSearchState::setSearchList(CSearchList* owner)
{
    wxMutexLocker lock(m_mutex);
    m_owner = owner;
}

CSearchList* PerSearchState::getSearchList() const
{
    wxMutexLocker lock(m_mutex);
    return m_owner;
}

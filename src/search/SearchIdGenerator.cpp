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

#include "SearchIdGenerator.h"
#include "../Logger.h"
#include <common/Format.h>
#include <algorithm>

namespace search {

SearchIdGenerator::SearchIdGenerator()
    : m_nextId(1) // Start from 1, 0 is invalid
{
}

SearchIdGenerator& SearchIdGenerator::Instance()
{
    static SearchIdGenerator instance;
    return instance;
}

uint32_t SearchIdGenerator::generateId()
{
    wxMutexLocker lock(m_mutex);
    
    uint32_t newId = generateNewId();
    m_activeIds.insert(newId);
    
    AddDebugLogLineC(logSearch, CFormat(wxT("SearchIdGenerator: Generated new ID %u"))
        % newId);
    
    return newId;
}

bool SearchIdGenerator::reserveId(uint32_t id)
{
    if (id == 0) {
        // 0 is invalid
        return false;
    }
    
    wxMutexLocker lock(m_mutex);
    
    if (!isIdAvailable(id)) {
        AddDebugLogLineC(logSearch, CFormat(wxT("SearchIdGenerator: ID %u is not available"))
            % id);
        return false;
    }
    
    m_activeIds.insert(id);
    
    // Remove from released IDs if it was there
    m_releasedIds.erase(id);
    
    // Update next ID if necessary
    if (id >= m_nextId) {
        m_nextId = id + 1;
    }
    
    AddDebugLogLineC(logSearch, CFormat(wxT("SearchIdGenerator: Reserved ID %u"))
        % id);
    
    return true;
}

bool SearchIdGenerator::isValidId(uint32_t id) const
{
    wxMutexLocker lock(m_mutex);
    return m_activeIds.find(id) != m_activeIds.end();
}

bool SearchIdGenerator::releaseId(uint32_t id)
{
    wxMutexLocker lock(m_mutex);
    
    auto it = m_activeIds.find(id);
    if (it == m_activeIds.end()) {
        AddDebugLogLineC(logSearch, CFormat(wxT("SearchIdGenerator: ID %u not found for release"))
            % id);
        return false;
    }
    
    m_activeIds.erase(it);
    m_releasedIds.insert(id);
    
    AddDebugLogLineC(logSearch, CFormat(wxT("SearchIdGenerator: Released ID %u"))
        % id);
    
    return true;
}

uint32_t SearchIdGenerator::peekNextId() const
{
    wxMutexLocker lock(m_mutex);
    
    // First try to reuse a released ID
    if (!m_releasedIds.empty()) {
        return *m_releasedIds.begin();
    }
    
    // Otherwise return the next new ID
    return m_nextId;
}

std::set<uint32_t> SearchIdGenerator::getActiveIds() const
{
    wxMutexLocker lock(m_mutex);
    return m_activeIds;
}

void SearchIdGenerator::clearAll()
{
    wxMutexLocker lock(m_mutex);
    
    size_t count = m_activeIds.size();
    m_activeIds.clear();
    m_releasedIds.clear();
    m_nextId = 1;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("SearchIdGenerator: Cleared all IDs (%u active IDs cleared)"))
        % count);
}

size_t SearchIdGenerator::getActiveCount() const
{
    wxMutexLocker lock(m_mutex);
    return m_activeIds.size();
}

uint32_t SearchIdGenerator::generateNewId()
{
    // CRITICAL FIX: For per-search tab architecture, IDs should NEVER be reused
    // Each search gets a unique, monotonically increasing ID
    // This prevents duplicate tab issues when IDs are reused

    // Always generate a new ID (never reuse released IDs)
    uint32_t newId = m_nextId++;

    // Handle wrap-around (unlikely but possible)
    if (m_nextId == 0) {
        m_nextId = 1;
    }

    return newId;
}

bool SearchIdGenerator::isIdAvailable(uint32_t id) const
{
    // Check if ID is already active
    if (m_activeIds.find(id) != m_activeIds.end()) {
        return false;
    }
    
    // Check if ID is in released pool (available for reuse)
    if (m_releasedIds.find(id) != m_releasedIds.end()) {
        return true;
    }
    
    // ID is available if it's not active and we haven't generated it yet
    // or it's greater than our current next ID
    return true;
}

} // namespace search
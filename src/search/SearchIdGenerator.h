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

#ifndef SEARCHIDGENERATOR_H
#define SEARCHIDGENERATOR_H

#include <cstdint>
#include <set>
#include <wx/thread.h>

namespace search {

/**
 * SearchIdGenerator - Thread-safe search ID generator with validation
 *
 * This class provides unique search IDs with validation to prevent
 * conflicts and ensure proper cleanup. It replaces the simple static
 * counter in CSearchList::GetNextSearchID().
 */
class SearchIdGenerator {
public:
    /**
     * Get the singleton instance
     */
    static SearchIdGenerator& Instance();
    
    /**
     * Generate a new unique search ID
     * 
     * @return A unique search ID
     */
    uint32_t generateId();
    
    /**
     * Reserve a specific search ID
     * 
     * @param id The ID to reserve
     * @return true if the ID was successfully reserved, false if already in use
     */
    bool reserveId(uint32_t id);
    
    /**
     * Check if a search ID is valid (exists and is active)
     * 
     * @param id The ID to check
     * @return true if the ID is valid, false otherwise
     */
    bool isValidId(uint32_t id) const;
    
    /**
     * Release a search ID (mark it as available for reuse)
     * 
     * @param id The ID to release
     * @return true if the ID was successfully released, false if not found
     */
    bool releaseId(uint32_t id);
    
    /**
     * Get the next available search ID without reserving it
     * Useful for previewing what the next ID will be
     * 
     * @return The next available search ID
     */
    uint32_t peekNextId() const;
    
    /**
     * Get all active search IDs
     * 
     * @return Set of active search IDs
     */
    std::set<uint32_t> getActiveIds() const;
    
    /**
     * Clear all search IDs (for testing or shutdown)
     */
    void clearAll();
    
    /**
     * Get the number of active search IDs
     */
    size_t getActiveCount() const;
    
private:
    // Private constructor for singleton
    SearchIdGenerator();
    
    // Delete copy constructor and copy assignment operator
    SearchIdGenerator(const SearchIdGenerator&) = delete;
    SearchIdGenerator& operator=(const SearchIdGenerator&) = delete;
    
    // Next ID to generate
    uint32_t m_nextId;
    
    // Set of active/reserved IDs
    std::set<uint32_t> m_activeIds;
    
    // Set of released IDs available for reuse
    std::set<uint32_t> m_releasedIds;
    
    // Mutex for thread-safe access
    mutable wxMutex m_mutex;
    
    // Helper methods
    uint32_t generateNewId();
    bool isIdAvailable(uint32_t id) const;
};

} // namespace search

#endif // SEARCHIDGENERATOR_H
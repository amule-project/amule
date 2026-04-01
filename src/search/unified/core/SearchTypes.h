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

#ifndef SEARCH_TYPES_H
#define SEARCH_TYPES_H

#include <cstdint>
#include <atomic>
#include <string>

namespace search {

/**
 * Search types supported by the unified search system
 */
enum class SearchType {
    LOCAL,      // Local file search (shared files)
    GLOBAL,     // Global server search
    KADEMLIA    // Kademlia network search
};

/**
 * Current state of a search
 */
enum class SearchState {
    IDLE,       // Search not started
    STARTING,   // Search is initializing
    RUNNING,    // Search is actively running
    PAUSED,     // Search is paused (can be resumed)
    COMPLETED,  // Search completed successfully
    FAILED,     // Search failed with error
    CANCELLED   // Search was cancelled by user
};

/**
 * Convert SearchType to string for logging
 */
inline const char* SearchTypeToString(SearchType type) {
    switch (type) {
        case SearchType::LOCAL:    return "LOCAL";
        case SearchType::GLOBAL:   return "GLOBAL";
        case SearchType::KADEMLIA: return "KADEMLIA";
        default:                   return "UNKNOWN";
    }
}

/**
 * Convert SearchState to string for logging
 */
inline const char* SearchStateToString(SearchState state) {
    switch (state) {
        case SearchState::IDLE:       return "IDLE";
        case SearchState::STARTING:   return "STARTING";
        case SearchState::RUNNING:    return "RUNNING";
        case SearchState::PAUSED:     return "PAUSED";
        case SearchState::COMPLETED:  return "COMPLETED";
        case SearchState::FAILED:     return "FAILED";
        case SearchState::CANCELLED:  return "CANCELLED";
        default:                      return "UNKNOWN";
    }
}

} // namespace search

#endif // SEARCH_TYPES_H

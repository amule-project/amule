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

#ifndef SEARCH_ID_H
#define SEARCH_ID_H

#include <cstdint>
#include <atomic>
#include <string>

namespace search {

/**
 * Unique identifier for a search
 * Thread-safe ID generation using atomic counter
 */
class SearchId {
public:
    uint64_t value;

    SearchId() : value(0) {}
    explicit SearchId(uint64_t v) : value(v) {}

    /**
     * Generate a new unique search ID
     * Thread-safe using atomic operations
     */
    static SearchId Generate() {
        static std::atomic<uint64_t> counter{1};
        return SearchId{counter.fetch_add(1, std::memory_order_relaxed)};
    }

    /**
     * Check if this ID is valid (non-zero)
     */
    bool IsValid() const {
        return value != 0;
    }

    /**
     * Comparison operators
     */
    bool operator==(const SearchId& other) const {
        return value == other.value;
    }

    bool operator!=(const SearchId& other) const {
        return value != other.value;
    }

    bool operator<(const SearchId& other) const {
        return value < other.value;
    }

    bool operator<=(const SearchId& other) const {
        return value <= other.value;
    }

    bool operator>(const SearchId& other) const {
        return value > other.value;
    }

    bool operator>=(const SearchId& other) const {
        return value >= other.value;
    }

    /**
     * Convert to string for logging
     */
    std::string ToString() const {
        return std::to_string(value);
    }

    /**
     * Invalid search ID constant
     */
    static SearchId Invalid() {
        return SearchId{0};
    }
};

} // namespace search

#endif // SEARCH_ID_H

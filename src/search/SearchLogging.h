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

#ifndef SEARCH_LOGGING_H
#define SEARCH_LOGGING_H

#include "../Logger.h"

// Central switch for search window debug logging
// Controlled by CMake option ENABLE_SEARCH_WINDOW_DEBUG
// Default to enabled if not defined by CMake
#ifndef ENABLE_SEARCH_WINDOW_DEBUG
#define ENABLE_SEARCH_WINDOW_DEBUG 0
#endif

namespace search {
    // Log categories for fine-grained control
    enum SearchLogCategory {
        LOG_SEARCH_GENERAL = 0,      // General search operations
        LOG_SEARCH_LABEL = 1,           // Tab label updates
        LOG_SEARCH_COUNT = 2,           // Hit count updates
        LOG_SEARCH_ROUTING = 3,         // Search result routing
        LOG_SEARCH_CONTROLLER = 4,      // Search controller operations
        LOG_SEARCH_MAX = 5              // Sentinel value
    };

    // Global enable/disable flags for each category
    extern bool g_searchLogEnabled[LOG_SEARCH_MAX];

    // Helper function to enable/disable a specific log category
    inline void SetSearchLogEnabled(SearchLogCategory category, bool enabled) {
        if (category >= 0 && category < LOG_SEARCH_MAX) {
            g_searchLogEnabled[category] = enabled;
        }
    }

    // Helper function to check if a log category is enabled
    inline bool IsSearchLogEnabled(SearchLogCategory category) {
        return ENABLE_SEARCH_WINDOW_DEBUG && 
               (category >= 0 && category < LOG_SEARCH_MAX) &&
               g_searchLogEnabled[category];
    }

    // Convenience macro for search window logging with category
    #define SEARCH_DEBUG_CAT(category, msg) \
        do { \
            if (search::IsSearchLogEnabled(category)) { \
                theLogger.AddLogLine(wxT(__FILE__), __LINE__, false, logStandard, msg); \
            } \
        } while(0)

    // Convenience macros for each specific category
    #define SEARCH_DEBUG(msg) SEARCH_DEBUG_CAT(search::LOG_SEARCH_GENERAL, msg)
    #define SEARCH_DEBUG_LABEL(msg) SEARCH_DEBUG_CAT(search::LOG_SEARCH_LABEL, msg)
    #define SEARCH_DEBUG_COUNT(msg) SEARCH_DEBUG_CAT(search::LOG_SEARCH_COUNT, msg)
    #define SEARCH_DEBUG_ROUTING(msg) SEARCH_DEBUG_CAT(search::LOG_SEARCH_ROUTING, msg)
    #define SEARCH_DEBUG_CONTROLLER(msg) SEARCH_DEBUG_CAT(search::LOG_SEARCH_CONTROLLER, msg)

    // Convenience macro for critical search window messages (always enabled)
    #define SEARCH_CRITICAL(msg) \
        do { \
            theLogger.AddLogLine(wxT(__FILE__), __LINE__, true, logSearch, msg); \
        } while(0)
}

#endif // SEARCH_LOGGING_H

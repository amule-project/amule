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

#ifndef SEARCHRESULTROUTER_H
#define SEARCHRESULTROUTER_H

#include <map>
#include <memory>
#include <vector>
#include <wx/string.h>
#include "../SearchFile.h"
#include "../SearchList.h"  // Add this include for CSearchList::CSearchParams

// Forward declarations
namespace search {
    class SearchController;
    class SearchModel;
    class SearchResultHandler;
}

namespace search {

/**
 * Search Result Router
 *
 * This class routes search results from network packets to the appropriate
 * controller and model. It serves as the central point for result
 * processing, ensuring that results go to the right place.
 */
class SearchResultRouter {
public:
    /**
     * Get the singleton instance
     */
    static SearchResultRouter& Instance();

    /**
     * Register a controller for a specific search ID
     *
     * @param searchId The search ID to register the controller for
     * @param controller The controller to handle results for this search
     */
    void RegisterController(uint32_t searchId, SearchController* controller);

    /**
     * Unregister a controller for a specific search ID
     *
     * @param searchId The search ID to unregister
     */
    void UnregisterController(uint32_t searchId);

    /**
     * Route a single result to the appropriate controller
     *
     * @param searchId The search ID this result belongs to
     * @param result The search result to route
     * @return true if the result was routed, false if no controller found
     */
    bool RouteResult(uint32_t searchId, CSearchFile* result);

    /**
     * Route multiple results to the appropriate controller
     *
     * @param searchId The search ID these results belong to
     * @param results Vector of search results to route
     * @return Number of results routed
     */
    size_t RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results);

    /**
     * Create a new search or reuse an existing one based on search text and type
     * This is the central entry point for all search initiation
     *
     * @param searchText The search query text
     * @param searchType The type of search (Global, Local, Kad)
     * @param params Additional search parameters
     * @return Search ID for the created/reused search
     */
    uint32_t CreateOrReuseSearch(const wxString& searchText, const wxString& searchType, const CSearchList::CSearchParams& params);

private:
    // Private constructor for singleton
    SearchResultRouter();

    // Delete copy constructor and copy assignment operator
    SearchResultRouter(const SearchResultRouter&) = delete;
    SearchResultRouter& operator=(const SearchResultRouter&) = delete;

    // Map of search IDs to controllers
    typedef std::map<uint32_t, SearchController*> ControllerMap;
    ControllerMap m_controllers;

    // Mutex for thread-safe access to controllers
    mutable wxMutex m_controllersMutex;

    // Package validator for result validation
};

} // namespace search

#endif // SEARCHRESULTROUTER_H

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

#include "SearchResultRouter.h"
#include "SearchController.h"
#include "SearchResultHandler.h"
#include "SearchLogging.h"
#include "../Logger.h"
#include <common/Format.h>
#include "../amule.h"
#include "../SearchList.h"

namespace search {

SearchResultRouter::SearchResultRouter()
{
}

SearchResultRouter& SearchResultRouter::Instance()
{
    static SearchResultRouter instance;
    return instance;
}

void SearchResultRouter::RegisterController(uint32_t searchId, SearchController* controller)
{
    m_controllers[searchId] = controller;
    SEARCH_DEBUG( 
        CFormat(wxT("Registered controller for search ID %u")) % searchId);
}

void SearchResultRouter::UnregisterController(uint32_t searchId)
{
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        m_controllers.erase(it);
        SEARCH_DEBUG( 
            CFormat(wxT("Unregistered controller for search ID %u")) % searchId);
    }
}

bool SearchResultRouter::RouteResult(uint32_t searchId, CSearchFile* result)
{
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route result to controller's handler
            handler->handleResult(searchId, result);

            SEARCH_DEBUG( 
                CFormat(wxT("Routed result for search ID %u")) % searchId);
            return true;
        }
    }

    // No controller registered for this search
    SEARCH_DEBUG( 
        CFormat(wxT("No controller registered for search ID %u, adding to SearchList")) % searchId);

    // Add result to SearchList for display
    if (theApp && theApp->searchlist) {
        result->SetSearchID(searchId);
        theApp->searchlist->AddToList(result, false);
        return true;
    }
    // Clean up the result since no one will handle it
    delete result;
    return false;
}

size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route all results to controller's handler
            handler->handleResults(searchId, results);

            SEARCH_DEBUG( 
                CFormat(wxT("Routing %zu results for search ID %u")) % results.size() % searchId);

            return results.size();
        }
    }

    // No controller registered for this search
    SEARCH_DEBUG( 
        CFormat(wxT("No controller registered for search ID %u, adding %zu results to SearchList")) 
        % searchId % results.size());

    // Add results to SearchList for display
    if (theApp && theApp->searchlist) {
        for (CSearchFile* result : results) {
            result->SetSearchID(searchId);
            theApp->searchlist->AddToList(result, false);
        }
        return results.size();
    }

    // Clean up all results since no one will handle them
    for (CSearchFile* result : results) {
        delete result;
    }

    return 0;
}

} // namespace search

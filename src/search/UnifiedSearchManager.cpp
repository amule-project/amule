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

#include "UnifiedSearchManager.h"
#include "SearchController.h"
#include "SearchControllerFactory.h"
#include "SearchLogging.h"
#include "../amule.h"
#include "../Logger.h"
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include <common/Format.h>

namespace search {

} // namespace search

namespace search {

UnifiedSearchManager::UnifiedSearchManager()
{
}

UnifiedSearchManager::~UnifiedSearchManager()
{
    wxMutexLocker lock(m_mutex);
    m_controllers.clear();
}

uint32_t UnifiedSearchManager::startSearch(const SearchParams& params, wxString& error)
{
    // Step 1: Validate prerequisites
    if (!validatePrerequisites(params, error)) {
        return 0;
    }

    // Step 2: Prepare search parameters (extract Kad keyword if needed)
    SearchParams preparedParams = params;
    if (preparedParams.searchType == ModernSearchType::KadSearch) {
        // Extract keywords from search string for Kad searches
        Kademlia::WordList words;
        Kademlia::CSearchManager::GetWords(preparedParams.searchString, &words);
        if (!words.empty()) {
            preparedParams.strKeyword = words.front();
        } else {
            error = _("No keyword for Kad search");
            return 0;
        }
    }

    // Step 3: Create appropriate controller
    auto controller = createSearchController(preparedParams);
    if (!controller) {
        error = _("Failed to create search controller");
        return 0;
    }

    // Step 4: Start the search
    controller->startSearch(preparedParams);

    // Step 5: Get search ID
    uint32_t searchId = controller->getSearchId();
    if (searchId == 0 || searchId == static_cast<uint32_t>(-1)) {
        error = _("Failed to generate search ID");
        return 0;
    }

    // Step 6: Store controller
    {
        wxMutexLocker lock(m_mutex);
        m_controllers[searchId] = std::move(controller);
    }

    AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search started with ID=%u, Type=%d"))
        % searchId % (int)preparedParams.searchType);

    return searchId;
}

void UnifiedSearchManager::stopSearch(uint32_t searchId)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        it->second->stopSearch();
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search stopped ID=%u"))
            % searchId);
    }
}

bool UnifiedSearchManager::requestMoreResults(uint32_t searchId, wxString& error)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it == m_controllers.end()) {
        error = _("Search not found");
        return false;
    }

    it->second->requestMoreResults();
    return true;
}

SearchState UnifiedSearchManager::getSearchState(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        return it->second->getState();
    }

    return SearchState::Idle;
}

std::vector<CSearchFile*> UnifiedSearchManager::getResults(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        return it->second->getResults();
    }

    return {};
}

size_t UnifiedSearchManager::getResultCount(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        return it->second->getResultCount();
    }

    return 0;
}

bool UnifiedSearchManager::isSearchActive(uint32_t searchId) const
{
    SearchState state = getSearchState(searchId);
    return state == SearchState::Searching || state == SearchState::Retrying;
}

void UnifiedSearchManager::handleResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        // Results are handled internally by the controller's SearchModel
        // We just need to ensure the controller is still active
        // The results are already added to the SearchModel by the result routing mechanism
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Received %zu results for search ID=%u"))
            % results.size() % searchId);
    }
}

void UnifiedSearchManager::markSearchComplete(uint32_t searchId, bool hasResults)
{
    AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search marked complete ID=%u, hasResults=%d"))
        % searchId % hasResults);

    // Notify callback if set
    if (m_onSearchCompleted) {
        m_onSearchCompleted(searchId, hasResults);
    }

    // Cleanup is handled by the controller's stopSearch method
}

void UnifiedSearchManager::setSearchCompletedCallback(std::function<void(uint32_t, bool)> callback)
{
    m_onSearchCompleted = std::move(callback);
}

std::unique_ptr<SearchController> UnifiedSearchManager::createSearchController(const SearchParams& params)
{
    return SearchControllerFactory::createController(params.searchType);
}

bool UnifiedSearchManager::validatePrerequisites(const SearchParams& params, wxString& error) const
{
    // Validate search parameters
    if (!params.isValid()) {
        error = _("Invalid search parameters");
        return false;
    }

    if (params.searchString.IsEmpty()) {
        error = _("Search string cannot be empty");
        return false;
    }

    // Validate network-specific prerequisites
    switch (params.searchType) {
        case ModernSearchType::LocalSearch:
        case ModernSearchType::GlobalSearch:
            // Check ED2K connection
            if (!theApp || !theApp->IsConnectedED2K()) {
                error = _("ED2K search requires connection to eD2k server");
                return false;
            }
            break;

        case ModernSearchType::KadSearch:
            // Check Kad network
            if (!theApp || !Kademlia::CKademlia::IsRunning()) {
                error = _("Kad search requires Kad network to be running");
                return false;
            }
            break;

        default:
            error = _("Unknown search type");
            return false;
    }

    return true;
}

void UnifiedSearchManager::cleanupSearch(uint32_t searchId)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        m_controllers.erase(it);
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Cleaned up search ID=%u"))
            % searchId);
    }
}

} // namespace search

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
#include "SearchControllerBase.h"
#include "SearchControllerFactory.h"
#include "SearchIdGenerator.h"
#include "SearchLogging.h"
#include "NetworkPacketHandler.h"
#include "SearchTimeoutManager.h"
#include "../amule.h"
#include "../SearchList.h"
#include "../Logger.h"
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../SearchFile.h"
#include "../MemFile.h"
#include "../updownclient.h"
#include "../Tag.h"
#include <common/Format.h>

namespace search {

} // namespace search

namespace search {

UnifiedSearchManager& UnifiedSearchManager::Instance()
{
    static UnifiedSearchManager instance;
    return instance;
}

UnifiedSearchManager::UnifiedSearchManager()
{
    // Set timeout callback to handle search timeouts
    SearchTimeoutManager::Instance().setTimeoutCallback([this](uint32_t searchId, SearchTimeoutManager::SearchType type, const wxString& reason) {
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search %u timed out (type=%d): %s"))
            % searchId % (int)type % reason);
        
        // Get result count before stopping (without holding mutex)
        size_t resultCount = 0;
        bool hasResults = false;
        {
            wxMutexLocker lock(m_mutex);
            auto it = m_controllers.find(searchId);
            if (it != m_controllers.end()) {
                resultCount = it->second->getResultCount();
                hasResults = (resultCount > 0);
            }
        }
        
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Timed out search %u has %zu results"))
            % searchId % resultCount);
        
        // Stop the search (this will acquire its own mutex)
        stopSearch(searchId);
        
        // Mark as complete
        markSearchComplete(searchId, hasResults);
        
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Timed out search %u marked as complete (hasResults=%d)"))
            % searchId % hasResults);
    });
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

    // Step 3: Generate search ID BEFORE creating controller
    // This ensures all controllers use the same ID generation mechanism
    uint32_t searchId = SearchIdGenerator::Instance().generateId();
    preparedParams.setSearchId(searchId);

    // Step 4: Create appropriate controller
    auto controller = createSearchController(preparedParams);
    if (!controller) {
        error = _("Failed to create search controller");
        return 0;
    }

    // Step 5: Start the search
    controller->startSearch(preparedParams);

    // Verify the controller used the correct search ID
    uint32_t controllerSearchId = controller->getSearchId();
    if (controllerSearchId != searchId) {
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search ID mismatch! Expected %u, got %u"))
            % searchId % controllerSearchId);
        // Use the controller's ID instead
        searchId = controllerSearchId;
    }

    if (searchId == 0 || searchId == static_cast<uint32_t>(-1)) {
        error = _("Failed to generate search ID");
        return 0;
    }

    // Step 6: Store controller
    {
        wxMutexLocker lock(m_mutex);
        m_controllers[searchId] = std::move(controller);
    }

    // Step 7: Register search ID with NetworkPacketHandler for packet routing
    bool isKadSearch = (preparedParams.searchType == ModernSearchType::KadSearch);
    NetworkPacketHandler::Instance().RegisterSearchID(searchId, isKadSearch);

    // Step 8: Register search with timeout manager
    SearchTimeoutManager::SearchType timeoutType = SearchTimeoutManager::LocalSearch;
    if (preparedParams.searchType == ModernSearchType::GlobalSearch) {
        timeoutType = SearchTimeoutManager::GlobalSearch;
    } else if (preparedParams.searchType == ModernSearchType::KadSearch) {
        timeoutType = SearchTimeoutManager::KadSearch;
    }
    SearchTimeoutManager::Instance().registerSearch(searchId, timeoutType);

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
        m_controllers.erase(it);
    }

    // Unregister search ID from NetworkPacketHandler
    NetworkPacketHandler::Instance().UnregisterSearchID(searchId);

    // Unregister search from timeout manager
    SearchTimeoutManager::Instance().unregisterSearch(searchId);
}

void UnifiedSearchManager::stopAllSearches()
{
    wxMutexLocker lock(m_mutex);

    for (auto& pair : m_controllers) {
        if (pair.second) {
            pair.second->stopSearch();
        }
    }

    AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Stopped all searches (%zu total)"))
        % m_controllers.size());
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

size_t UnifiedSearchManager::getShownResultCount(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        SearchControllerBase* baseCtrl = dynamic_cast<SearchControllerBase*>(it->second.get());
        if (baseCtrl && baseCtrl->getModel()) {
            return baseCtrl->getModel()->getShownResultCount();
        }
    }

    return 0;
}

size_t UnifiedSearchManager::getHiddenResultCount(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        SearchControllerBase* baseCtrl = dynamic_cast<SearchControllerBase*>(it->second.get());
        if (baseCtrl && baseCtrl->getModel()) {
            return baseCtrl->getModel()->getHiddenResultCount();
        }
    }

    return 0;
}

CSearchFile* UnifiedSearchManager::getResultByIndex(uint32_t searchId, size_t index) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        // Get results from controller and return by index
        auto results = it->second->getResults();
        if (index < results.size()) {
            return results[index];
        }
    }

    return nullptr;
}

CSearchFile* UnifiedSearchManager::getResultByHash(uint32_t searchId, const CMD4Hash& hash) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        auto results = it->second->getResults();
        for (auto* result : results) {
            if (result && result->GetFileHash() == hash) {
                return result;
            }
        }
    }

    return nullptr;
}

SearchModel* UnifiedSearchManager::getSearchModel(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        SearchControllerBase* baseCtrl = dynamic_cast<SearchControllerBase*>(it->second.get());
        if (baseCtrl) {
            return baseCtrl->getModel();
        }
    }

    return nullptr;
}

void UnifiedSearchManager::filterResults(uint32_t searchId, const wxString& filter, bool invert, bool knownOnly)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        SearchControllerBase* baseCtrl = dynamic_cast<SearchControllerBase*>(it->second.get());
        if (baseCtrl && baseCtrl->getModel()) {
            baseCtrl->getModel()->filterResults(filter, invert, knownOnly);
            AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Filter applied for search %u: filter='%s', invert=%d, knownOnly=%d"))
                % searchId % filter % invert % knownOnly);
        }
    }
}

void UnifiedSearchManager::clearFilters(uint32_t searchId)
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        SearchControllerBase* baseCtrl = dynamic_cast<SearchControllerBase*>(it->second.get());
        if (baseCtrl && baseCtrl->getModel()) {
            baseCtrl->getModel()->clearFilters();
            AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Filters cleared for search %u"))
                % searchId);
        }
    }
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

std::vector<uint32_t> UnifiedSearchManager::getActiveSearchIds() const
{
    wxMutexLocker lock(m_mutex);

    std::vector<uint32_t> ids;
    ids.reserve(m_controllers.size());

    for (const auto& pair : m_controllers) {
        if (pair.second && pair.second->getState() == SearchState::Searching) {
            ids.push_back(pair.first);
        }
    }

    return ids;
}

uint32_t UnifiedSearchManager::getSearchProgress(uint32_t searchId) const
{
    wxMutexLocker lock(m_mutex);

    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        return it->second->getProgress();
    }

    return 0;
}

void UnifiedSearchManager::removeResults(uint32_t searchId)
{
    // Delegate to CSearchList for result removal
    if (theApp && theApp->searchlist) {
        theApp->searchlist->RemoveResults(searchId);
    }
}

void UnifiedSearchManager::addFileToDownloadByHash(const CMD4Hash& hash, uint8_t category)
{
    // Delegate to CSearchList for download
    if (theApp && theApp->searchlist) {
        theApp->searchlist->AddFileToDownloadByHash(hash, category);
    }
}

void UnifiedSearchManager::setKadSearchFinished()
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->SetKadSearchFinished();
    }
}

const CSearchResultList& UnifiedSearchManager::getSearchResults(uint32_t searchId) const
{
    static CSearchResultList emptyList;
    if (theApp && theApp->searchlist) {
        return theApp->searchlist->GetSearchResults(searchId);
    }
    return emptyList;
}

CSearchFile* UnifiedSearchManager::getSearchFileById(uint32_t parentId) const
{
    // Search through all results to find the file with the given ECID
    // This is used by the remote GUI to find parent files
    if (theApp && theApp->searchlist) {
        // Get all results and search for the parent
        const CSearchResultList& allResults = theApp->searchlist->GetSearchResults(0xffffffff);
        for (CSearchFile* file : allResults) {
            if (file && file->ECID() == parentId) {
                return file;
            }
        }
    }
    return nullptr;
}

void UnifiedSearchManager::updateSearchFileByHash(const CMD4Hash& hash)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->UpdateSearchFileByHash(hash);
    }
}

void UnifiedSearchManager::processSearchAnswer(const uint8_t* packet, uint32_t size, bool optUTF8,
                                               uint32_t serverIP, uint16_t serverPort)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->ProcessSearchAnswer(packet, size, optUTF8, serverIP, serverPort);
    }
}

void UnifiedSearchManager::processUDPSearchAnswer(const CMemFile& packet, bool optUTF8,
                                                  uint32_t serverIP, uint16_t serverPort)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->ProcessUDPSearchAnswer(packet, optUTF8, serverIP, serverPort);
    }
}

void UnifiedSearchManager::processKadSearchKeyword(uint32_t searchID, const Kademlia::CUInt128* fileID,
                                                   const wxString& name, uint64_t size,
                                                   const wxString& type, uint32_t kadPublishInfo,
                                                   const TagPtrList& taglist)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->KademliaSearchKeyword(searchID, fileID, name, size, type, kadPublishInfo, taglist);
    }
}

void UnifiedSearchManager::localSearchEnd()
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->LocalSearchEnd();
    }
}

void UnifiedSearchManager::processSharedFileList(const uint8_t* packet, uint32_t size,
                                                 CUpDownClient* sender, bool* moreResultsAvailable,
                                                 const wxString& directory)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->ProcessSharedFileList(packet, size, sender, moreResultsAvailable, directory);
    }
}

void UnifiedSearchManager::addToList(CSearchFile* result, bool bClientResponse)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->AddToList(result, bClientResponse);
    }
}

void UnifiedSearchManager::mapKadSearchId(uint32_t kadSearchId, uint32_t searchId)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->mapKadSearchId(kadSearchId, searchId);
    }
}

void UnifiedSearchManager::removeKadSearchIdMapping(uint32_t kadSearchId)
{
    if (theApp && theApp->searchlist) {
        theApp->searchlist->removeKadSearchIdMapping(kadSearchId);
    }
}

} // namespace search

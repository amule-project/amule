
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

#include "KadSearchController.h"
#include "KadSearchPacketBuilder.h"
#include "SearchPackageValidator.h"
#include "SearchResultRouter.h"
#include "SearchLogging.h"
#include "UnifiedSearchManager.h"
#include "SearchIdGenerator.h"
#include "../amule.h"
#include "../SearchFile.h"
#include "../SearchList.h"
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../kademlia/kademlia/Search.h"
#include <wx/utils.h>

namespace search {

KadSearchController::KadSearchController()
    : SearchControllerBase()
    , m_maxNodesToQuery(DEFAULT_MAX_NODES)
    , m_nodesContacted(0)
    , m_kadSearch(nullptr)
{
}

KadSearchController::~KadSearchController()
{
    // Clean up Kademlia search object
    // Note: CSearchManager owns the search object and will delete it
    // Just call StopSearch to let CSearchManager handle cleanup
    if (m_kadSearch) {
        uint32_t kadSearchId = m_kadSearch->GetSearchID();
        // Remove the Kad search ID mapping
        if (theApp && theApp->searchlist) {
            UnifiedSearchManager::Instance().removeKadSearchIdMapping(kadSearchId);
        }
        Kademlia::CSearchManager::StopSearch(kadSearchId, false);
        m_kadSearch = nullptr;
    }
}

void KadSearchController::startSearch(const SearchParams& params)
{
    // Step 1: Validate prerequisites
    if (!validatePrerequisites()) {
	return;
    }

    // Step 2: Validate search parameters
    if (!validateSearchParams(params)) {
	return;
    }

    // Step 3: Prepare search
    initializeProgress();
    resetSearchState();

    // Generate search ID
    uint32_t searchId = 0;
    
    // Build search packet using KadSearchPacketBuilder
    KadSearchPacketBuilder packetBuilder;
    wxString error;
    
    // Build Kad search packet
    uint8_t* packetData = nullptr;
    uint32_t packetSize = 0;
    bool success = packetBuilder.CreateSearchPacket(params, packetData, packetSize);
    
    if (!success || !packetData) {
	error = wxT("Failed to create Kad search packet");
	return handleSearchError(0, error);
    }
    
    try {
	// Generate search ID
	searchId = params.getSearchId();
	if (searchId == 0 || searchId == static_cast<uint32_t>(-1)) {
	    // No search ID provided - this should not happen with UnifiedSearchManager
	    // Log an error and return
	    AddDebugLogLineC(logSearch, wxT("KadSearchController::startSearch: No search ID provided!"));
	    delete[] packetData;
	    return;
	}

	// Store search ID and state
	m_model->setSearchParams(params);
	m_model->setSearchId(searchId);
	m_model->setSearchState(SearchState::Searching);

	// Register with SearchResultRouter for result routing
	SearchResultRouter::Instance().RegisterController(searchId, this);

	// Send packet to Kad network
	if (theApp && Kademlia::CKademlia::IsRunning()) {
	    // Use legacy Kad search implementation
	    try {
		// Convert our search ID to Kademlia's format (0xffffff??)
		// This ensures Kademlia uses our ID instead of generating its own
		uint32_t kadSearchId = 0xffffff00 | (searchId & 0xff);

		Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(
		    params.strKeyword,
		    packetSize,
		    packetData,
		    kadSearchId
		);

		// Verify Kademlia used our ID
		if (search->GetSearchID() != kadSearchId) {
		    AddDebugLogLineC(logSearch, CFormat(wxT("Kademlia changed search ID: expected %u, got %u"))
			% kadSearchId % search->GetSearchID());
		    delete search;
		    error = _("Kademlia search ID mismatch");
		    handleSearchError(searchId, error);
		    delete[] packetData;
		    return;
		}

		// Map Kad search ID to original search ID for result routing
		if (theApp->searchlist) {
		    UnifiedSearchManager::Instance().mapKadSearchId(kadSearchId, searchId);
		}

		// Store the Kademlia search object for later reference
		m_kadSearch = search;

		notifySearchStarted(searchId);
	    } catch (const wxString& what) {
		error = wxString::Format(_("Failed to start Kad search: %s"), what.c_str());
		handleSearchError(searchId, error);
		throw;  // Re-throw to ensure packetData is cleaned up
	    }
	} else {
	    error = _("Kad network not available");
	    handleSearchError(searchId, error);
	}
    } catch (...) {
	// Clean up packet data on any exception
	delete[] packetData;
	throw;
    }
    
    // Clean up packet data on success
    delete[] packetData;
}

void KadSearchController::stopSearch()
{
    // Stop Kademlia search if active
    if (m_kadSearch) {
        uint32_t kadSearchId = m_kadSearch->GetSearchID();
        // Remove the Kad search ID mapping
        if (theApp && theApp->searchlist) {
            UnifiedSearchManager::Instance().removeKadSearchIdMapping(kadSearchId);
        }
        // CSearchManager owns the search object and will delete it
        Kademlia::CSearchManager::StopSearch(kadSearchId, false);
        // Just clear our pointer, don't delete - CSearchManager handles deletion
        m_kadSearch = nullptr;
    }

    // Unregister from SearchResultRouter
    long searchId = m_model->getSearchId();
    if (searchId != -1) {
        SearchResultRouter::Instance().UnregisterController(searchId);
    }

    // Clear results
    m_model->clearResults();

    // Use base class to handle common stop logic
    stopSearchBase();
}

void KadSearchController::requestMoreResults()
{
    uint32_t searchId = m_model->getSearchId();

    // Check if Kad search is still active
    // Check both the kad search object AND the model state
    bool isKadSearchActive = (m_kadSearch != nullptr);
    bool isModelSearching = (m_model->getSearchState() == SearchState::Searching);
    
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Search %u, m_kadSearch=%p, state=%d"))
        % searchId % m_kadSearch % (int)m_model->getSearchState());

    // If the search is not active, we can't request more results
    // But we should update the state to prevent getting stuck at [Searching]
    if (!isKadSearchActive && !isModelSearching) {
        // Search has already completed or was stopped
        // Update the state to prevent getting stuck at [Searching]
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Search %u is not active, updating state"))
            % searchId);

        // If the model still thinks it's searching, update it
        if (isModelSearching) {
            // Check if we have any results
            bool hasResults = (m_model->getResultCount() > 0);
            m_model->setSearchState(hasResults ? SearchState::Completed : SearchState::Completed);
            // Notify observers that search has completed
            notifySearchCompleted(searchId);
        }
        return;
    }

    // Check if Kad network is still running
    if (!Kademlia::CKademlia::IsRunning()) {
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Kad network is not available for search %u"))
            % searchId);
        return;
    }

    // Use the new RequestMoreResults() method in CSearch
    // This will re-contact nodes and request additional results using KADEMLIA_FIND_VALUE_MORE
    // Duplicate detection is handled by SearchModel::isDuplicate() which checks by hash and size
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Requesting more results for search ID %u"))
        % searchId);

    try {
        bool requested = m_kadSearch->RequestMoreResults();
        if (requested) {
            AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Successfully requested more results for search ID %u"))
                % searchId);
        } else {
            // Could not request more results (no nodes responded, already requesting, or search is stopping)
            // Silently fail - no need to show error to user
            AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Could not request more results for search ID %u (no suitable nodes)"))
                % searchId);
        }
    } catch (const wxString& e) {
        // Log error but don't show popup to user
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Exception for search %u: %s"))
            % searchId % e);
    } catch (...) {
        // Log error but don't show popup to user
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::requestMoreResults: Unknown exception for search %u"))
            % searchId);
    }
}

void KadSearchController::setMaxNodesToQuery(int maxNodes)
{
    m_maxNodesToQuery = maxNodes;
}

int KadSearchController::getMaxNodesToQuery() const
{
    return m_maxNodesToQuery;
}

void KadSearchController::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}

int KadSearchController::getRetryCount() const
{
    return m_retryCount;
}

bool KadSearchController::validateConfiguration() const
{
    if (!SearchControllerBase::validateConfiguration()) {
	return false;
    }

    if (m_maxNodesToQuery <= 0) {
	return false;
    }

    return true;
}

void KadSearchController::updateProgress()
{
    ProgressInfo info;

    // Calculate percentage based on nodes contacted vs max
    if (m_maxNodesToQuery > 0) {
	info.percentage = (m_nodesContacted * 100) / m_maxNodesToQuery;
    }

    info.serversContacted = 0; // Not applicable for Kad
    info.resultsReceived = getResultCount();

    // Set status based on state
    switch (getState()) {
	case SearchState::Searching:
	    info.currentStatus = _("Searching Kad network...");
	    break;
	case SearchState::Retrying:
	    info.currentStatus = wxString::Format(_("Retrying search (%d/%d)..."),
				                m_currentRetry, m_retryCount);
	    break;
	case SearchState::Completed:
	    info.currentStatus = _("Search completed");
	    break;
	default:
	    info.currentStatus = _("Idle");
	    break;
    }

    uint32_t searchId = m_model->getSearchId();
    notifyDetailedProgress(searchId, info);
    notifyProgress(searchId, info.percentage);
}

void KadSearchController::initializeProgress()
{
    m_nodesContacted = 0;
    updateProgress();
}

bool KadSearchController::validatePrerequisites()
{
    if (!SearchControllerBase::validatePrerequisites()) {
	return false;
    }

    if (!isValidKadNetwork()) {
	uint32_t searchId = m_model->getSearchId();
	handleSearchError(searchId, _("Kad network not available"));
	return false;
    }

    return true;
}

bool KadSearchController::isValidKadNetwork() const
{
    if (!theApp) {
	return false;
    }

    // Check if Kad is running
    return Kademlia::CKademlia::IsRunning();
}

void KadSearchController::onKadSearchComplete(uint32_t kadSearchId, bool hasResults)
{
    uint32_t searchId = m_model->getSearchId();
    
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::onKadSearchComplete: Kad search ID=%u, our ID=%u, hasResults=%d"))
        % kadSearchId % searchId % hasResults);
    
    // Update state to Completed
    m_model->setSearchState(SearchState::Completed);
    
    // Notify completion
    notifySearchCompleted(searchId);
    
    // Clear Kad search reference
    m_kadSearch = nullptr;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::onKadSearchComplete: Search %u marked as complete (hasResults=%d)"))
        % searchId % hasResults);
}

void KadSearchController::checkKadSearchState()
{
    if (!m_kadSearch) {
        // Search object is gone, mark as complete
        uint32_t searchId = m_model->getSearchId();
        bool hasResults = (m_model->getResultCount() > 0);
        
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::checkKadSearchState: Kad search object is null, marking search %u as complete"))
            % searchId);
        
        // Only mark as complete if we're still in Searching state
        if (m_model->getSearchState() == SearchState::Searching) {
            onKadSearchComplete(searchId, hasResults);
        }
        return;
    }
    
    // Check if Kad search is stopping
    if (m_kadSearch->Stopping()) {
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::checkKadSearchState: Kad search is stopping")));
        // Wait a bit for cleanup, then mark as complete
        uint32_t searchId = m_model->getSearchId();
        bool hasResults = (m_model->getResultCount() > 0);
        
        // Give it a moment to finish cleanup
        if (m_model->getSearchState() == SearchState::Searching) {
            onKadSearchComplete(searchId, hasResults);
        }
    }
}

} // namespace search

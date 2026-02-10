
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
            theApp->searchlist->removeKadSearchIdMapping(kadSearchId);
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
    
    try {
	// Build Kad search packet
	uint8_t* packetData = nullptr;
	uint32_t packetSize = 0;
	bool success = packetBuilder.CreateSearchPacket(params, packetData, packetSize);
	
	if (!success || !packetData) {
	    error = wxT("Failed to create Kad search packet");
	    return handleSearchError(0, error);
	}
	
	// Generate search ID
	searchId = GenerateSearchId();
	
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
		    theApp->searchlist->mapKadSearchId(kadSearchId, searchId);
		}

		// Store the Kademlia search object for later reference
		m_kadSearch = search;

		notifySearchStarted(searchId);
	    } catch (const wxString& what) {
		error = wxString::Format(_("Failed to start Kad search: %s"), what.c_str());
		handleSearchError(searchId, error);
	    }

	    // Clean up packet data
	    delete[] packetData;
	} else {
	    delete[] packetData;
	    error = _("Kad network not available");
	    handleSearchError(searchId, error);
	}
    } catch (const wxString& e) {
	error = wxString::Format(_("Failed to execute Kad search: %s"), e.c_str());
	handleSearchError(0, error);
    }
}

void KadSearchController::stopSearch()
{
    // Stop Kademlia search if active
    if (m_kadSearch) {
        uint32_t kadSearchId = m_kadSearch->GetSearchID();
        // Remove the Kad search ID mapping
        if (theApp && theApp->searchlist) {
            theApp->searchlist->removeKadSearchIdMapping(kadSearchId);
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
    // Kad searches don't support "more results" in the traditional sense
    // as they are keyword-based and query the entire network
    uint32_t searchId = m_model->getSearchId();
    handleSearchError(searchId, _("Kad searches query the entire network and don't support requesting more results"));
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
    // TODO: Implement Kad network check
    // For now, return true to allow testing
    return true;
}

uint32_t KadSearchController::GenerateSearchId()
{
    // Use the centralized SearchIdGenerator for consistency across all search types
    return search::SearchIdGenerator::Instance().generateId();
}

} // namespace search

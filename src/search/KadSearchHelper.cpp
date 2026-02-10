
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

#include "KadSearchHelper.h"
#include "SearchController.h"
#include "../amule.h"
#include "../SearchList.h"
#include "../MemFile.h"
#include "../Logger.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../kademlia/kademlia/Kademlia.h"
#include "PerSearchState.h"
#include <wx/utils.h>

// Kad headers
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../kademlia/kademlia/Search.h"

namespace search {

bool KadSearchHelper::CanPerformSearch()
{
	return Kademlia::CKademlia::IsRunning();
}

bool KadSearchHelper::ExtractKeyword(const wxString& searchString, wxString& keyword)
{
	if (!theApp || !theApp->searchlist) {
		return false;
	}

	// Use SearchList's word extraction (temporary)
	Kademlia::WordList words;
	Kademlia::CSearchManager::GetWords(searchString, &words);

	if (words.empty()) {
		return false;
	}

	keyword = words.front();
	return true;
}

bool KadSearchHelper::CreateSearchPacket(const SearchParams& params,
					 uint8_t*& packetData, uint32_t& packetSize)
{
	if (!theApp || !theApp->searchlist) {
		return false;
	}

	// Convert to old parameter format
	CSearchList::CSearchParams oldParams;
	oldParams.searchString = params.searchString;
	oldParams.strKeyword = params.strKeyword;
	oldParams.typeText = params.typeText;
	oldParams.extension = params.extension;
	oldParams.minSize = params.minSize;
	oldParams.maxSize = params.maxSize;
	oldParams.availability = params.availability;

	// Use the existing CreateSearchData function from SearchList
	// For Kad search, we always support 64-bit
	bool supports64bit = true;
	bool packetUsing64bit = false;
	
	// Use the global application instance to access SearchList
	if (!theApp || !theApp->searchlist) {
		return false;
	}
	
	CSearchList::CMemFilePtr data = theApp->searchlist->CreateSearchData(oldParams, KadSearch, supports64bit, packetUsing64bit);
	
	if (!data.get()) {
		return false;
	}
	
	// Allocate memory for the packet data
	packetSize = data->GetLength();
	packetData = new uint8_t[packetSize];
	
	// Add bounds checking - ensure we have valid data
	wxASSERT(packetSize > 0);
	if (packetSize == 0) {
		delete[] packetData;
		packetData = NULL;
		return false;
	}
	
	memcpy(packetData, data->GetRawBuffer(), packetSize);
	
	return true;
}

bool KadSearchHelper::StartSearch(const uint8_t* packetData, uint32_t packetSize,
				  const wxString& keyword, uint32_t& searchId)
{
	if (!Kademlia::CKademlia::IsRunning() || !packetData) {
		return false;
	}

	// Use Kademlia's search manager to start the search
	try {
		// If searchId is 0xffffffff, stop any existing search first
		if (searchId == 0xffffffff) {
			Kademlia::CSearchManager::StopSearch(0xffffffff, false);
		}

		// Start the Kad search
		// The tab must be created with the Kad search ID, so searchId is updated.
		Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(keyword, packetSize, packetData, searchId);

		searchId = search->GetSearchID();
		
		// Create per-search state for this Kad search
		if (theApp && theApp->searchlist) {
			// We need to get the search string, but we only have the keyword
			// For now, use the keyword as the search string
			wxString searchString = keyword;
			
			auto* searchState = theApp->searchlist->getOrCreateSearchState(searchId, KadSearch, searchString);
			if (searchState) {
				// Initialize Kad search state
				searchState->setKadSearchFinished(false);
				searchState->setKadSearchRetryCount(0);
			}
		}
		
		return true;
	} catch (const wxString& what) {
		AddLogLineC(what);
		return false;
	}
}

bool KadSearchHelper::StopSearch(uint32_t searchId)
{
	if (!Kademlia::CKademlia::IsRunning()) {
		return false;
	}

	// Stop the Kad search
	Kademlia::CSearchManager::StopSearch(searchId, true);
	
	// Remove per-search state
	if (theApp && theApp->searchlist) {
		theApp->searchlist->removeSearchState(searchId);
	}
	
	return true;
}

void KadSearchHelper::FreeSearchPacket(uint8_t* packetData)
{
	if (packetData) {
		delete[] packetData;
	}
}

} // namespace search

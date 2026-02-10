
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

#include "ED2KSearchHelper.h"
#include "SearchController.h"
#include "../amule.h"
#include "../ServerConnect.h"
#include "../SearchList.h"
#include "../Statistics.h"
#include "../Packet.h"
#include <protocol/Protocols.h>
#include "../include/common/MacrosProgramSpecific.h"
#include "../Server.h"
#include "../SearchList.h"
#include "../Packet.h"
#include "../MemFile.h"
#include "../OtherFunctions.h"
#include <wx/utils.h>

namespace search {

bool ED2KSearchHelper::CanPerformSearch()
{
	return theApp && theApp->IsConnectedED2K();
}

bool ED2KSearchHelper::SupportsLargeFiles()
{
	if (!theApp || !theApp->serverconnect) {
		return false;
	}

	CServer* server = theApp->serverconnect->GetCurrentServer();
	return server != NULL && (server->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
}

bool ED2KSearchHelper::CreateSearchPacket(const SearchParams& params, ModernSearchType searchType,
					  uint8_t*& packetData, uint32_t& packetSize)
{
	if (!theApp || !theApp->searchlist) {
		return false;
	}

	// Convert ModernSearchType to legacy SearchType
	SearchType legacyType = static_cast<SearchType>(static_cast<int>(searchType));
	
	// Convert SearchParams to CSearchParams
	CSearchList::CSearchParams legacyParams;
	legacyParams.searchString = params.searchString;
	legacyParams.strKeyword = params.strKeyword;
	legacyParams.typeText = params.typeText;
	legacyParams.extension = params.extension;
	legacyParams.minSize = params.minSize;
	legacyParams.maxSize = params.maxSize;
	legacyParams.availability = params.availability;
	legacyParams.searchType = legacyType;
	
	// Use the existing CreateSearchData function from SearchList
	bool supports64bit = SupportsLargeFiles();
	bool packetUsing64bit = false;
	legacyParams.searchString = params.searchString;
	legacyParams.strKeyword = params.strKeyword;
	legacyParams.typeText = params.typeText;
	legacyParams.extension = params.extension;
	legacyParams.minSize = params.minSize;
	legacyParams.maxSize = params.maxSize;
	legacyParams.availability = params.availability;
	legacyParams.searchType = legacyType;
	
	// Use the existing CreateSearchData function from SearchList
	// Note: This requires access to the SearchList instance
	// For now, we'll use the global application instance
	if (!theApp || !theApp->searchlist) {
		return false;
	}
	
	CSearchList::CMemFilePtr data = theApp->searchlist->CreateSearchData(legacyParams, legacyType, supports64bit, packetUsing64bit);
	
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

bool ED2KSearchHelper::SendSearchPacket(const uint8_t* packetData, uint32_t packetSize,
					 ModernSearchType searchType)
{
	if (!packetData || packetSize == 0) {
		return false;
	}
	
	// Convert ModernSearchType to legacy SearchType
	SearchType legacyType = static_cast<SearchType>(static_cast<int>(searchType));
	
	// Create a CPacket from the raw data
	CPacket* searchPacket = new CPacket(packetSize, OP_EDONKEYPROT, OP_SEARCHREQUEST);
	
	// Add bounds checking - ensure packet size matches data size
	wxASSERT(packetSize == searchPacket->GetPacketSize());
	if (packetSize != searchPacket->GetPacketSize()) {
		delete searchPacket;
		return false;
	}
	
	memcpy(const_cast<uint8_t*>(searchPacket->GetDataBuffer()), packetData, packetSize);
	
	// Send the packet based on search type
	bool isLocalSearch = (legacyType == LocalSearch);
	
	NOT_ON_REMOTEGUI(
		theStats::AddUpOverheadServer(searchPacket->GetPacketSize())
	);
	
	// Use the global server connection
	if (!theApp || !theApp->serverconnect) {
		delete searchPacket;
		return false;
	}
	
	theApp->serverconnect->SendPacket(searchPacket, isLocalSearch);
	
	return true;
}

void ED2KSearchHelper::FreeSearchPacket(uint8_t* packetData)
{
	if (packetData) {
		delete[] packetData;
	}
}

} // namespace search

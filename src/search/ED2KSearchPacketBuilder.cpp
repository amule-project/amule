
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

#include "ED2KSearchPacketBuilder.h"
#include "SearchController.h"

#include "../SearchList.h"
#include "../amule.h"
#include "../MemFile.h"
#include <wx/utils.h>

namespace search {

bool ED2KSearchPacketBuilder::CreateSearchPacket(const SearchParams& params, bool supports64bit,
					     uint8_t*& packetData, uint32_t& packetSize)
{
    if (!theApp || !theApp->searchlist) {
	return false;
    }

    // Create a mutable copy for the search (keyword may be modified by parser)
    SearchParams mutableParams = params;

    // Determine search type
    ::SearchType type = static_cast<SearchType>(static_cast<int>(ModernSearchType::LocalSearch));

    // Use SearchList's CreateSearchData method with search::SearchParams
    bool packetUsing64bit = false;
    CSearchList::CMemFilePtr data = theApp->searchlist->CreateSearchData(
	mutableParams, type, supports64bit, packetUsing64bit);

    if (data.get() == NULL) {
	return false;
    }

    // Store packet data
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

void ED2KSearchPacketBuilder::FreeSearchPacket(uint8_t* packetData)
{
    if (packetData) {
	delete[] packetData;
    }
}

bool ED2KSearchPacketBuilder::EncodeSearchParams(const SearchParams& params, bool supports64bit,
					       uint8_t*& packetData, uint32_t& packetSize)
{
    // For now, we use SearchList's StartNewSearch method
    // This is temporary during migration
    // We'll implement proper packet encoding in Phase 3
    return false;
}

} // namespace search

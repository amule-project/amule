
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

#include "KadSearchPacketBuilder.h"
#include "SearchController.h"
#include "SearchLogging.h"
#include "../SearchList.h"
#include "../amule.h"
#include "../MemFile.h"
#include <wx/utils.h>
#include <common/Format.h>

namespace search {

bool KadSearchPacketBuilder::CreateSearchPacket(const SearchParams& params,
						uint8_t*& packetData, uint32_t& packetSize)
{
    if (!theApp || !theApp->searchlist) {
	AddDebugLogLineC(logSearch, wxT("KadSearchPacketBuilder: theApp or searchlist is NULL"));
	return false;
    }

    // Check if strKeyword is set (required for Kad searches)
    if (params.strKeyword.IsEmpty()) {
	AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchPacketBuilder: strKeyword is empty, searchString='%s'"))
	    % params.searchString);
	return false;
    }

    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchPacketBuilder: Creating packet for keyword='%s', searchString='%s'"))
	% params.strKeyword % params.searchString);

    // For Kad searches, we need to build the packet directly
    // The legacy parser doesn't work well for Kad searches because it expects
    // the search string to be parsed, but Kad uses the extracted keyword directly
    // We'll use the legacy CreateSearchData method but with the keyword as the search string

    // Convert to old parameter format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.strKeyword;  // Use keyword as search string for Kad
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;

    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchPacketBuilder: Using keyword as search string: '%s'"))
	% oldParams.searchString);

    // Use SearchList's CreateSearchData method
    bool packetUsing64bit = false;
    CSearchList::CMemFilePtr data = theApp->searchlist->CreateSearchData(
	oldParams, ::KadSearch, true, packetUsing64bit, oldParams.strKeyword);

    if (data.get() == NULL) {
	AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchPacketBuilder: CreateSearchData returned NULL for keyword='%s'"))
	    % oldParams.strKeyword);
	return false;
    }

    // Store packet data
    packetSize = data->GetLength();

    // Validate packet size before allocating memory
    if (packetSize == 0) {
	AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchPacketBuilder: Packet size is 0 for keyword='%s'"))
	    % oldParams.strKeyword);
	packetData = NULL;
	return false;
    }

    wxASSERT(packetSize > 0);

    packetData = new uint8_t[packetSize];
    memcpy(packetData, data->GetRawBuffer(), packetSize);

    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchPacketBuilder: Successfully created packet of size %u for keyword='%s'"))
	% packetSize % oldParams.strKeyword);

    return true;
}

void KadSearchPacketBuilder::FreeSearchPacket(uint8_t* packetData)
{
    if (packetData) {
	delete[] packetData;
    }
}

bool KadSearchPacketBuilder::EncodeSearchParams(const SearchParams& params,
					       uint8_t*& packetData, uint32_t& packetSize)
{
    // For now, we use SearchList's CreateSearchData method
    // This is temporary during migration
    // We'll implement proper packet encoding in Phase 3
    return false;
}

} // namespace search

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

#include "NetworkPacketHandler.h"
#include "../SearchFile.h"
#include "../MemFile.h"
#include "../SearchList.h"
#include "../Logger.h"
#include "../amule.h"
#include <common/Format.h>
#include "SearchResultRouter.h"
#include "SearchLogging.h"

// Kademlia includes
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"

namespace search {

NetworkPacketHandler::NetworkPacketHandler()
{
}

NetworkPacketHandler& NetworkPacketHandler::Instance()
{
    static NetworkPacketHandler instance;
    return instance;
}

size_t NetworkPacketHandler::ProcessED2KTCPSearchResult(
    const uint8_t* packet,
    uint32_t size,
    bool optUTF8,
    uint32_t serverIP,
    uint16_t serverPort)
{
    // For now, delegate to legacy CSearchList for compatibility
    // TODO: Migrate to unified packet processing
    if (theApp && theApp->searchlist) {
        theApp->searchlist->ProcessSearchAnswer(packet, size, optUTF8, serverIP, serverPort);
        return 1; // Return count (legacy doesn't track this)
    }
    return 0;
}

size_t NetworkPacketHandler::ProcessED2KUDPSearchResult(
    const uint8_t* packet,
    bool optUTF8,
    uint32_t serverIP,
    uint16_t serverPort)
{
    // For now, delegate to legacy CSearchList for compatibility
    // TODO: Migrate to unified packet processing
    CMemFile dataFile(const_cast<uint8_t*>(packet), 1024);
    if (theApp && theApp->searchlist) {
        theApp->searchlist->ProcessUDPSearchAnswer(dataFile, optUTF8, serverIP, serverPort);
        return 1; // Return count (legacy doesn't track this)
    }
    return 0;
}

size_t NetworkPacketHandler::ProcessKadSearchResult(
    uint32_t searchID,
    const Kademlia::CUInt128* fileID,
    const wxString& name,
    uint64_t size,
    const wxString& type,
    uint32_t kadPublishInfo,
    const std::vector<Kademlia::Tag*>& tagList)
{
    // For now, delegate to legacy CSearchList for compatibility
    // TODO: Migrate to unified packet processing
    if (theApp && theApp->searchlist) {
        // Convert vector to list for legacy API
        TagPtrList tagListConverted;
        for (auto* tag : tagList) {
            tagListConverted.push_back(reinterpret_cast<CTag*>(tag));
        }
        theApp->searchlist->KademliaSearchKeyword(searchID, fileID, name, size, type, kadPublishInfo, tagListConverted);
        return 1; // Return count (legacy doesn't track this)
    }
    return 0;
}

void NetworkPacketHandler::RegisterSearchID(uint32_t searchID, bool isKadSearch)
{
    wxMutexLocker lock(m_mutex);
    m_registeredSearchIDs[searchID] = isKadSearch;
    AddDebugLogLineC(logSearch, CFormat(wxT("NetworkPacketHandler: Registered search ID %u (Kad=%d)"))
        % searchID % isKadSearch);
}

void NetworkPacketHandler::UnregisterSearchID(uint32_t searchID)
{
    wxMutexLocker lock(m_mutex);
    m_registeredSearchIDs.erase(searchID);
    AddDebugLogLineC(logSearch, CFormat(wxT("NetworkPacketHandler: Unregistered search ID %u"))
        % searchID);
}

bool NetworkPacketHandler::IsSearchIDRegistered(uint32_t searchID) const
{
    wxMutexLocker lock(m_mutex);
    return m_registeredSearchIDs.find(searchID) != m_registeredSearchIDs.end();
}

CSearchFile* NetworkPacketHandler::CreateSearchFileFromED2KPacket(
    const uint8_t* packet,
    bool optUTF8,
    uint32_t searchID)
{
    // TODO: Implement packet parsing
    // For now, return nullptr (legacy handles this)
    return nullptr;
}

CSearchFile* NetworkPacketHandler::CreateSearchFileFromKadPacket(
    uint32_t searchID,
    const Kademlia::CUInt128* fileID,
    const wxString& name,
    uint64_t size,
    const wxString& type,
    uint32_t kadPublishInfo,
    const std::vector<Kademlia::Tag*>& tagList)
{
    // TODO: Implement packet creation
    // For now, return nullptr (legacy handles this)
    return nullptr;
}

} // namespace search

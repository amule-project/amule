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

#ifndef NETWORKPACKETHANDLER_H
#define NETWORKPACKETHANDLER_H

#include <vector>
#include <memory>
#include <wx/string.h>
#include <wx/thread.h>
#include <cstdint>
#include <map>
#include <list>

// Forward declarations
class CSearchFile;
class CMD4Hash;
class CTag;

namespace Kademlia {
    class CUInt128;
    class Tag;
}

typedef std::list<CTag*> TagPtrList;

namespace search {

/**
 * NetworkPacketHandler - Unified interface for handling network packets
 *
 * This class provides a centralized location for processing search results
 * from different networks (ED2K and Kad). It routes packets to the appropriate
 * controller and ensures consistent result handling.
 */
class NetworkPacketHandler {
public:
    /**
     * Get the singleton instance
     */
    static NetworkPacketHandler& Instance();

    /**
     * Process ED2K TCP search result packet
     *
     * @param packet Raw packet data
     * @param size Packet size
     * @param optUTF8 Whether server supports UTF8
     * @param serverIP Server IP address
     * @param serverPort Server port
     * @return Number of results processed
     */
    size_t ProcessED2KTCPSearchResult(
        const uint8_t* packet,
        uint32_t size,
        bool optUTF8,
        uint32_t serverIP,
        uint16_t serverPort);

    /**
     * Process ED2K UDP search result packet
     *
     * @param packet Raw packet data
     * @param optUTF8 Whether server supports UTF8
     * @param serverIP Server IP address
     * @param serverPort Server port
     * @return Number of results processed
     */
    size_t ProcessED2KUDPSearchResult(
        const uint8_t* packet,
        bool optUTF8,
        uint32_t serverIP,
        uint16_t serverPort);

    /**
     * Process Kad search result packet
     *
     * @param searchID Kad search ID
     * @param fileID File hash
     * @param name File name
     * @param size File size
     * @param type File type
     * @param kadPublishInfo Kad publish information
     * @param tagList List of tags
     * @return Number of results processed (0 or 1)
     */
    size_t ProcessKadSearchResult(
        uint32_t searchID,
        const Kademlia::CUInt128* fileID,
        const wxString& name,
        uint64_t size,
        const wxString& type,
        uint32_t kadPublishInfo,
        const std::vector<Kademlia::Tag*>& tagList);

    /**
     * Register a search ID for packet routing
     *
     * @param searchID The search ID to register
     * @param isKadSearch Whether this is a Kad search
     */
    void RegisterSearchID(uint32_t searchID, bool isKadSearch = false);

    /**
     * Unregister a search ID
     *
     * @param searchID The search ID to unregister
     */
    void UnregisterSearchID(uint32_t searchID);

    /**
     * Check if a search ID is registered
     *
     * @param searchID The search ID to check
     * @return true if registered
     */
    bool IsSearchIDRegistered(uint32_t searchID) const;

private:
    // Private constructor for singleton
    NetworkPacketHandler();

    // Delete copy constructor and copy assignment operator
    NetworkPacketHandler(const NetworkPacketHandler&) = delete;
    NetworkPacketHandler& operator=(const NetworkPacketHandler&) = delete;

    // Helper methods
    CSearchFile* CreateSearchFileFromED2KPacket(
        const uint8_t* packet,
        bool optUTF8,
        uint32_t searchID);

    CSearchFile* CreateSearchFileFromKadPacket(
        uint32_t searchID,
        const Kademlia::CUInt128* fileID,
        const wxString& name,
        uint64_t size,
        const wxString& type,
        uint32_t kadPublishInfo,
        const std::vector<Kademlia::Tag*>& tagList);

    // Map of registered search IDs
    typedef std::map<uint32_t, bool> SearchIDMap;
    SearchIDMap m_registeredSearchIDs;

    // Mutex for thread-safe access
    mutable wxMutex m_mutex;
};

} // namespace search

#endif // NETWORKPACKETHANDLER_H

//
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef MAGNETPROTOCOLDETECTOR_H
#define MAGNETPROTOCOLDETECTOR_H

#include "MD4Hash.h"
#include "MagnetURI.h"
#include <wx/string.h>
#include <vector>

/**
 * @enum MagnetProtocol
 * @brief Supported P2P protocols in magnet links
 */
enum MagnetProtocol {
    PROTOCOL_UNKNOWN = 0,
    PROTOCOL_ED2K,
    PROTOCOL_SHA1,
    PROTOCOL_MD5
};

/**
 * @enum ProtocolPreference
 * @brief User preference for protocol handling
 */
enum ProtocolPreference {
    PREFERENCE_ED2K_ONLY = 0,
    PREFERENCE_KADEMLIA_ONLY,
    PREFERENCE_HYBRID_AUTO,
    PREFERENCE_HYBRID_ED2K_FIRST,
    PREFERENCE_HYBRID_KAD_FIRST
};

/**
 * @struct MagnetProtocolInfo
 * @brief Information about detected protocol in magnet link
 */
struct MagnetProtocolInfo {
    MagnetProtocol protocol;
    wxString hash;
    wxString displayName;
    wxString trackers;
    uint64_t fileSize;

    MagnetProtocolInfo() : protocol(PROTOCOL_UNKNOWN), fileSize(0) {}
};

/**
 * @class CMagnetProtocolDetector
 * @brief Detects and handles multiple P2P protocols in magnet URIs
 */
class CMagnetProtocolDetector
{
public:
    CMagnetProtocolDetector(const wxString& magnetUri);

    /**
     * @brief Detect all supported protocols in the magnet URI
     */
    std::vector<MagnetProtocolInfo> DetectProtocols() const;

    /**
     * @brief Get the preferred protocol based on user preferences
     */
    MagnetProtocolInfo GetPreferredProtocol(ProtocolPreference preference) const;

    /**
     * @brief Check if magnet URI contains ED2K hash
     */
    bool HasED2K() const;

    /**
     * @brief Check if magnet URI contains Kademlia hash
     */
    bool HasKademlia() const;

    /**
     * @brief Get ED2K hash if available
     */
    CMD4Hash GetED2KHash() const;

    /**
     * @brief Get Kademlia info hash if available
     */
    wxString GetKadHash() const;

    /**
     * @brief Get file name from magnet URI
     */
    wxString GetFileName() const;

    /**
     * @brief Get file size from magnet URI
     */
    uint64_t GetFileSize() const;

    /**
     * @brief Get trackers from magnet URI
     */
    std::vector<wxString> GetTrackers() const;

    /**
     * @brief Convert to protocol-specific download format
     */
    wxString ConvertToPreferredFormat(ProtocolPreference preference) const;

private:
    wxString m_magnetUri;
    CMagnetURI m_parser;

    /**
     * @brief Extract hash from xt field
     */
    wxString ExtractHashFromXT(const wxString& xtValue) const;

    /**
     * @brief Detect protocol from xt field value
     */
    MagnetProtocol DetectProtocolFromXT(const wxString& xtValue) const;
};

#endif // MAGNETPROTOCOLDETECTOR_H

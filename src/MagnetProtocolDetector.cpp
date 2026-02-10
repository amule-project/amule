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

#include "MagnetProtocolDetector.h"
#include "Logger.h"
#include <wx/tokenzr.h>

CMagnetProtocolDetector::CMagnetProtocolDetector(const wxString& magnetUri)
    : m_magnetUri(magnetUri), m_parser(magnetUri)
{
}

std::vector<MagnetProtocolInfo> CMagnetProtocolDetector::DetectProtocols() const
{
    std::vector<MagnetProtocolInfo> protocols;

    // Get all xt (exact topic) fields
    auto xtFields = m_parser.GetField(wxT("xt"));
    for (const auto& xtValue : xtFields) {
	MagnetProtocolInfo info;
	info.protocol = DetectProtocolFromXT(xtValue);
	info.hash = ExtractHashFromXT(xtValue);

	if (info.protocol != PROTOCOL_UNKNOWN) {
	    // Get additional information
	    auto dnFields = m_parser.GetField(wxT("dn"));
	    if (!dnFields.empty()) {
		info.displayName = *dnFields.begin();
	    }

	    auto xlFields = m_parser.GetField(wxT("xl"));
	    if (!xlFields.empty()) {
		unsigned long long size = 0;
		(*xlFields.begin()).ToULongLong(&size);
		info.fileSize = size;
	    }

	    // Collect trackers
	    auto trFields = m_parser.GetField(wxT("tr"));
	    for (const auto& tracker : trFields) {
		if (!info.trackers.empty()) {
		    info.trackers += wxT(",");
		}
		info.trackers += tracker;
	    }

	    protocols.push_back(info);
	}
    }

    return protocols;
}

MagnetProtocolInfo CMagnetProtocolDetector::GetPreferredProtocol(ProtocolPreference preference) const
{
    auto protocols = DetectProtocols();
    if (protocols.empty()) {
	return MagnetProtocolInfo();
    }

    // Filter based on preference
    std::vector<MagnetProtocolInfo> filtered;

    for (const auto& protocol : protocols) {
	switch (preference) {
	    case PREFERENCE_ED2K_ONLY:
		if (protocol.protocol == PROTOCOL_ED2K) filtered.push_back(protocol);
		break;
	    case PREFERENCE_BITTORRENT_ONLY:
		if (protocol.protocol == PROTOCOL_BITTORRENT) filtered.push_back(protocol);
		break;
	    case PREFERENCE_HYBRID_AUTO:
	    case PREFERENCE_HYBRID_ED2K_FIRST:
	    case PREFERENCE_HYBRID_BT_FIRST:
		filtered.push_back(protocol);
		break;
	}
    }

    if (filtered.empty()) {
	return MagnetProtocolInfo();
    }

    // For hybrid preferences, sort by priority
    if (preference == PREFERENCE_HYBRID_ED2K_FIRST) {
	std::sort(filtered.begin(), filtered.end(),
	    [](const MagnetProtocolInfo& a, const MagnetProtocolInfo& b) {
		return a.protocol == PROTOCOL_ED2K && b.protocol != PROTOCOL_ED2K;
	    });
    } else if (preference == PREFERENCE_HYBRID_BT_FIRST) {
	std::sort(filtered.begin(), filtered.end(),
	    [](const MagnetProtocolInfo& a, const MagnetProtocolInfo& b) {
		return a.protocol == PROTOCOL_BITTORRENT && b.protocol != PROTOCOL_BITTORRENT;
	    });
    }

    return filtered.front();
}

bool CMagnetProtocolDetector::HasED2K() const
{
    auto protocols = DetectProtocols();
    for (const auto& protocol : protocols) {
	if (protocol.protocol == PROTOCOL_ED2K) {
	    return true;
	}
    }
    return false;
}

bool CMagnetProtocolDetector::HasBitTorrent() const
{
    auto protocols = DetectProtocols();
    for (const auto& protocol : protocols) {
	if (protocol.protocol == PROTOCOL_BITTORRENT) {
	    return true;
	}
    }
    return false;
}

CMD4Hash CMagnetProtocolDetector::GetED2KHash() const
{
    auto protocols = DetectProtocols();
    for (const auto& protocol : protocols) {
	if (protocol.protocol == PROTOCOL_ED2K) {
	    CMD4Hash hash;
	    if (hash.Decode(protocol.hash)) {
		return hash;
	    }
	}
    }
    return CMD4Hash();
}

wxString CMagnetProtocolDetector::GetBTHash() const
{
    auto protocols = DetectProtocols();
    for (const auto& protocol : protocols) {
	if (protocol.protocol == PROTOCOL_BITTORRENT) {
	    return protocol.hash;
	}
    }
    return wxEmptyString;
}

wxString CMagnetProtocolDetector::GetFileName() const
{
    auto dnFields = m_parser.GetField(wxT("dn"));
    if (!dnFields.empty()) {
	return *dnFields.begin();
    }
    return wxEmptyString;
}

uint64_t CMagnetProtocolDetector::GetFileSize() const
{
    auto xlFields = m_parser.GetField(wxT("xl"));
    if (!xlFields.empty()) {
	unsigned long long size = 0;
	(*xlFields.begin()).ToULong(&size);
	return size;
    }
    return 0;
}

std::vector<wxString> CMagnetProtocolDetector::GetTrackers() const
{
    std::vector<wxString> trackers;
    auto trFields = m_parser.GetField(wxT("tr"));
    for (const auto& tracker : trFields) {
	trackers.push_back(tracker);
    }
    return trackers;
}

wxString CMagnetProtocolDetector::ConvertToPreferredFormat(ProtocolPreference preference) const
{
    auto preferred = GetPreferredProtocol(preference);
    if (preferred.protocol == PROTOCOL_UNKNOWN) {
	return wxEmptyString;
    }

    switch (preferred.protocol) {
	case PROTOCOL_ED2K:
	    return wxString::Format(wxT("ed2k://|file|%s|%llu|%s|/"),
		GetFileName(), GetFileSize(), preferred.hash);
	case PROTOCOL_BITTORRENT:
	    return m_magnetUri; // Return original magnet for BT
	default:
	    return wxEmptyString;
    }
}

wxString CMagnetProtocolDetector::ExtractHashFromXT(const wxString& xtValue) const
{
    // xt format: urn:protocol:hash
    wxString prefix = wxT("urn:");
    size_t pos = xtValue.Find(prefix);
    if (pos != wxNOT_FOUND) {
	size_t colonPos = xtValue.find(:, pos + prefix.length());
	if (colonPos != wxString::npos) {
	    return xtValue.substr(colonPos + 1);
	}
    }
    return xtValue; // Return whole value if format not recognized
}

MagnetProtocol CMagnetProtocolDetector::DetectProtocolFromXT(const wxString& xtValue) const
{
    if (xtValue.StartsWith(wxT("urn:ed2k:"))) {
	return PROTOCOL_ED2K;
    } else if (xtValue.StartsWith(wxT("urn:btih:"))) {
	return PROTOCOL_BITTORRENT;
    } else if (xtValue.StartsWith(wxT("urn:sha1:"))) {
	return PROTOCOL_SHA1;
    } else if (xtValue.StartsWith(wxT("urn:md5:"))) {
	return PROTOCOL_MD5;
    }

    return PROTOCOL_UNKNOWN;
}

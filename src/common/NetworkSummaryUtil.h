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

#ifndef NETWORK_SUMMARY_UTIL_H
#define NETWORK_SUMMARY_UTIL_H

#include <atomic>
#include <cstdint>
#include <string>
#include <wx/string.h>

class CNetworkSummaryUtil
{
public:
    CNetworkSummaryUtil();
    
    // Record network activity
    void record_tcp_activity(uint64_t bytes_received, uint64_t bytes_sent);
    void record_udp_activity(uint64_t bytes_received, uint64_t bytes_sent);
    
    // Get summary statistics
    uint64_t get_total_received() const;
    uint64_t get_total_sent() const;
    uint64_t get_tcp_received() const;
    uint64_t get_tcp_sent() const;
    uint64_t get_udp_received() const;
    uint64_t get_udp_sent() const;
    
    // Reset counters
    void reset_counters();
    
	// Generate human-readable summary
	wxString get_summary() const;
    
private:
    std::atomic<uint64_t> m_tcp_received;
    std::atomic<uint64_t> m_tcp_sent;
    std::atomic<uint64_t> m_udp_received;
    std::atomic<uint64_t> m_udp_sent;
};

#endif // NETWORK_SUMMARY_UTIL_H
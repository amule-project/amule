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
// You should be received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "NetworkSummaryUtil.h"
#include <common/Format.h>
#include <string>

CNetworkSummaryUtil::CNetworkSummaryUtil()
    : m_tcp_received(0)
    , m_tcp_sent(0)
    , m_udp_received(0)
    , m_udp_sent(0)
{
}

void CNetworkSummaryUtil::record_tcp_activity(uint64_t bytes_received, uint64_t bytes_sent)
{
    m_tcp_received += bytes_received;
    m_tcp_sent += bytes_sent;
}

void CNetworkSummaryUtil::record_udp_activity(uint64_t bytes_received, uint64_t bytes_sent)
{
    m_udp_received += bytes_received;
    m_udp_sent += bytes_sent;
}

uint64_t CNetworkSummaryUtil::get_total_received() const
{
    return m_tcp_received + m_udp_received;
}

uint64_t CNetworkSummaryUtil::get_total_sent() const
{
    return m_tcp_sent + m_udp_sent;
}

uint64_t CNetworkSummaryUtil::get_tcp_received() const
{
    return m_tcp_received;
}

uint64_t CNetworkSummaryUtil::get_tcp_sent() const
{
    return m_tcp_sent;
}

uint64_t CNetworkSummaryUtil::get_udp_received() const
{
    return m_udp_received;
}

uint64_t CNetworkSummaryUtil::get_udp_sent() const
{
    return m_udp_sent;
}

void CNetworkSummaryUtil::reset_counters()
{
    m_tcp_received = 0;
    m_tcp_sent = 0;
    m_udp_received = 0;
    m_udp_sent = 0;
}

wxString CNetworkSummaryUtil::get_summary() const
{
    uint64_t total_received = get_total_received();
    uint64_t total_sent = get_total_sent();
    
    auto format_bytes = [](uint64_t bytes) -> wxString {
        if (bytes < 1024) return wxString::Format("%llu B", bytes);
        if (bytes < 1024*1024) return wxString::Format("%llu KB", bytes/1024);
        if (bytes < 1024*1024*1024) return wxString::Format("%llu MB", bytes/(1024*1024));
        return wxString::Format("%.2f GB", (double)bytes/(1024*1024*1024));
    };
    
    return CFormat("Network Summary - Total: %s received, %s sent | TCP: %s received, %s sent | UDP: %s received, %s sent")
        % format_bytes(total_received)
        % format_bytes(total_sent)
        % format_bytes(m_tcp_received)
        % format_bytes(m_tcp_sent)
        % format_bytes(m_udp_received)
        % format_bytes(m_udp_sent);
}
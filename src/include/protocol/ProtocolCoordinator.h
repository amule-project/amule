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
#pragma once

#include "protocol/Protocols.h"
#include "protocol/ed2k/Constants.h"
#include "protocol/kad/Constants.h"
#include "../../MD4Hash.h"
#include "../../common/NetworkPerformanceMonitor.h"
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class CPartFile;
class CKnownFile;
class CUpDownClient;
class CServer;

namespace ProtocolIntegration {

// Network condition metrics for protocol selection
struct NetworkConditions {
    double bandwidth_kbps;          // Available bandwidth in kbps
    uint32_t latency_ms;            // Network latency in milliseconds
    double packet_loss_rate;        // Packet loss rate (0.0 to 1.0)
    uint32_t connection_stability;  // Connection stability score (0-100)
    bool supports_nat_traversal;    // Supports NAT traversal techniques
    bool high_bandwidth_mode;       // High bandwidth mode available
};

enum class ProtocolType {
    ED2K,
    KADEMLIA,
    HYBRID_AUTO
};

struct SourceEndpoint {
    ProtocolType protocol;
    std::string address;
    uint16_t port;
    double reliability_score;
    double bandwidth_estimate;
    uint32_t latency_ms;

    // Cross-protocol metadata
    CMD4Hash ed2k_hash;         // For ED2K
    bool supports_hybrid;       // Supports cross-protocol transfers

    bool operator==(const SourceEndpoint& other) const;
    bool is_duplicate(const SourceEndpoint& other) const;
};

class ProtocolCoordinator {
public:
    static ProtocolCoordinator& instance();

    // Source discovery and management
    std::vector<SourceEndpoint> discover_sources(
	const CPartFile* file,
	ProtocolType preferred = ProtocolType::HYBRID_AUTO,
	uint32_t max_sources = 50);


    bool add_source(const SourceEndpoint& source, CPartFile* file);
    bool remove_duplicate_sources(CPartFile* file);

    // Protocol selection and optimization
    ProtocolType select_optimal_protocol(
	const CPartFile* file,
	const NetworkConditions& conditions) const;

    bool should_switch_protocol(
	const CPartFile* file,
	ProtocolType new_protocol,
	const NetworkConditions& conditions) const;

    // Bandwidth management

    // Statistics and monitoring
    struct CoordinationStats {
	uint32_t total_sources_discovered;
	uint32_t cross_protocol_sources;
	uint32_t protocol_switches;
	uint32_t duplicate_sources_removed;
	double avg_discovery_time_ms;
	double cross_protocol_success_rate;
    };

    CoordinationStats get_stats() const;

    // Configuration
    void enable_hybrid_mode(bool enable);
    bool is_hybrid_mode_enabled() const;

    void set_max_cross_protocol_sources(uint32_t max);
    uint32_t get_max_cross_protocol_sources() const;

private:
    ProtocolCoordinator();
    ~ProtocolCoordinator();

    class Impl;
    std::unique_ptr<Impl> pimpl_;

    // Disable copying
    ProtocolCoordinator(const ProtocolCoordinator&) = delete;
    ProtocolCoordinator& operator=(const ProtocolCoordinator&) = delete;
};

// Helper functions
double calculate_client_reliability(const CUpDownClient* client);
bool add_ed2k_source(const SourceEndpoint& source, CPartFile* file);
bool add_kad_source(const SourceEndpoint& source, CPartFile* file);

} // namespace ProtocolIntegration

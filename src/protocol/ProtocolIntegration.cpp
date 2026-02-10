#include "protocol/ProtocolCoordinator.h"
#include "ClientList.h"
#include "ServerList.h"
#include "DownloadQueue.h"
#include "SharedFiles.h"
#include "common/NetworkPerformanceMonitor.h"

using namespace ProtocolIntegration;

ProtocolCoordinator& ProtocolCoordinator::instance() {
    static ProtocolCoordinator instance;
    return instance;
}

std::vector<SourceEndpoint> ProtocolCoordinator::discover_sources(
    const CPartFile* file,
    ProtocolType preferred,
    uint32_t max_sources) {

    std::vector<SourceEndpoint> sources;

    // Get ED2K sources from existing infrastructure
    if (preferred == ProtocolType::ED2K || preferred == ProtocolType::HYBRID_AUTO) {
	auto ed2k_sources = theApp->downloadqueue->GetSourcesForFile(file);
	for (const auto& client : ed2k_sources) {
	    SourceEndpoint endpoint;
	    endpoint.protocol = ProtocolType::ED2K;
	    endpoint.address = Uint32toStringIP(client->GetIP());
	    endpoint.port = client->GetUserPort();
	    endpoint.ed2k_hash = file->GetFileHash();
	    endpoint.reliability_score = calculate_client_reliability(client);
	    sources.push_back(endpoint);
	}
    }

    // Note: Hybrid mode is disabled - only ED2K and Kademlia are supported

    // Remove duplicates and limit results
    remove_duplicate_sources(sources);
    if (sources.size() > max_sources) {
	sources.resize(max_sources);
    }

    return sources;
}

bool ProtocolCoordinator::add_source(const SourceEndpoint& source, CPartFile* file) {
    switch (source.protocol) {
	case ProtocolType::ED2K:
	    return add_ed2k_source(source, file);
	case ProtocolType::KADEMLIA:
	    return add_kad_source(source, file);
	default:
	    return false;
    }
}

bool ProtocolCoordinator::add_ed2k_source(const SourceEndpoint& source, CPartFile* file) {
    try {
	// Create ED2K client from endpoint
	auto client = std::make_shared<CUpDownClient>(
	    nullptr, // socket will be created later
	    source.address,
	    source.port,
	    source.ed2k_hash
	);

	// Add to download queue
	return theApp->downloadqueue->AddSource(client.get(), file);
    } catch (...) {
	return false;
    }
}

// BitTorrent support removed - no longer needed

ProtocolType ProtocolCoordinator::select_optimal_protocol(
    const CPartFile* file,
    const NetworkConditions& conditions) const {

    // Only ED2K and Kademlia are supported
    // Simple heuristic based on file properties and network conditions
    double ed2k_score = calculate_ed2k_score(file, conditions);

    if (ed2k_score > 50.0) {
	return ProtocolType::ED2K;
    } else {
	return ProtocolType::KADEMLIA;
    }
}

double ProtocolCoordinator::calculate_ed2k_score(
    const CPartFile* file,
    const NetworkConditions& conditions) const {

    // ED2K works better for:
    // - Older files with many sources
    // - Stable, low-latency connections
    // - Files with good ED2K availability

    double score = 0.0;

    // Factor in file age and source count
    if (file->GetSourceCount() > 10) {
	score += 30.0;
    }

    // Network conditions
    if (conditions.latency_ms < 100) {
	score += 20.0;
    }

    return score;
}

// BitTorrent score calculation removed - no longer needed

// BitTorrent metadata conversion removed - no longer needed

// Network performance monitoring integration
void monitor_cross_protocol_traffic(const SourceEndpoint& source, uint64_t bytes) {
    auto& monitor = network_perf::g_network_perf_monitor;

    switch (source.protocol) {
	case ProtocolType::ED2K:
	    monitor.record_tcp_received(bytes);
	    break;
	case ProtocolType::KADEMLIA:
	    monitor.record_udp_received(bytes);
	    break;
	default:
	    monitor.record_received(bytes);
    }
}

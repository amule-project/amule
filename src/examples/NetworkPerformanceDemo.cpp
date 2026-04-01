#include "common/NetworkPerformanceMonitor.h"
#include "common/ModernLogging.h"
#include <thread>
#include <chrono>

using namespace network_perf;

void simulate_network_activity() {
    // Simulate network traffic
    for (int i = 0; i < 100; ++i) {
        // Simulate sending data
        size_t bytes_sent = 1024 + (i * 50);
        g_network_perf_monitor.record_sent(bytes_sent);
        
        // Simulate receiving data
        size_t bytes_received = 512 + (i * 25);
        g_network_perf_monitor.record_received(bytes_received);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void demonstrate_performance_monitoring() {
    modern_log::Log("Starting network performance demo...");
    
    // Start performance monitoring
    simulate_network_activity();
    
    // Generate and display performance report
    auto report = g_network_perf_monitor.generate_report();
    modern_log::Log(report.str());
    
    // Get detailed statistics
    auto stats = g_network_perf_monitor.get_statistics();
    
    modern_utils::StringBuffer detail_report(256);
    detail_report.append("Detailed Statistics:\n")
                .append("  Total Data: ").append(stats.bytes_sent + stats.bytes_received).append(" bytes\n")
                .append("  Average Throughput: ").append(stats.bytes_per_second / 1024).append(" KB/s\n")
                .append("  Packet Rate: ").append(stats.packets_per_second).append(" packets/s");
    
    modern_log::Log(detail_report.str());
}

int main() {
    demonstrate_performance_monitoring();
    return 0;
}

#include "common/PerformanceUtils.h"
#include "common/ModernLogging.h"
#include <iostream>

// Example: Optimized logging with StringBuffer
void optimized_logging_demo() {
    using namespace modern_utils;
    
    // Before: Multiple wxString::Format calls
    // modern_log::Log(wxString::Format("Processing file %s (size: %d bytes)", name.c_str(), size));
    
    // After: Single StringBuffer allocation
    StringBuffer buf(256);
    buf.append("Processing file ")
       .append("data.bin")
       .append(" (size: ")
       .append(1024)
       .append(" bytes)");
    
    modern_log::Log(std::string_view(buf.str()));
}

// Example: Compile-time hash for fast string comparisons
constexpr const char* MSG_NETWORK = "network_event";
constexpr const char* MSG_FILE = "file_event";

void fast_message_routing(const char* message) {
    switch (ct_hash(message)) {
        case ct_hash(MSG_NETWORK):
            // Handle network event
            break;
        case ct_hash(MSG_FILE):
            // Handle file event
            break;
        default:
            // Unknown message
            break;
    }
}

// Performance measurement utility
class PerformanceTimer {
    std::chrono::high_resolution_clock::time_point start;
    const char* name;
    
public:
    PerformanceTimer(const char* name_) : name(name_) {
        start = std::chrono::high_resolution_clock::now();
    }
    
    ~PerformanceTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        modern_log::Log(std::string_view(
            std::string(name) + ": " + std::to_string(duration.count()) + "μs"
        ));
    }
};

void example_usage() {
    // Measure performance of critical operations
    PerformanceTimer timer("Critical operation");
    
    // ... perform critical operation ...
}

// Compile-time optimizations for frequently-called functions
[[nodiscard]] inline bool is_valid_ip(const char* ip) {
    // Fast validation without regex
    int dots = 0;
    int nums = 0;
    for (const char* p = ip; *p; ++p) {
        if (*p == '.') {
            ++dots;
            nums = 0;
        } else if (*p >= '0' && *p <= '9') {
            ++nums;
            if (nums > 3) return false;
        } else {
            return false;
        }
    }
    return dots == 3;
}

int main() {
    // Demonstrate optimizations
    optimized_logging_demo();
    fast_message_routing("network_event");
    
    return 0;
}
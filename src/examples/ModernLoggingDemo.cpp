//
// Modern Logging Demo - Example usage of aMule's modern C++20 logging
//

#include "common/ModernLogging.h"
#include <iostream>
#include <vector>

void DemoBasicUsage() {
    std::cout << "=== Basic Modern Logging Demo ===" << std::endl;
    
    // Basic logging
    modern_log::Log("This is a normal log message");
    modern_log::Log("This is a critical error", true);
    
    #ifdef USE_CPP20
    std::cout << "\n=== C++20 Features Demo ===" << std::endl;
    
    // C++20 string_view usage
    std::string_view sv_message = "String view message from C++20";
    modern_log::Log(sv_message);
    
    // Using source_location for better debugging
    modern_log::Log("Message with automatic source location", false);
    
    // Example with lambda and modern C++
    auto log_from_lambda = []() {
        modern_log::Log("Logging from lambda function");
    };
    log_from_lambda();
    #endif
}

void DemoPerformanceSensitive() {
    std::cout << "\n=== Performance Sensitive Demo ===" << std::endl;
    
    // For performance-critical code, prefer string_view
    #ifdef USE_CPP20
    constexpr std::string_view perf_message = "High-performance logging";
    for (int i = 0; i < 5; ++i) {
        modern_log::Log(perf_message);
    }
    #else
    // Traditional wxString for compatibility
    for (int i = 0; i < 5; ++i) {
        modern_log::Log(wxString::Format("Compatibility message %d", i));
    }
    #endif
}

void DemoRealWorldScenario() {
    std::cout << "\n=== Real-world Network Scenario ===" << std::endl;
    
    // Simulate network event handling
    modern_log::Log("Processing incoming network packet");
    
    // Error scenario
    modern_log::Log("Socket connection timeout", true);
    
    // Success scenario  
    modern_log::Log("File download completed successfully");
}

int main() {
    std::cout << "aMule Modern Logging Demonstration" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        DemoBasicUsage();
        DemoPerformanceSensitive();
        DemoRealWorldScenario();
        
        std::cout << "\n=== Demo Completed ===" << std::endl;
        modern_log::Log("All demos executed successfully");
        
    } catch (const std::exception& e) {
        modern_log::Log(wxString("Exception caught: ") + e.what(), true);
        return 1;
    }
    
    return 0;
}
#include <iostream>
#include <cstdint>

int main() {
    uint32_t ip = 2248339345;  // This is the IP from the crash
    uint16_t port = 4671;      // This is the port from the crash
    
    std::cout << "IP: " << ip << " (0x" << std::hex << ip << std::dec << ")\n";
    std::cout << "IP bytes: " 
              << (uint8_t)ip << "." 
              << (uint8_t)(ip>>8) << "." 
              << (uint8_t)(ip>>16) << "." 
              << (uint8_t)(ip>>24) << "\n";
    
    // The issue: %u expects unsigned int, but we're passing uint8_t (unsigned char)
    // In C/C++, when you pass a char to a variadic function, it gets promoted to int
    // But wxWidgets' type-checking might be stricter
    
    std::cout << "\nSize of types:\n";
    std::cout << "sizeof(uint8_t): " << sizeof(uint8_t) << "\n";
    std::cout << "sizeof(unsigned char): " << sizeof(unsigned char) << "\n";
    std::cout << "sizeof(unsigned int): " << sizeof(unsigned int) << "\n";
    std::cout << "sizeof(uint32_t): " << sizeof(uint32_t) << "\n";
    
    return 0;
}

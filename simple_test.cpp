#include <iostream>
#include <cstdint>

// Simulate the issue
void test_format_issue() {
    uint32_t ip = 2248339345;  // 0x8602ef91
    
    // This is what was causing the crash:
    // CFormat("%u") % (uint8)ip
    // %u expects unsigned int (4 bytes), but uint8 is 1 byte
    
    std::cout << "Testing type sizes:\n";
    std::cout << "sizeof(uint8_t): " << sizeof(uint8_t) << "\n";
    std::cout << "sizeof(unsigned char): " << sizeof(unsigned char) << "\n";
    std::cout << "sizeof(unsigned int): " << sizeof(unsigned int) << "\n";
    
    std::cout << "\nIP: " << ip << " (0x" << std::hex << ip << std::dec << ")\n";
    std::cout << "IP bytes as uint8: " 
              << (unsigned int)(uint8_t)ip << "." 
              << (unsigned int)(uint8_t)(ip>>8) << "." 
              << (unsigned int)(uint8_t)(ip>>16) << "." 
              << (unsigned int)(uint8_t)(ip>>24) << "\n";
    
    // The fix is to cast to unsigned int
    std::cout << "\nWith fix (cast to unsigned int):\n";
    std::cout << "Values: " 
              << (unsigned int)(uint8_t)ip << ", "
              << (unsigned int)(uint8_t)(ip>>8) << ", "
              << (unsigned int)(uint8_t)(ip>>16) << ", "
              << (unsigned int)(uint8_t)(ip>>24) << "\n";
}

int main() {
    test_format_issue();
    return 0;
}

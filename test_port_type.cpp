#include <iostream>
#include <cstdint>

int main() {
    std::cout << "Type sizes:\n";
    std::cout << "sizeof(uint16_t): " << sizeof(uint16_t) << "\n";
    std::cout << "sizeof(unsigned short): " << sizeof(unsigned short) << "\n";
    std::cout << "sizeof(unsigned int): " << sizeof(unsigned int) << "\n";
    
    uint16_t port = 4671;
    std::cout << "\nPort: " << port << "\n";
    std::cout << "Port as unsigned int: " << (unsigned int)port << "\n";
    
    return 0;
}

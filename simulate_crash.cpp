#include <iostream>
#include <cstdint>
#include <cstdio>

// Simulate what wxWidgets CFormat might be doing with type checking
template<typename T>
struct wxFormatStringSpecifier {
    static const int value = 0;
};

// Specializations for different types
template<>
struct wxFormatStringSpecifier<unsigned int> {
    static const int value = 1;
};

template<>
struct wxFormatStringSpecifier<unsigned char> {
    static const int value = 2;
};

// Simulated wxArgNormalizer that does type checking
template<typename T>
class wxArgNormalizer {
public:
    wxArgNormalizer(T value, const char* fmt, int index) {
        // This is the assert that's failing
        int argtype = (std::is_same<T, unsigned int>::value) ? 1 : 
                     (std::is_same<T, unsigned char>::value) ? 2 : 0;
        
        // In the real code: assert("(argtype & (wxFormatStringSpecifier<T>::value)) == argtype")
        // For %u, wxFormatStringSpecifier<unsigned int>::value should match
        // But we're passing uint8 (unsigned char)
        
        std::cout << "Argument " << index << ": type=" << argtype 
                  << ", expected for %u=" << wxFormatStringSpecifier<unsigned int>::value
                  << "\n";
        
        if (argtype != wxFormatStringSpecifier<unsigned int>::value) {
            std::cout << "ERROR: Type mismatch! %u expects unsigned int, got ";
            if (std::is_same<T, unsigned char>::value) {
                std::cout << "unsigned char\n";
            } else {
                std::cout << "other type\n";
            }
        }
    }
};

// Test the problematic code
void test_problem() {
    uint32_t ip = 2248339345;
    uint16_t port = 4671;
    
    std::cout << "Testing original (buggy) code:\n";
    std::cout << "CFormat(\"%u.%u.%u.%u:%u\") % (uint8)ip % (uint8)(ip>>8) % (uint8)(ip>>16) % (uint8)(ip>>24) % port\n";
    
    // This would trigger the assert
    // wxArgNormalizer<uint8> a1((uint8)ip, "%u", 1);
    // wxArgNormalizer<uint8> a2((uint8)(ip>>8), "%u", 2);
    // wxArgNormalizer<uint8> a3((uint8)(ip>>16), "%u", 3);
    // wxArgNormalizer<uint8> a4((uint8)(ip>>24), "%u", 4);
    // wxArgNormalizer<uint16> a5(port, "%u", 5);
    
    std::cout << "\nTesting fixed code:\n";
    std::cout << "CFormat(\"%u.%u.%u.%u:%u\") % (unsigned int)(uint8)ip % (unsigned int)(uint8)(ip>>8) % (unsigned int)(uint8)(ip>>16) % (unsigned int)(uint8)(ip>>24) % port\n";
    
    // This should work
    // wxArgNormalizer<unsigned int> a1((unsigned int)(uint8)ip, "%u", 1);
    // wxArgNormalizer<unsigned int> a2((unsigned int)(uint8)(ip>>8), "%u", 2);
    // wxArgNormalizer<unsigned int> a3((unsigned int)(uint8)(ip>>16), "%u", 3);
    // wxArgNormalizer<unsigned int> a4((unsigned int)(uint8)(ip>>24), "%u", 4);
    // wxArgNormalizer<uint16> a5(port, "%u", 5); // Still wrong! port is uint16, not unsigned int
    
    std::cout << "\nNote: port is uint16, which also needs to be cast to unsigned int!\n";
}

int main() {
    test_problem();
    return 0;
}

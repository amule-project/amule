#include <iostream>
#include <string>
#include <vector>

// Simulate the corrupted data
std::string corrupted_data = "\xEF\xBF\xBD\xE5\x95\x90\xEF\xBF\xBD\xEF\xBF\xBD" 
                             "a H. Ultimate ChatGPT Handbook for Enterprises...with Python and Java 2023.pdf";

// Helper function to detect and fix double-encoded UTF-8
bool IsDoubleEncodedUTF8(const char* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = static_cast<unsigned char>(data[i]);
        
        // Check for UTF-8 continuation bytes (0x80-0xBF) that shouldn't appear
        // at the start of a latin-1 string
        if ((c >= 0x80 && c <= 0xBF) && (i == 0 || (data[i-1] < 0xC2))) {
            return true;
        }
        
        // Check for the "ï¿½" pattern (0xEF 0xBF 0xBD)
        if (i + 2 < len && 
            static_cast<unsigned char>(data[i]) == 0xEF &&
            static_cast<unsigned char>(data[i+1]) == 0xBF &&
            static_cast<unsigned char>(data[i+2]) == 0xBD) {
            return true;
        }
    }
    return false;
}

int main() {
    std::cout << "Original corrupted data (hex): ";
    for (size_t i = 0; i < 12 && i < corrupted_data.size(); ++i) {
        printf("%02X ", (unsigned char)corrupted_data[i]);
    }
    std::cout << "...\n";
    
    std::cout << "As string: " << corrupted_data << "\n";
    
    if (IsDoubleEncodedUTF8(corrupted_data.c_str(), corrupted_data.size())) {
        std::cout << "Detected as double-encoded UTF-8!\n";
        
        // Show what the latin-1 interpretation would be
        std::cout << "Latin-1 interpretation: ";
        for (size_t i = 0; i < 12 && i < corrupted_data.size(); ++i) {
            unsigned char c = corrupted_data[i];
            if (c >= 32 && c <= 126) {
                std::cout << c;
            } else {
                printf("\\x%02X", c);
            }
        }
        std::cout << "\n";
        
        // The fix would be to decode as latin-1, then as UTF-8
        // But since we can't easily do that in C++ without wxWidgets,
        // let's just show what should happen
        
        std::cout << "After fixing, the first 12 bytes should be removed.\n";
        std::cout << "Expected result: " << corrupted_data.substr(12) << "\n";
    } else {
        std::cout << "Not detected as double-encoded UTF-8.\n";
    }
    
    return 0;
}

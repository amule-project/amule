#include <iostream>
#include <cstdint>
#include <wx/string.h>
#include <common/Format.h>

// Simulate our fixed function
inline wxString Uint32_16toStringIP_Port(uint32 ip, uint16 port)
{
    return CFormat(wxT("%u.%u.%u.%u:%u")) % (unsigned int)(uint8)ip % (unsigned int)(uint8)(ip>>8) % (unsigned int)(uint8)(ip>>16) % (unsigned int)(uint8)(ip>>24) % port;
}

int main() {
    // Test with the IP from the crash
    uint32 ip = 2248339345;  // 0x8602ef91
    uint16 port = 4671;
    
    wxString result = Uint32_16toStringIP_Port(ip, port);
    std::cout << "IP: " << ip << " (0x" << std::hex << ip << std::dec << ")\n";
    std::cout << "Result: " << result.ToUTF8().data() << "\n";
    
    // Verify the bytes
    std::cout << "IP bytes: " 
              << (unsigned int)(uint8)ip << "." 
              << (unsigned int)(uint8)(ip>>8) << "." 
              << (unsigned int)(uint8)(ip>>16) << "." 
              << (unsigned int)(uint8)(ip>>24) << ":" 
              << port << "\n";
    
    return 0;
}

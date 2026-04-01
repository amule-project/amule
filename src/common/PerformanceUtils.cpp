#include "PerformanceUtils.h"
#include <chrono>

namespace modern_utils {

// StringBuffer implementation
StringBuffer::StringBuffer(size_t capacity) {
    buffer.reserve(capacity);
}

StringBuffer& StringBuffer::append(const char* str) {
    buffer.append(str);
    return *this;
}

StringBuffer& StringBuffer::append(const wxString& str) {
    buffer.append(str.ToUTF8());
    return *this;
}

StringBuffer& StringBuffer::append(std::string_view sv) {
    buffer.append(sv);
    return *this;
}

StringBuffer& StringBuffer::append(char c) {
    buffer.push_back(c);
    return *this;
}

StringBuffer& StringBuffer::append(int value) {
    buffer += std::to_string(value);
    return *this;
}

// PerformanceTimer implementation
PerformanceTimer::PerformanceTimer(const char* name_) : name(name_) {
    start = std::chrono::high_resolution_clock::now();
}

PerformanceTimer::~PerformanceTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // For demonstration - in production, this would use the logging system
    #ifdef __DEBUG__
    printf("[Performance] %s: %ld μs\n", name, duration.count());
    #endif
}

// Fast IP validation implementation
bool is_valid_ip(const char* ip) {
    int dots = 0;
    int nums = 0;
    int current_num = 0;
    
    for (const char* p = ip; *p; ++p) {
        if (*p == '.') {
            if (nums == 0 || current_num > 255) return false;
            ++dots;
            nums = 0;
            current_num = 0;
        } else if (*p >= '0' && *p <= '9') {
            current_num = current_num * 10 + (*p - '0');
            ++nums;
            if (nums > 3 || current_num > 255) return false;
        } else {
            return false;
        }
    }
    return dots == 3 && nums > 0 && current_num <= 255;
}

} // namespace modern_utils
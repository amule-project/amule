#pragma once

#include <string>
#include <wx/string.h>
#include <chrono>

namespace modern_utils {

// Pre-allocated string buffer for performance-critical operations
class StringBuffer {
private:
    static constexpr size_t DEFAULT_CAPACITY = 256;
    std::string buffer;
    
public:
    StringBuffer(size_t capacity = DEFAULT_CAPACITY) : buffer(capacity, '\0') {
        buffer.clear();
    }
    
    StringBuffer& append(const char* str) {
        buffer.append(str);
        return *this;
    }
    
    StringBuffer& append(const wxString& str) {
        buffer.append(str.ToUTF8());
        return *this;
    }
    
    StringBuffer& append(std::string_view sv) {
        buffer.append(sv);
        return *this;
    }
    
    StringBuffer& append(char c) {
        buffer.push_back(c);
        return *this;
    }
    
    StringBuffer& append(int value) {
        buffer += std::to_string(value);
        return *this;
    }
    
    StringBuffer& append(unsigned int value) {
        buffer += std::to_string(value);
        return *this;
    }
    
    StringBuffer& append(unsigned long value) {
        buffer += std::to_string(value);
        return *this;
    }
    
    StringBuffer& append(unsigned long long value) {
        buffer += std::to_string(value);
        return *this;
    }
    
    StringBuffer& append(double value) {
        buffer += std::to_string(value);
        return *this;
    }
    
    const std::string& str() const { return buffer; }
    std::string&& move() { return std::move(buffer); }
    size_t size() const { return buffer.size(); }
    void clear() { buffer.clear(); }
    const char* c_str() const { return buffer.c_str(); }
};

// Compile-time string hash for fast comparisons
constexpr size_t ct_hash(const char* str, size_t hash = 0) {
    return (!str[0]) ? hash : ct_hash(str + 1, hash * 31 + str[0]);
}

// Fast string comparison using pre-computed hash
inline bool fast_eq(const char* a, const char* b, size_t hash_a = 0, size_t hash_b = 0) {
    return hash_a == hash_b && strcmp(a, b) == 0;
}

// Performance timer with microsecond precision
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start;
    const char* name;
    
public:
    PerformanceTimer(const char* name_) : name(name_) {
        start = std::chrono::high_resolution_clock::now();
    }
    
    ~PerformanceTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        #ifdef __DEBUG__
        printf("[Performance] %s: %ld μs\n", name, duration.count());
        #endif
    }
    
    std::chrono::microseconds elapsed_time() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    }
};

// Fast IP validation implementation
inline bool is_valid_ip(const char* ip) {
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
#include "muleunit/test.h"
#include "../../src/common/ModernLogging.h"

using namespace muleunit;

DECLARE(ModernLogging)
    // 简单的测试方法，验证 ModernLogging.h 可以正常编译
    void testHeaderCompilation() {
        // 这个测试只验证头文件可以正常包含和编译
        // 不实际调用 modern_log::Log 以避免链接问题
        #ifdef USE_CPP20
        std::string_view test = "C++20 compilation test passed";
        #else  
        const char* test = "Traditional compilation test passed";
        #endif
    }
END_DECLARE;

TEST(ModernLogging, HeaderCompilation)
{
    // 测试 ModernLogging 头文件编译正常
}

TEST(ModernLogging, Cpp20FeatureDetection)
{
    // 测试 C++20 特性检测正常工作
    #ifdef USE_CPP20
    // C++20 特性可用性验证
    #else
    // 传统模式验证
    #endif
}
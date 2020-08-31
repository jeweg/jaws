#include "pch.hpp"
#include "jaws/util/timer.hpp"
#ifdef _WIN32
#    include "jaws/windows.hpp"
#else
#    include <chrono>
#endif

namespace jaws::util {

uint64_t get_time_point()
{
#ifdef _WIN32
    // Some versions of Visual C++ used a non-hires clock to implement std::chrono::high_resolution_clock.
    static uint64_t perf_freq = []() {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        return static_cast<uint64_t>(freq.QuadPart);
    }();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<uint64_t>(counter.QuadPart * 1000000) / perf_freq;
#else
    auto time_point = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch()).count();
#endif
}

}

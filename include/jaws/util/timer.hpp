#pragma once
#include "jaws/core.hpp"

namespace jaws::util {

extern JAWS_API uint64_t get_time_point();

// std::abs replacement that is actually constexpr.
template <typename T>
inline constexpr T cabs(T value)
{
    return value < static_cast<T>(0) ? -value : value;
}

constexpr double get_delta_time_us(uint64_t t0, uint64_t t1)
{
    return cabs(static_cast<int64_t>(t1) - static_cast<int64_t>(t0)) / 1.0;
}

constexpr double get_delta_time_ms(uint64_t t0, uint64_t t1)
{
    return cabs(static_cast<int64_t>(t1) - static_cast<int64_t>(t0)) / 1000.0;
}

constexpr double GetDeltaTime_s(uint64_t t0, uint64_t t1)
{
    return cabs(static_cast<int64_t>(t1) - static_cast<int64_t>(t0)) / 1000000.0;
}

} // namespace jaws::util

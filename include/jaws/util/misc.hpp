#pragma once
#include "jaws/core.hpp"
#include "jaws/assume.hpp"
#include "jaws/fatal.hpp"
#include "jaws/filesystem.hpp"

#include <cctype>

#if defined(JAWS_OS_WIN)
#    include "jaws/windows.hpp"
#elif defined(JAWS_OS_LINUX) \
    && (_BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED || _POSIX_C_SOURCE >= 200112L)
#    include <unistd.h>
#endif

#include <type_traits>
#include <vector>
#include <sstream>

namespace jaws::util {


inline std::string to_human_size(size_t size)
{
    // Nice idea, from https://github.com/jherico/Vulkan/blob/cpp/examples/context/context.cpp

    static const std::vector<std::string> SUFFIXES{{"B", "KiB", "MiB", "GiB", "TiB", "PiB"}};
    size_t suffixIndex = 0;
    while (suffixIndex < SUFFIXES.size() - 1 && size > 1024) {
        size >>= 10;
        ++suffixIndex;
    }

    std::stringstream buffer;
    buffer << size << " " << SUFFIXES[suffixIndex];
    return buffer.str();
};


inline std::string to_lower(const std::string_view& str)
{
    std::string r(str);
    std::transform(str.begin(), str.end(), r.begin(), [](char c) { return std::tolower(c); });
    return r;
}


template <class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}


inline std::filesystem::path get_executable_path()
{
    static std::filesystem::path path = []() {
#if defined(JAWS_OS_WIN)
        HMODULE module = GetModuleHandleA(nullptr);
        char buffer[3000] = {0};
        // size - 1 so we are guaranteed a null byte at the end no matter what.
        DWORD len = GetModuleFileNameA(module, buffer, sizeof(buffer) - 1);
        JAWS_ASSUME(len > 0);
        return std::filesystem::path(std::string(buffer));
#elif defined(JAWS_OS_LINUX) \
    && (_BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED || _POSIX_C_SOURCE >= 200112L)
        // See https://stackoverflow.com/a/1528493 and https://linux.die.net/man/2/readlink
        char buffer[1024];
        static_assert(sizeof(char) == 1, "ouch"); // Feeling like it :)
        auto len = ::readlink("/proc/self/exe", buffer, sizeof(buffer)); // No null byte is written.
        if (len <= 0) { return std::filesystem::path{}; }
        return std::filesystem::path(std::string(buffer[0], buffer[len]));
#else
        JAWS_FATAL1("Not implemented");
#endif
    }();
    return path;
}


// This one's fun. We can't directly invoke the default c'tor for the
// default argument here:
// void create(const CreateInfo& = {});
// because (https://stackoverflow.com/a/17436088) of clang's (and gcc's?)
// parsing order. But deferring it like this works.
template <typename T>
T make_default()
{
    return T{};
};

} // namespace jaws::util

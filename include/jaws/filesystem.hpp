#pragma once
// Mostly grabbed from https://stackoverflow.com/a/53365539
// Also see lengty discussion here: https://gitlab.kitware.com/cmake/cmake/issues/17834
// and https://github.com/lethal-guitar/RigelEngine/blob/master/cmake/Modules/FindFilesystem.cmake

#if defined(__cpp_lib_filesystem)
#    define NEEDS_EXPERIMENTAL_FILESYSTEM 0
#elif defined(__cpp_lib_experimental_filesystem)
#    define NEEDS_EXPERIMENTAL_FILESYSTEM 1
#elif !defined(__has_include)
#    define NEEDS_EXPERIMENTAL_FILESYSTEM 1
#elif __has_include(<filesystem>)
#    ifdef _MSC_VER
#        if __has_include(<yvals_core.h>)
#            include <yvals_core.h>
#            if defined(_HAS_CXX17) && _HAS_CXX17
#                define NEEDS_EXPERIMENTAL_FILESYSTEM 0
#            endif
#        endif
#        ifndef NEEDS_EXPERIMENTAL_FILESYSTEM
#            define NEEDS_EXPERIMENTAL_FILESYSTEM 1
#        endif
#    else
#        define NEEDS_EXPERIMENTAL_FILESYSTEM 0
#    endif
#elif __has_include(<experimental/filesystem>)
#    define NEEDS_EXPERIMENTAL_FILESYSTEM 1
#else
#    error Failed to find either <filesystem> or <experimental/filesystem>
#endif

#if NEEDS_EXPERIMENTAL_FILESYSTEM
#    define FILESYSTEM_NS experimental::filesystem
#    include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#    define FILESYSTEM_NS std::filesystem
#    include <filesystem>
#endif
#undef NEEDS_EXPERIMENTAL_FILESYSTEM

namespace FILESYSTEM_NS {

template <typename H>
H AbslHashValue(H h, const std::filesystem::path& p)
{
    return H::combine(std::move(h), p.string());
}

} // namespace FILESYSTEM_NS

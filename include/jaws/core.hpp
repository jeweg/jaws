#pragma once
#include <typeinfo>

#if defined WIN32 || defined _WIN32
#    define JAWS_OS_WIN
#elif defined(__APPLE__) && defined(__MACH__)
#    define JAWS_OS_MAC
#else
#    define JAWS_OS_LINUX
#endif

#if defined(_MSC_VER)
//  Microsoft
#    define JAWS_API_EXPORT __declspec(dllexport)
#    define JAWS_API_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
//  GCC
#    define JAWS_API_EXPORT __attribute__((visibility("default")))
#    define JAWS_API_IMPORT
#else
//  do nothing and hope for the best?
#    define JAWS_API_EXPORT
#    define JAWS_API_IMPORT
#    pragma warning Unknown dynamic link import / export semantics.
#endif

#if defined(JAWS_STATIC)
#    define JAWS_API
#elif defined(JAWS_EXPORTS)
#    define JAWS_API JAWS_API_EXPORT
#else
#    define JAWS_API JAWS_API_IMPORT
#endif

#if defined(_MSC_VER)
#    if defined(_CPPRTTI)
#        define JAWS_RTTI_ENABLED
#    endif
#elif defined(__clang__)
#    if __has_feature(cxx_rtti)
#        define JAWS_RTTI_ENABLED
#    endif
#elif defined(__GNUG__)
#    if defined(__GXX_RTTI)
#        define JAWS_RTTI_ENABLED
#    endif
#endif

#if defined(__GNUC__)
#    define JAWS_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#    define JAWS_CURRENT_FUNCTION __FUNCSIG__
#elif defined(__FUNC__)
#    define JAWS_CURRENT_FUNCTION __FUNC__
#else
#    define JAWS_CURRENT_FUNCTION __func__
#endif

#if defined(__ICC)
#define JAWS_DEPRECATED __attribute__((deprecated))
#define JAWS_DEPRECATED_MSG __attribute__((deprecated))
#elif defined(__GNUC__)
#define JAWS_DEPRECATED __attribute__((deprecated))
#define JAWS_DEPRECATED_MSG(msg) __attribte__((deprecated(msg)))
#elif defined(_MSC_VER)
#define JAWS_DEPRECATED __declspec(deprecated)
#define JAWS_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#else
#define JAWS_DEPRECATED 
#define JAWS_DEPRECATED_MSG(msg) 
#endif

// TODO: maybe move elsewhere.
namespace jaws {

#if defined(JAWS_RTTI_ENABLED)
template <typename T>
constexpr const char* get_rtti_type_name()
{
    return typeid(T).name();
}
#else
template <typename T>
constexpr const char* get_rtti_type_name()
{
    return "";
}
#endif

} // namespace jaws


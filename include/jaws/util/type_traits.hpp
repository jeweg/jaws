#pragma once
#include <type_traits>

namespace jaws::util {

template <typename T, typename Enable = void>
struct type_traits
{
    static constexpr const char *string_repr = "T";
};

template <typename T>
struct type_traits<T, typename std::enable_if_t<std::is_rvalue_reference_v<T>>>
{
    static constexpr const char *string_repr = "T&&";
};

template <typename T>
struct type_traits<
    T,
    typename std::enable_if_t<std::is_lvalue_reference_v<T> && std::is_const_v<typename std::remove_reference_t<T>>>>
{
    static constexpr const char *string_repr = "const T&";
};

template <typename T>
struct type_traits<
    T,
    typename std::enable_if_t<std::is_lvalue_reference_v<T> && !std::is_const_v<typename std::remove_reference_t<T>>>>
{
    static constexpr const char *string_repr = "T&";
};

template <typename T>
struct type_traits<
    T,
    typename std::enable_if_t<std::is_pointer_v<T> && std::is_const_v<typename std::remove_pointer_t<T>>>>
{
    static constexpr const char *string_repr = "const T*";
};

template <typename T>
struct type_traits<
    T,
    typename std::enable_if_t<std::is_pointer_v<T> && !std::is_const_v<typename std::remove_pointer_t<T>>>>
{
    static constexpr const char *string_repr = "T*";
};

//=========================================================================
// Convenience functions

// Not sure if worth the little convenience.
template <typename T>
const char *get_type_string_repr(T &&expr)
{
    return jaws::util::type_traits<T>::string_repr;
};

} // namespace jaws::util

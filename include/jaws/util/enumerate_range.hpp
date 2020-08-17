#pragma once

#include <iterator>
#include <tuple>
#include <utility>
#include <type_traits>

//=========================================================================
// enumerate_range

namespace jaws::util::detail {

template <typename WrappedIteratorType>
class WrappingIterator
{
    int _pos = 0;
    WrappedIteratorType _iter;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<int, WrappedIteratorType>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    WrappingIterator() = default;
    WrappingIterator(int pos, WrappedIteratorType iter) : _pos(pos), _iter(std::forward<WrappedIteratorType>(iter)) {}

    friend bool operator==(const WrappingIterator &lhs, const WrappingIterator &rhs) { return lhs._iter == rhs._iter; }
    friend bool operator!=(const WrappingIterator &lhs, const WrappingIterator &rhs) { return !(lhs == rhs); }

    auto operator*() { return std::tuple<int, decltype(*_iter)>(_pos, *_iter); }
    auto operator++()
    {
        ++_pos;
        ++_iter;
        return *this;
    }

    auto operator++(int)
    {
        auto before(*this);
        operator++();
        return before;
    }
};

template <typename RangeType>
class EnumerateRangeProxy
{
    RangeType _range;

public:
    explicit EnumerateRangeProxy(RangeType range) : _range(range) {}
    auto begin() { return WrappingIterator<decltype(std::begin(_range))>(0, std::begin(_range)); }
    auto end() { return WrappingIterator<decltype(std::end(_range))>(0, std::end(_range)); }
};

} // namespace jaws::util::detail

namespace jaws::util {

template <typename RangeType>
auto enumerate_range(const RangeType &r, std::enable_if_t<std::is_lvalue_reference_v<RangeType>, int> = 0)
{
    return detail::EnumerateRangeProxy<const RangeType &>(r);
}
template <typename RangeType>
auto enumerate_range(RangeType &r)
{
    return detail::EnumerateRangeProxy<RangeType &>(r);
}
template <typename RangeType>
auto enumerate_range(RangeType &&r, std::enable_if_t<std::is_rvalue_reference_v<RangeType &&>, int> = 0)
{
    // Holding the original range as RangeType&& does not extend its lifetime to the end of the loop body,
    // but our rvalue-ref will live long enough, we just have to move the original range into it.
    // Note that we use std::forward over std::move to avoid clang warnings; we limit this overload through
    // SFINAE to rvalue ref anyway, so the std::forward is equivalent to std::move here.
    return detail::EnumerateRangeProxy<RangeType>(std::forward<RangeType>(r));
}
template <typename RangeElemType>
auto enumerate_range(std::initializer_list<RangeElemType> r)
{
    return detail::EnumerateRangeProxy<std::initializer_list<RangeElemType>>(r);
}

} // namespace jaws::util

//=========================================================================
// enumerate_range_total

namespace jaws::util::detail {

template <typename WrappedIteratorType>
class WrappingIteratorTotal : public std::iterator<std::forward_iterator_tag, std::tuple<int, int, WrappedIteratorType>>
{
    int _pos = 0;
    int _total = 0;
    WrappedIteratorType _iter;

public:
    WrappingIteratorTotal() = default;
    WrappingIteratorTotal(int pos, int total, WrappedIteratorType iter) :
        _pos(pos), _total(total), _iter(std::forward<WrappedIteratorType>(iter))
    {}

    friend bool operator==(const WrappingIteratorTotal &lhs, const WrappingIteratorTotal &rhs)
    {
        return lhs._iter == rhs._iter;
    }
    friend bool operator!=(const WrappingIteratorTotal &lhs, const WrappingIteratorTotal &rhs) { return !(lhs == rhs); }

    auto operator*() { return std::tuple<int, int, decltype(*_iter)>(_pos, _total, *_iter); }
    auto operator++()
    {
        ++_pos;
        ++_iter;
        return *this;
    }

    auto operator++(int)
    {
        auto before(*this);
        operator++();
        return before;
    }
};

template <typename RangeType>
class EnumerateRangeTotalProxy
{
    RangeType _range;
    int _total = 0;

public:
    // explicit EnumerateRangeTotalProxy(RangeType range) : _range(std::forward<RangeType>(range))
    explicit EnumerateRangeTotalProxy(RangeType range) : _range(range)
    {
        _total = static_cast<int>(std::distance(std::begin(_range), std::end(_range)));
    }
    auto begin() { return WrappingIteratorTotal<decltype(std::begin(_range))>(0, _total, std::begin(_range)); }
    auto end() { return WrappingIteratorTotal<decltype(std::end(_range))>(0, _total, std::end(_range)); }
};

} // namespace jaws::util::detail

namespace jaws::util {

template <typename RangeType>
auto enumerate_range_total(const RangeType &r, std::enable_if_t<std::is_lvalue_reference_v<RangeType>, int> = 0)
{
    return detail::EnumerateRangeTotalProxy<const RangeType &>(r);
}
template <typename RangeType>
auto enumerate_range_total(RangeType &r)
{
    return detail::EnumerateRangeTotalProxy<RangeType &>(r);
}
template <typename RangeType>
auto enumerate_range_total(RangeType &&r, std::enable_if_t<std::is_rvalue_reference_v<RangeType &&>, int> = 0)
{
    // Holding the original range as RangeType&& does not extend its lifetime to the end of the loop body,
    // but our rvalue-ref will live long enough, we just have to move the original range into it.
    // Note that we use std::forward over std::move to avoid clang warnings; we limit this overload through
    // SFINAE to rvalue ref anyway, so the std::forward is equivalent to std::move here.
    return detail::EnumerateRangeTotalProxy<RangeType>(std::forward<RangeType>(r));
}
template <typename RangeElemType>
auto enumerate_range_total(std::initializer_list<RangeElemType> r)
{
    return detail::EnumerateRangeTotalProxy<std::initializer_list<RangeElemType>>(r);
}

} // namespace jaws::util

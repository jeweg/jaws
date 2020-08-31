#pragma once
#include "jaws/core.hpp"

#include "absl/hash/hash.h"
#include "absl/container/flat_hash_map.h"
#include <tuple>

namespace jaws::util {


// Usable as ad-hoc wrapper in combine calls and as
// both a hasher type and an eq type for hash maps.
struct Prehashed
{
    Prehashed() = default;
    explicit Prehashed(size_t hash) : hash(hash) {}

    template <typename H>
    friend H AbslHashValue(H h, Prehashed ph)
    {
        return H::combine(std::move(h), ph.hash);
    }
    size_t hash = 0;
    size_t operator()(size_t value) const { return value; }

    bool operator()(size_t l, size_t r) const { return l == r; }
};


template <typename... Args>
size_t combined_hash(const Args &... args)
{
    using TupleType = std::tuple<const Args &...>;
    TupleType t = std::make_tuple(args...);
    return absl::Hash<TupleType>{}(t);
}


#if 0

template <typename CachedValue, typename... Keys>
class HashedCache
{
private:
    // We use tuple because abseil's hashes already have an overload for them.
    using TupleType = std::tuple<const Keys&...>;

    // https://github.com/abseil/abseil-cpp/blob/master/absl/container/flat_hash_map.h
    absl::flat_hash_map<size_t, CachedValue, Prehashed> _hash_map;

public:
    // We don't get forwarding references here. We'd need to be a template member function
    // ourselves for that.

    void insert(CachedValue&& value, const Keys&... keys)
    {
        size_t hash_key = absl::Hash<TupleType>{}(std::make_tuple(keys...));
        _hash_map.insert_or_assign(hash_key, std::move(value));
    }

    void insert(const CachedValue& value, const Keys&... keys)
    {
        auto t = std::make_tuple(keys...);
        size_t hash_key = absl::Hash<TupleType>{}(t);
        _hash_map.insert_or_assign(hash_key, value);
    }

    CachedValue* lookup(const Keys&... keys)
    {
        // auto t = std::make_tuple(std::forward<const Keys>(keys)...);
        auto t = std::make_tuple(keys...);
        size_t hash_key = absl::Hash<TupleType>{}(t);

        auto iter = _hash_map.find(hash_key);
        if (iter == _hash_map.end()) { return nullptr; }
        return &iter->second;
    }
};
#endif

}

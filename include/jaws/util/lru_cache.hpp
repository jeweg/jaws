#pragma once
#include "jaws/util/hashing.hpp"
#include "absl/container/flat_hash_map.h"
#include <list>
#include <limits>

namespace jaws::util {

template <
    typename Key,
    typename Value,
    typename Hash = absl::container_internal::hash_default_hash<Key>,
    typename Eq = absl::container_internal::hash_default_eq<Key>>
class LruCache
{
private:
    int _max_elem_count = 0;
    int _max_age = 0;
    // If the lock overflows (not UB b/c unsigned!), we
    // correct for it in the age calculation.
    uint32_t _clock = 0;
    struct Element
    {
        Element(Element &&) = default;
        Element &operator=(Element &&) = default;

        Element(const Element &) = delete;
        Element &operator=(const Element &) = delete;

        template <typename K, typename V>
        Element(uint32_t access_time, K &&key, V &&value) :
            access_time(access_time), key(std::forward<K>(key)), value(std::forward<V>(value))
        {}

        uint32_t access_time = 0;
        Key key;
        Value value;
    };

    // Elements, ordered from oldest to newest
    mutable std::list<Element> _lru_list;

    // Hashed index of list elements
    using ListIter = typename std::list<Element>::iterator;
    mutable absl::flat_hash_map<Key, ListIter, Hash, Eq> _hash_map;


public:
    explicit LruCache(int max_elem_count = -1, int max_age = -1) : _max_elem_count(max_elem_count), _max_age(max_age) {}

    void tick_clock()
    {
        // Even *if* this overflows, this shouldn't do much harm.
        ++_clock;
        purge(_max_elem_count, _max_age);
    }

    template <typename K, typename V>
    void insert(K &&key, V &&value)
    {
        auto iter = _hash_map.find(key);
        if (iter != _hash_map.end()) {
            // Key exists. We override the value and move
            // the element to the end of the list.
            // Note that this can't change the number of elements in the cache.
            iter->second->access_time = _clock;
            _lru_list.splice(_lru_list.end(), _lru_list, iter->second);
        } else {
            // Insert new element.
            // Move (if forwarding ref allows it) key and value at their latest
            // occurence in the function.
            _lru_list.emplace_back(_clock, key, std::forward<V>(value));
            _hash_map.insert(std::make_pair(std::forward<K>(key), --_lru_list.end()));
            purge(_max_elem_count, _max_age);
        }
    }


    const Value *lookup(const Key &key) const
    {
        auto iter = _hash_map.find(key);
        if (iter != _hash_map.end()) {
            // Update time, move to end of list (keeps elem ptr stable), return value.
            iter->second->access_time = _clock;
            _lru_list.splice(_lru_list.end(), _lru_list, iter->second);
            return &iter->second->value;
        } else {
            return nullptr;
        }
    }

    Value *lookup(const Key &key) { return const_cast<Value *>(const_cast<const LruCache *>(this)->lookup(key)); }

    // Does lookup. If it found a value, it calls the specified functor
    // with signature (const Key&, const Value*). If the functor returns true,
    // the element is erased from the cache directly and this function
    // returns nullptr. It's meant for cases where we have extra information
    // in the elements to decide if a cached value is stale.
    // It can of course be done in separate steps, but that'd require
    // multiple lookups whereas with this we hold the internal iterators already
    // in hand.
    template <typename Func>
    const Value *lookup_unless(const Key &key, Func unless_func) const
    {
        auto iter = _hash_map.find(key);
        if (iter != _hash_map.end()) {
            if (unless_func(key, &iter->second->value)) {
                _lru_list.erase(iter->second);
                _hash_map.erase(iter);
                return nullptr;
            }
            // Update time, move to end of list (keeps elem ptr stable), return value.
            iter->second->access_time = _clock;
            _lru_list.splice(_lru_list.end(), _lru_list, iter->second);
            return &iter->second->value;
        } else {
            return nullptr;
        }
    }

    template <typename Func>
    Value *lookup_unless(const Key &key, Func unless_func)
    {
        return const_cast<Value *>(const_cast<const LruCache *>(this)->lookup_unless<Func>(key, unless_func));
    }


    bool erase(const Key &key)
    {
        auto iter = _hash_map.find(key);
        if (iter == _hash_map.end()) { return false; }
        _lru_list.erase(iter->second);
        _hash_map.erase(iter);
    }

    void purge() { purge(_max_elem_count, _max_age); }

    void purge(int max_elem_count, int max_age)
    {
        if (max_age < 0) {
            if (max_elem_count < 0) {
                // Disabled invalidation policies.
                return;
            }
            if (get_size() <= max_elem_count) {
                // Haven't reached critical size yet.
                return;
            }
        }
        // Remove elems until size is okay and the age is young enough.
        // We iterate from the oldest element and erase from the hash map until
        // both criteria agree we don't need to erase any more.
        // erasing from the list is delayed until the end because erasing the
        // full range should be slightly more efficient.
        ListIter erase_iter = _lru_list.begin();
        int size = get_size();
        for (;;) {
            if (erase_iter == _lru_list.end()) { break; }
            bool erase_this = false;
            if (max_elem_count >= 0 && size > max_elem_count) {
                erase_this = true;
            } else if (max_age >= 0) {
                uint32_t at = erase_iter->access_time;
                // This trick corrects for overflow. If clock < access_time, the clock has overflowed
                // and we need to correct the delta by adding the highest value. This does that without branching.
                uint32_t age = std::numeric_limits<uint32_t>::max() * static_cast<uint32_t>(_clock < at) + _clock - at;
                if (age > max_age) { erase_this = true; }
            }
            if (erase_this) {
                _hash_map.erase(erase_iter->key);
                --size;
                ++erase_iter;
            } else {
                break;
            }
        }
        _lru_list.erase(_lru_list.begin(), erase_iter);
    }

    void clear()
    {
        _lru_list.clear();
        _hash_map.clear();
    }

    bool empty() const { return _hash_map.empty(); }
    size_t size() const { return _hash_map.size(); }
    int get_size() const { return static_cast<int>(size()); }
};


/*
template <typename Value>
using PrehashedLruCache = LruCache<size_t, Value, jaws::util::Prehashed, jaws::util::Prehashed>;


// Adapter for key convenience -- key is not stored, though.
template <typename Key, typename Value>
class NoKeysStoredLruCache : public LruCache<size_t, Value, jaws::util::Prehashed, jaws::util::Prehashed>
{
    using BaseClass = LruCache<size_t, Value, jaws::util::Prehashed, jaws::util::Prehashed>;

public:
    const Value *lookup_keyed(const Key &key) const
    {
        size_t hash_value = absl::Hash<Key>{}(key);
        return BaseClass::lookup(hash_value);
    };
    Value *lookup_keyed(const Key &key)
    {
        return const_cast<Value *>(const_cast<const NoKeysStoredLruCache *>(this)->lookup_keyed(key));
    }

    template <typename K, typename V>
    void insert_keyed(K &&key, V &&value)
    {
        size_t hash_value = absl::Hash<Key>{}(std::forward(key));
        BaseClass::insert(hash_value, std::forward(value));
    }
};
*/


}; // namespace jaws::util

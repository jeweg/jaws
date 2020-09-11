#pragma once
#include "jaws/util/hashing.hpp"
#include "absl/container/flat_hash_map.h"
#include "jaws/util/misc.hpp"
#include <list>
#include <limits>

namespace jaws::util {

// LRU cache w/ pointer stability and optional deleter observer.
// which, if set, gets called before, not instead of, the destructor.
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

        Element(uint32_t access_time, const Key &key, const Value &value) :
            access_time(access_time), key(key), value(value)
        {}

        Element(uint32_t access_time, const Key &key, Value &&value) :
            access_time(access_time), key(key), value(forward_to_move_or_copy<Value>(std::move(value)))
        {}

        struct DummyTag
        {};

        // We use the tag type here to avoid ambiguous constructor overloads
        // when Args is just Value.
        template <typename... Args>
        Element(DummyTag, uint32_t access_time, const Key &key, Args &&... args) :
            access_time(access_time), key(key), value(std::forward<Args>(args)...)
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

    // Base class for type erasure.
    struct AbstractDeleteObserver
    {
        virtual ~AbstractDeleteObserver() = default;
        virtual void about_to_delete(const Key &, Value &) = 0;
    };
    template <typename Func>
    struct DeleteObserver final : AbstractDeleteObserver
    {
        DeleteObserver(Func func) : func(func) {}
        void about_to_delete(const Key &key, Value &value) override { func(key, value); }

        Func func;
    };
    // Type-erased deleter
    AbstractDeleteObserver *_delete_observer = nullptr;

    // Updating an existing entry or inserting a new entry is almost identical
    // in the overloads, except for how they pass arguments or do the value updating.
    // We let the overloads express only those differences by using this common
    // function with two customization points.
    template <typename UpdateFunc, typename InsertFunc>
    Value *UpdateOrInsert(const Key &key, UpdateFunc ufunc, InsertFunc ifunc)
    {
        auto iter = _hash_map.find(key);
        if (iter != _hash_map.end()) {
            // Key exists. We override the value and move the element to the end of the list.
            // Note that this can't change the number of elements in the cache.
            iter->second->access_time = _clock;
            ufunc(iter->second->value);
            _lru_list.splice(_lru_list.end(), _lru_list, iter->second);
            return &iter->second->value;
        } else {
            // Insert new element.
            ifunc();
            _hash_map.insert(std::make_pair(key, --_lru_list.end()));
            purge(_max_elem_count, _max_age);
            // TODO: purging right away what we just inserted probably indicates a problem, though.
            // Let's assume on it for now.
            JAWS_ASSUME(!_lru_list.empty());
            if (_lru_list.empty()) {
                return nullptr;
            } else {
                return &_lru_list.back().value;
            }
        }
    }

public:
    explicit LruCache(int max_elem_count = -1, int max_age = -1) : _max_elem_count(max_elem_count), _max_age(max_age) {}

    template <typename DeleteObserverFunc>
    explicit LruCache(DeleteObserverFunc delete_observer, int max_elem_count = -1, int max_age = -1) :
        _delete_observer(new DeleteObserver(delete_observer)), _max_elem_count(max_elem_count), _max_age(max_age)
    {}

    ~LruCache() { delete _delete_observer; }


    void advance_clock()
    {
        // Yes, this may eventually overflow. However, it being unsigned that is not undefined behaviour,
        // and also it cannot do much harm here. When purging, we detect likely overflow and correct for it.
        ++_clock;
        purge(_max_elem_count, _max_age);
    }


    Value *insert(const Key &key, const Value &value)
    {
        return UpdateOrInsert(
            key, [&](Value &val) { val = value; }, [&]() { _lru_list.emplace_back(_clock, key, value); });
    }


    Value *insert(const Key &key, Value &&value)
    {
        return UpdateOrInsert(
            key,
            [&](Value &val) { val = forward_to_move_or_copy<Value>(std::move(value)); },
            [&]() { _lru_list.emplace_back(_clock, key, std::move(value)); });
    }

    template <typename... Args>
    Value *emplace(const Key &key, Args &&... args)
    {
        return UpdateOrInsert(
            key,
            [&](Value &val) {
                // Manual delete and placement new here makes the least amount of assumptions about Value
                val.~Value();
                new (&val) Value(std::forward<Args>(args)...);
            },
            [&]() { _lru_list.emplace_back(Element::DummyTag{}, _clock, key, std::forward<Args>(args)...); });
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

    // Does a lookup. Then, if it found a value, it calls the specified functor
    // with signature (const Key&, Value&). If the functor returns true,
    // the element is erased from the cache directly and this function
    // returns nullptr. It's meant for cases where we have extra information
    // in the elements to decide if a cached value is stale.
    // It can of course be done in separate steps, but that'd require
    // multiple lookups whereas with this we hold the internal iterators already
    // in hand.
    template <typename Func>
    const Value *lookup_or_remove(const Key &key, Func remove_classifier_func) const
    {
        auto iter = _hash_map.find(key);
        if (iter != _hash_map.end()) {
            if (remove_classifier_func(key, iter->second->value)) {
                if (_delete_observer) { _delete_observer->del(iter->second->key, iter->second->value); }
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
    Value *lookup_or_remove(const Key &key, Func unless_func)
    {
        return const_cast<Value *>(const_cast<const LruCache *>(this)->lookup_or_remove<Func>(key, unless_func));
    }


    bool remove(const Key &key)
    {
        auto iter = _hash_map.find(key);
        if (iter == _hash_map.end()) { return false; }
        if (_delete_observer) { _delete_observer->del(key, iter->second.value); }
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
                if (_delete_observer) { _delete_observer->about_to_delete(erase_iter->key, erase_iter->value); }
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
        if (_delete_observer) {
            for (auto iter = _lru_list.begin(), end_iter = _lru_list.end(); iter != end_iter; ++iter) {
                _delete_observer->about_to_delete(iter->key, iter->value);
            }
        }
        _lru_list.clear();
        _hash_map.clear();
    }

    bool empty() const { return _hash_map.empty(); }
    size_t size() const { return _hash_map.size(); }
    int get_size() const { return static_cast<int>(size()); }
};


}; // namespace jaws::util

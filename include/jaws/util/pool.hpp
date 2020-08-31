#pragma once

#include "jaws/assume.hpp"
#include "jaws/util/misc.hpp"
#include <vector>
#include <utility>

namespace jaws::util {


// TODO: check incoming ids against pool id. might also assign pool ids automatically.

template <typename ElementType>
class Pool
{
public:
    Pool(uint16_t pool_id, uint32_t initial_capacity, float growth_factor = 1.5);

    using element_type = ElementType;

    template <typename... Args>
    uint64_t emplace(Args &&... args);

    // I've tried turning the two insert overloads into one function
    // using forward references, but it seemed to me to get unnecessarily
    // convoluted and hard to maintain.
    uint64_t insert(ElementType &&elem);
    uint64_t insert(const ElementType &elem);

    bool remove(uint64_t id);

    ElementType *lookup(uint64_t id);
    const ElementType *lookup(uint64_t id) const;

    bool empty() const { return get_element_count() == 0; }
    uint32_t get_element_count() const;

    uint32_t get_capacity() const;

private:
    struct PoolSlot
    {
        uint16_t generation : 15;
        uint16_t holds_element : 1;

        // union: bad because it doesn't handle destructors.
        // std::variant: how can we handle the case of ElementType being uint32_t, the same as its next-free-index
        // variant type?
        // Hence this implementation: TODO

        std::array<uint8_t, (sizeof(ElementType) > 4 ? sizeof(ElementType) : 4)> data;

        uint32_t &next_free_index()
        {
            JAWS_ASSUME(!holds_element);
            return *reinterpret_cast<uint32_t *>(data.data());
        }

        ElementType *element()
        {
            JAWS_ASSUME(holds_element);
            return reinterpret_cast<ElementType *>(data.data());
        }

        const ElementType *element() const
        {
            JAWS_ASSUME(holds_element);
            return reinterpret_cast<const ElementType *>(data.data());
        }

        PoolSlot() : generation(0), holds_element(0) {}
        ~PoolSlot() { cleanup(); }
        PoolSlot(const PoolSlot &) = delete;
        PoolSlot &operator=(const PoolSlot &) = delete;
        PoolSlot(PoolSlot &&) = default;
        PoolSlot &operator=(PoolSlot &&) = default;

        void cleanup()
        {
            if (holds_element) { element()->~ElementType(); }
            holds_element = 0;
        }
    };

    uint32_t alloc_pool_slot(); //< Returns the index.

    static constexpr uint32_t extract_index(uint64_t id) { return id & 0xffffffff; }
    static constexpr uint16_t extract_generation(uint64_t id) { return (id >> 32) & 0x7fff; }
    static constexpr uint16_t extract_pool_id(uint64_t id) { return (id >> 48) & 0xffff; }

    static constexpr uint16_t next_generation(uint16_t gen) { return (gen + 1) % 0x8000; }
    constexpr uint64_t make_id(uint32_t index, uint16_t gen)
    {
        return (static_cast<uint64_t>(_pool_id) << 48) | (static_cast<uint64_t>(gen) << 32) | index;
    }

    uint16_t _pool_id;
    uint32_t _element_count;
    float _growth_factor;
    uint32_t _first_free_elem;
    std::vector<PoolSlot> _pool_slots;
}; // namespace jaws::util


template <typename ElementType>
Pool<ElementType>::Pool(uint16_t pool_id, uint32_t initial_capacity, float growth_factor) :
    _pool_id(pool_id), _element_count(0), _growth_factor(growth_factor), _first_free_elem(0)
{
    if (initial_capacity > 0) {
        _pool_slots.resize(initial_capacity);
        for (uint32_t i = 0; i < initial_capacity - 1; ++i) { _pool_slots[i].next_free_index() = i + 1; }
        _pool_slots.back().next_free_index() = -1;
    }
}


template <typename ElementType>
uint32_t Pool<ElementType>::alloc_pool_slot()
{
    if (_first_free_elem == -1) {
        // Pool is exhausted. Grow it if we can.
        if (_growth_factor <= 0) {
            // Dynamic growing is disabled.
            return -1;
        } else {
            uint32_t old_capacity = get_capacity();
            uint32_t new_capacity = static_cast<uint32_t>(old_capacity * _growth_factor);
            // Grow by a minimum of 1 element.
            new_capacity = new_capacity > old_capacity ? new_capacity : old_capacity + 1;

            _pool_slots.resize(new_capacity);

            // Create a free list in the newly added vector region.
            for (uint32_t i = old_capacity; i < new_capacity - 1; ++i) { _pool_slots[i].next_free_index() = i + 1; }
            _pool_slots.back().next_free_index() = -1;
            _first_free_elem = old_capacity;
        }
    }

    uint32_t this_index = _first_free_elem;
    _first_free_elem = _pool_slots[this_index].next_free_index();
    ++_element_count;
    PoolSlot &slot = _pool_slots[this_index];
    slot.generation = next_generation(slot.generation);
    slot.holds_element = 1;
    return this_index;
}


template <typename ElementType>
template <typename... Args>
uint64_t Pool<ElementType>::emplace(Args &&... args)
{
    auto index = alloc_pool_slot();
    if (index == -1) { return -1; }
    auto &slot = _pool_slots[index];
    new (&slot.data) ElementType(std::forward<Args>(args)...);
    return make_id(index, slot.generation);
}


template <typename ElementType>
uint64_t Pool<ElementType>::insert(ElementType &&elem)
{
    auto index = alloc_pool_slot();
    if (index == -1) { return -1; }
    auto &slot = _pool_slots[index];
    // We have an rvalue ref elem on our hands.
    // But this alone does not mean we can just std::move it on to the ElementType
    // constructor. That would assume ElementType has a move constructor and
    // it might not. In those cases we try to call the copy constructor instead, i.e.
    // pass const ElementType& instead of ElementType&&.
    // forward_to_move_or_copy does exactly that cast, depending on whether
    // ElementType has a move constructor.
    new (&slot.data) ElementType(forward_to_move_or_copy<ElementType>(std::move(elem)));
    return make_id(index, slot.generation);
}


template <typename ElementType>
uint64_t Pool<ElementType>::insert(const ElementType &elem)
{
    auto index = alloc_pool_slot();
    if (index == -1) { return -1; }
    auto &slot = _pool_slots[index];
    new (&slot.data) ElementType(elem);
    return make_id(index, slot.generation);
}


template <typename ElementType>
bool Pool<ElementType>::remove(uint64_t id)
{
    uint32_t index = extract_index(id);
    if (index >= get_capacity()) { return false; }
    _pool_slots[index].cleanup();
    --_element_count;
    return true;
}


template <typename ElementType>
ElementType *Pool<ElementType>::lookup(uint64_t id)
{
    return const_cast<ElementType *>(const_cast<const Pool *>(this)->lookup(id));
}

template <typename ElementType>
const ElementType *Pool<ElementType>::lookup(uint64_t id) const
{
    uint32_t index = extract_index(id);
    if (index >= get_capacity()) { return nullptr; }
    const PoolSlot &slot = _pool_slots[index];
    if (!slot.holds_element || extract_generation(id) != slot.generation) {
        return nullptr;
    } else {
        return slot.element();
    }
}


template <typename ElementType>
uint32_t Pool<ElementType>::get_element_count() const
{
    return _element_count;
}


template <typename ElementType>
uint32_t Pool<ElementType>::get_capacity() const
{
    return static_cast<uint32_t>(_pool_slots.size());
}


}

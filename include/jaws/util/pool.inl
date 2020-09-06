#pragma once

#include "jaws/util/misc.hpp"
#include <algorithm>

namespace jaws::util {


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
Pool<ElementType, NumericIdType, generation_bits, index_bits>::Pool(
    NumericIdType initial_capacity, float growth_factor) :
    _element_count(0), _growth_factor(growth_factor), _first_free_elem(0)
{
    if (initial_capacity > 0) {
        _pool_slots.resize(initial_capacity);
        for (size_t i = 0; i < _pool_slots.size(); ++i) { _pool_slots[i].as_next_free_index() = i + 1; }
        _pool_slots.back().as_next_free_index() = -1;
    }
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
Pool<ElementType, NumericIdType, generation_bits, index_bits>::~Pool()
{
    for (PoolSlot &slot : _pool_slots) { slot.cleanup(); }
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
size_t Pool<ElementType, NumericIdType, generation_bits, index_bits>::alloc_pool_slot()
{
    if (_pool_slots.empty()) {
        _pool_slots.resize(1);
        _pool_slots.back().as_next_free_index() = -1;
    }
    if (_first_free_elem == -1) {
        // Pool is exhausted. Grow it if we can.
        if (_growth_factor <= 1) {
            // Dynamic growing is disabled.
            return -1;
        } else {
            size_t old_capacity = get_capacity();
            size_t new_capacity = std::min(
                MAX_ELEM_COUNT, std::max(old_capacity + 1, static_cast<size_t>(old_capacity * _growth_factor)));

            if (new_capacity == old_capacity) { return -1; }
            _pool_slots.resize(new_capacity);

            // Create a free list in the newly added vector region.
            for (size_t i = old_capacity; i < new_capacity - 1; ++i) { _pool_slots[i].as_next_free_index() = i + 1; }
            _pool_slots.back().as_next_free_index() = -1;
            _first_free_elem = old_capacity;
        }
    }

    size_t this_index = _first_free_elem;
    _first_free_elem = _pool_slots[this_index].as_next_free_index();
    ++_element_count;
    PoolSlot &slot = _pool_slots[this_index];
    slot.generation = (slot.generation + 1) % (static_cast<NumericIdType>(1) << generation_bits);
    slot.holds_element = true;
    return this_index;
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
template <typename... Args>
typename Pool<ElementType, NumericIdType, generation_bits, index_bits>::Id
Pool<ElementType, NumericIdType, generation_bits, index_bits>::emplace(Args &&... args)
{
    size_t index = alloc_pool_slot();
    if (index == -1) { return {}; }
    auto &slot = _pool_slots[index];
    new (&slot.data) ElementType(std::forward<Args>(args)...);
    return Id(slot.generation, index);
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
typename Pool<ElementType, NumericIdType, generation_bits, index_bits>::Id
Pool<ElementType, NumericIdType, generation_bits, index_bits>::insert(ElementType &&elem)
{
    size_t index = alloc_pool_slot();
    if (index == -1) { return {}; }
    auto &slot = _pool_slots[index];
    // We have an rvalue ref elem on our hands.
    // But this alone does not mean we can just std::move it on to the ElementType
    // constructor. That would assume ElementType has a move constructor and
    // it might not. In those cases we try to call the copy constructor instead, i.e.
    // pass const ElementType& instead of ElementType&&.
    // forward_to_move_or_copy does exactly that cast, depending on whether
    // ElementType has a move constructor.
    new (&slot.data) ElementType(forward_to_move_or_copy<ElementType>(std::move(elem)));
    return Id(slot.generation, index);
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
typename Pool<ElementType, NumericIdType, generation_bits, index_bits>::Id
Pool<ElementType, NumericIdType, generation_bits, index_bits>::insert(const ElementType &elem)
{
    auto index = alloc_pool_slot();
    if (index == -1) { return {}; }
    auto &slot = _pool_slots[index];
    new (&slot.data) ElementType(elem);
    return Id(slot.generation, index);
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
bool Pool<ElementType, NumericIdType, generation_bits, index_bits>::remove(Id id)
{
    auto index = id.get_index();
    if (index >= get_capacity()) { return false; }
    PoolSlot &slot = _pool_slots[index];
    slot.cleanup();
    // Make slot the head of the free list.
    slot.as_next_free_index() = _first_free_elem;
    _first_free_elem = index;
    --_element_count;
    return true;
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
ElementType *Pool<ElementType, NumericIdType, generation_bits, index_bits>::lookup(Id id)
{
    return const_cast<ElementType *>(const_cast<const Pool *>(this)->lookup(id));
}

template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
const ElementType *Pool<ElementType, NumericIdType, generation_bits, index_bits>::lookup(Id id) const
{
    auto index = id.get_index();
    if (index >= get_capacity()) { return nullptr; }
    const PoolSlot &slot = _pool_slots[index];
    if (!slot.holds_element || id.get_generation() != slot.generation) {
        return nullptr;
    } else {
        return slot.as_element();
    }
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
bool Pool<ElementType, NumericIdType, generation_bits, index_bits>::empty() const
{
    return _element_count == 0;
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
size_t Pool<ElementType, NumericIdType, generation_bits, index_bits>::get_element_count() const
{
    return _element_count;
}


template <typename ElementType, typename NumericIdType, size_t generation_bits, size_t index_bits>
size_t Pool<ElementType, NumericIdType, generation_bits, index_bits>::get_capacity() const
{
    return _pool_slots.size();
}

}

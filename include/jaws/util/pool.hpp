#pragma once

#include "jaws/assume.hpp"
#include "jaws/util/misc.hpp"

#include <vector>
#include <memory>

namespace jaws::util {

template <typename ElemType>
class Pool
{
public:
    ElemType *insert(ElemType &&);
    ElemType *insert(const ElemType &);

    template <typename... Args>
    ElemType *emplace(Args &&... args);

    void free(ElemType *);
    void clear();
    size_t size() const { return _element_count; }
    bool empty() const { return _element_count == 0; }

private:
    struct MemoryBlockDeleter
    {
        void operator()(ElemType *ptr) { ::operator delete(ptr); }
    };

    ElemType *allocate();

    using FreeStack = std::vector<ElemType *>;
    FreeStack _free_stack;
    using MemoryBlock = std::unique_ptr<ElemType, MemoryBlockDeleter>;
    using MemoryBlockList = std::vector<MemoryBlock>;
    MemoryBlockList _memory_blocks;
    size_t _element_count = 0;
};


template <typename ElemType>
ElemType *Pool<ElemType>::allocate()
{
    if (_free_stack.empty()) {
        // Grow the vector by 50% its current size
        const size_t num_added = empty() ? 32 : _element_count / 2;
        _memory_blocks.emplace_back(static_cast<ElemType *>(::operator new(sizeof(ElemType) * num_added)));

        // Add newly aquired slots to free list
        ElemType *ptr = static_cast<ElemType *>(_memory_blocks.back().get()) + num_added;
        _free_stack.reserve(num_added);
        // Add in inverse order so we fill the blocks up from their start. Eases debugging.
        for (size_t i = 0; i < num_added; ++i) { _free_stack.push_back(--ptr); }
    }
    ElemType *result_ptr = _free_stack.back();
    _free_stack.pop_back();
    ++_element_count;
    return result_ptr;
}


template <typename ElemType>
ElemType *Pool<ElemType>::insert(ElemType &&elem)
{
    ElemType *result_ptr = allocate();
    new (result_ptr) ElemType(forward_to_move_or_copy<ElemType>(std::move(elem)));
    return result_ptr;
}


template <typename ElemType>
ElemType *Pool<ElemType>::insert(const ElemType &elem)
{
    ElemType *result_ptr = allocate();
    new (result_ptr) ElemType(forward_to_move_or_copy<ElemType>(std::move(elem)));
    return result_ptr;
}


template <typename ElemType>
template <typename... Args>
ElemType *Pool<ElemType>::emplace(Args &&... args)
{
    ElemType *result_ptr = allocate();
    new (result_ptr) ElemType(std::forward<Args>(args)...);
    return result_ptr;
}


template <typename ElemType>
void Pool<ElemType>::free(ElemType *ptr)
{
    ptr->~ElemType();
    _free_stack.push_back(ptr);
    --_element_count;
}


template <typename ElemType>
void Pool<ElemType>::clear()
{
    // If the emplaces/removes are not properly balanced, we cannot
    // call destructors. We do not keep track of which slots in a memory block
    // are currently used, at least not directly (it's the inverse of the free stack
    // entries, but that's not trivially usable for destructor calls in a performant way.
    JAWS_ASSUME(empty());
    _element_count = 0;
    _memory_blocks.clear();
    _free_stack.clear();
}
}

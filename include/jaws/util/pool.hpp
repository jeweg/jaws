#pragma once

#include <vector>
#include <memory>

namespace jaws::util {

template <typename ElemType>
class Pool
{
public:
    Pool(size_t min_alignment = 0);

    template <typename... Args>
    ElemType *emplace(Args &&... args);

    void remove(ElemType *);
    void clear();
    size_t size() const { return _element_count; }
    bool empty() const { return _element_count == 0; }

private:
    std::vector<ElemType *> _free_slots;
    std::vector<std::unique_ptr<ElemType[]>> _memory_pages;
    size_t _min_alignment = 0;
    size_t _element_count = 0;
};


template <typename ElemType>
Pool<ElemType>::Pool(size_t min_alignment) : _min_alignment(min_alignment)
{}


template <typename ElemType>
template <typename... Args>
ElemType *Pool<ElemType>::emplace(Args &&... args)
{
    if (_free_slots.empty()) {
        // Grow the vector by 50% its current size
        const size_t num_added = empty() ? 32 : _element_count / 2;
        //_memory_pages.push_back(std::make_unique<ElemType[]>(new ElemType[num_added]));
        _memory_pages.emplace_back(new ElemType[num_added]);

        // Add newly aquired slots to free list
        _free_slots.reserve(_free_slots.size() + num_added);
        ElemType *ptr = _memory_pages.back().get();
        for (size_t i = 0; i < num_added; ++i) { _free_slots.push_back(ptr++); }
    }
    ElemType *result_ptr = _free_slots.back();
    _free_slots.pop_back();
    new (result_ptr) ElemType(std::forward<Args>(args)...);
    ++_element_count;
    return result_ptr;
}


template <typename ElemType>
void Pool<ElemType>::remove(ElemType *ptr)
{
    ptr->~ElemType();
    _free_slots.push_back(ptr);
}


template <typename ElemType>
void Pool<ElemType>::clear()
{
    // Actually drop the reserved elements here.
    _memory_pages.swap(std::vector<std::unique_ptr<ElemType[]>>{});
    _free_slots.swap(std::vector<ElemType *>{});
    _element_count = 0;
}


}

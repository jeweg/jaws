#pragma once
#include <functional>

#include <iostream>

namespace jaws::util {

template <typename T>
ref_ptr<T>::ref_ptr(T *ptr) noexcept : _ptr(ptr)
{
    if (_ptr) { ref_ptr_add_ref(_ptr); }
}

template <typename T>
ref_ptr<T>::ref_ptr(std::nullptr_t) noexcept
{}

template <typename T>
ref_ptr<T>::ref_ptr(const ref_ptr &other) : _ptr(other._ptr)
{
    if (_ptr) { ref_ptr_add_ref(_ptr); }
}

template <typename T>
ref_ptr<T> &ref_ptr<T>::operator=(std::nullptr_t)
{
    reset();
    return *this;
}

template <typename T>
ref_ptr<T> &ref_ptr<T>::operator=(const ref_ptr &other)
{
    _ptr = other._ptr;
    if (_ptr) { ref_ptr_add_ref(_ptr); }
    return *this;
}

template <typename T>
ref_ptr<T>::~ref_ptr()
{
    if (_ptr) { ref_ptr_remove_ref(_ptr); }
}

template <typename T>
void ref_ptr<T>::reset()
{
    if (_ptr) { ref_ptr_remove_ref(_ptr); }
    _ptr = nullptr;
}

template <typename T>
void ref_ptr<T>::reset(T *ptr)
{
    if (_ptr) { ref_ptr_remove_ref(_ptr); }
    _ptr = ptr;
    if (_ptr) { ref_ptr_add_ref(_ptr); }
}

template <typename T>
T &ref_ptr<T>::operator*() const noexcept
{
    return *_ptr;
}

template <typename T>
T *ref_ptr<T>::operator->() const noexcept
{
    return _ptr;
}

template <typename T>
T *ref_ptr<T>::get() const noexcept
{
    return _ptr;
}

template <typename T>
ref_ptr<T>::operator bool() const noexcept
{
    return _ptr != nullptr;
}

template <typename T>
void ref_ptr<T>::swap(ref_ptr &other) noexcept
{
    std::swap(_ptr, other._ptr);
}

template <typename T>
bool operator==(const ref_ptr<T> &a, const ref_ptr<T> &b) noexcept
{
    return a.get() == b.get();
}

template <typename T>
bool operator!=(const ref_ptr<T> &a, const ref_ptr<T> &b) noexcept
{
    return a.get != b.get();
}

template <typename T>
bool operator<(const ref_ptr<T> &a, const ref_ptr<T> &b) noexcept
{
    return std::less<T *>()(a.get(), b.get());
}

template <typename T>
void swap(ref_ptr<T> &a, ref_ptr<T> &b) noexcept
{
    a.swap(b);
}

//-------------------------------------------------------------------------
// RefCounted

template <typename Derived>
size_t RefCounted<Derived>::add_ref()
{
    return _ref_count.fetch_add(1, std::memory_order_acq_rel) + 1;
}

template <typename Derived>
size_t RefCounted<Derived>::remove_ref()
{
    return _ref_count.fetch_sub(1, std::memory_order_acq_rel) - 1;
}

template <typename Derived>
size_t RefCounted<Derived>::get_ref_count() const noexcept
{
    return _ref_count.load(std::memory_order_acquire);
}

}

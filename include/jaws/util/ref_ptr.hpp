#pragma once

#include <cstddef>
#include <atomic>

/*
 * ref_ptr<T> is the handle (value type).
 * It calls overloads of ref_ptr_add_ref and ref_ptr_remove_ref for ref counting
 * and deletion.
 * RefCounted is a CRTP helper base that implements those and required plumbing in
 * a thread-safe way.
 * The code more or less reimplements boost::intrusive_ptr, notably when it comes to
 * the memory model details.
 */

namespace jaws::util {

template <typename T>
class ref_ptr
{
public:
    using ElementType = T;

    ref_ptr() noexcept = default;

    // Allows "return nullptr" instead of sth like "return {}".
    ref_ptr(std::nullptr_t) noexcept;

    explicit ref_ptr(T*) noexcept;

    ref_ptr(const ref_ptr&);
    ref_ptr(ref_ptr&&) noexcept = default;

    ref_ptr& operator=(std::nullptr_t);
    ref_ptr& operator=(const ref_ptr&);
    ref_ptr& operator=(ref_ptr&&) noexcept = default;
    ~ref_ptr();

    void reset();
    void reset(T*);

    T& operator*() const noexcept;
    T* operator->() const noexcept;
    T* get() const noexcept;

    operator bool() const noexcept;
    void swap(ref_ptr&) noexcept;

private:
    T* _ptr = nullptr;
};

template <typename T>
bool operator==(const ref_ptr<T>& a, const ref_ptr<T>& b) noexcept;
template <typename T>
bool operator!=(const ref_ptr<T>& a, const ref_ptr<T>& b) noexcept;
template <typename T>
bool operator<(const ref_ptr<T>& a, const ref_ptr<T>& b) noexcept;
template <typename T>
void swap(ref_ptr<T>& a, ref_ptr<T>& b) noexcept;


template <typename Derived>
class RefCounted
{
public:
    RefCounted() noexcept = default;
    RefCounted(const RefCounted&) = delete;
    RefCounted& operator=(const RefCounted&) = delete;

    RefCounted(RefCounted&& other)
    : _ref_count ( std::move(other._ref_count))
    {

    }
    RefCounted& operator=(RefCounted&& other)
    {
        _ref_count = std::move(other._ref_count);
        return *this;
    }

    [[nodiscard]] size_t get_ref_count() const noexcept;

protected:
    ~RefCounted() noexcept = default;

private:
    // Note: it's important to go through Derived and not RefCounted<Derived>.
    // We don't want to require virtual destructors or RTTI (dynamic_cast) for this.
    friend void ref_ptr_remove_ref(Derived* rc)
    {
        if (rc) {
            const size_t now = rc->remove_ref();
            if (now == 0) { delete rc; }
        }
    }

    friend void ref_ptr_add_ref(Derived* rc)
    {
        if (rc) { rc->add_ref(); }
    }

    size_t add_ref();
    size_t remove_ref();
    std::atomic_int_least32_t _ref_count = 0;
};

}; // namespace jaws::util

#include "ref_ptr.inl"

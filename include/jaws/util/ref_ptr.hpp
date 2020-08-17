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

    // Allows "return nullptr" instead of e.g. "return {}".
    ref_ptr(std::nullptr_t) noexcept;

    explicit ref_ptr(T *) noexcept;

    ref_ptr(const ref_ptr &);
    ref_ptr(ref_ptr &&) noexcept = default;

    ref_ptr &operator=(std::nullptr_t);
    ref_ptr &operator=(const ref_ptr &);
    ref_ptr &operator=(ref_ptr &&) noexcept = default;
    ~ref_ptr();

    void reset();
    void reset(T *);

    T &operator*() const noexcept;
    T *operator->() const noexcept;
    T *get() const noexcept;

    operator bool() const noexcept;
    void swap(ref_ptr &) noexcept;

private:
    T *_ptr = nullptr;
};

template <typename T>
bool operator==(const ref_ptr<T> &a, const ref_ptr<T> &b) noexcept;
template <typename T>
bool operator!=(const ref_ptr<T> &a, const ref_ptr<T> &b) noexcept;
template <typename T>
bool operator<(const ref_ptr<T> &a, const ref_ptr<T> &b) noexcept;
template <typename T>
void swap(ref_ptr<T> &a, ref_ptr<T> &b) noexcept;


template <typename Derived>
class RefCounted
{
public:
    RefCounted() noexcept = default;

protected:
    RefCounted(RefCounted &&other) : _ref_count(other._ref_count.load(std::memory_order_acquire)) {}
    RefCounted &operator=(RefCounted &&other) = delete;

    [[nodiscard]] size_t get_ref_count() const noexcept;

protected:
    ~RefCounted() noexcept = default;

    virtual void on_all_references_dropped() {}

private:
    // Note: it's important to go through Derived and not RefCounted<Derived>.
    // We don't want to require virtual destructors or RTTI (dynamic_cast) for this.
    friend void ref_ptr_remove_ref(Derived *rc)
    {
        if (rc) {
            const size_t now = rc->remove_ref();
            if (now == 0) { rc->on_all_references_dropped(); }
        }
    }

    friend void ref_ptr_add_ref(Derived *rc)
    {
        if (rc) { rc->add_ref(); }
    }

    size_t add_ref();
    size_t remove_ref();
    std::atomic_int_least32_t _ref_count = 0;
};


// TODO: embrace this to be a special case and use a custom deleter.
// We don't want to have simple shared ownership for our resource objects like shaders.
// That makes this more vulkan-subsystem-specific, though.

template <typename ResourceType>
class RefCountedHandle
{
public:
    RefCountedHandle(std::nullptr_t) noexcept : _resource_ptr(nullptr) {}
    RefCountedHandle &operator=(std::nullptr_t) { _resource_ptr = nullptr; }
    operator bool() const noexcept { return !!_resource_ptr; }

private:
    RefCountedHandle(ResourceType *r) noexcept : _resource_ptr(r) {}
    ref_ptr<ResourceType> _resource_ptr;
};


}; // namespace jaws::util

#include "ref_ptr.inl"

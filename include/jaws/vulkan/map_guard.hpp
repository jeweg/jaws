#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/util/misc.hpp"

namespace jaws::vulkan {

class MapGuard final : public jaws::util::NonCopyable
{
public:
    MapGuard(Device *device, VmaAllocation vma_allocation);
    ~MapGuard() noexcept;

    operator bool() const { return _ptr != nullptr; }

    void *data() { return _ptr; }

    template <typename T>
    T *data_as()
    {
        return static_cast<T *>(_ptr);
    }

private:
    VmaAllocator _vma_allocator = VK_NULL_HANDLE;
    VmaAllocation _vma_allocation = VK_NULL_HANDLE;
    void *_ptr = nullptr;
};

}

#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/ref_ptr.hpp"

namespace jaws::vulkan {

class Device;

class BufferResource : public jaws::util::RefCounted<BufferResource>
{
public:
    BufferResource() = default;
    BufferResource(Device *device, const VkBufferCreateInfo &ci, VmaMemoryUsage usage);
    ~BufferResource();

    Device *device = nullptr;
    VkBuffer handle = VK_NULL_HANDLE;
    VmaAllocation vma_allocation = VK_NULL_HANDLE;
    uint64_t id;
};


class Buffer : public jaws::util::RefCountedHandle<BufferResource>
{
public:
    VkBuffer get_vk_handle() const { return _resource_ptr ? _resource_ptr->handle : VK_NULL_HANDLE; }

private:
    friend class Device;
    Buffer(BufferResource *r) noexcept : jaws::util::RefCountedHandle<BufferResource>(r) {}
};

}

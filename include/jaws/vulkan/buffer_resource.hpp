#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/ref_ptr.hpp"
#include "jaws/util/pool.hpp"
#include "jaws/vulkan/map_guard.hpp"

namespace jaws::vulkan {

class Device;


class BufferResource final : public jaws::util::RefCounted<BufferResource>
{
public:
    BufferResource() = default;
    ~BufferResource();

    VkBuffer vk_handle() const { return _vk_buffer; }

    MapGuard map();

private:
    friend class jaws::util::Pool<BufferResource>;
    friend class jaws::util::detail::RefCountedAccessor;

    BufferResource(Device *device, VkBuffer, VmaAllocation);

    Device *_device = nullptr;
    VkBuffer _vk_buffer = VK_NULL_HANDLE;
    VmaAllocation _vma_allocation = VK_NULL_HANDLE;
};

}

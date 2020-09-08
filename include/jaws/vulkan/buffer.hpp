#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/misc.hpp"

namespace jaws::vulkan {

class Device;

class Buffer : jaws::util::MovableOnly
{
public:
    Buffer() = default;
    Buffer(Device *device, const VkBufferCreateInfo &ci, VmaMemoryUsage usage);
    ~Buffer();

    Device *device = nullptr;
    VkBuffer handle = VK_NULL_HANDLE;
    VmaAllocation vma_allocation = VK_NULL_HANDLE;
    uint64_t id;
};

}

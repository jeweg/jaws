#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/misc.hpp"

namespace jaws::vulkan {

class Device;

class Image : jaws::util::MovableOnly
{
public:
    Image() = default;
    Image(Device *device, const VkImageCreateInfo &ci, VmaMemoryUsage usage);
    ~Image();

    Device *device = nullptr;
    VkImage handle = VK_NULL_HANDLE;
    VmaAllocation vma_allocation = VK_NULL_HANDLE;
    uint64_t id;
};

}

#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vulkan/buffer.hpp"
#include "jaws/vulkan/device.hpp"

namespace jaws::vulkan {

BufferResource::BufferResource(Device *device, const VkBufferCreateInfo &ci, VmaMemoryUsage usage) : device(device)
{
    JAWS_ASSUME(device);
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = usage;
    VkResult result = vmaCreateBuffer(device->get_vma_allocator(), &ci, &alloc_info, &handle, &vma_allocation, nullptr);
    JAWS_VK_HANDLE_FATAL(result);
}


BufferResource::~BufferResource()
{
    vmaDestroyBuffer(device->get_vma_allocator(), handle, vma_allocation);
}

}

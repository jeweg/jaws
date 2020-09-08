#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vulkan/image.hpp"
#include "jaws/vulkan/device.hpp"

namespace jaws::vulkan {

Image::Image(Device *device, const VkImageCreateInfo &ci, VmaMemoryUsage usage) : device(device)
{
    JAWS_ASSUME(device);
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = usage;
    VkResult result = vmaCreateImage(device->get_vma_allocator(), &ci, &alloc_info, &handle, &vma_allocation, nullptr);
    JAWS_VK_HANDLE_FATAL(result);
}


Image::~Image()
{
    vmaDestroyImage(device->get_vma_allocator(), handle, vma_allocation);
}

}

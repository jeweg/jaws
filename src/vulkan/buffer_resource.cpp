#include "pch.hpp"
#include "jaws/vulkan/buffer_resource.hpp"
#include "jaws/vulkan/device.hpp"

namespace jaws::vulkan {

BufferResource::BufferResource(Device *device, VkBuffer vk_buffer, VmaAllocation vma_allocation) :
    _device(device), _vk_buffer(vk_buffer), _vma_allocation(vma_allocation)
{
    JAWS_ASSUME(device);
}


BufferResource::~BufferResource()
{
    // TODO: queue for deletion
}


MapGuard BufferResource::map()
{
    // Guaranteed copy elision.
    return MapGuard(_device, _vma_allocation);
}

}

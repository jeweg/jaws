#include "pch.hpp"
#include "jaws/vulkan/image_resource.hpp"
#include "jaws/vulkan/device.hpp"

namespace jaws::vulkan {

ImageResource::ImageResource(Device *device, VkImage vk_image, VmaAllocation vma_allocation) :
    _device(device), _vk_image(vk_image), _vma_allocation(vma_allocation)
{
    JAWS_ASSUME(device);
}


ImageResource::~ImageResource()
{
    // TODO: queue for deletion
}

}

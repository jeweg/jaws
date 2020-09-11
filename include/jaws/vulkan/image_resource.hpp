#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/ref_ptr.hpp"
#include "jaws/util/pool.hpp"

namespace jaws::vulkan {

class Device;

class ImageResource final : public jaws::util::RefCounted<ImageResource>
{
public:
    ImageResource() = default;
    ~ImageResource();

    VkImage vk_handle() const { return _vk_image; }

private:
    friend class jaws::util::detail::RefCountedAccessor;
    friend class jaws::util::Pool<ImageResource>;

    ImageResource(Device *device, VkImage, VmaAllocation);

    Device *_device = nullptr;
    VkImage _vk_image = VK_NULL_HANDLE;
    VmaAllocation _vma_allocation = VK_NULL_HANDLE;
};

}

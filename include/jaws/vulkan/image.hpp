#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/ref_ptr.hpp"

namespace jaws::vulkan {

class Device;

class ImageResource : public jaws::util::RefCounted<ImageResource>
{
public:
    ImageResource() = default;
    ImageResource(Device *device, const VkImageCreateInfo &ci, VmaMemoryUsage usage);
    ~ImageResource();

    Device *device = nullptr;
    VkImage handle = VK_NULL_HANDLE;
    VmaAllocation vma_allocation = VK_NULL_HANDLE;
    uint64_t id;
};


class Image : public jaws::util::RefCountedHandle<ImageResource>
{
public:
    VkImage get_vk_handle() const { return _resource_ptr ? _resource_ptr->handle : VK_NULL_HANDLE; }

private:
    friend class Device;
    Image(ImageResource *r) noexcept : jaws::util::RefCountedHandle<ImageResource>(r) {}
};

}

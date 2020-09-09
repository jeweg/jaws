#pragma once
#include "jaws/core.hpp"
#include "jaws/vfs/path.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/buffer.hpp"
#include "jaws/vulkan/image.hpp"
#include "jaws/vulkan/framebuffer.hpp"
#include "jaws/vulkan/extension.hpp"
#include "jaws/util/misc.hpp"
#include "jaws/util/hashing.hpp"
#include "jaws/util/ref_ptr.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/util/pool.hpp"

#include <vector>

namespace jaws::vulkan {

class JAWS_API Device
{
public:
    struct CreateInfo
    {
        uint32_t gpu_group_index = 0;
        ExtensionList required_extensions;
        ExtensionList optional_extensions;

        CreateInfo &set_gpu_group_index(uint32_t v)
        {
            gpu_group_index = v;
            return *this;
        }
        CreateInfo &set_required_extensions(ExtensionList v)
        {
            required_extensions = std::move(v);
            return *this;
        }
        CreateInfo &set_optional_extensions(ExtensionList v)
        {
            optional_extensions = std::move(v);
            return *this;
        }
    };

    Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
    ~Device();

    void create(Context *, const CreateInfo & = jaws::util::make_default<CreateInfo>());
    void destroy();

    void wait_idle();

    VkInstance get_instance() const { return _vk_instance; }

    Context *get_context() const { return _context; }
    VkDevice vk_handle() const { return _vk_device; }

    VkPhysicalDevice get_physical_device(uint32_t index = 0) const;

    enum class Queue
    {
        Graphics = 0,
        Present,
        Compute,
        Transfer,
        AsyncTransfer,
        AsyncCompute,
        ELEM_COUNT
    };
    VkQueue get_queue(Queue q);
    uint32_t get_queue_family(Queue q) const;

    Shader get_shader(const ShaderCreateInfo &);

    const VolkDeviceTable &vk_funcs() const;

    const ExtensionList &get_extensions() const;
    // Sediment* get_sediment() { return _sediment.get(); }

    // TODO: later on decide how to handle capability flags less ad-hoc.
    bool supports_validation_cache() const;

    //----------------------------------------------------------------
    // Low-level resource creation. Uses vulkan-memory-allocator.
    // For now expose all of VmaMemoryUsage. Later on I'd like to consolidate this
    // a little, abstract the best way to transfer to GPU for a particular device, etc.

    BufferPool::Id create_buffer(const VkBufferCreateInfo &ci, VmaMemoryUsage usage);
    Buffer *get_buffer(BufferPool::Id);

    ImagePool::Id create_image(const VkImageCreateInfo &ci, VmaMemoryUsage usage);
    Image *get_image(ImagePool::Id);

    void *map_buffer(BufferPool::Id);
    void unmap_buffer(BufferPool::Id);

    VmaAllocator get_vma_allocator() const { return _vma_allocator; }

    VkFramebuffer get_framebuffer(const FramebufferCreateInfo &);


private:
    friend Context;
    Context *_context = nullptr;

    VkInstance _vk_instance = VK_NULL_HANDLE;
    VkPhysicalDeviceGroupProperties _gpu_group;
    VkDevice _vk_device = VK_NULL_HANDLE;

    VolkDeviceTable _f;

    ExtensionList _extensions;
    bool _has_cap_validation_cache = false;

    VmaVulkanFunctions _vma_vulkan_functions;
    VmaAllocator _vma_allocator = VK_NULL_HANDLE;

    std::vector<VkQueue> _unique_queues;
    struct QueueInfo
    {
        uint32_t family_index = INVALID_INDEX;
        size_t unique_queue_index = 0;
    };
    std::array<QueueInfo, static_cast<size_t>(Queue::ELEM_COUNT)> _queue_infos;
    QueueInfo &get_queue_info(Queue q);
    const QueueInfo &get_queue_info(Queue q) const;

    // std::unique_ptr<Sediment> _sediment;
    std::unique_ptr<ShaderSystem> _shader_system;

    BufferPool _buffer_pool;
    ImagePool _image_pool;

    std::unique_ptr<FramebufferCache> _framebuffer_cache;
};


inline VkPhysicalDevice Device::get_physical_device(uint32_t index) const
{
    JAWS_ASSUME(index < _gpu_group.physicalDeviceCount);
    return _gpu_group.physicalDevices[index];
}


inline VkQueue Device::get_queue(Queue q)
{
    return _unique_queues[get_queue_info(q).unique_queue_index];
}


inline uint32_t Device::get_queue_family(Queue q) const
{
    return get_queue_info(q).family_index;
}


inline Device::QueueInfo &Device::get_queue_info(Queue q)
{
    JAWS_ASSUME(q != Queue::ELEM_COUNT);
    return _queue_infos[static_cast<size_t>(q)];
}


inline const Device::QueueInfo &Device::get_queue_info(Queue q) const
{
    JAWS_ASSUME(q != Queue::ELEM_COUNT);
    return _queue_infos[static_cast<size_t>(q)];
}


inline const VolkDeviceTable &Device::vk_funcs() const
{
    return _f;
};


inline const ExtensionList &Device::get_extensions() const
{
    return _extensions;
}


inline bool Device::supports_validation_cache() const
{
    return _has_cap_validation_cache;
}

}

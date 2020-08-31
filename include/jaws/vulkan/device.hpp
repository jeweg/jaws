#pragma once
#include "jaws/core.hpp"
#include "jaws/vfs/path.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/buffer.hpp"
#include "jaws/vulkan/image.hpp"
#include "jaws/vulkan/extension.hpp"
#include "jaws/util/misc.hpp"
#include "jaws/util/hashing.hpp"
#include "jaws/util/ref_ptr.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/util/pool.hpp"

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
    VkDevice get_device() const { return _device; }

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

    const VolkDeviceTable &vk() const;

    const ExtensionList &get_extensions() const;
    // Sediment* get_sediment() { return _sediment.get(); }

    // TODO: later on decide how to handle capability flags less ad-hoc.
    bool supports_validation_cache() const;

    //----------------------------------------------------------------
    // Low-level resource creation. Uses vulkan-memory-allocator.
    // For now expose all of VmaMemoryUsage. Later on I'd like to consolidate this
    // a little, abstract the best way to transfer to GPU for a particular device, etc.

    Image create_image(const VkImageCreateInfo &, VmaMemoryUsage usage);
    Buffer create_buffer(const VkBufferCreateInfo &, VmaMemoryUsage usage);

    //----------------------------------------------------------------

private:
    friend class BufferResource;
    friend class ImageResource;

    VmaAllocator get_vma_allocator() const { return _vma_allocator; }


private:
    friend Context;
    Context *_context = nullptr;

    VkInstance _vk_instance;
    VkPhysicalDeviceGroupProperties _gpu_group;
    VkDevice _device = VK_NULL_HANDLE;

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


    jaws::util::Pool<ImageResource> _image_pool;
    jaws::util::Pool<BufferResource> _buffer_pool;
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


inline const VolkDeviceTable &Device::vk() const
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

#include "pch.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/util/enumerate_range.hpp"
#include "jaws/vulkan/shader_system.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "framebuffer_cache.hpp"

namespace jaws::vulkan {

Device::Device() = default;

Device::~Device()
{
    destroy();
}


void Device::create(Context *context, const CreateInfo &ci)
{
    VkResult result;
    auto &logger = get_logger(Category::Vulkan);

    JAWS_ASSUME(context);

    _context = context;
    _vk_instance = context->get_instance();

    //=========================================================================
    // Physical device group

    auto gpu_groups = enumerated<VkPhysicalDeviceGroupProperties>(vkEnumeratePhysicalDeviceGroups, {}, _vk_instance);

    if (ci.gpu_group_index < 0 || ci.gpu_group_index >= gpu_groups.size()) { JAWS_FATAL1("no such gpu group index"); }

    _gpu_group = gpu_groups[ci.gpu_group_index];

    JAWS_ASSUME(_gpu_group.physicalDeviceCount > 0);
    JAWS_ASSUME(_gpu_group.physicalDevices);

    // We chose the 1st phys device in the group for things like extension querying.
    // Must hope any others support the same.
    // TODO: maybe sanity check for that, or even add logic to use their lowest common denominator.

    VkPhysicalDevice physical_device = _gpu_group.physicalDevices[0];

    //=========================================================================
    // Handle extensions

    ExtensionList wanted_extensions;
    bool vma_can_use_dedicated_alloc = false;
    {
        auto optional_extensions = ci.optional_extensions;
        auto required_extensions = ci.required_extensions;

        // Extensions required for vulkan-memory-allocators's dedicated alloc feature.
        const ExtensionList dedicated_alloc_extensions(
            {"VK_KHR_get_memory_requirements2", "VK_KHR_dedicated_allocation"});
        optional_extensions.add(dedicated_alloc_extensions);

        // Validation cache
        optional_extensions.add("VK_EXT_validation_cache");

        ExtensionList avail_extensions;
        for (const auto &props :
             enumerated(vkEnumerateDeviceExtensionProperties, VkExtensionProperties{}, physical_device, nullptr)) {
            avail_extensions.add(Extension(props.extensionName, props.specVersion));
        }

        std::string err;
        wanted_extensions = ExtensionList::resolve(avail_extensions, required_extensions, optional_extensions, &err);
        if (!err.empty()) { JAWS_FATAL1(err); }

        // TODO: wanted_extensions is that we pass to vkCreateDevice.
        // Is this guaranteed to either get us the extensions or fail (I suspect it is)?
        // If not, we will have to query the actually active extensions after device creation.
        _extensions = wanted_extensions;

        vma_can_use_dedicated_alloc = wanted_extensions.contains(dedicated_alloc_extensions);
    }

    //=========================================================================
    // Choose queue families for the various types of queues.
    /*
    The logic here:
    -1. Choose async-compute: find a family only supports compute. if that fails, choose one that doesn't support
    graphics and supports compute. If that fails, use the first family that supports compute. 0. Choose transfer: find a
    family only supports transfer. if that fails, choose one that doesn't support graphics and supports transfer. If
    that fails, use the first family that supports transfer.
    1. Choose present, graphics, computeg: find a family that supports graphics, present and compute for all three. If
    that fails: find a family that support graphics and present and use it for those two. If that fails: find a family
    that supports graphics and compute and use it for those two. After that, for any of those three that aren't chosen
    yet: use the first family that supports each  type.

    In other words:
    - transfer and async-compute want to be as exclusive as possible
    - graphics, present, computeg want to be close.

    The common thing here is a query that tests for some bits set while some bits should not be set.
    */

    // logger.info("queue family properties: {}", queue_family_properties_to_string(*this));

    VkQueueFamilyProperties2 queue_fam_elem = {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};
    auto all_family_props = jaws::vulkan::enumerated_void<VkQueueFamilyProperties2>(
        vkGetPhysicalDeviceQueueFamilyProperties2, queue_fam_elem, physical_device);

    // First transform into a form that's more easily digested than the properties interface.
    struct FamilyBit
    {
        enum : uint32_t
        {
            Graphics = 1,
            Compute = 2,
            Transfer = 4,
            Present = 8,
        };
    };
    std::vector<int> family_bits;
    family_bits.resize(all_family_props.size());

    for (const auto &[i, family_props] : jaws::util::enumerate_range(all_family_props)) {
        int fb = 0;
        // Note that graphics or compute capability implies transfer capability.
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            fb |= FamilyBit::Graphics;
            fb |= FamilyBit::Transfer;
        }
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            fb |= FamilyBit::Compute;
            fb |= FamilyBit::Transfer;
        }
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) { fb |= FamilyBit::Transfer; }

        if (!_context->is_headless()) {
            if (_context->get_presentation_support_callback()(_vk_instance, physical_device, i)) {
                fb |= FamilyBit::Present;
            }
        }
        family_bits[i] = fb;
    }

    // Returns a queue family index where some capabilities (mask_of_set_bits) must be present and
    // other capabilities (mask_of_unset_bits) must be absent.
    auto find_family = [&](uint32_t wanted_caps, uint32_t unwanted_caps) -> uint32_t {
        // In headless mode, we transparently ignore the present capability by modifying
        // the bit mask. It becomes neither wanted nor unwanted.
        if (_context->is_headless()) {
            wanted_caps &= ~FamilyBit::Present;
            unwanted_caps &= ~FamilyBit::Present;
        }
        for (auto [i, bits] : jaws::util::enumerate_range(family_bits)) {
            if ((bits & wanted_caps) != wanted_caps) { continue; }
            if ((~bits & unwanted_caps) != unwanted_caps) { continue; }
            return i;
        }
        return INVALID_INDEX;
    };
    auto is_queue_invalid = [this](Queue q) { return get_queue_info(q).family_index == INVALID_INDEX; };

    // Find async compute queue family, as exclusive as possible.
    get_queue_info(Queue::AsyncCompute).family_index =
        find_family(FamilyBit::Compute, FamilyBit::Graphics | FamilyBit::Transfer | FamilyBit::Present);
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index =
            find_family(FamilyBit::Compute, FamilyBit::Graphics | FamilyBit::Transfer);
    }
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index = find_family(FamilyBit::Compute, FamilyBit::Graphics);
    }
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index = find_family(FamilyBit::Compute, 0);
    }

    // Find async transfer queue family, as exclusive as possible.
    get_queue_info(Queue::AsyncTransfer).family_index =
        find_family(FamilyBit::Transfer, FamilyBit::Graphics | FamilyBit::Compute | FamilyBit::Present);
    if (is_queue_invalid(Queue::AsyncTransfer)) {
        get_queue_info(Queue::AsyncTransfer).family_index =
            find_family(FamilyBit::Transfer, FamilyBit::Graphics | FamilyBit::Compute);
    }
    if (is_queue_invalid(Queue::AsyncTransfer)) {
        get_queue_info(Queue::AsyncTransfer).family_index = find_family(FamilyBit::Transfer, FamilyBit::Graphics);
    }
    if (is_queue_invalid(Queue::AsyncTransfer)) {
        get_queue_info(Queue::AsyncTransfer).family_index = find_family(FamilyBit::Transfer, 0);
    }

    // Try to place graphics, present, compute, transfer on the same family.
    // (graphics/compute implies transfer availability, but let's ignore that, that is handled by
    // the family bits and will fall out of this just the same.
    uint32_t fam_index;
    if ((fam_index =
             find_family(FamilyBit::Graphics | FamilyBit::Compute | FamilyBit::Present | FamilyBit::Transfer, 0))
        != INVALID_INDEX) {
        get_queue_info(Queue::Graphics).family_index = fam_index;
        get_queue_info(Queue::Compute).family_index = fam_index;
        get_queue_info(Queue::Present).family_index = fam_index;
        get_queue_info(Queue::Transfer).family_index = fam_index;
    } else if (
        (fam_index = find_family(FamilyBit::Graphics | FamilyBit::Present | FamilyBit::Transfer, 0)) != INVALID_INDEX) {
        get_queue_info(Queue::Graphics).family_index = fam_index;
        get_queue_info(Queue::Present).family_index = fam_index;
        get_queue_info(Queue::Transfer).family_index = fam_index;
    } else if (
        (fam_index = find_family(FamilyBit::Graphics | FamilyBit::Compute | FamilyBit::Transfer, 0)) != INVALID_INDEX) {
        get_queue_info(Queue::Graphics).family_index = fam_index;
        get_queue_info(Queue::Compute).family_index = fam_index;
        get_queue_info(Queue::Transfer).family_index = fam_index;
    }

    // Any leftovers use their first available queue.
    if (is_queue_invalid(Queue::Graphics)) {
        get_queue_info(Queue::Graphics).family_index = find_family(FamilyBit::Graphics, 0);
    }
    if (is_queue_invalid(Queue::Present)) {
        get_queue_info(Queue::Present).family_index = find_family(FamilyBit::Present, 0);
    }
    if (is_queue_invalid(Queue::Compute)) {
        get_queue_info(Queue::Compute).family_index = find_family(FamilyBit::Compute, 0);
    }
    if (is_queue_invalid(Queue::Transfer)) {
        get_queue_info(Queue::Transfer).family_index = find_family(FamilyBit::Transfer, 0);
    }

    if (is_queue_invalid(Queue::Graphics)) { JAWS_FATAL1("No graphics queue family found"); }
    if (!_context->is_headless()) {
        if (get_queue_info(Queue::Present).family_index == INVALID_INDEX) {
            JAWS_FATAL1("No present queue family found");
        }
    }
    if (is_queue_invalid(Queue::Compute)) { JAWS_FATAL1("No compute queue family found"); }
    if (is_queue_invalid(Queue::Transfer)) { JAWS_FATAL1("No transfer queue family found"); }

    // Leftover async ones fall back to their non-async siblings.
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index = get_queue_info(Queue::Compute).family_index;
    }
    if (is_queue_invalid(Queue::AsyncTransfer)) {
        get_queue_info(Queue::AsyncTransfer).family_index = get_queue_info(Queue::Transfer).family_index;
    }


    logger.info("selected queue families:");
    logger.info("    graphics: {}", get_queue_info(Queue::Graphics).family_index);
    if (!_context->is_headless()) { logger.info("    present: {}", get_queue_info(Queue::Present).family_index); }
    logger.info("    compute: {}", get_queue_info(Queue::Compute).family_index);
    logger.info("    transfer: {}", get_queue_info(Queue::Transfer).family_index);
    logger.info("    async compute: {}", get_queue_info(Queue::AsyncCompute).family_index);
    logger.info("    async transfer: {}", get_queue_info(Queue::AsyncTransfer).family_index);

    //=========================================================================
    // Create logical device

    std::vector<VkDeviceQueueCreateInfo> queue_cis;
    {
        // Build the queue create infos -- one ci for one queue per queue family.
        // I don't know of any advantages of using more than one queue per family.
        auto add_queue_info = [&](uint32_t family_index) {
            VkDeviceQueueCreateInfo queue_ci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
            queue_ci.queueCount = 1;
            static const float queue_prio = 0.5f;
            queue_ci.pQueuePriorities = &queue_prio;
            queue_ci.queueFamilyIndex = family_index;
            queue_cis.push_back(queue_ci);
        };

        for (auto &qi : _queue_infos) {
            // family already requested?
            bool family_handled = false;
            for (auto const &ci : queue_cis) {
                if (ci.queueFamilyIndex == qi.family_index) {
                    family_handled = true;
                    break;
                }
            }
            if (!family_handled) { add_queue_info(qi.family_index); }
        }

        auto wanted_extensions_ptrs = wanted_extensions.as_char_ptrs();

        VkDeviceCreateInfo device_ci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_ci.queueCreateInfoCount = static_cast<uint32_t>(queue_cis.size());
        device_ci.pQueueCreateInfos = queue_cis.data();
        device_ci.enabledExtensionCount = static_cast<uint32_t>(wanted_extensions_ptrs.size());
        device_ci.ppEnabledExtensionNames = wanted_extensions_ptrs.data();

        // Setting layers on the device level is deprecated, yet vulkan-tutorial.com
        // recommends still setting it for backwards compatibility.
        // We'll follow this here and set them to the instance layers.
        auto layers_ptrs = context->get_instance_layers().as_char_ptrs();
        device_ci.enabledLayerCount = static_cast<uint32_t>(layers_ptrs.size());
        device_ci.ppEnabledLayerNames = layers_ptrs.data();

        // https://github.com/KhronosGroup/Vulkan-Docs/blob/master/appendices/VK_KHR_device_group_creation.txt#L81
        // If the first device group has more than one physical device. create
        // a logical device using all of the physical devices.
        VkDeviceGroupDeviceCreateInfoKHR device_group_ci = {VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO_KHR};
        // if (_gpu_group.physicalDeviceCount > 1) {
        device_group_ci.physicalDeviceCount = _gpu_group.physicalDeviceCount;
        device_group_ci.pPhysicalDevices = _gpu_group.physicalDevices;
        device_ci.pNext = &device_group_ci;
        //}

        result = vkCreateDevice(physical_device, &device_ci, nullptr, &_vk_device);
        JAWS_VK_HANDLE_FATAL(result);

        _has_cap_validation_cache = _extensions.contains("VK_EXT_validation_cache");
    }
    logger.info("device: {}", (void *)_vk_device);

    //=========================================================================

    volkLoadDeviceTable(&_f, _vk_device);

    //=========================================================================
    // Retrieve queue objects. We need them later.

    _unique_queues.assign(queue_cis.size(), VK_NULL_HANDLE);
    for (const auto &[i, qci] : jaws::util::enumerate_range(queue_cis)) {
        _f.vkGetDeviceQueue(_vk_device, qci.queueFamilyIndex, 0, &_unique_queues[i]);
        // For each queue info using that family, set the unique queue index.
        for (auto &qi : _queue_infos) {
            if (qi.family_index == qci.queueFamilyIndex) { qi.unique_queue_index = i; }
        }
    }

    //=========================================================================
    // Init vulkan memory allocator

    _vma_vulkan_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    _vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;

    _vma_vulkan_functions.vkAllocateMemory = _f.vkAllocateMemory;
    _vma_vulkan_functions.vkAllocateMemory = _f.vkAllocateMemory;
    _vma_vulkan_functions.vkFreeMemory = _f.vkFreeMemory;
    _vma_vulkan_functions.vkMapMemory = _f.vkMapMemory;
    _vma_vulkan_functions.vkUnmapMemory = _f.vkUnmapMemory;
    _vma_vulkan_functions.vkFlushMappedMemoryRanges = _f.vkFlushMappedMemoryRanges;
    _vma_vulkan_functions.vkInvalidateMappedMemoryRanges = _f.vkInvalidateMappedMemoryRanges;
    _vma_vulkan_functions.vkBindBufferMemory = _f.vkBindBufferMemory;
    _vma_vulkan_functions.vkBindImageMemory = _f.vkBindImageMemory;
    _vma_vulkan_functions.vkGetBufferMemoryRequirements = _f.vkGetBufferMemoryRequirements;
    _vma_vulkan_functions.vkGetImageMemoryRequirements = _f.vkGetImageMemoryRequirements;
    _vma_vulkan_functions.vkCreateBuffer = _f.vkCreateBuffer;
    _vma_vulkan_functions.vkDestroyBuffer = _f.vkDestroyBuffer;
    _vma_vulkan_functions.vkCreateImage = _f.vkCreateImage;
    _vma_vulkan_functions.vkDestroyImage = _f.vkDestroyImage;
    _vma_vulkan_functions.vkCmdCopyBuffer = _f.vkCmdCopyBuffer;
#if VMA_DEDICATED_ALLOCATION
    _vma_vulkan_functions.vkGetBufferMemoryRequirements2KHR = _f.vkGetBufferMemoryRequirements2KHR;
    _vma_vulkan_functions.vkGetImageMemoryRequirements2KHR = _f.vkGetImageMemoryRequirements2KHR;
#endif
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = _vk_device;
    allocator_info.pVulkanFunctions = &_vma_vulkan_functions;
    if (vma_can_use_dedicated_alloc) { allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT; }
    vmaCreateAllocator(&allocator_info, &_vma_allocator);

#if 0
    // Let's test this.
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = 65536;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer buffer;
    VmaAllocation allocation;
    result = vmaCreateBuffer(_vma_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
    JAWS_VK_HANDLE_FATAL(result);

    vmaDestroyBuffer(_vma_allocator, buffer, allocation);
#endif

    /*
    _sediment = std::make_unique<Sediment>();
    _sediment->create(this);
    */
}


void Device::destroy()
{
    if (_vk_device) {
        wait_idle();
        _buffer_pool.clear();
        _image_pool.clear();
        vmaDestroyAllocator(_vma_allocator);
        _f.vkDestroyDevice(_vk_device, nullptr);
        _vk_device = VK_NULL_HANDLE;
    }
}


void Device::wait_idle()
{
    JAWS_ASSUME(_vk_device);
    JAWS_VK_HANDLE_FATAL(vkDeviceWaitIdle(_vk_device));
}


Shader Device::get_shader(const ShaderCreateInfo &ci)
{
    if (!_shader_system) {
        _shader_system = std::make_unique<ShaderSystem>();
        _shader_system->create(this);
    }
    return _shader_system->get_shader(ci);
}


BufferPool::Id Device::create_buffer(const VkBufferCreateInfo &ci, VmaMemoryUsage usage)
{
    return _buffer_pool.emplace(this, ci, usage);
}


ImagePool::Id Device::create_image(const VkImageCreateInfo &ci, VmaMemoryUsage usage)
{
    return _image_pool.emplace(this, ci, usage);
}


Buffer *Device::get_buffer(BufferPool::Id id)
{
    return _buffer_pool.lookup(id);
}


Image *Device::get_image(ImagePool::Id id)
{
    return _image_pool.lookup(id);
}


void *Device::map_buffer(BufferPool::Id id)
{
    Buffer *b = get_buffer(id);
    JAWS_ASSUME(b);
    void *ptr = nullptr;
    vmaMapMemory(_vma_allocator, b->vma_allocation, &ptr);
    return ptr;
}


void Device::unmap_buffer(BufferPool::Id id)
{
    Buffer *b = get_buffer(id);
    JAWS_ASSUME(b);
    vmaUnmapMemory(_vma_allocator, b->vma_allocation);
}


VkFramebuffer Device::get_framebuffer(const FramebufferCreateInfo &ci)
{
    if (!_framebuffer_cache) {
        _framebuffer_cache = std::make_unique<FramebufferCache>();
        _framebuffer_cache->create(this);
    }
    return _framebuffer_cache->get_framebuffer(ci);
}

}

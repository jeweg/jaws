#include "pch.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/util/enumerate_range.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/utils.hpp"
#include "jaws/vulkan/shader_system.hpp"
#include "jaws/assume.hpp"
#include "jaws/logging.hpp"
#include "jaws/fatal.hpp"
#include "to_string.hpp"
#include <vector>


static VkBool32 VKAPI_PTR vulkan_debug_utils_messenger_cb(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
    spdlog::level::level_enum log_level = spdlog::level::info;
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        log_level = spdlog::level::err;
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        log_level = spdlog::level::warn;
    } else if (message_severity & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        log_level = spdlog::level::info;
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        log_level = spdlog::level::debug;
    }
    if (callback_data) {
        auto &logger = jaws::get_logger(jaws::Category::VulkanValidation);
        logger.log(log_level, callback_data->pMessage);
    }
    return VK_FALSE;
}


namespace jaws::vulkan {

Context::Context() = default;

Context::~Context()
{
    destroy();
}


void Context::create(const Context::CreateInfo &ci)
{
    VkResult result;
    auto &logger = jaws::get_logger(Category::Vulkan);

    if (!ci.vkGetInstanceProcAddr) {
        result = volkInitialize();
    } else {
        volkInitializeCustom(ci.vkGetInstanceProcAddr);
    }

    if (ci.presentation_support_callback) {
        // We do this even in headless mode because the capability output
        // uses it if set. And the caps don't change with the current headless mode.
        _presentation_support_callback = ci.presentation_support_callback;
    }

    if (!ci.headless && !ci.presentation_support_callback) {
        JAWS_FATAL1("non-headless currently requires a set presentation support callback!");
    }

    _headless = ci.headless;

    logger.info("instance properties: {}", instance_properties_to_string());

    //=========================================================================
    // Handle layers

    ExtensionList wanted_layers;
    {
        auto optional_layers = ci.optional_layers;
        auto required_layers = ci.required_layers;

        // Add defaults
        if (ci.debugging) { optional_layers.add("VK_LAYER_KHRONOS_validation"); }

        ExtensionList avail_layers;
        for (const auto &layer : enumerated<VkLayerProperties>(vkEnumerateInstanceLayerProperties, {})) {
            avail_layers.add(Extension(layer.layerName, layer.specVersion));
        }

        logger.info("avail_layers: {}", avail_layers.to_string(true));
        logger.info("required_layers: {}", required_layers.to_string(true));
        logger.info("optional_layers: {}", optional_layers.to_string(true));

        std::string err;
        _instance_extensions = ExtensionList::resolve(avail_layers, required_layers, optional_layers, &err);
        if (!err.empty()) { JAWS_FATAL1(err); }

        logger.info("selecting layers: {}", _instance_extensions.to_string(true));
    }

    //=========================================================================
    // Handle extensions

    {
        auto optional_extensions = ci.optional_instance_extensions;
        auto required_extensions = ci.required_instance_extensions;

        // Add defaults
        if (!ci.headless) {
            required_extensions.add("VK_KHR_surface");
#if defined(JAWS_OS_WIN)
            required_extensions.add("VK_KHR_win32_surface");
#elif defined(JAWS_OS_LINUX)
            // How do we choose between xcb and xlib? Does glfw require one?
            required_extensions.add("VK_KHR_xcb_surface");
            // extensions_ptrs.push_back("VK_KHR_xlib_surface");
#else
#    error Unsupported platform!
#endif
        }
        if (ci.debugging) { optional_extensions.add("VK_EXT_debug_utils"); }
        required_extensions.add("VK_KHR_get_surface_capabilities2");

        ExtensionList avail_extensions;
        for (const auto &e : enumerated<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, {}, nullptr)) {
            avail_extensions.add(Extension(e.extensionName, e.specVersion));
        }

        logger.info("avail_extensions: {}", avail_extensions.to_string(true));
        logger.info("required_extensions: {}", required_extensions.to_string(true));
        logger.info("optional_extensions: {}", optional_extensions.to_string(true));

        std::string err;
        _instance_extensions = ExtensionList::resolve(avail_extensions, required_extensions, optional_extensions, &err);
        if (!err.empty()) { JAWS_FATAL1(err); }
        logger.info("selecting extensions: {}", _instance_extensions.to_string(true));
    }

    //=========================================================================
    // Instance
    {
        VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
        app_info.pApplicationName = ci.app_name;
        app_info.applicationVersion = VK_MAKE_VERSION(ci.app_version_major, ci.app_version_minor, ci.app_version_patch);
        app_info.pEngineName = "jaws";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = volkGetInstanceVersion();

        auto wanted_extensions_ptrs = _instance_extensions.as_char_ptrs();
        auto wanted_layers_ptrs = _instance_layers.as_char_ptrs();

        VkInstanceCreateInfo instance_ci = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        instance_ci.pApplicationInfo = &app_info;
        instance_ci.enabledExtensionCount = static_cast<uint32_t>(wanted_extensions_ptrs.size());
        instance_ci.ppEnabledExtensionNames = wanted_extensions_ptrs.data();
        instance_ci.enabledLayerCount = static_cast<uint32_t>(wanted_layers_ptrs.size());
        instance_ci.ppEnabledLayerNames = wanted_layers_ptrs.data();

        result = vkCreateInstance(&instance_ci, nullptr, &_vk_instance);
        JAWS_VK_HANDLE_FATAL(result);
        volkLoadInstance(_vk_instance);

        logger.info("instance: {}", (void *)_vk_instance);

        //-------------------------------------------------------------------------
        // Debug reporting
        VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = {
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        debug_messenger_ci.pfnUserCallback = &vulkan_debug_utils_messenger_cb;
        result = vkCreateDebugUtilsMessengerEXT(_vk_instance, &debug_messenger_ci, nullptr, &_debug_messenger);
        JAWS_VK_HANDLE_FATAL(result);

        //=========================================================================
        // Enumerate physical devices

        {
            for (auto pd : enumerated<VkPhysicalDevice>(vkEnumeratePhysicalDevices, {}, _vk_instance)) {
                VkPhysicalDeviceProperties props;
                vkGetPhysicalDeviceProperties(pd, &props);
            }

            /*
            enumerated
            uint32_t count;
            do {
                result = vkEnumeratePhysicalDevices(_vk_instance, &count, nullptr);
                JAWS_VK_HANDLE_FATAL(result);
                if (result == VK_SUCCESS && count > 0) {
                    _physical_devices.resize(count);
                    result = vkEnumeratePhysicalDevices(_vk_instance, &count, _physical_devices.data());
                    JAWS_VK_HANDLE_FATAL(result);
                }
            } while (result == VK_INCOMPLETE);
            if (_physical_devices.empty()) { JAWS_FATAL1("No vulkan-capable physical_devices found"); }
             */
        }
        // logger.info("physical device: {}", (void *) _physical_device);
    }
#if 0

    //=========================================================================
    // Handle device extensions
    // this is still pretty murky.

    ExtensionList avail_device_extensions;
    for (const auto &props : enumerated(vkEnumerateDeviceExtensionProperties, VkExtensionProperties{}, _physical_device, nullptr)) {
        avail_device_extensions.emplace_back(props.extensionName, props.specVersion);
    }

    const ExtensionList dedicated_alloc_extensions = {
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_dedicated_allocation"
    };
    ExtensionList optional_device_extensions;
    optional_device_extensions.insert(optional_device_extensions.end(),
        dedicated_alloc_extensions.begin(), dedicated_alloc_extensions.end());

    ExtensionList required_device_extensions = {};
    if (!ci.headless) {
        required_device_extensions.push_back(Extension(std::string(VK_KHR_SWAPCHAIN_EXTENSION_NAME), 1));
    }

    ExtensionList requested_device_extensions = {};
    for (const auto &ext : required_device_extensions) {
        if (!find(avail_device_extensions, ext)) {
            // TODO: not fatal.
            JAWS_FATAL1("required device extension not available");
        }
        requested_device_extensions.push_back(ext);
    }
    for (const auto &ext : optional_device_extensions) {
        if (!find(avail_device_extensions, ext)) {
            logger.warn("optional device extension not available: {}", ext.name);
            continue;
        }
        requested_device_extensions.push_back(ext);
    }

    const bool vma_can_use_dedicated_alloc = contains(requested_device_extensions, dedicated_alloc_extensions);
    logger.info("vma can use dedicated allocation: {}", vma_can_use_dedicated_alloc);

    //=========================================================================
    // Choose queue families for the various types of queues.
    /*
    The logic here:
    -1. Choose async-compute: find a family only supports compute. if that fails, choose one that doesn't support graphics and supports compute. If that fails, use the first family that supports compute.
    0. Choose transfer: find a family only supports transfer. if that fails, choose one that doesn't support graphics and supports transfer. If that fails, use the first family that supports transfer.
    1. Choose present, graphics, computeg: find a family that supports graphics, present and compute for all three. If that fails: find a family that support graphics and present and use it for those two.
      If that fails: find a family that supports graphics and compute and use it for those two.
      After that, for any of those three that aren't chosen yet: use the first family that supports each  type.

    In other words:
    - transfer and async-compute want to be as exclusive as possible
    - graphics, present, computeg want to be close.

    The common thing here is a query that tests for some bits set while some bits should not be set.
    */

    logger.info("queue family properties: {}", queue_family_properties_to_string(*this));

    VkQueueFamilyProperties2 queue_fam_elem = { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 };
    auto all_family_props = jaws::vulkan::enumerated_void<VkQueueFamilyProperties2>(vkGetPhysicalDeviceQueueFamilyProperties2, queue_fam_elem, _physical_device);

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

    for (const auto&[i, family_props] : jaws::util::enumerate_range(all_family_props)) {
        int fb = 0;
        // Note that graphics or compute capability implies transfer capability.
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) { fb |= FamilyBit::Graphics; fb |= FamilyBit::Transfer; }
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) { fb |= FamilyBit::Compute; fb |= FamilyBit::Transfer; }
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) { fb |= FamilyBit::Transfer; }

        if (!ci.headless) {
            if (ci.presentation_support_callback(_vk_instance, _physical_device, i)) { fb |= FamilyBit::Present; }
        }
        family_bits[i] = fb;
    }

    // Returns a queue family index where some capabilities (mask_of_set_bits) must be present and
    // other capabilities (mask_of_unset_bits) must be absent.
    auto find_family = [&](uint32_t wanted_caps, uint32_t unwanted_caps) -> uint32_t {
        // In headless mode, we transparently ignore the present capability by modifying
        // the bit mask. It becomes neither wanted nor unwanted.
        if (ci.headless) {
            wanted_caps &= ~FamilyBit::Present;
            unwanted_caps &= ~FamilyBit::Present;
        }
        for (auto[i, bits] : jaws::util::enumerate_range(family_bits)) {
            if ((bits & wanted_caps) != wanted_caps) { continue; }
            if ((~bits & unwanted_caps) != unwanted_caps) { continue; }
            return i;
        }
        return INVALID_INDEX;
    };
    auto is_queue_invalid = [this](Queue q) {
        return get_queue_info(q).family_index == INVALID_INDEX;
    };

    // Find async compute queue family, as exclusive as possible.
    get_queue_info(Queue::AsyncCompute).family_index = find_family(FamilyBit::Compute, FamilyBit::Graphics | FamilyBit::Transfer | FamilyBit::Present);
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index = find_family(FamilyBit::Compute, FamilyBit::Graphics | FamilyBit::Transfer);
    }
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index = find_family(FamilyBit::Compute, FamilyBit::Graphics);
    }
    if (is_queue_invalid(Queue::AsyncCompute)) {
        get_queue_info(Queue::AsyncCompute).family_index = find_family(FamilyBit::Compute, 0);
    }

    // Find async transfer queue family, as exclusive as possible.
    get_queue_info(Queue::AsyncTransfer).family_index = find_family(FamilyBit::Transfer, FamilyBit::Graphics | FamilyBit::Compute | FamilyBit::Present);
    if (is_queue_invalid(Queue::AsyncTransfer)) {
        get_queue_info(Queue::AsyncTransfer).family_index = find_family(FamilyBit::Transfer, FamilyBit::Graphics | FamilyBit::Compute);
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
    if ((fam_index = find_family(FamilyBit::Graphics | FamilyBit::Compute | FamilyBit::Present | FamilyBit::Transfer, 0)) != INVALID_INDEX) {
        get_queue_info(Queue::Graphics).family_index = fam_index;
        get_queue_info(Queue::Compute).family_index = fam_index;
        get_queue_info(Queue::Present).family_index = fam_index;
        get_queue_info(Queue::Transfer).family_index = fam_index;
    } else if ((fam_index = find_family(FamilyBit::Graphics | FamilyBit::Present | FamilyBit::Transfer, 0)) != INVALID_INDEX) {
        get_queue_info(Queue::Graphics).family_index = fam_index;
        get_queue_info(Queue::Present).family_index = fam_index;
        get_queue_info(Queue::Transfer).family_index = fam_index;
    } else if ((fam_index = find_family(FamilyBit::Graphics | FamilyBit::Compute | FamilyBit::Transfer, 0)) != INVALID_INDEX) {
        get_queue_info(Queue::Graphics).family_index = fam_index;
        get_queue_info(Queue::Compute).family_index = fam_index;
        get_queue_info(Queue::Transfer).family_index = fam_index;
    }

    // Any leftovers use their first available queue.
    if (is_queue_invalid(Queue::Graphics)) { get_queue_info(Queue::Graphics).family_index = find_family(FamilyBit::Graphics, 0); }
    if (is_queue_invalid(Queue::Present)) { get_queue_info(Queue::Present).family_index = find_family(FamilyBit::Present, 0); }
    if (is_queue_invalid(Queue::Compute)) { get_queue_info(Queue::Compute).family_index = find_family(FamilyBit::Compute, 0); }
    if (is_queue_invalid(Queue::Transfer)) { get_queue_info(Queue::Transfer).family_index = find_family(FamilyBit::Transfer, 0); }

    if (is_queue_invalid(Queue::Graphics)) { JAWS_FATAL1("No graphics queue family found"); }
    if (!ci.headless) {
        if (get_queue_info(Queue::Present).family_index == INVALID_INDEX) { JAWS_FATAL1("No present queue family found"); }
    }
    if (is_queue_invalid(Queue::Compute)) { JAWS_FATAL1("No compute queue family found"); }
    if (is_queue_invalid(Queue::Transfer)) { JAWS_FATAL1("No transfer queue family found"); }

    // Leftover async ones fall back to their non-async siblings.
    if (is_queue_invalid(Queue::AsyncCompute)) { get_queue_info(Queue::AsyncCompute).family_index = get_queue_info(Queue::Compute).family_index; }
    if (is_queue_invalid(Queue::AsyncTransfer)) { get_queue_info(Queue::AsyncTransfer).family_index = get_queue_info(Queue::Transfer).family_index; }


    logger.info("selected queue families:");
    logger.info("    graphics: {}", get_queue_info(Queue::Graphics).family_index);
    if (!ci.headless) {
        logger.info("    present: {}", get_queue_info(Queue::Present).family_index);
    }
    logger.info("    compute: {}", get_queue_info(Queue::Compute).family_index);
    logger.info("    transfer: {}", get_queue_info(Queue::Transfer).family_index);
    logger.info("    async compute: {}", get_queue_info(Queue::AsyncCompute).family_index);
    logger.info("    async transfer: {}", get_queue_info(Queue::AsyncTransfer).family_index);

    //=========================================================================
    // Create a logical device using the chosen queues.
    {
        // Build the queue create infos -- one ci for one queue per queue family.
        // I don't know of any advantages of using more than one queue per family.
        std::vector<VkDeviceQueueCreateInfo> queue_cis;
        auto add_queue_info = [&](uint32_t family_index) {
            VkDeviceQueueCreateInfo queue_ci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
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
            if (!family_handled) {
                add_queue_info(qi.family_index);
            }
        }

        std::vector<char const *> device_extension_ptrs;
        device_extension_ptrs.reserve(requested_device_extensions.size());
        for (const auto &ext : requested_device_extensions) {
            device_extension_ptrs.push_back(ext.name.c_str());
        }

        VkDeviceCreateInfo device_ci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        device_ci.queueCreateInfoCount = static_cast<uint32_t>(queue_cis.size());
        device_ci.pQueueCreateInfos = queue_cis.data();
        device_ci.enabledExtensionCount = static_cast<uint32_t>(device_extension_ptrs.size());
        device_ci.ppEnabledExtensionNames = device_extension_ptrs.data();

        result = vkCreateDevice(_physical_device, &device_ci, nullptr, &_device);
        JAWS_VK_HANDLE_FATAL(result);

        // Retrieve queue objects. We need them later.
        _unique_queues.assign(queue_cis.size(), VK_NULL_HANDLE);
        for (const auto&[i, qci] : jaws::util::enumerate_range(queue_cis)) {
            vkGetDeviceQueue(_device, qci.queueFamilyIndex, 0, &_unique_queues[i]);
            // For each queue info using that family, set the unique queue index.
            for (auto &qi : _queue_infos) {
                if (qi.family_index == qci.queueFamilyIndex) {
                    qi.unique_queue_index = i;
                }
            }
        }
    }
    logger.info("device: {}", (void *) _device);
    volkLoadDevice(_device);

    //=========================================================================
    // Init vulkan memory allocator

    _vma_vulkan_functions.vkAllocateMemory = vkAllocateMemory;
    _vma_vulkan_functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    _vma_vulkan_functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    _vma_vulkan_functions.vkAllocateMemory = vkAllocateMemory;
    _vma_vulkan_functions.vkFreeMemory = vkFreeMemory;
    _vma_vulkan_functions.vkMapMemory = vkMapMemory;
    _vma_vulkan_functions.vkUnmapMemory = vkUnmapMemory;
    _vma_vulkan_functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    _vma_vulkan_functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    _vma_vulkan_functions.vkBindBufferMemory = vkBindBufferMemory;
    _vma_vulkan_functions.vkBindImageMemory = vkBindImageMemory;
    _vma_vulkan_functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    _vma_vulkan_functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    _vma_vulkan_functions.vkCreateBuffer = vkCreateBuffer;
    _vma_vulkan_functions.vkDestroyBuffer = vkDestroyBuffer;
    _vma_vulkan_functions.vkCreateImage = vkCreateImage;
    _vma_vulkan_functions.vkDestroyImage = vkDestroyImage;
    _vma_vulkan_functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
#    if VMA_DEDICATED_ALLOCATION
    _vma_vulkan_functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
    _vma_vulkan_functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
#    endif
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = _physical_device;
    allocator_info.device = _device;
    allocator_info.pVulkanFunctions = &_vma_vulkan_functions;
    if (vma_can_use_dedicated_alloc) { allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT; }
    vmaCreateAllocator(&allocator_info, &_vma_allocator);

#    if 0
    // Let's test this.
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = 65536;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkBuffer buffer;
    VmaAllocation allocation;
    result = vmaCreateBuffer(_vma_allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
    JAWS_VK_HANDLE_FATAL(result);

    vmaDestroyBuffer(_vma_allocator, buffer, allocation);
#    endif

    _shader_system = std::make_unique<ShaderSystem>();
    _shader_system->create(_instance);
#endif
}


void Context::destroy()
{
    vkDestroyDebugUtilsMessengerEXT(_vk_instance, _debug_messenger, nullptr);
    vkDestroyInstance(_vk_instance, nullptr);
}


/*
Shader* Context::get_shader(const std::string& main_source_file) const
{
    JAWS_ASSUME(_shader_system);
    return _shader_system->get_shader(main_source_file);
}
 */

}

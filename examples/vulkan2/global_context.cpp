#include "global_context.hpp"
#include "jaws/vulkan/utils.hpp"
#include "jaws/fatal.hpp"
#include "jaws/logging.hpp"

namespace {

static jaws::LoggerPtr logger;

static bool GetQueueFamilySupportsPresentation(VkPhysicalDevice device, uint32_t queue_family_index)
{
    // See glfw3 sources for other platforms' versions.
#if defined(JAWS_OS_WIN)
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(device, queue_family_index);
#else
    return false;
#endif
}


} // namespace

namespace jaws::vulkan {


GlobalContext::GlobalContext(
    const std::string& app_name,
    int app_version_major,
    int app_version_minor,
    int app_version_patch,
    const std::vector<std::string>& required_extensions,
    bool enable_validation) :
    _enable_validation(enable_validation)
{
    if (!logger) { logger = jaws::GetLoggerPtr(jaws::Category::General); }

    VkResult result = {};

    result = volkInitialize();
    Handle(result);

    result = vkEnumerateInstanceVersion(&_vulkan_version);
    Handle(result);

    logger->info(
        "Vulkan version {}.{}.{}",
        VK_VERSION_MAJOR(_vulkan_version),
        VK_VERSION_MINOR(_vulkan_version),
        VK_VERSION_PATCH(_vulkan_version));

    std::vector<const char*> extensions_ptrs;
    for (const auto& r : required_extensions) { extensions_ptrs.push_back(r.c_str()); }
#if defined(JAWS_OS_WIN)
    // We need those with or without glfw.
    extensions_ptrs.push_back("VK_KHR_surface");
    extensions_ptrs.push_back("VK_KHR_win32_surface");
#endif
    if (true) {
        extensions_ptrs.push_back("VK_EXT_debug_utils");
        extensions_ptrs.push_back("VK_EXT_debug_report");
    }
    std::vector<const char*> layers_ptrs;
    if (_enable_validation) { layers_ptrs.push_back("VK_LAYER_LUNARG_standard_validation"); }

    //=========================================================================
    // Instance
    {
        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = app_name.c_str();
        app_info.applicationVersion = VK_MAKE_VERSION(app_version_major, app_version_minor, app_version_patch);
        app_info.pEngineName = "MyEngine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = _vulkan_version;

        // VK_KHR_win32_surface

        VkInstanceCreateInfo instance_ci = {};
        instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_ci.pApplicationInfo = &app_info;
        instance_ci.enabledExtensionCount = static_cast<uint32_t>(extensions_ptrs.size());
        instance_ci.ppEnabledExtensionNames = extensions_ptrs.data();
        instance_ci.enabledLayerCount = static_cast<uint32_t>(layers_ptrs.size());
        instance_ci.ppEnabledLayerNames = layers_ptrs.data();

        result = vkCreateInstance(&instance_ci, nullptr, &_instance);
        Handle(result);

        logger->info("instance: {}", (void*)_instance);
    }

#if !defined(VK_KHR_win32_surface)
    int* crash = nullptr;
    *crash = 3;
#endif

    volkLoadInstance(_instance);

    //=========================================================================
    // Enumerate physical devices, choose the 1st one (for now), print memory and queue family props

    {
        uint32_t count;
        std::vector<VkPhysicalDevice> physical_devices;
        do {
            result = vkEnumeratePhysicalDevices(_instance, &count, nullptr);
            Handle(result);
            if (result == VK_SUCCESS && count > 0) {
                physical_devices.resize(count);
                result = vkEnumeratePhysicalDevices(_instance, &count, physical_devices.data());
                Handle(result);
            }
        } while (result == VK_INCOMPLETE);
        if (physical_devices.empty()) { JAWS_FATAL1("No vulkan-capable physical_devices found"); }
        _physical_device = physical_devices[0];
    }
    logger->info("physical device: {}", (void*)_physical_device);

    //=========================================================================
    // Now find queue families for 1) graphics and 2) presenting to our surface.
    // Ideally we want a family supporting both, but if we cannot have that we use
    // the first available one we find for each.

    constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();
    _graphics_queue_family_index = _present_queue_family_index = INVALID_INDEX;
    {
        uint32_t count;
        std::vector<VkQueueFamilyProperties> queue_family_properties;
        vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &count, nullptr);
        if (count > 0) {
            queue_family_properties.resize(count);
            vkGetPhysicalDeviceQueueFamilyProperties(_physical_device, &count, queue_family_properties.data());
        }

        for (size_t i = 0; i < queue_family_properties.size(); ++i) {
            const bool supports_graphics = queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
            const bool supports_present =
                GetQueueFamilySupportsPresentation(_physical_device, static_cast<uint32_t>(i));

            if (supports_graphics && supports_present) {
                // We'd strongly prefer if the chosen graphics queue family also supports present,
                // hence the short circuit here.
                _graphics_queue_family_index = _present_queue_family_index = static_cast<uint32_t>(i);
                break;
            }
            if (supports_graphics && _graphics_queue_family_index == INVALID_INDEX ) {
                _graphics_queue_family_index = static_cast<uint32_t>(i);
            }
            if (supports_present && _present_queue_family_index == INVALID_INDEX) {
                _present_queue_family_index = static_cast<uint32_t>(i);
            }
        }
    }

    if (_graphics_queue_family_index == INVALID_INDEX) { JAWS_FATAL1("No graphics queue family found"); }
    if (_present_queue_family_index == INVALID_INDEX) { JAWS_FATAL1("No present queue family found"); }
    logger->info("Selected family {} for graphics", _graphics_queue_family_index);
    logger->info("Selected family {} for present", _present_queue_family_index);


    //=========================================================================
    // Create a logical device using the chosen queues.

    {
        std::vector<VkDeviceQueueCreateInfo> queue_cis;
        const float queue_prio = 0.0f;
        {
            VkDeviceQueueCreateInfo queue_ci = {};
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.queueCount = 1;
            queue_ci.pQueuePriorities = &queue_prio;
            queue_ci.queueFamilyIndex = _graphics_queue_family_index;
            queue_cis.push_back(queue_ci);
        }
        if (_present_queue_family_index != _graphics_queue_family_index) {
            VkDeviceQueueCreateInfo queue_ci = {};
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.queueCount = 1;
            queue_ci.pQueuePriorities = &queue_prio;
            queue_ci.queueFamilyIndex = _present_queue_family_index;
            queue_cis.push_back(queue_ci);
        }

        std::vector<char const*> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

        VkDeviceCreateInfo device_ci = {};
        device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_ci.queueCreateInfoCount = static_cast<uint32_t>(queue_cis.size());
        device_ci.pQueueCreateInfos = queue_cis.data();
        device_ci.enabledLayerCount = 0;
        device_ci.ppEnabledLayerNames = nullptr;
        device_ci.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        device_ci.ppEnabledExtensionNames = device_extensions.data();

        result = vkCreateDevice(_physical_device, &device_ci, nullptr, &_device);
        Handle(result);
    }
    logger->info("device: {}", (void*)_device);

    volkLoadDevice(_device);

    //-------------------------------------------------------------------------
    // Retrieve queue objects

    {
        VkDeviceQueueInfo2 qi = {};
        qi.queueFamilyIndex = _graphics_queue_family_index;
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        vkGetDeviceQueue2(_device, &qi, &_graphics_queue);
    }
    {
        VkDeviceQueueInfo2 qi = {};
        qi.queueFamilyIndex = _present_queue_family_index;
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        vkGetDeviceQueue2(_device, &qi, &_present_queue);
    }
}


GlobalContext::~GlobalContext() {

    if ( _device ) {
        vkDestroyDevice(_device, nullptr);
        _device = nullptr;
    }
    // The physical device is not destroyed.
    if ( _instance ) {
        vkDestroyInstance(_instance, nullptr);
        _instance = nullptr;
    }
}

} // namespace jaws::vulkan

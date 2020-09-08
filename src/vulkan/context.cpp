#include "pch.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/util/enumerate_range.hpp"
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
        required_extensions.add(VK_KHR_SURFACE_EXTENSION_NAME);

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
        debug_messenger_ci.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_messenger_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                         | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                         | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_messenger_ci.pfnUserCallback = &vulkan_debug_utils_messenger_cb;
        result = vkCreateDebugUtilsMessengerEXT(_vk_instance, &debug_messenger_ci, nullptr, &_debug_messenger);
        JAWS_VK_HANDLE_FATAL(result);

        // Inject debug message to validate it works.
        {
            VkDebugUtilsMessengerCallbackDataEXT data = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT};
            data.pMessage = "Validation output callback works.";
            vkSubmitDebugUtilsMessageEXT(
                _vk_instance,
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                &data);
        }

        //=========================================================================
        // Enumerate physical devices

        {
            for (auto pd : enumerated<VkPhysicalDevice>(vkEnumeratePhysicalDevices, {}, _vk_instance)) {
                VkPhysicalDeviceProperties props;
                vkGetPhysicalDeviceProperties(pd, &props);
            }
        }
        // logger.info("physical device: {}", (void *) _physical_device);
    }
}


void Context::destroy()
{
    vkDestroyDebugUtilsMessengerEXT(_vk_instance, _debug_messenger, nullptr);
    vkDestroyInstance(_vk_instance, nullptr);
}

}

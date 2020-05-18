#include "jaws/jaws.hpp"
#include "jaws/logging.hpp"
#include "jaws/vfs/file_system_vfs.hpp"
#include "jaws/util/indentation.hpp"

#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/window_context.hpp"
//#include "jaws/vulkan/utils.hpp"

//#include "jaws/vulkan/vulkan.hpp"
//#include "global_context.hpp"

#include "build_info.h"

#define GLFW_INCLUDE_NONE
//#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

//=========================================================================

jaws::LoggerPtr logger;

//=========================================================================

static void glfwErrorCallback(int error, const char *description) {
    logger->error("GLFW error {}: {}", error, description);
    JAWS_FATAL1("Error");
}

int main(int argc, char **argv) {
    return jaws::util::Main(argc, argv, [](auto, auto) {

        jaws::Instance instance;
        instance.add_vfs("files", std::make_unique<jaws::vfs::FileSystemVfs>(build_info::PROJECT_SOURCE_DIR / "shaders"));

        // Piggypacking onto one of jaws' logger categories here.
        logger = jaws::GetLoggerPtr(jaws::Category::General);

        if (!glfwInit()) { JAWS_FATAL1("Unable to initialize GLFW"); }
        glfwSetErrorCallback(&glfwErrorCallback);
        if (!glfwVulkanSupported()) { JAWS_FATAL1("glfwVulkanSupported -> false"); }

        uint32_t required_extension_count = 0;
        const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

        jaws::vulkan::Context context;
        context.create(&instance, jaws::vulkan::Context::CreateInfo{}
            //.set_vkGetInstanceProcAddr(glfwGetInstanceProcAddress) // Should work with or without this.
            .set_required_instance_extensions({required_extensions, required_extension_count})
            .set_presentation_support_callback(&glfwGetPhysicalDevicePresentationSupport)
            .set_headless(false));

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow *glfw_window = glfwCreateWindow(800, 600, "jaws example", nullptr, nullptr);
        if (!glfw_window) { JAWS_FATAL1("Failed to create window"); }

        VkSurfaceKHR surface;
        if (VkResult err = glfwCreateWindowSurface(context.get_instance(), glfw_window, nullptr, &surface)) {
            JAWS_FATAL1("Failed to create window surface");
        }

        jaws::vulkan::Device device;
        device.create(&context);

        jaws::vulkan::WindowContext window;
        jaws::vulkan::WindowContext::CreateInfo window_ci;
        window_ci.surface = surface;
        window.create(&device, window_ci);


        //Shader* my_shader = get_shader()





        return 0;
    });
}


#if 0
//=========================================================================

jaws::LoggerPtr logger;

constexpr bool ENABLE_VALIDATION = true;
constexpr bool ENABLE_DEBUG = true;
constexpr int VERBOSITY = 2;
constexpr int INITIAL_WINDOW_WIDTH = 300;
constexpr int INITIAL_WINDOW_HEIGHT = 300;

//=========================================================================

//=========================================================================

inline std::string to_string(const std::vector<std::string>& sl)
{
    std::ostringstream oss;
    bool not_first = false;
    for (const auto& str : sl) {
        if (not_first) { oss << ", "; }
        not_first = true;
        oss << str;
    }
    return oss.str();
}

//=========================================================================

namespace jaws::vulkan {

template <typename T, typename F>
T ChooseBest(const std::vector<T>& supported_list, F f)
{
    JAWS_ASSUME(!supported_list.empty());
    T best_one = supported_list[0];
    int max_weight = std::numeric_limits<int>::min(); // or zero?
    for (auto s : supported_list) {
        int weight = f(s);
        if (weight > max_weight) {
            max_weight = weight;
            best_one = s;
        }
    }
    return best_one;
}

void Log(
    const VkLayerProperties& p,
    jaws::LoggerPtr logger,
    const spdlog::level::level_enum log_level = spdlog::level::level_enum::info,
    const int verbosity = 0,
    const int initial_indent_level = 0,
    const std::string& indent_step = {})
{
    auto log = jaws::util::IndentationLogger(logger, log_level, initial_indent_level, indent_step);
    log("layerName: {}", p.layerName);
    log("specVersion: {}.{}.{}",
        VK_VERSION_MAJOR(p.specVersion),
        VK_VERSION_MINOR(p.specVersion),
        VK_VERSION_PATCH(p.specVersion));
    log("implementationVersion: {}.{}.{}",
        VK_VERSION_MAJOR(p.implementationVersion),
        VK_VERSION_MINOR(p.implementationVersion),
        VK_VERSION_PATCH(p.implementationVersion));
    log("description: {}", p.description);
}

std::vector<const char*> StringsToPointers(const std::vector<std::string>& sl)
{
    std::vector<const char*> result;
    for (const auto& s : sl) { result.push_back(s.c_str()); }
    return result;
}

} // namespace jaws::vulkan

//=========================================================================

static void glfwErrorCallback(int error, const char* description)
{
    logger->error("GLFW error {}: {}", error, description);
    JAWS_FATAL1("Error");
}

int run()
{
    if (!glfwInit()) { JAWS_FATAL1("Unable to initialize GLFW"); }
    glfwSetErrorCallback(&glfwErrorCallback);

    // GLFW requires a number of extensions.
    std::vector<std::string> exts;
    {
        uint32_t required_extension_count = 0;
        const char** required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
        if (!required_extensions) { JAWS_FATAL1("glfwGetRequiredInstanceExtensions failed"); }
        for (size_t i = 0; i < required_extension_count; i++) { exts.push_back(std::string(required_extensions[i])); }
    }

    jaws::vulkan::GlobalContext global_context("jaws-examples", 1, 0, 0, exts);


    return 0;
}













#    if 0
int run()
{
    using jaws::vulkan::Handle;

    Context context;

    VkResult result;
    result = volkInitialize();
    Handle(result);

    uint32_t vulkan_version;
    result = vkEnumerateInstanceVersion(&vulkan_version);
    Handle(result);

    logger->info(
        "Vulkan version {}.{}.{}",
        VK_VERSION_MAJOR(vulkan_version),
        VK_VERSION_MINOR(vulkan_version),
        VK_VERSION_PATCH(vulkan_version));

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "MyApp";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "MyEngine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = vulkan_version;



    //=========================================================================
    // Extensions

    std::vector<std::string> extensions_to_enable;
    {
        if (!glfwInit()) { JAWS_FATAL1("Unable to initialize GLFW"); }
        glfwSetErrorCallback(&glfwErrorCallback);

        // GLFW requires a number of extensions.
        {
            uint32_t required_extension_count = 0;
            const char** required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
            if (!required_extensions) { JAWS_FATAL1("glfwGetRequiredInstanceExtensions failed"); }
            for (size_t i = 0; i < required_extension_count; i++) {
                extensions_to_enable.push_back(required_extensions[i]);
            }
            logger->info("GLFW requires these extensions: {}", to_string(extensions_to_enable));
        }
        if (ENABLE_DEBUG) {
            extensions_to_enable.push_back("VK_EXT_debug_utils");
            extensions_to_enable.push_back("VK_EXT_debug_report");
        }
    }

    //=========================================================================
    // Layers

    std::vector<std::string> layers_to_enable;
    {
        uint32_t prop_count;
        std::vector<VkLayerProperties> props;
        do {
            result = vkEnumerateInstanceLayerProperties(&prop_count, nullptr);
            Handle(result);
            if (result == VK_SUCCESS && prop_count > 0) {
                props.resize(prop_count);
                result = vkEnumerateInstanceLayerProperties(&prop_count, props.data());
                Handle(result);
            }
        } while (result == VK_INCOMPLETE);

        // for (const auto& p : props) { jaws::vulkan::Log(p, logger); }

        if (ENABLE_VALIDATION) { layers_to_enable.push_back("VK_LAYER_LUNARG_standard_validation"); }

        for (const std::string& lte : layers_to_enable) {
            if (std::find_if(
                    props.cbegin(),
                    props.cend(),
                    [&lte](const VkLayerProperties& prop) { return lte == prop.layerName; })
                == props.cend()) {
                logger->error("layer {} not found", lte);
            }
        }
    }

    //=========================================================================
    // Instance

    VkInstance instance = nullptr;
    {
        auto extensions_ptrs = jaws::vulkan::StringsToPointers(extensions_to_enable);
        auto layers_ptrs = jaws::vulkan::StringsToPointers(layers_to_enable);

        VkInstanceCreateInfo instance_ci = {};
        instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_ci.pApplicationInfo = &app_info;
        instance_ci.enabledExtensionCount = static_cast<uint32_t>(extensions_ptrs.size());
        instance_ci.ppEnabledExtensionNames = extensions_ptrs.data();
        instance_ci.enabledLayerCount = static_cast<uint32_t>(layers_ptrs.size());
        instance_ci.ppEnabledLayerNames = layers_ptrs.data();

        result = vkCreateInstance(&instance_ci, nullptr, &instance);
        Handle(result);

        logger->info("instance: {}", (void*)instance);
    }

    volkLoadInstance(instance);

    //=========================================================================
    // Enumerate physical devices, choose the 1st one (for now), print memory and queue family props

    VkPhysicalDevice physical_device = nullptr;
    {
        uint32_t count;
        std::vector<VkPhysicalDevice> physical_devices;
        do {
            result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
            Handle(result);
            if (result == VK_SUCCESS && count > 0) {
                physical_devices.resize(count);
                result = vkEnumeratePhysicalDevices(instance, &count, physical_devices.data());
                Handle(result);
            }
        } while (result == VK_INCOMPLETE);
        if (physical_devices.empty()) { JAWS_FATAL1("No vulkan-capable physical_devices found"); }
        physical_device = physical_devices[0];
    }
    logger->info("physical device: {}", (void*)physical_device);

    //=========================================================================
    // Back to GLFW now that we have a vulkan instance because we need a surface
    // for the next step. See http://www.glfw.org/docs/latest/vulkan_guide.html,
    // but we will not use GLFW for Vulkan handling beyond getting a window and a surface.

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window =
        glfwCreateWindow(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, "jaws example", nullptr, nullptr);
    if (!window) { JAWS_FATAL1("Failed to create window"); }

    VkSurfaceKHR surface;
    result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    Handle(result);

    //=========================================================================
    // Now find queue families for 1) graphics and 2) presenting to our surface.
    // Ideally we want a family supporting both, but if we cannot have that we use
    // the first available one we find for each.

    int graphics_queue_family_index = -1;
    int present_queue_family_index = -1;
    {
        uint32_t count;
        std::vector<VkQueueFamilyProperties> queue_family_properties;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);
        if (count > 0) {
            queue_family_properties.resize(count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_family_properties.data());
        }

        for (size_t i = 0; i < queue_family_properties.size(); ++i) {
            const auto& this_props = queue_family_properties[i];

            VkBool32 supports_present = false;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_device, static_cast<uint32_t>(i), surface, &supports_present);
            Handle(result);
            const bool supports_graphics = this_props.queueFlags & VK_QUEUE_GRAPHICS_BIT;

            if (supports_graphics && supports_present) {
                // We'd strongly prefer if the chosen graphics queue family also supports present,
                // hence the short circuit here.
                graphics_queue_family_index = present_queue_family_index = static_cast<int>(i);
                break;
            }
            if (supports_graphics && graphics_queue_family_index < 0) {
                graphics_queue_family_index = static_cast<int>(i);
            }
            if (supports_present && present_queue_family_index < 0) {
                present_queue_family_index = static_cast<int>(i);
            }
        }
    }

    if (graphics_queue_family_index < 0) { JAWS_FATAL1("No graphics queue family found"); }
    if (present_queue_family_index < 0) { JAWS_FATAL1("No present queue family found"); }
    logger->info("Selected family {} for graphics", graphics_queue_family_index);
    logger->info("Selected family {} for present", present_queue_family_index);

    //=========================================================================
    // Create a logical device using the chosen queues.

    VkDevice device = nullptr;
    {
        std::vector<VkDeviceQueueCreateInfo> queue_cis;
        const float queue_prio = 0.0f;
        {
            VkDeviceQueueCreateInfo queue_ci = {};
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.queueCount = 1;
            queue_ci.pQueuePriorities = &queue_prio;
            queue_ci.queueFamilyIndex = graphics_queue_family_index;
            queue_cis.push_back(queue_ci);
        }
        if (present_queue_family_index != graphics_queue_family_index) {
            VkDeviceQueueCreateInfo queue_ci = {};
            queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_ci.queueCount = 1;
            queue_ci.pQueuePriorities = &queue_prio;
            queue_ci.queueFamilyIndex = present_queue_family_index;
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

        result = vkCreateDevice(physical_device, &device_ci, nullptr, &device);
        Handle(result);
    }
    logger->info("device: {}", (void*)device);

    volkLoadDevice(device);

    //=========================================================================
    // Query supported surface formats and select one.
    // We use a system that assigns a weight to each format it finds and then
    // chooses the one with the highest weight.

    VkSurfaceFormatKHR surface_format = {};
    {
        uint32_t count;
        std::vector<VkSurfaceFormatKHR> surface_formats;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, surface_formats.data());
        Handle(result);
        if (result == VK_SUCCESS && count > 0) {
            surface_formats.resize(count);
            result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, surface_formats.data());
            Handle(result);
        }
        if ( surface_formats.empty()) {
            JAWS_FATAL1("no surface formats available!");
        }

        surface_format = jaws::vulkan::ChooseBest(surface_formats, [](auto sf) {
          if (sf.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && sf.format == VK_FORMAT_B8G8R8A8_SRGB) {
              return 1000;}
          else if (sf.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && sf.format == VK_FORMAT_B8G8R8A8_UNORM) {
              return 100;
          } else if (sf.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
              return 50;
          }
          return 0;
        });
    }

    //=========================================================================

    // vkEnumerateInstanceExtensionProperties(


    /*
    vkEnumerateInstanceExtensionProperties(

    if (ENABLE_VALIDATION ) {

        vkEnumerateInstanceLayerProperties(


    }

    VkInstanceCreateInfo instance_ci = {};
    instance_ci.pApplicationInfo = &app_info;
     */
    return 0;
}
#    endif


int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, [](auto, auto) {
        // Piggypacking onto one of jaws' logger categories here.
        logger = jaws::GetLoggerPtr(jaws::Category::General);

        return run();

        // We could add that catch block into jaws::util::Main,
        // but I don't want vulkan dependencies there. Nothing
        // wrong with deeper nested try-catch blocks where we
        // have more domain knowledge like "vulkan hpp might throw!".
        /*
        try {
            return run();
        } catch (const vk::SystemError& e) {
            JAWS_FATAL2(jaws::FatalError::UncaughtException, std::string("vk::SystemError: ") + e.what());
            return -3;
        }
         */
    });
}
#endif

#include "jaws/jaws.hpp"
#include "jaws/util/misc.hpp"
#include "jaws/util/file_observer.hpp"
#include "jaws/vulkan/utils.hpp"
#include "jaws/filesystem.hpp"
#include "build_info.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#if _WIN32
#    define GLFW_EXPOSE_NATIVE_WIN32
#else
#    define GLFW_EXPOSE_NATIVE_GLX
#endif
#include "GLFW/glfw3native.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <array>
#include <fstream>

#if 0
//=========================================================================

namespace fs = std::filesystem;
using jaws::vulkan::to_string;
using std::to_string;

// A ptr so we can declare it statically and only set it on init.
jaws::LoggerPtr logger;

constexpr bool ENABLE_VALIDATION_LAYERS = true;
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr bool VERBOSE_PRINT = false;

static void glfwErrorCallback(int error, const char* description)
{
    logger->error("GLFW error {}: {}", error, description);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugCallback(
    VkDebugReportFlagsEXT msg_flags,
    VkDebugReportObjectTypeEXT obj_type,
    uint64_t src_object,
    size_t location,
    int32_t msg_code,
    const char* layer_prefix,
    const char* msg,
    void* user_data)
{
    // Map type flags to log level
    auto log_level = spdlog::level::off;
    if (msg_flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        log_level = spdlog::level::info;
    } else if (msg_flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        log_level = spdlog::level::warn;
    } else if (msg_flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        log_level = spdlog::level::warn;
    } else if (msg_flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        log_level = spdlog::level::err;
    } else if (msg_flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        log_level = spdlog::level::debug;
    }

    logger->log(log_level, "[{}] Code {}:{}", layer_prefix, msg_code, msg);

    // false indicates that layer should not bail-out of an API call that had validation failures. This may mean that
    // the app dies inside the driver due to invalid parameter(s). That's what would happen without validation layers,
    // so we'll keep that behavior here.
    return false;
}

class MyIncludeHandler : public shaderc::CompileOptions::IncluderInterface
{
public:
    /*
// Source text inclusion via #include is supported with a pair of callbacks
// to an "includer" on the client side.  The first callback processes an
// inclusion request, and returns an include result.  The includer owns
// the contents of the result, and those contents must remain valid until the
// second callback is invoked to release the result.  Both callbacks take a
// user_data argument to specify the client context.
// To return an error, set the source_name to an empty string and put your
// error message in content.

// An include result.
    typedef struct shaderc_include_result {
        // The name of the source file.  The name should be fully resolved
        // in the sense that it should be a unique name in the context of the
        // includer.  For example, if the includer maps source names to files in
        // a filesystem, then this name should be the absolute path of the file.
        // For a failed inclusion, this string is empty.
        const char* source_name;
        size_t source_name_length;
        // The text contents of the source file in the normal case.
        // For a failed inclusion, this contains the error message.
        const char* content;
        size_t content_length;
        // User data to be passed along with this request.
        void* user_data;   } shaderc_include_result;
     */

    shaderc_include_result* GetInclude(
        const char* requested_source,
        shaderc_include_type type,
        /* type is one of:
        shaderc_include_type_relative,  // E.g. #include "source"
        shaderc_include_type_standard   // E.g. #include <source>
        */
        const char* requesting_source,
        size_t include_depth) override
    {
        logger->info("#include handling: {} | {} | {} | {}", requested_source, type, requesting_source, include_depth);
        auto h = std::make_unique<Handle>();

        h->shaderc_result.source_name = "xxxxx";
        h->shaderc_result.source_name_length = 5;
        h->shaderc_result.content = "";
        h->shaderc_result.content_length = 0;
        h->shaderc_result.user_data = nullptr;

        alive_handles.push_back(std::move(h));
        return &alive_handles.back()->shaderc_result;
    }

    void ReleaseInclude(shaderc_include_result* data) override
    {
        for (auto iter = alive_handles.begin(); iter != alive_handles.end(); ++iter) {
            if (&iter->get()->shaderc_result == data) {
                alive_handles.erase(iter);
                return;
            }
        }
    }

private:
    struct Handle
    {
        shaderc_include_result shaderc_result;
    };
    std::vector<std::unique_ptr<Handle>> alive_handles;
};

void run()
{
    constexpr uint32_t WINDOW_WIDTH{200};
    constexpr uint32_t WINDOW_HEIGHT{200};
    constexpr vk::Format DEPTH_FORMAT{vk::Format::eD16Unorm};

    if (!glfwInit()) { JAWS_FATAL1("Unable to initialize GLFW"); }
    glfwSetErrorCallback(&glfwErrorCallback);

    //=========================================================================
    // Build a list of extensions to enable

    jaws::vulkan::StringList extensions_to_enable;

    // GLFW requires a number of extensions. We can ask it which ones.
    {
        uint32_t required_extension_count = 0;
        auto required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
        if (!required_extensions) { JAWS_FATAL1("glfwGetRequiredInstanceExtensions failed"); }
        for (size_t i = 0; i < required_extension_count; i++) {
            extensions_to_enable.push_back(required_extensions[i]);
        }
        logger->info("GLFW requires these extensions: {}", to_string(extensions_to_enable));
    }

    // ...and add our own.
    extensions_to_enable.push_back("VK_EXT_debug_utils");
    extensions_to_enable.push_back("VK_EXT_debug_report");

    //=========================================================================
    // Build a list of layers to enable

    jaws::vulkan::StringList layers_to_enable;
    std::vector<vk::LayerProperties> layer_props = vk::enumerateInstanceLayerProperties();

    if (ENABLE_VALIDATION_LAYERS /* enable debug layers */) {
        // I copied these lists from reference examples.
        // I don't know if they're optimal or sufficient or why there is a fallback list.

        static const jaws::vulkan::StringList valid_layers1 = {"VK_LAYER_GOOGLE_threading",
                                                               "VK_LAYER_LUNARG_parameter_validation",
                                                               "VK_LAYER_LUNARG_object_tracker",
                                                               "VK_LAYER_LUNARG_image",
                                                               "VK_LAYER_LUNARG_core_validation",
                                                               "VK_LAYER_LUNARG_swapchain",
                                                               "VK_LAYER_GOOGLE_unique_objects",
                                                               /*"VK_LAYER_LUNARG_api_dump"*/};
        // Standard validation means the following layers in order:
        // VK_LAYER_GOOGLE_threading
        // VK_LAYER_LUNARG_parameter_validation
        // VK_LAYER_LUNARG_device_limits
        // VK_LAYER_LUNARG_object_tracker
        // VK_LAYER_LUNARG_image
        // VK_LAYER_LUNARG_core_validation
        // VK_LAYER_LUNARG_swapchain
        // VK_LAYER_GOOGLE_unique_objects
        static const jaws::vulkan::StringList valid_layers2 = {
            "VK_LAYER_LUNARG_standard_validation" /*, "VK_LAYER_LUNARG_api_dump"*/};

        // This first try fails because VK_LAYER_LUNARG_image is unavailable. That layer should be available with the
        // LunarG SDK. I don't know why I don't have that layer or why the example code requests it if it's not common,
        // but let's go with it for now.
        bool validation_layer_found = jaws::vulkan::HasAllLayers(layer_props, valid_layers1);
        if (validation_layer_found) {
            layers_to_enable.insert(layers_to_enable.end(), valid_layers1.begin(), valid_layers1.end());
        } else {
            validation_layer_found = jaws::vulkan::HasAllLayers(layer_props, valid_layers2);
            if (validation_layer_found) {
                layers_to_enable.insert(layers_to_enable.end(), valid_layers2.begin(), valid_layers2.end());
            }
        }
        if (!validation_layer_found) { JAWS_FATAL1("Required validation layer not found"); }
    }

    //=========================================================================
    // Create Vulkan instance

    vk::ApplicationInfo appInfo("MyApp", 1, "MyEngine", 1, VK_API_VERSION_1_1);

    vk::InstanceCreateInfo instance_ci({}, &appInfo);
    // TODO: Required lifetime for these?
    auto layers_ptrs = jaws::vulkan::ToPointers(layers_to_enable);
    auto extensions_ptrs = jaws::vulkan::ToPointers(extensions_to_enable);

    logger->info("Enabling layers: {}", to_string(layers_to_enable));
    logger->info("Enabling extensions: {}", to_string(extensions_to_enable));

    instance_ci.setEnabledLayerCount(static_cast<std::uint32_t>(layers_ptrs.size()));
    instance_ci.setPpEnabledLayerNames(layers_ptrs.data());
    instance_ci.setEnabledExtensionCount(static_cast<std::uint32_t>(extensions_ptrs.size()));
    instance_ci.setPpEnabledExtensionNames(extensions_ptrs.data());

    auto instance = vk::createInstanceUnique(instance_ci);

    logger->info("instance properties:");
    jaws::vulkan::PrintInstanceProperties(logger, jaws::LogLevel::info, 1);

    //=========================================================================
    // Enumerate physical devices, choose the 1st one (for now), print memory and queue family props

    std::vector<vk::PhysicalDevice> physical_devices = instance->enumeratePhysicalDevices();
    if (physical_devices.empty()) { JAWS_FATAL1("No vulkan-capable physical_devices found"); }

    logger->info("Vulkan-capable physical devices ({}):", physical_devices.size());
    for (size_t i = 0; i < physical_devices.size(); ++i) {
        auto pd = physical_devices[i];
        logger->info("Physical device {}:", i);
        jaws::vulkan::Print(pd, logger, jaws::LogLevel::info, VERBOSE_PRINT, 1);
    }

    // Note: no ownership handle for physical devices, as they are not created, just enumerated.
    auto physical_device = physical_devices[0];
    logger->info("Selected physical device (physical_device) 0.");

    logger->info("Memory types:");
    vk::PhysicalDeviceMemoryProperties device_memory_props = physical_device.getMemoryProperties();
    jaws::vulkan::Print(device_memory_props, logger, jaws::LogLevel::info, 1);

    std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();
    logger->info("Queue families ({}):", queue_family_properties.size());
    for (size_t i = 0; i < queue_family_properties.size(); ++i) {
        const auto& props = queue_family_properties[i];
        logger->info("    QueueFamilyProperties {}:", i);
        jaws::vulkan::Print(props, logger, jaws::LogLevel::info, VERBOSE_PRINT, 2);
    }

    //=========================================================================
    // Some functions can only be resolved at runtime. Used this dynamic dispatch loader there.

    vk::DispatchLoaderDynamic dynamic_dispatcher(instance.get());

    //=========================================================================
    // Create debug report callback

    // TODO: for some reason this freezes inside the validation layer. Wrong dynamic dispatch loader?

    auto debug_callback = instance->createDebugReportCallbackEXTUnique(
        vk::DebugReportCallbackCreateInfoEXT{}
            .setFlags(
                vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning
                | vk::DebugReportFlagBitsEXT::eDebug | vk::DebugReportFlagBitsEXT::eInformation
                | vk::DebugReportFlagBitsEXT::ePerformanceWarning)
            .setPfnCallback(&MyDebugCallback),
        nullptr,
        dynamic_dispatcher);

    //=========================================================================
    // Back to GLFW now that we have a vulkan instance because we need a surface
    // for the next step. See http://www.glfw.org/docs/latest/vulkan_guide.html,
    // but we will not use GLFW for Vulkan handling beyond getting a window and a surface.

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "jaws example", nullptr, nullptr);
    if (!window) { JAWS_FATAL1("Failed to create window"); }

    VkSurfaceKHR surface;
    if (VkResult err = glfwCreateWindowSurface((VkInstance)*instance, window, nullptr, &surface)) {
        JAWS_FATAL1("Failed to create window surface");
    }

    //=========================================================================
    // Now find queue families for 1) graphics and 2) presenting to our surface.
    // Ideally we want a family supporting both, but in the absence of that we use
    // the first available one we find for each.

    int graphics_queue_family_index = -1;
    int present_queue_family_index = -1;

    for (size_t i = 0; i < queue_family_properties.size(); ++i) {
        const bool supports_graphics = !!(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics);
        const bool supports_present = physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface) != 0;
        if (supports_graphics && supports_present) {
            // We'd strongly prefer if the chosen graphics queue family also supports present,
            // hence the short circuit here.
            graphics_queue_family_index = present_queue_family_index = static_cast<int>(i);
            break;
        }
        if (supports_graphics && graphics_queue_family_index < 0) { graphics_queue_family_index = static_cast<int>(i); }
        if (supports_present && present_queue_family_index < 0) { present_queue_family_index = static_cast<int>(i); }
    }

    if (graphics_queue_family_index < 0) { JAWS_FATAL1("No graphics queue family found"); }
    if (present_queue_family_index < 0) { JAWS_FATAL1("No present queue family found"); }

    logger->info("Selected family {} for graphics", graphics_queue_family_index);
    logger->info("Selected family {} for present", present_queue_family_index);

    //=========================================================================
    // Create a logical device using the chosen queues.

    float queue_priority = 0.0f;
    vk::DeviceQueueCreateInfo device_queue_create_info(
        {}, static_cast<uint32_t>(graphics_queue_family_index), 1, &queue_priority);
    std::vector<char const*> device_extension_names = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    auto device = physical_device.createDeviceUnique(
        vk::DeviceCreateInfo{}
            .setQueueCreateInfoCount(1)
            .setPQueueCreateInfos(&device_queue_create_info)
            .setEnabledLayerCount(0)
            .setPpEnabledLayerNames(nullptr)
            .setEnabledExtensionCount(static_cast<uint32_t>(device_extension_names.size()))
            .setPpEnabledExtensionNames(device_extension_names.data()));

    // We will need to handle to the graphics queue later on.
    vk::Queue graphics_queue = device->getQueue(graphics_queue_family_index, 0);
    vk::Queue present_queue = device->getQueue(present_queue_family_index, 0);

    //=========================================================================
    // Query supported surface formats and select one.
    // We use a system that assigns a weight to each format it finds and then
    // chooses the one with the highest weight.

    std::vector<vk::SurfaceFormatKHR> surface_formats = physical_device.getSurfaceFormatsKHR(surface);
    JAWS_ASSUME(!surface_formats.empty());

    logger->info("Surface formats:");
    for (auto f : surface_formats) { logger->info("    format: {}, {}", to_string(f.format), to_string(f.colorSpace)); }
    auto surface_format = jaws::vulkan::ChooseBest(surface_formats, [](auto sf) {
        if (sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && sf.format == vk::Format::eB8G8R8A8Srgb) {
            return 1000;
        } else if (sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear && sf.format == vk::Format::eB8G8R8A8Unorm) {
            return 100;
        } else if (sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return 50;
        }
        return 0;
    });
    logger->info(
        "Selected surface_format: {}, {}", to_string(surface_format.format), to_string(surface_format.colorSpace));

    vk::SurfaceCapabilitiesKHR surface_caps = physical_device.getSurfaceCapabilitiesKHR(surface);
    logger->info("Surface capabilities:");
    jaws::vulkan::Print(surface_caps, logger, jaws::LogLevel::info, 1);

    //=========================================================================
    // Create a swap chain

    //-------------------------------------------------------------------------
    // Choose a present mode. Again we assign a weight to each present mode and
    // choose the best (highest weighted) one.

    vk::Extent2D swapchain_extent;
    // See
    // https://github.com/KhronosGroup/Vulkan-Hpp/blob/eaf09ee61e6cb964cf72e0023cd30777f8e3f9fe/samples/05_InitSwapchain/05_InitSwapchain.cpp#L95
    if (surface_caps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        // If the surface size is undefined, the size is set to the size of the images requested.
        swapchain_extent.width =
            jaws::util::Clamp(WINDOW_WIDTH, surface_caps.minImageExtent.width, surface_caps.maxImageExtent.width);
        swapchain_extent.height =
            jaws::util::Clamp(WINDOW_HEIGHT, surface_caps.minImageExtent.height, surface_caps.maxImageExtent.height);
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchain_extent = surface_caps.currentExtent;
    }

    logger->info("Using swap chain extent {}", to_string(swapchain_extent));

    std::vector<vk::PresentModeKHR> present_modes = physical_device.getSurfacePresentModesKHR(surface);
    logger->info("Available surface present modes:");
    for (auto pm : present_modes) { logger->info("    {}", to_string(pm)); }
    auto present_mode = jaws::vulkan::ChooseBest(present_modes, [](auto p) {
        switch (p) {
        case vk::PresentModeKHR::eMailbox: return 1000;
        case vk::PresentModeKHR::eFifoRelaxed: return 500;
        case vk::PresentModeKHR::eFifo: return 1;
        default: return 0;
        }
    });

    // temp override
    present_mode = vk::PresentModeKHR ::eFifo;

    logger->info("Selected present mode: {}", to_string(present_mode));

    // Use identity if available, otherwise current.
    vk::SurfaceTransformFlagBitsKHR pre_transform =
        (surface_caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
            vk::SurfaceTransformFlagBitsKHR::eIdentity :
            surface_caps.currentTransform;

    // For blending with other windows in the window system.
    // We probably almost always don't want this and hence specify opaque here.
    vk::CompositeAlphaFlagBitsKHR composite_alpha = /*
        (surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ?
            vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
            (surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ?
            vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
            (surface_caps.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ?
            vk::CompositeAlphaFlagBitsKHR::eInherit :
            */
        vk::CompositeAlphaFlagBitsKHR::eOpaque;

    // start w/ minimum count required to function
    uint32_t swapchain_image_count = surface_caps.minImageCount;
    // "it is recommended to request at least one more image than the minimum"
    swapchain_image_count += 1;
    if (surface_caps.maxImageCount > 0) {
        // Cap the value
        swapchain_image_count = std::min(surface_caps.maxImageCount, swapchain_image_count);
    }

    auto swap_chain_ci =
        vk::SwapchainCreateInfoKHR{}
            .setSurface(surface)
            .setMinImageCount(swapchain_image_count)
            .setImageFormat(surface_format.format)
            .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
            .setImageExtent(swapchain_extent)
            .setImageArrayLayers(1) // No stereo
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setQueueFamilyIndexCount(0)
            .setPQueueFamilyIndices(nullptr)
            .setPreTransform(pre_transform)
            .setCompositeAlpha(composite_alpha)
            .setPresentMode(present_mode)
            .setClipped(true) // We don't care about pixels obscured by other windows
            .setOldSwapchain(nullptr); // https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation


    uint32_t queue_family_indices[2] = {static_cast<uint32_t>(graphics_queue_family_index),
                                        static_cast<uint32_t>(present_queue_family_index)};
    if (graphics_queue_family_index != present_queue_family_index) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        swap_chain_ci.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndexCount(2)
            .setPQueueFamilyIndices(queue_family_indices);
    }

    auto swap_chain = device->createSwapchainKHRUnique(swap_chain_ci);

    // You need to communicate to Vulkan how you intend to use these swapchain images by
    // creating image views. A "view" is essentially additional information attached
    // to a resource that describes how that resource is used.
    std::vector<vk::Image> swap_chain_images = device->getSwapchainImagesKHR(swap_chain.get());

    logger->info("Got {} swap chain images", swap_chain_images.size());

    //-------------------------------------------------------------------------
    // Create image views for swap chain images so that we can eventually render into them.

    std::vector<vk::UniqueImageView> swap_chain_image_views;
    swap_chain_image_views.reserve(swap_chain_images.size());
    for (auto image : swap_chain_images) {
        /*
        const vk::ComponentMapping identity_component_mapping(
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity,
            vk::ComponentSwizzle::eIdentity);
            */
        auto subresource_range = vk::ImageSubresourceRange{}
                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1);
        auto image_view_ci = vk::ImageViewCreateInfo{}
                                 .setImage(image)
                                 .setViewType(vk::ImageViewType::e2D)
                                 //.setComponents(identity_component_mapping) // 4x Identity is default anyway
                                 .setSubresourceRange(subresource_range)
                                 .setFormat(surface_format.format);
        swap_chain_image_views.push_back(device->createImageViewUnique(image_view_ci));
    }

    //=========================================================================
    // Depth buffer, image and image view

    vk::FormatProperties format_props = physical_device.getFormatProperties(DEPTH_FORMAT);

    // TODO: See discussion here:
    // https://www.reddit.com/r/vulkan/comments/71k4gy/why_is_vk_image_tiling_linear_so_limited/
    // OTOH, https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/06_InitDepthBuffer/06_InitDepthBuffer.cpp
    // prefers linear tiling. Not sure.
    vk::ImageTiling tiling;
    if (format_props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        tiling = vk::ImageTiling::eOptimal;
    } else if (format_props.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        tiling = vk::ImageTiling::eLinear;
    } else {
        JAWS_FATAL1("DepthStencilAttachment is not supported for depth format");
    }

    auto depth_image =
        device->createImageUnique(vk::ImageCreateInfo{}
                                      .setImageType(vk::ImageType::e2D)
                                      .setFormat(DEPTH_FORMAT)
                                      .setExtent(vk::Extent3D(swapchain_extent.width, swapchain_extent.height, 1))
                                      .setMipLevels(1)
                                      .setArrayLayers(1)
                                      .setSamples(vk::SampleCountFlagBits::e1)
                                      .setTiling(tiling)
                                      .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

    auto memory_reqs = device->getImageMemoryRequirements(depth_image.get());
    auto depth_memory = device->allocateMemoryUnique(vk::MemoryAllocateInfo(
        memory_reqs.size,
        jaws::vulkan::FindMemoryTypeIndex(physical_device, memory_reqs, {vk::MemoryPropertyFlagBits::eDeviceLocal})));

    device->bindImageMemory(depth_image.get(), depth_memory.get(), 0);

    auto depth_view = device->createImageViewUnique(
        vk::ImageViewCreateInfo{}
            .setImage(depth_image.get())
            .setViewType(vk::ImageViewType::e2D)
            .setFormat(DEPTH_FORMAT)
            // VkComponentMapping specifies a remapping of color components (or of depth or stencil components after
            // they have been converted into color components):
            .setComponents(vk::ComponentMapping{}
                               .setR(vk::ComponentSwizzle::eR)
                               .setG(vk::ComponentSwizzle::eG)
                               .setB(vk::ComponentSwizzle::eB)
                               .setA(vk::ComponentSwizzle::eA))
            // VkImageSubresourceRange selects the set of mipmap levels and array layers to be accessible to the view:
            .setSubresourceRange(vk::ImageSubresourceRange{}
                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(1)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(1)));

    //===================================================================
    // Uniform buffer with view transform data

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    glm::mat4x4 model = glm::mat4(1.0f);
    // vulkan clip space has inverted y and half z!
    glm::mat4x4 clip =
        glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);

    // This is what we'll put into the buffer:
    glm::mat4x4 mvpc = clip * projection * view * model;

    auto uniform_data_buffer =
        device->createBufferUnique(vk::BufferCreateInfo({}, sizeof(mvpc), vk::BufferUsageFlagBits::eUniformBuffer));


    device->setDebugUtilsObjectNameEXT(
        vk::DebugUtilsObjectNameInfoEXT{}
            .setObjectType(vk::ObjectType::eUnknown)
            .setObjectHandle(reinterpret_cast<uint64_t>(&uniform_data_buffer.get()))
            .setPObjectName("jwXXX 1"),
        dynamic_dispatcher);

    auto uniform_buffer_memory_reqs = device->getBufferMemoryRequirements(uniform_data_buffer.get());
    auto uniform_data_memory = device->allocateMemoryUnique(vk::MemoryAllocateInfo(
        memory_reqs.size,
        jaws::vulkan::FindMemoryTypeIndex(
            physical_device,
            uniform_buffer_memory_reqs,
            {vk::MemoryPropertyFlagBits::eHostVisible, vk::MemoryPropertyFlagBits::eHostCoherent})));

    // Store data in the buffer memory
    int8_t* data_ptr =
        static_cast<int8_t*>(device->mapMemory(uniform_data_memory.get(), 0, uniform_buffer_memory_reqs.size));
    std::memcpy(data_ptr, &mvpc, sizeof(mvpc));
    device->unmapMemory(uniform_data_memory.get());

    device->bindBufferMemory(uniform_data_buffer.get(), uniform_data_memory.get(), 0);

    //=========================================================================
    // Descriptor set layout

    // This says
    // "in slot 0 (from shader pov) there's a uniform buffer with cardinality 1, accessible only from the vertex
    // shader."
    vk::DescriptorSetLayoutBinding descriptor_set_layout_binding(
        0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);

    // Create a DescriptorSetLayout collecting a number of bindings.
    auto descriptor_set_layout = device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo{}.setBindingCount(1).setPBindings(&descriptor_set_layout_binding));

    // Create a pipelineLayout using that DescriptorSetLayout
    auto pipeline_layout = device->createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo{}.setSetLayoutCount(1).setPSetLayouts(&descriptor_set_layout.get()));

    //=========================================================================
    // Descriptor pool and descriptor set

    // Create a descriptor pool
    vk::DescriptorPoolSize pool_size(vk::DescriptorType::eUniformBuffer, 1);
    auto descriptor_pool =
        device->createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{}
                                               .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
                                               .setMaxSets(1)
                                               .setPoolSizeCount(1)
                                               .setPPoolSizes(&pool_size));

    // Allocate a descriptor set and fill with our descriptor set layouts
    auto descriptor_sets = device->allocateDescriptorSetsUnique(vk::DescriptorSetAllocateInfo{}
                                                                    .setDescriptorPool(descriptor_pool.get())
                                                                    .setDescriptorSetCount(1)
                                                                    .setPSetLayouts(&descriptor_set_layout.get()));

    vk::DescriptorBufferInfo descriptor_buffer_info =
        vk::DescriptorBufferInfo{}.setBuffer(uniform_data_buffer.get()).setOffset(0).setRange(sizeof(glm::mat4x4));

    device->updateDescriptorSets(
        {vk::WriteDescriptorSet{}
             .setDstSet(descriptor_sets.front().get())
             .setDstBinding(0)
             .setDstArrayElement(0)
             .setDescriptorCount(1)
             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
             .setPBufferInfo(&descriptor_buffer_info)},
        {});

    //=========================================================================
    // Render subpass and pass.
    // The pass holds descriptions of all attachments used in its subpasses.
    // Each subpass references one or more of those attachments through its index.

    // Describe attachments for render subpass
    std::vector<vk::AttachmentDescription> attachments;
    attachments.push_back(vk::AttachmentDescription{}
                              .setFormat(surface_format.format)
                              .setSamples(vk::SampleCountFlagBits::e1)
                              .setLoadOp(vk::AttachmentLoadOp::eClear)
                              .setStoreOp(vk::AttachmentStoreOp::eStore)
                              .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                              .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                              .setInitialLayout(vk::ImageLayout::eUndefined)
                              .setFinalLayout(vk::ImageLayout::ePresentSrcKHR));
    /*
    attachments.push_back(vk::AttachmentDescription{}
        .setFormat(DEPTH_FORMAT)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal));
        */
    vk::AttachmentReference color_reference(0, vk::ImageLayout::eColorAttachmentOptimal);
    // vk::AttachmentReference depth_reference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    auto subpass = vk::SubpassDescription{}
                       .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                       .setColorAttachmentCount(1)
                       .setPColorAttachments(&color_reference);

    auto subpass_dep =
        vk::SubpassDependency{}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL) // TODO: really no C++ wrapper here?
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask({})
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);

    //.setPDepthStencilAttachment(&depth_reference);
    auto render_pass = device->createRenderPassUnique(vk::RenderPassCreateInfo{}
                                                          .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
                                                          .setPAttachments(attachments.data())
                                                          .setSubpassCount(1)
                                                          .setPSubpasses(&subpass)
                                                          .setDependencyCount(1)
                                                          .setPDependencies(&subpass_dep));


    //=========================================================================
    // Create frame buffers for the swap chain image views. This is required to be able
    // to render into them.

    std::vector<vk::UniqueFramebuffer> swap_chain_framebuffers;
    swap_chain_framebuffers.reserve(swap_chain_image_views.size());
    for (const auto& image_view : swap_chain_image_views) {
        auto framebuffer_ci = vk::FramebufferCreateInfo{}
                                  .setRenderPass(render_pass.get())
                                  .setAttachmentCount(1)
                                  .setPAttachments(&image_view.get())
                                  .setWidth(swap_chain_ci.imageExtent.width)
                                  .setHeight(swap_chain_ci.imageExtent.height)
                                  .setLayers(1);

        swap_chain_framebuffers.push_back(device->createFramebufferUnique(framebuffer_ci));
    };

    for (auto& fb : swap_chain_framebuffers) {
        device->setDebugUtilsObjectNameEXT(
            vk::DebugUtilsObjectNameInfoEXT{}
                .setObjectType(vk::ObjectType::eUnknown)
                .setObjectHandle(reinterpret_cast<uint64_t>(&fb.get()))
                .setPObjectName("jwXXX 2"),
            dynamic_dispatcher);
    }


    //=========================================================================
    // Create pipeline

    std::vector<vk::DynamicState> dynamic_state_enables;

    //-------------------------------------------------------------------------
    // Pipeline creation input 0: shader stages

    logger->info("executable path: {}", jaws::util::GetExecutablePath());


    auto BuildGlslShaderModule = [&device](const fs::path& file_path) {
        if (!fs::exists(file_path)) {
            logger->error("Shader file {} does not exist!", file_path);
            //return {};
        }
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs) {
            logger->error("Shader file {} could not be opened!", file_path);
            //return {};
        }
        std::string code((std::istreambuf_iterator<char>(ifs)), {});
        ifs.close();

        // Why I'm doing this? Because the interface forces me to pass something, and the only thing I could
        // always pass is infer_from_source, but I don't want to force the pragma everywhere.
        // Where it makes sense, I'm keeping this compatible with
        // https://github.com/google/shaderc/tree/master/glslc#311-shader-stage-specification
        shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source;
        std::string extension = file_path.extension().string();
        std::string lower_extension = extension;
        std::transform(lower_extension.begin(), lower_extension.end(), lower_extension.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        if (lower_extension == ".frag") {
            shader_kind = shaderc_glsl_default_fragment_shader;
        } else if (lower_extension == ".vert") {
            shader_kind = shaderc_glsl_default_vertex_shader;
        } else if (lower_extension == ".tesc") {
            shader_kind = shaderc_glsl_default_tess_control_shader;
        } else if (lower_extension == ".tese") {
            shader_kind = shaderc_glsl_default_tess_evaluation_shader;
        } else if (lower_extension == ".geom") {
            shader_kind = shaderc_glsl_default_geometry_shader;
        } else if (lower_extension == ".comp") {
            shader_kind = shaderc_glsl_default_compute_shader;
        }
        // TODO: Might wanna extend this.

        shaderc::Compiler compiler;
        shaderc::CompileOptions compile_options;

        compile_options.SetIncluder(std::make_unique<MyIncludeHandler>());

        auto compile_result = compiler.CompileGlslToSpv(
            code.c_str(),
            code.size(),
            shader_kind,
            file_path.filename()
                .string()
                .c_str(), // file name for error msgs, doesn't have to be a valid path or filename
            compile_options);
        shaderc_compilation_status status = compile_result.GetCompilationStatus();
        if (status != shaderc_compilation_status_success) {
            logger->error(
                "Shader compilation error ({}): {}",
                jaws::vulkan::ToString(status),
                compile_result.GetErrorMessage());
            JAWS_FATAL1("Shader compilation failed");
        } else {
            logger->info(
                "Successfully compiled shader ({} errors, {} warnings)",
                compile_result.GetNumErrors(),
                compile_result.GetNumWarnings());
        }
        return device->createShaderModuleUnique(
            vk::ShaderModuleCreateInfo{}
                .setPCode(compile_result.cbegin())
                .setCodeSize(sizeof(uint32_t) * std::distance(compile_result.cbegin(), compile_result.cend())));
    };

    const fs::path root_dir = build_info::PROJECT_SOURCE_DIR;
    auto shader_module_vs = BuildGlslShaderModule(root_dir / "shader.vert");
    auto shader_module_fs = BuildGlslShaderModule(root_dir / "shader.frag");

    jaws::util::FileObserver file_observer;
    file_observer.AddObservedFile(root_dir / "shader.vert");
    file_observer.AddObservedFile(root_dir / "shader.frag");

    std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stages = {

        // Vertex shader
        vk::PipelineShaderStageCreateInfo{}
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(shader_module_vs.get())
            .setPName("main"),

        // Fragment shader
        vk::PipelineShaderStageCreateInfo{}
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(shader_module_fs.get())
            .setPName("main"),
    };

    //-------------------------------------------------------------------------
    // Pipeline creation input 1: vertex input state

    auto pipeline_vertex_input_ci = vk::PipelineVertexInputStateCreateInfo{};

    //-------------------------------------------------------------------------
    // Pipeline creation input 2: input assembly state

    auto pipeline_ia_state_ci = vk::PipelineInputAssemblyStateCreateInfo{}
                                    .setTopology(vk::PrimitiveTopology::eTriangleList)
                                    .setPrimitiveRestartEnable(false);

    //-------------------------------------------------------------------------
    // Pipeline creation input 3: tesselation state

    // -- not used in this example --

    //-------------------------------------------------------------------------
    // Pipeline creation input 4: viewport state

    std::array<vk::Viewport, 1> viewports = {vk::Viewport{}
                                                 .setX(0.f)
                                                 .setY(0.f)
                                                 .setWidth(static_cast<float>(swapchain_extent.width))
                                                 .setHeight(static_cast<float>(swapchain_extent.height))
                                                 .setMinDepth(0.f)
                                                 .setMaxDepth(1.f)};
    std::array<vk::Rect2D, 1> scissor_rects = {vk::Rect2D({0, 0}, swapchain_extent)};

    auto pipeline_viewport_state_ci = vk::PipelineViewportStateCreateInfo{}
                                          .setViewportCount(static_cast<uint32_t>(viewports.size()))
                                          .setPViewports(viewports.data())
                                          .setScissorCount(static_cast<uint32_t>(scissor_rects.size()))
                                          .setPScissors(scissor_rects.data());

    // dynamic_state_enables.push_back(vk::DynamicState::eViewport);

    //-------------------------------------------------------------------------
    // Pipeline creation input 5: rasterization state

    auto pipeline_rasterization_state_ci = vk::PipelineRasterizationStateCreateInfo{}
                                               .setPolygonMode(vk::PolygonMode::eFill)
                                               .setCullMode(vk::CullModeFlagBits::eBack)
                                               //.setCullMode(vk::CullModeFlagBits::eNone)
                                               .setFrontFace(vk::FrontFace::eClockwise)
                                               .setDepthClampEnable(false)
                                               .setRasterizerDiscardEnable(false)
                                               .setDepthBiasEnable(false)
                                               .setLineWidth(1.0f);

    //-------------------------------------------------------------------------
    // Pipeline creation input 6: multisample state

    auto pipeline_multisample_state_ci = vk::PipelineMultisampleStateCreateInfo{}
                                             .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                                             .setSampleShadingEnable(false);

    //-------------------------------------------------------------------------
    // Pipeline creation input 7: depth/stencil state

    auto pipeline_depth_stencil_state_ci = vk::PipelineDepthStencilStateCreateInfo{}
                                               .setDepthTestEnable(true)
                                               .setDepthWriteEnable(true)
                                               .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                                               .setDepthBoundsTestEnable(false)
                                               .setStencilTestEnable(false);
    pipeline_depth_stencil_state_ci.back.setFailOp(vk::StencilOp::eKeep)
        .setPassOp(vk::StencilOp::eKeep)
        .setCompareOp(vk::CompareOp::eAlways);
    pipeline_depth_stencil_state_ci.front = pipeline_depth_stencil_state_ci.back;

    //-------------------------------------------------------------------------
    // Pipeline creation input 8: color blend state

    std::vector<vk::PipelineColorBlendAttachmentState> blend_attachment_states = {
        vk::PipelineColorBlendAttachmentState{}
            .setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA)
            .setBlendEnable(false)};

    auto pipeline_color_blend_state_ci = vk::PipelineColorBlendStateCreateInfo{}
                                             .setAttachmentCount(static_cast<uint32_t>(blend_attachment_states.size()))
                                             .setPAttachments(blend_attachment_states.data())
                                             .setBlendConstants({0}) // default anyway
                                             .setLogicOpEnable(false)
                                             .setLogicOp(vk::LogicOp::eCopy) // default is eClear, but logic is disabled
        ;

    //-------------------------------------------------------------------------
    // Pipeline creation input 9: dynamic state

    auto pipeline_dynamic_state_ci = vk::PipelineDynamicStateCreateInfo{}
                                         .setDynamicStateCount(static_cast<uint32_t>(dynamic_state_enables.size()))
                                         .setPDynamicStates(dynamic_state_enables.data());

    //-------------------------------------------------------------------------
    // Actually create pipeline real soon

    auto graphics_pipeline_ci = vk::GraphicsPipelineCreateInfo{}
                                    .setLayout(*pipeline_layout)
                                    .setStageCount(static_cast<uint32_t>(pipeline_shader_stages.size()))
                                    .setPStages(pipeline_shader_stages.data())
                                    .setPDynamicState(&pipeline_dynamic_state_ci)
                                    .setRenderPass(*render_pass)
                                    .setSubpass(0)
                                    .setPVertexInputState(&pipeline_vertex_input_ci)
                                    .setPInputAssemblyState(&pipeline_ia_state_ci)
                                    .setPRasterizationState(&pipeline_rasterization_state_ci)
                                    .setPColorBlendState(&pipeline_color_blend_state_ci)
                                    .setPViewportState(&pipeline_viewport_state_ci)
                                    .setPDepthStencilState(&pipeline_depth_stencil_state_ci)
                                    .setPMultisampleState(&pipeline_multisample_state_ci);

    auto pipeline_cache =
        device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo{}.setInitialDataSize(0).setPInitialData(nullptr));

    std::vector<vk::UniquePipeline> graphics_pipeline =
        device->createGraphicsPipelinesUnique(pipeline_cache.get(), {graphics_pipeline_ci});

    pipeline_cache.reset();

    //=========================================================================
    // Query pool

    auto timestamp_query_pool = device->createQueryPoolUnique(
        vk::QueryPoolCreateInfo{}.setQueryType(vk::QueryType::eTimestamp).setQueryCount(100));

    //=========================================================================
    // Command pool and command buffers

    auto command_pool =
        device->createCommandPoolUnique(vk::CommandPoolCreateInfo{}.setQueueFamilyIndex(graphics_queue_family_index));

    std::vector<vk::UniqueCommandBuffer> command_buffers = device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo{}
            .setCommandPool(command_pool.get())
            .setCommandBufferCount(static_cast<uint32_t>(swap_chain_framebuffers.size()))
            .setLevel(vk::CommandBufferLevel::ePrimary));

    //-------------------------------------------------------------------------
    // A command buffer for each swap chain framebuffer:

    for (size_t i = 0; i < command_buffers.size(); ++i) {
        auto& cb = command_buffers[i];

        cb->begin(vk::CommandBufferBeginInfo{});

        // Might not want to do this every frame, but then we'd need different
        // command buffers... Not sure.
        cb->resetQueryPool(timestamp_query_pool.get(), 0, 100);

        //-------------------------------------------------------------------------
        // Timestamp before aach render pass

        cb->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_query_pool.get(), 0);

        //-------------------------------------------------------------------------
        // Command: clear and begin render pass

        const std::array<vk::ClearValue, 3> clear_values = {
            vk::ClearValue{}.setColor(std::array<float, 4>{1.0f, 0.0f, 0.0f, 1.0f}),
            vk::ClearValue{}.setColor(std::array<float, 4>{0.0f, 1.0f, 0.0f, 1.0f}),
            vk::ClearValue{}.setColor(std::array<float, 4>{0.0f, 0.0f, 1.0f, 1.0f}),
        };
        // std::array<float, 4> clear_color{0.f, 1.f, 0.f, 1.f};
        auto clear_value = clear_values[i % clear_values.size()];
        cb->beginRenderPass(
            vk::RenderPassBeginInfo{}
                .setFramebuffer(swap_chain_framebuffers[i].get())
                .setRenderPass(render_pass.get())
                .setRenderArea(vk::Rect2D({0, 0}, swapchain_extent))
                .setClearValueCount(1)
                .setPClearValues(&clear_value),
            vk::SubpassContents::eInline);

        //-------------------------------------------------------------------------
        // Command: bind pipeline

        cb->bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline[0].get());

        //-------------------------------------------------------------------------
        // Command: draw

        cb->draw(3 /*1000 * 1000*/, 1, 0, 0);

        //-------------------------------------------------------------------------

        cb->endRenderPass();

        cb->writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, timestamp_query_pool.get(), 1);
        cb->end();
    }

    //=========================================================================
    // The render loop

    // The semaphores implement GPU-GPU synchronization.
    // The GPU (driver) does the signaling and waiting.
    std::array<vk::UniqueSemaphore, MAX_FRAMES_IN_FLIGHT> image_available_semaphores;
    std::array<vk::UniqueSemaphore, MAX_FRAMES_IN_FLIGHT> render_finished_semaphores;

    // The fences implement CPU-GPU synchronization.
    // GPU signals, CPU waits and resets.
    std::array<vk::UniqueFence, MAX_FRAMES_IN_FLIGHT> in_flight_fences;

    for (auto& s : image_available_semaphores) { s = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{}); }
    for (auto& s : render_finished_semaphores) { s = device->createSemaphoreUnique(vk::SemaphoreCreateInfo{}); }
    for (auto& f : in_flight_fences) {
        f = device->createFenceUnique(vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled));
    }

    // HWND hwnd = glfwGetWin32Window(window);
    // HDC hdc = GetDC(hwnd);

    uint64_t global_frame = 0;
    while (!glfwWindowShouldClose(window)) {
        const size_t mod_frame_in_flight = global_frame % MAX_FRAMES_IN_FLIGHT;

        for (const auto changed_file : file_observer.Poll()) {
            logger->info("!!! file changed: {} {}", changed_file.handle, changed_file.canon_path);
        }

        glfwPollEvents();

        // This makes the current thread wait if MAX_FRAMES_IN_FLIGHT are actually still in flight (have not
        // finished).
        device->waitForFences(
            {in_flight_fences[mod_frame_in_flight].get()}, true, std::numeric_limits<uint64_t>::max());
        device->resetFences({in_flight_fences[mod_frame_in_flight].get()});

        uint32_t curr_swapchain_image_index = device
                                                  ->acquireNextImageKHR(
                                                      swap_chain.get(),
                                                      std::numeric_limits<uint64_t>::max(),
                                                      image_available_semaphores[mod_frame_in_flight].get(),
                                                      vk::Fence{})
                                                  .value;
        // logger->info("frame {}: curr_swapchain_image_index = {}", frame_number, curr_swapchain_image_index);

        // For each semaphore: which stages wait for it?
        const vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        std::array<vk::Semaphore, 1> wait_semaphores = {image_available_semaphores[mod_frame_in_flight].get()};
        std::array<vk::Semaphore, 1> signal_semaphores = {render_finished_semaphores[mod_frame_in_flight].get()};

        auto submit_info = vk::SubmitInfo{}
                               .setWaitSemaphoreCount(static_cast<uint32_t>(wait_semaphores.size()))
                               .setPWaitSemaphores(wait_semaphores.data()) // wait for these semaphores...
                               .setPWaitDstStageMask(&wait_stages) //         ...in these stages
                               .setCommandBufferCount(1)
                               .setPCommandBuffers(&command_buffers[curr_swapchain_image_index].get())
                               .setSignalSemaphoreCount(static_cast<uint32_t>(
                                   signal_semaphores.size())) // After finishing execution, signal these semaphores.
                               .setPSignalSemaphores(signal_semaphores.data());

        graphics_queue.submit({submit_info}, in_flight_fences[mod_frame_in_flight].get());


        auto present_info = vk::PresentInfoKHR{}
                                .setWaitSemaphoreCount(static_cast<uint32_t>(signal_semaphores.size()))
                                .setPWaitSemaphores(signal_semaphores.data())
                                .setSwapchainCount(1)
                                .setPSwapchains(&swap_chain.get())
                                .setPImageIndices(&curr_swapchain_image_index);
        present_queue.presentKHR(present_info);

        // Overkill synchronization, but I don't have a grip on this yet.
        device->waitIdle();
        std::array<uint64_t, 2> timestamp_query_result_data = {0};
        device->getQueryPoolResults(
            timestamp_query_pool.get(),
            0,
            2,
            vk::ArrayProxy<uint64_t>(timestamp_query_result_data),
            0,
            vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64);

        // Narrowing should be okay here since we subtract first.
        auto timestamp_delta_ns = (timestamp_query_result_data[1] - timestamp_query_result_data[0])
                                  * physical_device.getProperties().limits.timestampPeriod;
        //logger->info("last frame took {}us", timestamp_delta_ns / 1000);

        ++global_frame;
    }

    // Wait until the device has finished any operations.
    // Otherwise we might delete resources while they are still in use.
    device->waitIdle();

    glfwTerminate();
}

int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, [](auto, auto) {
        // Piggypacking onto one of jaws' logger categories here.
        logger = jaws::GetLoggerPtr(jaws::Category::General);

        // We could add that catch block into jaws::util::Main,
        // but I don't want vulkan dependencies there. Nothing
        // wrong with deeper nested try-catch blocks where we
        // have more domain knowledge like "vulkan hpp might throw!".
        try {
            run();
            return 0;
        } catch (const vk::SystemError& e) {
            JAWS_FATAL2(jaws::FatalError::UncaughtException, std::string("vk::SystemError: ") + e.what());
            return -3;
        }
    });
}
#else

int main(int argc, char **argv)
{
    return 0;
}

#endif

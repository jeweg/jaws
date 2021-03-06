#include "surface_context.hpp"
#include "jaws/vulkan/utils.hpp"
#include "jaws/assume.hpp"
#include "jaws/fatal.hpp"
#include "jaws/logging.hpp"
#include "jaws/util/misc.hpp"

namespace {

static jaws::LoggerPtr logger;

}

namespace jaws::vulkan {


SurfaceContext::SurfaceContext(GlobalContext& global_context, VkSurfaceKHR surface) :
    _global_context(global_context),
    _surface(surface)
{
    if (!logger) { logger = jaws::GetLoggerPtr(jaws::Category::General); }
    const auto physical_device = _global_context.GetPhysicalDevice();

    VkResult result = {};

    //=========================================================================
    // Query surface capabilities

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, _surface, &_surface_caps);
    Handle(result);

    //=========================================================================
    // Query supported surface formats and select one.
    // We use a system that assigns a weight to each format it finds and then
    // chooses the one with the highest weight.

    VkPhysicalDeviceSurfaceInfo2KHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    surface_info.surface = _surface;

    uint32_t count;
    std::vector<VkSurfaceFormat2KHR> surface_formats;
    result = vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &count, nullptr);
    Handle(result);
    if (result == VK_SUCCESS && count > 0) {
        surface_formats.resize(count);
        result = vkGetPhysicalDeviceSurfaceFormats2KHR(physical_device, &surface_info, &count, surface_formats.data());
        Handle(result);
    }
    JAWS_ASSUME(!surface_formats.empty());

    _surface_format = jaws::vulkan::ChooseBest(surface_formats, [](auto sf) {
        if (sf.surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && sf.surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
            return 1000;
        } else if (sf.surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && sf.surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
            return 100;
        } else if (sf.surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
            return 50;
        }
        return 0;
    });
}


SurfaceContext::~SurfaceContext() {}


void SurfaceContext::CreateSwapChain(uint32_t width, uint32_t height)
{
    const auto physical_device = _global_context.GetPhysicalDevice();
    VkResult result = {};

    //-------------------------------------------------------------------------
    // Extent

    VkExtent2D extent;
    if (_surface_caps.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        // See
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/eaf09ee61e6cb964cf72e0023cd30777f8e3f9fe/samples/05_InitSwapchain/05_InitSwapchain.cpp#L95
        // If the surface size is undefined, the size is set to the size of the images requested.
        extent.width = jaws::util::Clamp(width, _surface_caps.minImageExtent.width, _surface_caps.maxImageExtent.width);
        extent.height =
            jaws::util::Clamp(height, _surface_caps.minImageExtent.height, _surface_caps.maxImageExtent.height);
    } else {
        // If the surface size is defined, the swap chain size must match
        extent = _surface_caps.currentExtent;
    }

    //-------------------------------------------------------------------------
    // Present mode

    VkPhysicalDeviceSurfaceInfo2KHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    surface_info.surface = _surface;

    uint32_t count;
    std::vector<VkPresentModeKHR> present_modes;
    result = vkGetPhysicalDeviceSurfacePresentModes2EXT(physical_device, &surface_info, &count, nullptr);
    Handle(result);
    if (result == VK_SUCCESS && count > 0) {
        present_modes.resize(count);
        result =
            vkGetPhysicalDeviceSurfacePresentModes2EXT(physical_device, &surface_info, &count, present_modes.data());
        Handle(result);
    }
    JAWS_ASSUME(!present_modes.empty());

    auto present_mode = jaws::vulkan::ChooseBest(present_modes, [&](auto pm) {
        // We assign some default weights first to have a reasonable fallback order
        // and then assign a much higher weight to the mode or modes that suit the
        // requested behavior obviously best.
        int weight = 0;
        if (pm == VK_PRESENT_MODE_FIFO_KHR) {
            weight = 10;
        } else if (pm == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            weight = 9;
        } else if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
            weight = 8;
        }

        // This behavior weighs vsync matching higher than frame dropping behavior.

        if (_enable_vsync && _allow_frame_drops) {
            if (VK_PRESENT_MODE_MAILBOX_KHR == pm) {
                weight = 1000;
            } else if (VK_PRESENT_MODE_FIFO_KHR == pm) {
                weight = 100;
            }
        } else if (!_enable_vsync && _allow_frame_drops) {
            if (VK_PRESENT_MODE_IMMEDIATE_KHR == pm) {
                weight = 1000;
            } else if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == pm) {
                weight = 100;
            }
        } else if (_enable_vsync && !_allow_frame_drops) {
            if (VK_PRESENT_MODE_FIFO_KHR == pm) {
                weight = 1000;
            } else if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == pm) {
                weight = 100;
            }
        } else if (!_enable_vsync && !_allow_frame_drops) {
            if (VK_PRESENT_MODE_IMMEDIATE_KHR == pm) {
                weight = 1000;
            } else if (VK_PRESENT_MODE_FIFO_RELAXED_KHR == pm) {
                weight = 100;
            }
        }
        return weight;
    });

    //-------------------------------------------------------------------------
    // Transform

    // Use identity if available, otherwise current.
    VkSurfaceTransformFlagBitsKHR pre_transform =
        (_surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR :
            _surface_caps.currentTransform;

    //-------------------------------------------------------------------------
    // Image count

    // start w/ minimum count required to function
    uint32_t swapchain_image_count = _surface_caps.minImageCount;
    // "it is recommended to request at least one more image than the minimum"
    swapchain_image_count += 1;
    if (_surface_caps.maxImageCount > 0) {
        // Cap the value
        swapchain_image_count = std::min(_surface_caps.maxImageCount, swapchain_image_count);
    }

    //-------------------------------------------------------------------------
    // Swap chain

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = _surface;
    swapchain_ci.minImageCount = swapchain_image_count;
    swapchain_ci.imageFormat = _surface_format.surfaceFormat.format;
    swapchain_ci.imageColorSpace = _surface_format.surfaceFormat.colorSpace;
    swapchain_ci.imageExtent = extent;
    swapchain_ci.imageArrayLayers = 1; // No stereo
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.preTransform = pre_transform;
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = true;
    swapchain_ci.oldSwapchain = nullptr; // https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation

    std::array<uint32_t, 2> queue_family_indices = {_global_context.GetGraphicsQueueFamilyIndex(),
                                                    _global_context.GetPresentQueueFamilyIndex()};
    if (queue_family_indices[0] != queue_family_indices[1]) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
        swapchain_ci.pQueueFamilyIndices = queue_family_indices.data();
    }
    result = vkCreateSwapchainKHR(_global_context.GetDevice(), &swapchain_ci, nullptr, &_swapchain);
    Handle(result);
    logger->info("swapchain: {}", (void*)_swapchain);
};

} // namespace jaws::vulkan

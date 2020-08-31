#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vulkan/window_context.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/utils.hpp"
#include "jaws/util/misc.hpp"
#include "to_string.hpp"

namespace jaws::vulkan {

WindowContext::~WindowContext()
{
    destroy();
}

void WindowContext::create(Device *device, const CreateInfo &ci)
{
    auto &logger = get_logger(jaws::Category::Vulkan);

    JAWS_ASSUME(device);
    JAWS_ASSUME(ci.surface);

    _device = device;
    _surface = ci.surface;
    _enable_vsync = ci.enable_vsync;
    _allow_frame_drops = ci.allow_frame_drops;

    // logger.info("surface capabilities: {}", surface_capabilities_to_string(*_device, _surface));

    //=========================================================================
    // Query supported surface formats and select one.
    // We use a system that assigns a weight to each format it finds and then
    // chooses the one with the highest weight.

    // Not sure how common this extension is. Could add fallback.
    JAWS_ASSUME(_device->get_context()->get_instance_extensions().contains("VK_KHR_get_surface_capabilities2"));

    VkPhysicalDeviceSurfaceInfo2KHR surface_info = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surface_info.surface = _surface;

    // SFI: support
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSurfaceFullScreenExclusiveWin32InfoEXT.html
    // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSurfaceFullScreenExclusiveInfoEXT.html

    auto surface_formats = enumerated<VkSurfaceFormat2KHR>(
        vkGetPhysicalDeviceSurfaceFormats2KHR,
        {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR},
        _device->get_physical_device(),
        &surface_info);
    JAWS_ASSUME(!surface_formats.empty());

    /*
    logger.info("surface formats:");
    for (auto f : surface_formats) {
        logger.info("    format: {}, {}", to_string(f.surfaceFormat.format), to_string(f.surfaceFormat.colorSpace));
    }
    */
    _surface_format = choose_best(surface_formats, [](auto sf) {
        if (sf.surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            && sf.surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB) {
            return 1000;
        } else if (
            sf.surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            && sf.surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
            return 100;
        } else if (sf.surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return 50;
        }
        return 0;
    });

    /*
    logger.info(
        "selected surface format: {}, {}",
        to_string(_surface_format.surfaceFormat.format),
        to_string(_surface_format.surfaceFormat.colorSpace));
        */

    const auto physical_device = _device->get_physical_device();

    _surface_capabilities = {};
    VkResult result =
        vkGetPhysicalDeviceSurfaceCapabilities2KHR(physical_device, &surface_info, &_surface_capabilities);
    JAWS_VK_HANDLE_FATAL(result);
}

void WindowContext::destroy()
{
    if (_surface) {
        vkDestroySurfaceKHR(_device->get_instance(), _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
}


void WindowContext::create_swap_chain(uint32_t width, uint32_t height)
{
    auto &logger = get_logger(Category::Vulkan);

    JAWS_ASSUME(_device->get_extensions().contains(VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    JAWS_ASSUME(_device->get_context()->get_instance_extensions().contains("VK_KHR_surface"));

    const auto physical_device = _device->get_physical_device();
    VkResult result = {};

    const auto &surfaceCapabilities = _surface_capabilities.surfaceCapabilities;


    //-------------------------------------------------------------------------
    // Extent

    VkExtent2D extent;
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        // See
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/eaf09ee61e6cb964cf72e0023cd30777f8e3f9fe/samples/05_InitSwapchain/05_InitSwapchain.cpp#L95
        // If the surface size is undefined, the size is set to the size of the images requested.
        extent.width = jaws::util::clamp(
            width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height = jaws::util::clamp(
            height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    } else {
        // If the surface size is defined, the swap chain size must match
        extent = surfaceCapabilities.currentExtent;
    }

    //-------------------------------------------------------------------------
    // Present mode

#if 0
    // TODO: this is only available with VK_EXT_full_screen_exclusive which seems to be Win32-only atm.
    VkPhysicalDeviceSurfaceInfo2KHR surface_info = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surface_info.surface = _surface;
    //...
    result = vkGetPhysicalDeviceSurfacePresentModes2EXT(physical_device, &surface_info, &count, nullptr);
    typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes);
    ...
#endif

    auto present_modes =
        enumerated<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, {}, physical_device, _surface);
    JAWS_ASSUME(!present_modes.empty());

    auto present_mode = jaws::vulkan::choose_best(present_modes, [&](auto pm) {
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
        (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ?
            VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR :
            surfaceCapabilities.currentTransform;

    //-------------------------------------------------------------------------
    // Image count

    // start w/ minimum count required to function
    uint32_t swapchain_image_count = surfaceCapabilities.minImageCount;
    // "it is recommended to request at least one more image than the minimum"
    swapchain_image_count += 1;
    if (surfaceCapabilities.maxImageCount > 0) {
        // Cap the value
        swapchain_image_count = std::min(surfaceCapabilities.maxImageCount, swapchain_image_count);
    }

    //-------------------------------------------------------------------------
    // Swap chain

    VkSwapchainCreateInfoKHR swapchain_ci = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_ci.surface = _surface;
    swapchain_ci.minImageCount = swapchain_image_count;
    swapchain_ci.imageFormat = _surface_format.surfaceFormat.format;
    swapchain_ci.imageColorSpace = _surface_format.surfaceFormat.colorSpace;
    swapchain_ci.imageExtent = extent;
    swapchain_ci.imageArrayLayers = 1; // No stereo
    swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.preTransform = pre_transform;
    // For blending with other windows in the window system.
    // We probably almost always don't want this and hence specify opaque here.
    swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode = present_mode;
    swapchain_ci.clipped = true;
    swapchain_ci.oldSwapchain = nullptr; // https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation

    std::array<uint32_t, 2> queue_family_indices = {
        _device->get_queue_family(Device::Queue::Graphics), _device->get_queue_family(Device::Queue::Present)};
    if (queue_family_indices[0] != queue_family_indices[1]) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
        swapchain_ci.pQueueFamilyIndices = queue_family_indices.data();
    }
    result = vkCreateSwapchainKHR(_device->get_device(), &swapchain_ci, nullptr, &_swapchain);
    JAWS_VK_HANDLE_FATAL(result);
    logger.info("swapchain: {}", (void *)_swapchain);

    //-------------------------------------------------------------------------
    // The swap chain images must be recreated whenever the swap chain changes.
    // Other resources might depend on it (render passes might depend on the swapchain format,
    // pipelines might depend on viewport and scissor size (although they could be dynamic state),
    // but that's not a given. hashing could probably be used to solve this.

    std::vector<VkImage> swapchain_images =
        enumerated<VkImage>(vkGetSwapchainImagesKHR, nullptr, _device->get_device(), _swapchain);
    logger.info("retrieved {} swapchain images", swapchain_images.size());

    //-------------------------------------------------------------------------
    // Create image views for the swap chain images so that we can eventually render into them.

    std::vector<VkImageView> swapchain_image_views(swapchain_images.size());
    for (size_t i = 0; i < swapchain_images.size(); ++i) {
        VkImageSubresourceRange subresource_range = {};
        subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource_range.baseMipLevel = 0;
        subresource_range.levelCount = 1;
        subresource_range.baseArrayLayer = 0;
        subresource_range.layerCount = 1;

        VkImageViewCreateInfo ci = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ci.image = swapchain_images[i];
        ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ci.subresourceRange = subresource_range;
        ci.format = _surface_format.surfaceFormat.format;

        result = vkCreateImageView(_device->get_device(), &ci, nullptr, &swapchain_image_views[i]);
        JAWS_VK_HANDLE_FATAL(result);
    }

    //-------------------------------------------------------------------------
    // Create depth buffer

    constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D24_UNORM_S8_UINT;

    {
        VkFormatProperties2 props2;
        vkGetPhysicalDeviceFormatProperties2(_device->get_physical_device(), DEPTH_FORMAT, &props2);
        const VkFormatProperties &props = props2.formatProperties;

        // TODO: See discussion here:
        // https://www.reddit.com/r/vulkan/comments/71k4gy/why_is_vk_image_tiling_linear_so_limited/
        // OTOH,
        // https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/06_InitDepthBuffer/06_InitDepthBuffer.cpp
        // prefers linear tiling. Not sure.
        VkImageTiling tiling;
        if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_OPTIMAL;
        } else if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_LINEAR;
        } else {
            JAWS_FATAL1("DepthStencilAttachment is not supported for depth format");
        }


        VkImageCreateInfo ci = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = DEPTH_FORMAT;
        ci.extent.width = extent.width;
        ci.extent.height = extent.height;
        std::memcpy(&ci.extent, &extent, sizeof(extent));
        ci.mipLevels = 1;
        ci.arrayLayers = 1;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.tiling = tiling;
        ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        _depth_image = _device->create_image(ci, VMA_MEMORY_USAGE_GPU_ONLY);

#if 0
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
    }
};

}

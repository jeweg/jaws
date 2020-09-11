#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/misc.hpp"

namespace jaws::vulkan {

class Device;

// surface, swap chain.
// not the actual platform thingie that represents the window.
// we want to leave to glfw or similar for now.
// lifetime must be stricly inside its context's lifetime.
class JAWS_API WindowContext
{
public:
    WindowContext() = default;
    WindowContext(const WindowContext &) = delete;
    WindowContext &operator=(const WindowContext &) = delete;
    ~WindowContext();

    struct CreateInfo
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        bool enable_vsync = true;
        bool allow_frame_drops = false;

        CreateInfo &set_surface(VkSurfaceKHR v)
        {
            surface = v;
            return *this;
        }
        CreateInfo &set_enable_vsync(VkSurfaceKHR v)
        {
            enable_vsync = v;
            return *this;
        }
        CreateInfo &set_allow_frame_drops(VkSurfaceKHR v)
        {
            allow_frame_drops = v;
            return *this;
        }
    };

    void create(Device *device, const CreateInfo & = jaws::util::make_default<CreateInfo>());
    void destroy();

    VkFormat get_surface_format() const;
    VkExtent2D get_extent() const { return _current_extent; }
    uint32_t get_width() const { return _current_extent.width; }
    uint32_t get_height() const { return _current_extent.height; }

private:
public: // remove later on
    void create_swap_chain(uint32_t width, uint32_t height);

private:
public: // remove later on
    Device *_device = nullptr;
    VkSurfaceKHR _surface;
    bool _enable_vsync;
    bool _allow_frame_drops;

    VkSurfaceCapabilities2KHR _surface_capabilities;
    VkSurfaceCapabilitiesFullScreenExclusiveEXT _surface_capabilites_fse;

    VkSurfaceFormat2KHR _surface_format;

    VkSwapchainKHR _swapchain;
    std::vector<VkImageView> _swapchain_image_views;

    VkExtent2D _current_extent;

    Image _depth_image;
    VkImageView _depth_view;
};

}

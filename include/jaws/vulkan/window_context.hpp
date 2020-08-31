#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/image.hpp"
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

private:
public: // remove later on
    void create_swap_chain(uint32_t width, uint32_t height);

private:
    Device *_device = nullptr;
    VkSurfaceKHR _surface;
    bool _enable_vsync;
    bool _allow_frame_drops;
    VkSurfaceCapabilities2KHR _surface_capabilities;
    VkSurfaceFormat2KHR _surface_format;
    VkSwapchainKHR _swapchain;

    Image _depth_image;

    // The idea is to generate a hash of the params and save that here.
    // check it to test whether to recreate the swap chain.
    size_t swapchain_parameter_hash = 0;
};

}; // namespace jaws::vulkan

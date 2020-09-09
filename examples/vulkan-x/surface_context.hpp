#pragma once

#include "global_context.hpp"

namespace jaws::vulkan {

class SurfaceContext
{
public:
    SurfaceContext(GlobalContext&, VkSurfaceKHR surface);
    ~SurfaceContext();


private:
    void CreateSwapChain(uint32_t width, uint32_t height);

private:
    // Params for the next CreateSwapChain:
    // maybe set via setters and pick up lazily when we need to render.

    bool _enable_vsync = true;
    bool _allow_frame_drops = false;

    // The idea is to generate a hash of the params and save that here.
    // check it to test whether to recreate the swap chain.
    size_t swapchain_parameter_hash = 0;

private:
    GlobalContext& _global_context;
    VkSurfaceKHR _surface;
    VkSurfaceCapabilitiesKHR _surface_caps;
    VkSurfaceFormat2KHR _surface_format;
    VkSwapchainKHR _swapchain;
};

}; // namespace jaws::vulkan

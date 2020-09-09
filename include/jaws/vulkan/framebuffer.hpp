#pragma once
#include "jaws/core.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/lru_cache.hpp"

namespace jaws::vulkan {

// TODO: a bit too low-level probably => err on the low-level side for now.
struct FramebufferCreateInfo
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 0;
    std::vector<VkImageView *> attachments;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkFramebufferCreateFlags flags = 0;

    friend bool operator==(const FramebufferCreateInfo &a, const FramebufferCreateInfo &b)
    {
        return a.width == b.width && a.height == b.height && a.layers == b.layers && a.render_pass == b.render_pass
               && a.attachments == b.attachments;
    }

    friend bool operator!=(const FramebufferCreateInfo &a, const FramebufferCreateInfo &b) { return !(a == b); }

    template <typename H>
    friend H AbslHashValue(H h, const FramebufferCreateInfo &v)
    {
        return H::combine(std::move(h), v.width, v.height, v.layers, v.attachments, v.render_pass);
    }
};

}

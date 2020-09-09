#pragma once
#include "jaws/core.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/framebuffer.hpp"
#include "jaws/util/lru_cache.hpp"

namespace jaws::vulkan {

class FramebufferCache final
{
public:
    FramebufferCache();
    ~FramebufferCache();

    bool create(Device *);
    void destroy();

    VkFramebuffer get_framebuffer(const FramebufferCreateInfo &);

private:
    void delete_framebuffer(const FramebufferCreateInfo &, VkFramebuffer fb);
    Device *_device = nullptr;
    util::LruCache<FramebufferCreateInfo, VkFramebuffer> _cache;
};

}

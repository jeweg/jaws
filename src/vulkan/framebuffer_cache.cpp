#include "framebuffer_cache.hpp"
#include "jaws/vulkan/device.hpp"

#include <functional>

namespace jaws::vulkan {

FramebufferCache::FramebufferCache() :
    _cache(std::bind(&FramebufferCache::delete_framebuffer, this, std::placeholders::_1, std::placeholders::_2), 0, 10)
{}


FramebufferCache::~FramebufferCache()
{
    destroy();
}


bool FramebufferCache::create(Device *device)
{
    JAWS_ASSUME(device);
    _device = device;
    return true;
}


void FramebufferCache::destroy()
{
    _cache.clear();
}


VkFramebuffer FramebufferCache::get_framebuffer(const FramebufferCreateInfo &ci)
{
    VkFramebuffer *fb = _cache.lookup(ci);
    if (fb) { return *fb; }
    VkFramebufferCreateInfo vci = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    vci.width = ci.width;
    vci.height = ci.height;
    vci.layers = ci.layers;
    vci.renderPass = ci.render_pass;
    vci.flags = ci.flags;
    vci.attachmentCount = static_cast<uint32_t>(ci.attachments.size());
    vci.pAttachments = *ci.attachments.data();

    VkFramebuffer output_fb;
    VkResult result = vkCreateFramebuffer(_device->vk_handle(), &vci, nullptr, &output_fb);
    JAWS_VK_HANDLE_FATAL(result);
    _cache.insert(ci, output_fb);
    return output_fb;
}


void FramebufferCache::delete_framebuffer(const FramebufferCreateInfo &, VkFramebuffer fb)
{
    auto logger = jaws::get_logger(jaws::Category::General);
    vkDestroyFramebuffer(_device->vk_handle(), fb, nullptr);
}


}

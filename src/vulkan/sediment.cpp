#include "pch.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/sediment.hpp"
#include "fossilize.hpp"

namespace jaws::vulkan {

Sediment::~Sediment()
{
    destroy();
}


void Sediment::create(Device* device)
{
    _device = device;
}


void Sediment::destroy() {}


VkShaderModule Sediment::lookup_or_create(const VkShaderModuleCreateInfo* ci, const Hash* ci_hash, Hash* out_hash)
{
    JAWS_ASSUME(ci || ci_hash);
    Hash hash = ci_hash ? *ci_hash : Fossilize::Hashing::compute_hash_shader_module(*ci);
    if (out_hash) { *out_hash = hash; }
    if (VkShaderModule* cached = _shader_modules.lookup(hash)) { return *cached; }
    VkShaderModule resource = VK_NULL_HANDLE;
    if (!ci) {
        VkResult result = _device->vk().vkCreateShaderModule(_device->get_device(), ci, nullptr, &resource);
        JAWS_VK_HANDLE_RETURN(result, VK_NULL_HANDLE);
        _shader_modules.insert(hash, resource);
    }
    return resource;
}


} // namespace jaws::vulkan

#include "pch.hpp"
#include "jaws/vulkan/shader_resource.hpp"
#include "jaws/vulkan/device.hpp"

namespace jaws::vulkan {

ShaderResource::ShaderResource(Device *device, VkShaderModule vk_shader_module) :
    _device(device), _vk_shader_module(vk_shader_module)
{
    JAWS_ASSUME(device);
}


ShaderResource::~ShaderResource()
{
    // TODO: queue for deletion
}


void ShaderResource::on_all_references_dropped()
{
    // We probably don't want to do anything special here.
    // The ShaderResource can remain cached for now.
}

}

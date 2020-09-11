#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/util/ref_ptr.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/vfs/path.hpp"

#include <vector>

namespace jaws::vulkan {

class ShaderResource final : public jaws::util::RefCounted<ShaderResource>
{
public:
    ShaderResource() = default;
    ~ShaderResource() noexcept;

    ShaderResource(ShaderResource &&) noexcept = default;

    VkShaderModule vk_handle() const { return _vk_shader_module; }

private:
    friend jaws::util::detail::RefCountedAccessor;
    friend class util::LruCache<ShaderCreateInfo, ShaderResource>;

    ShaderResource(Device *device, VkShaderModule vk_shader_module);

    void on_all_references_dropped() override;

    Device *_device = nullptr;
    VkShaderModule _vk_shader_module = VK_NULL_HANDLE;
    std::vector<FileWithFingerprint> involved_files;
};

}

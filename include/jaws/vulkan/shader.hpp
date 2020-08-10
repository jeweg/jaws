#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/ref_ptr.hpp"

#include <vector>
#include <utility>
#include <string>

namespace jaws::vulkan {

struct ShaderCreateInfo
{
    jaws::vfs::Path main_source_file;
    std::vector<jaws::vfs::Path> include_paths;
    std::vector<std::pair<std::string, std::string>> compile_definitions;
    std::string entry_point_name = "main";

    ShaderCreateInfo &set_main_source_file(jaws::vfs::Path v)
    {
        main_source_file = std::move(v);
        return *this;
    }
    ShaderCreateInfo &set_include_paths(std::vector<jaws::vfs::Path> v)
    {
        include_paths = std::move(v);
        return *this;
    }
    ShaderCreateInfo &set_compile_definitions(std::vector<std::pair<std::string, std::string>> v)
    {
        compile_definitions = std::move(v);
        return *this;
    }
    ShaderCreateInfo &set_entry_point_name(std::string v)
    {
        entry_point_name = std::move(v);
        return *this;
    }

    template <typename H>
    friend H AbslHashValue(H h, const ShaderCreateInfo &sd)
    {
        return H::combine(std::move(h), sd.main_source_file, sd.include_paths, sd.compile_definitions);
    }
};


class Shader : public jaws::util::RefCounted<Shader>
{
public:
    Shader(Device *device) : _device(device) { JAWS_ASSUME(device); }

private:
    Device *_device;
    VkShaderModule _shader_module = VK_NULL_HANDLE;
};

using ShaderPtr = jaws::util::ref_ptr<Shader>;

} // namespace jaws::vulkan

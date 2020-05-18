#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/ref_ptr.hpp"

namespace jaws::vulkan {

struct ShaderCreateInfo
{
    JAWS_NP_MEMBER2(jaws::vfs::Path, main_source_file);
    JAWS_NP_MEMBER2(std::vector<jaws::vfs::Path>, include_paths);
    JAWS_NP_MEMBER2((std::vector<std::pair<std::string, std::string>>), compile_definitions);
    JAWS_NP_MEMBER3(std::string, entry_point_name, "main");

    template <typename H>
    friend H AbslHashValue(H h, const ShaderCreateInfo& sd)
    {
        return H::combine(std::move(h), sd.main_source_file, sd.include_paths, sd.compile_definitions);
    }
};


class Shader : public jaws::util::RefCounted<Shader>
{
public:
    VkShaderModule _shader_module;
};

using ShaderPtr = jaws::util::ref_ptr<Shader>;

} // namespace jaws::vulkan

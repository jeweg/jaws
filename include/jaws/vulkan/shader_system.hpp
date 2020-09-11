#pragma once
#include "jaws/core.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vfs/path.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/vulkan/shader_resource.hpp"
#include "jaws/util/misc.hpp"
//#include "sediment.hpp"
#include <tuple>
#include <memory>
#include <vector>

namespace jaws::vulkan {

struct ShaderCreateInfo final
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

    friend bool operator==(const ShaderCreateInfo &a, const ShaderCreateInfo &b)
    {
        return a.main_source_file == b.main_source_file && a.entry_point_name == b.entry_point_name
               && a.include_paths == b.include_paths && a.compile_definitions == b.compile_definitions;
    }

    friend bool operator!=(const ShaderCreateInfo &a, const ShaderCreateInfo &b) { return !(a == b); }

    template <typename H>
    friend H AbslHashValue(H h, const ShaderCreateInfo &v)
    {
        return H::combine(std::move(h), v.main_source_file, v.include_paths, v.compile_definitions);
    }
};


class ShaderSystem final : jaws::util::NonCopyable
{
public:
    ShaderSystem();
    ~ShaderSystem();
    ShaderSystem(ShaderSystem &&) = default;
    ShaderSystem &operator=(ShaderSystem &&) = default;

    void create(Device *);
    void destroy();

    Shader get_shader(const ShaderCreateInfo &);

private:
    Device *_device = nullptr;
    VkValidationCacheEXT _validation_cache = VK_NULL_HANDLE;
    util::LruCache<ShaderCreateInfo, ShaderResource> _shader_cache;
};

}

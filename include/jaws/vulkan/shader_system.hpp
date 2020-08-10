#pragma once
#include "jaws/core.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vfs/path.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/shader.hpp"
#include "jaws/util/lru_cache.hpp"
//#include "sediment.hpp"
#include <tuple>

namespace jaws::vulkan {


class ShaderSystem
{
    /*
2. The shader system can be instructed to compile a single shader (i.e. a pipeline's particular stage) by
   specifying a vfs name and a shader source file.
   The type of shader can be specified or deduced from an extension (if any) or set with the code pragma. *preferably in
that order* It will compile the specified source files, resolving include files to possibly yield more files using the
originally specified vfs. It will store a list of file dependencies for this shader. Optionally and if supported, it
will set up automatic observation of all addressed files through the vfs interface. *triggering a refresh for a shader
module is always possibly, the file observation just adds an automatic path* The subsystem owns the returned shader
handles, and each handle has a hash value attached through which we will detect necessary pipeline rebuilds.
     */

public:
    using FileWithFingerprint = std::tuple<jaws::vfs::Path, size_t>;

    ShaderSystem();
    ~ShaderSystem();
    ShaderSystem(const ShaderSystem&) = delete;
    ShaderSystem& operator=(const ShaderSystem&) = delete;

    void create(Device*);
    void destroy();

    ShaderPtr get_shader(const ShaderCreateInfo&);

private:
    Device* _device = nullptr;
    VkValidationCacheEXT _validation_cache = VK_NULL_HANDLE;

    struct FullInfo
    {
        ShaderCreateInfo ci;
        std::vector<FileWithFingerprint> involved_files;
        // Sediment::Hash sediment_hash;
    };
    jaws::util::NoKeysStoredLruCache<ShaderCreateInfo, FullInfo> _full_info_cache;
};

} // namespace jaws::vulkan

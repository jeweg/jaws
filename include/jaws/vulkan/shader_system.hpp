#pragma once
#include "jaws/core.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vfs/path.hpp"
#include "jaws/vulkan/fwd.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/shader.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/util/misc.hpp"
//#include "sediment.hpp"
#include <tuple>
#include <memory>
#include <vector>

namespace jaws::vulkan {

class ShaderSystem final
{
    /*

    create_info (lists the file names and everything needed to resolve and interpret the contents, but NOT the file
    contents)
        -> full info (create_info + file fingerprints)
            -> shader resource, relevant reflection output


    */

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
    ShaderSystem(const ShaderSystem &) = delete;
    ShaderSystem &operator=(const ShaderSystem &) = delete;

    void create(Device *);
    void destroy();

    Shader get_shader(const ShaderCreateInfo &);

private:
    friend struct ShaderResource;
    void shader_got_unreferenced(ShaderResource *)
    {
        // schedule for actual deletion of vulkan resources. not within a frame, though!
    }

private:
    Device *_device = nullptr;
    VkValidationCacheEXT _validation_cache = VK_NULL_HANDLE;

    struct CachedShaderInformation : util::MovableOnly
    {
        std::vector<FileWithFingerprint> involved_files;
        ShaderResource shader_resource;
    };

    util::LruCache<ShaderCreateInfo, CachedShaderInformation> _shader_cache;
};

}

#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/util/lru_cache.hpp"
#include "jaws/util/hashing.hpp"
#include <cinttypes>

namespace jaws::vulkan {

class Device;

// Low-level resource creation layer, cached to possibly allow fossilize warmup.
// They create the resources or retrieve it from cache.
// It uses the ci as key. If you have the fossilize-hash of the ci already in hand, pass that
// and it will use it instead of hashing the ci. pass nullptr as ci_hash if you don't.
// In either case it will return the computed or received hash in out_hash.
// TODO: the idea here is that we move these low level pipeline resources (create functions
// and the hash maps) into a class. we can then even replace the whole thing with an alternative
// non-fossilize implementation that doesn't cache, for when we think that's redundant b/c we have caching on
// higher layers.
class JAWS_API Sediment // Sediment, with fossils.. get it?
{
public:
    Sediment() = default;
    ~Sediment();

    void create(Device*);
    void destroy();

    using Hash = uint64_t;

    VkShaderModule lookup_or_create(const VkShaderModuleCreateInfo* ci, const Hash* ci_hash, Hash* out_hash);

private:
    Device* _device = nullptr;
    jaws::util::PrehashedLruCache<VkShaderModule> _shader_modules;
};

}

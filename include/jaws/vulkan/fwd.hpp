#pragma once

#include "jaws/util/ref_ptr.hpp"
#include "jaws/vfs/path.hpp"

#include <tuple>

namespace jaws::vulkan {

class Context;
class Device;
class WindowContext;
class FramebufferCache;

class BufferResource;
using Buffer = jaws::util::ref_ptr<BufferResource>;

class ImageResource;
using Image = jaws::util::ref_ptr<ImageResource>;

class ShaderResource;
using Shader = jaws::util::ref_ptr<ShaderResource>;
class ShaderSystem;
struct ShaderCreateInfo;

using FileWithFingerprint = std::tuple<jaws::vfs::Path, size_t>;

}

#pragma once
#include "jaws/util/Pool.hpp"

namespace jaws::vulkan {

class Context;
class Device;
class WindowContext;

class ShaderSystem;
struct ShaderCreateInfo;
class Shader;
class FramebufferCache;

class Buffer;
class Image;

using BufferPool = jaws::util::Pool<Buffer, uint64_t, 16, 32>;
using ImagePool = jaws::util::Pool<Image, uint64_t, 16, 32>;

}

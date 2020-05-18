#pragma once
#include "jaws/util/ref_ptr.hpp"

namespace jaws::vulkan {

class Context;
class Device;
class WindowContext;

class ShaderSystem;
struct ShaderCreateInfo;
class Shader;
using ShaderPtr = jaws::util::ref_ptr<Shader>;

} // namespace jaws::vulkan


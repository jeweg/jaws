#pragma once
#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include <string>

namespace jaws::vulkan {

class Context;

extern JAWS_API std::string version_to_string(uint32_t);
extern JAWS_API std::string instance_properties_to_string();
extern JAWS_API std::string queue_family_properties_to_string(const Context &);
extern JAWS_API std::string surface_capabilities_to_string(const Context &, VkSurfaceKHR);

// #BEGIN# generated code. Do not remove this marker.
// #END# generated code. Do not remove this marker.

}

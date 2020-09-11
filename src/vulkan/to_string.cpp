#include "../pch.hpp"
#include "to_string.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/util/string_lines_builder.hpp"
#include "jaws/util/enumerate_range.hpp"
#include "absl/strings/str_join.h"
#include <tuple>
#include <iostream>
#include <string>

namespace jaws::vulkan {

std::string version_to_string(uint32_t v)
{
    auto s = fmt::format("{}.{}.{}", VK_VERSION_MAJOR(v), VK_VERSION_MINOR(v), VK_VERSION_PATCH(v));
    return s;
}


std::string instance_properties_to_string()
{
    VkResult r;
    uint32_t version;
    r = vkEnumerateInstanceVersion(&version);
    JAWS_VK_HANDLE_RETURN(r, {});

    jaws::util::StringLinesBuilder sb;
    auto vts = version_to_string(version);
    sb.append_format("version: {}", vts);
    // sb.append_format("version: {}", version_to_string(version));

    auto instance_exts = enumerated<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, {}, nullptr);
    sb.append_format("global extensions ({}):", instance_exts.size());
    {
        auto _ = sb.indent_guard();
        for (const auto &[i, total, props] : jaws::util::enumerate_range_total(instance_exts)) {
            sb.append_format(
                "[{}/{}] extensionName: {}, version: {}",
                i,
                total,
                props.extensionName,
                version_to_string(props.specVersion));
        }
    }

    auto layer_list = enumerated<VkLayerProperties>(vkEnumerateInstanceLayerProperties, {});
    sb.append_format("layers ({}):", layer_list.size());
    {
        auto _ = sb.indent_guard();
        for (const auto &[i, total, props] : jaws::util::enumerate_range_total(layer_list)) {
            sb.append_format("[{}/{}] layerName: {}", i, total, props.layerName);
            auto _ = sb.indent_guard();
            sb.append_format("description: {} ", props.description);
            sb.append_format("specVersion: {} ", version_to_string(props.specVersion));
            sb.append_format("implementationVersion: {} ", props.implementationVersion);

            auto layer_ext_props =
                enumerated<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, {}, props.layerName);
            if (!layer_ext_props.empty()) {
                sb.append("extensions:");
                auto _ = sb.indent_guard();
                for (const auto &layer_ext_prop : layer_ext_props) {
                    sb.append_format(
                        "extensionName: {}, version: {}",
                        layer_ext_prop.extensionName,
                        version_to_string(layer_ext_prop.specVersion));
                }
            }
        }
    }
    return sb.str();
}


std::string queue_family_properties_to_string(const Device &device)
{
    VkQueueFamilyProperties2 elem = {};
    elem.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    auto all_family_props = jaws::vulkan::enumerated_void<VkQueueFamilyProperties2>(
        vkGetPhysicalDeviceQueueFamilyProperties2, elem, device.get_physical_device());
    jaws::util::StringLinesBuilder sb;
    sb.append_format("queue family properties ({}):", all_family_props.size());
    sb.indent();
    for (const auto &[i, total, family_props] : jaws::util::enumerate_range_total(all_family_props)) {
        std::vector<std::string> caps_strs;
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            caps_strs.emplace_back("Graphics");
        }
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) { caps_strs.emplace_back("Compute"); }
        if (family_props.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            caps_strs.emplace_back("Transfer");
        }
        if (auto cb = device.get_context()->get_presentation_support_callback()) {
            if (cb(device.get_context()->get_instance(), device.get_physical_device(), i)) {
                caps_strs.emplace_back("Present");
            }
        }
        sb.append_format("[{}/{}]: {}", i, total, absl::StrJoin(caps_strs, ", "));
    }
    return sb.str();
}

std::string surface_capabilities_to_string(const Device &device, VkSurfaceKHR surface)
{
    VkResult result;
    VkPhysicalDeviceSurfaceInfo2KHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    surface_info.surface = surface;
    VkSurfaceCapabilities2KHR caps2 = {};
    caps2.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
    result = vkGetPhysicalDeviceSurfaceCapabilities2KHR(device.get_physical_device(), &surface_info, &caps2);
    const VkSurfaceCapabilitiesKHR &caps = caps2.surfaceCapabilities;

    /*
typedef struct VkSurfaceCapabilitiesKHR {
    uint32_t                         minImageCount;
    uint32_t                         maxImageCount;
    VkExtent2D                       currentExtent;
    VkExtent2D                       minImageExtent;
    VkExtent2D                       maxImageExtent;
    uint32_t                         maxImageArrayLayers;
    VkSurfaceTransformFlagsKHR       supportedTransforms;
    VkSurfaceTransformFlagBitsKHR    currentTransform;
    VkCompositeAlphaFlagsKHR         supportedCompositeAlpha;
    VkImageUsageFlags                supportedUsageFlags;
     *
     * */
    jaws::util::StringLinesBuilder sb;
    // sb.append_format("surface capabilities ({}):", all_family_props.size());
    sb.indent();

    return sb.str();
}


// #BEGIN# generated code. Do not remove this marker.
// #END# generated code. Do not remove this marker.

}

#pragma once
#include "jaws/core.hpp"
#include "jaws/jaws.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/extension.hpp"
#include "jaws/util/misc.hpp"
#include "absl/types/span.h"
#include "absl/container/flat_hash_map.h"
#include <vector>
#include <array>

namespace jaws::vulkan {

// For now this is the instance (one-per-process) plus the GPU (physical device).
// The reason is that we know nothing about multi-GPU. Do we want device groups (SLI) or
// separate ones? What would a use case be? And I cannot test multi-GPU at the moment anyway.
// The same reasoning goes for the logical VkDevice really, doesn't it? So we put it here as well for now.
// From there follows the non-use volkLoadDeviceTable, really.
// We can change all that if we ever have more than one device.
class JAWS_API Context
{
public:

    using PresentationSupportCallback = std::function<bool(VkInstance, VkPhysicalDevice, uint32_t)>;

    struct CreateInfo
    {
        JAWS_NP_MEMBER3(const char*, app_name, nullptr);
        JAWS_NP_MEMBER3(PFN_vkGetInstanceProcAddr, vkGetInstanceProcAddr, nullptr);
        JAWS_NP_MEMBER2(ExtensionList, required_layers);
        JAWS_NP_MEMBER2(ExtensionList, optional_layers);
        JAWS_NP_MEMBER2(ExtensionList, required_instance_extensions);
        JAWS_NP_MEMBER2(ExtensionList, optional_instance_extensions);
        JAWS_NP_MEMBER3(bool, headless, false);
        JAWS_NP_MEMBER3(bool, debugging, true);
        JAWS_NP_MEMBER3(int32_t, app_version_major, 1);
        JAWS_NP_MEMBER3(int32_t, app_version_minor, 0);
        JAWS_NP_MEMBER3(int32_t, app_version_patch, 0);
        JAWS_NP_MEMBER2(PresentationSupportCallback, presentation_support_callback);
    };

    Context();
    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;
    ~Context();

    //static std::vector<const char*> defaultInstanceExtensions(absl::Span<const char* const> extra = {});
    //static bool extensionListContains(absl::Span<const char* const> list, const char* ext);

    void create(jaws::Jaws* jaws, const CreateInfo & = jaws::util::make_default<CreateInfo>());
    void destroy();

    Jaws* get_jaws() const { return _jaws; }

    [[nodiscard]] VkInstance get_instance() const;
    [[nodiscard]] PresentationSupportCallback get_presentation_support_callback() const;

    bool is_headless() const { return _headless; }

private:
    Jaws *_jaws = nullptr;
    VkInstance _vk_instance = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> _physical_devices;
    PresentationSupportCallback _presentation_support_callback;
    VkDebugReportCallbackEXT _debug_report_callback = VK_NULL_HANDLE;

    bool _headless = false;

};

inline VkInstance Context::get_instance() const {
    return _vk_instance;
}

inline Context::PresentationSupportCallback Context::get_presentation_support_callback() const {
    return _presentation_support_callback;
}

}

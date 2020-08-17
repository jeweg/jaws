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
        const char *app_name = nullptr;
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
        ExtensionList required_layers;
        ExtensionList optional_layers;
        ExtensionList required_instance_extensions;
        ExtensionList optional_instance_extensions;
        bool headless = false;
        bool debugging = true;
        int32_t app_version_major = 1;
        int32_t app_version_minor = 0;
        int32_t app_version_patch = 0;
        PresentationSupportCallback presentation_support_callback;

        CreateInfo &set_app_name(const char *v)
        {
            app_name = v;
            return *this;
        }
        CreateInfo &set_vkGetInstanceProcAddr(PFN_vkGetInstanceProcAddr v)
        {
            vkGetInstanceProcAddr = v;
            return *this;
        }
        CreateInfo &set_required_layers(ExtensionList v)
        {
            required_layers = std::move(v);
            return *this;
        }
        CreateInfo &set_optional_layers(ExtensionList v)
        {
            optional_layers = std::move(v);
            return *this;
        }
        CreateInfo &set_required_instance_extensions(ExtensionList v)
        {
            required_instance_extensions = std::move(v);
            return *this;
        }
        CreateInfo &set_optional_instance_extensions(ExtensionList v)
        {
            optional_instance_extensions = std::move(v);
            return *this;
        }
        CreateInfo &set_headless(bool v)
        {
            headless = v;
            return *this;
        }
        CreateInfo &set_debugging(bool v)
        {
            debugging = v;
            return *this;
        }
        CreateInfo &set_app_version_major(int32_t v)
        {
            app_version_major = v;
            return *this;
        }
        CreateInfo &set_app_version_minor(int32_t v)
        {
            app_version_minor = v;
            return *this;
        }
        CreateInfo &set_app_version_patch(int32_t v)
        {
            app_version_patch = v;
            return *this;
        }
        CreateInfo &set_presentation_support_callback(PresentationSupportCallback v)
        {
            presentation_support_callback = v;
            return *this;
        }
    };

    Context();
    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;
    ~Context();

    // static std::vector<const char*> defaultInstanceExtensions(absl::Span<const char* const> extra = {});
    // static bool extensionListContains(absl::Span<const char* const> list, const char* ext);

    void create(const CreateInfo & = jaws::util::make_default<CreateInfo>());
    void destroy();

    [[nodiscard]] VkInstance get_instance() const;
    [[nodiscard]] PresentationSupportCallback get_presentation_support_callback() const;

    bool is_headless() const { return _headless; }

private:
    VkInstance _vk_instance = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> _physical_devices;
    PresentationSupportCallback _presentation_support_callback;
    VkDebugReportCallbackEXT _debug_report_callback = VK_NULL_HANDLE;

    bool _headless = false;
};

inline VkInstance Context::get_instance() const
{
    return _vk_instance;
}

inline Context::PresentationSupportCallback Context::get_presentation_support_callback() const
{
    return _presentation_support_callback;
}

} // namespace jaws::vulkan

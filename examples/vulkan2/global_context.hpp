#pragma once

#include "jaws/core.hpp"
#include "jaws/vulkan/vulkan.hpp"

#include <string>
#include <vector>

namespace jaws::vulkan {

// For now this is everything that's not window/swap chain dependent,
// except for glfw handling, that should be part of the application.

class GlobalContext
{
public:
    GlobalContext(
        const std::string& app_name,
        int app_version_major,
        int app_version_minor,
        int app_version_patch,
        const std::vector<std::string>& required_extensions,
        bool enable_validation = true);

    ~GlobalContext();

    [[nodiscard]] VkInstance GetInstance() const;
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const;
    [[nodiscard]] VkDevice GetDevice() const;
    [[nodiscard]] VkQueue GetGraphicsQueue() const;
    [[nodiscard]] VkQueue GetPresentQueue() const;
    [[nodiscard]] uint32_t GetGraphicsQueueFamilyIndex() const;
    [[nodiscard]] uint32_t GetPresentQueueFamilyIndex() const;

private:
    VkInstance _instance = nullptr;
    VkPhysicalDevice _physical_device = nullptr;
    VkDevice _device = nullptr;
    uint32_t _vulkan_version = 0;
    const bool _enable_validation = true;
    VkQueue _graphics_queue = nullptr;
    VkQueue _present_queue = nullptr;
    uint32_t _graphics_queue_family_index = 0;
    uint32_t _present_queue_family_index = 0;
};


inline VkInstance GlobalContext::GetInstance() const
{
    return _instance;
}


inline VkPhysicalDevice GlobalContext::GetPhysicalDevice() const
{
    return _physical_device;

}


inline VkDevice GlobalContext::GetDevice() const
{
    return _device;
}


inline VkQueue GlobalContext::GetGraphicsQueue() const
{
    return _graphics_queue;
}


inline VkQueue GlobalContext::GetPresentQueue() const
{
    return _present_queue;
}


inline uint32_t GlobalContext::GetGraphicsQueueFamilyIndex() const
{
    return _graphics_queue_family_index;
}


inline uint32_t GlobalContext::GetPresentQueueFamilyIndex() const
{
    return _present_queue_family_index;
}

} // namespace jaws::vulkan

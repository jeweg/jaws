/*
Here we experiment with https://github.com/KhronosGroup/Vulkan-Hpp.
We follow https://github.com/jherico/vulkan#examples while learning things with
the intention of eventually growing this into a vulkan backend.
So things like outputting diagnostics information will come in handy now and later.

Some vulkan insights:
* https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/

I will resist the desire to make classes for now. I want to understand exactly
the steps necessary.
They are:

* Create vulkan instance

*/
//#include "diagnostics.hpp"
#include "jaws/jaws.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

jaws::LoggerPtr logger;

void HandleResult(vk::Result r, const std::string& msg = std::string())
{
    if (r != vk::Result::eSuccess) { logger->error("[HandleResult] {}: {}", msg, to_string(r)); }
}


int main(int argc, char** argv)
{
    logger = jaws::GetLoggerPtr(jaws::Category::General);

    vk::Result result;

    vk::ApplicationInfo appInfo("MyApp", 1, "MyEngine", 1, VK_API_VERSION_1_1);

    //==============================================================================
    // Instance

    vk::InstanceCreateInfo ini_ci(vk::InstanceCreateFlags(), &appInfo);
    vk::Instance instance;
    std::tie(result, instance) = vk::createInstance(ini_ci);
    HandleResult(result, "create instance");

    //==============================================================================
    // Enumerate physical devices and choose one

    logger->info("Enumerating physical devices");
    std::vector<vk::PhysicalDevice> physical_devices;
    std::tie(result, physical_devices) = instance.enumeratePhysicalDevices();
    HandleResult(result, "enumerate physical devices");
    if (physical_devices.empty()) { logger->error("no GPUs"); }
    logger->info("  Found {} physical device(s)", physical_devices.size());
}

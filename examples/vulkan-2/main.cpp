#include "jaws/jaws.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"

int main(int argc, char **argv)
{
    jaws::InitGuard _(argc, argv);

    jaws::vulkan::Context context;
    context.create(jaws::vulkan::Context::CreateInfo{}.set_headless(true));

    jaws::vulkan::Device device;
    device.create(&context);

    auto compute_queue = device.get_queue(jaws::vulkan::Device::Queue::Compute);


    device.wait_idle();

    return 0;
}

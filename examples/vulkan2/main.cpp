#include "jaws/jaws.hpp"
#include "jaws/logging.hpp"
#include "jaws/vfs/vfs.hpp"
#include "jaws/vfs/file_system_backend.hpp"
#include "jaws/util/indentation.hpp"
#include "jaws/util/main_wrapper.hpp"

#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/window_context.hpp"
#include "jaws/vulkan/shader_system.hpp"

#include "build_info.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>

jaws::Logger *logger = nullptr;

//=========================================================================

static void glfwErrorCallback(int error, const char *description)
{
    logger->error("GLFW error {}: {}", error, description);
    JAWS_FATAL1("Error");
}


int main(int argc, char **argv)
{
    return jaws::util::Main(argc, argv, [](auto, auto) {
        // Put a file-system-backed virtual file system starting in the project source dir subdir "shaders".
        jaws::get_vfs().add_backend(
            "files", std::make_unique<jaws::vfs::FileSystemBackend>(build_info::PROJECT_SOURCE_DIR / "shaders"));

        // Piggypacking onto one of jaws' logger categories here.
        logger = &jaws::get_logger();

        if (!glfwInit()) { JAWS_FATAL1("Unable to initialize GLFW"); }
        glfwSetErrorCallback(&glfwErrorCallback);
        if (!glfwVulkanSupported()) { JAWS_FATAL1("glfwVulkanSupported -> false"); }

        uint32_t glfw_required_extension_count = 0;
        const char **glfw_required_extensions = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);
        jaws::vulkan::ExtensionList instance_extensions({glfw_required_extensions, glfw_required_extension_count});
        instance_extensions.add("VK_KHR_surface");

        //----------------------------------------------------------------------
        // * Init instance w/ proper layers and instance extensions
        // * Choose extensions as instructed/available

        jaws::vulkan::Context context;
        context.create(jaws::vulkan::Context::CreateInfo{}
                           //.set_vkGetInstanceProcAddr(glfwGetInstanceProcAddress) // Should work with or without this.
                           .set_required_instance_extensions(instance_extensions)
                           .set_presentation_support_callback(&glfwGetPhysicalDevicePresentationSupport)
                           .set_headless(false));

        //----------------------------------------------------------------------
        // We will use GLFW3 only for getting the window and the vulkan surface.

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow *glfw_window = glfwCreateWindow(800, 600, "jaws example", nullptr, nullptr);
        if (!glfw_window) {
            glfwTerminate();
            JAWS_FATAL1("Failed to create window");
        }

        VkSurfaceKHR surface;
        if (VkResult err = glfwCreateWindowSurface(context.get_instance(), glfw_window, nullptr, &surface)) {
            glfwTerminate();
            JAWS_FATAL1("Failed to create window surface");
        }

        //----------------------------------------------------------------------
        // * Choose physical device
        // * Choose extensions as instructed/available
        // * Selects queue families for graphics, compute, transfer, async compute, async transfer
        // * Creates logical device
        // * Init volk and vulkan-memory-allocator

        jaws::vulkan::Device device;
        auto device_ci = jaws::vulkan::Device::CreateInfo{}.set_required_extensions({VK_KHR_SWAPCHAIN_EXTENSION_NAME});
        device.create(&context, device_ci);

        //----------------------------------------------------------------------
        // * Chooose a surface format

        jaws::vulkan::WindowContext window_context;
        window_context.create(&device, jaws::vulkan::WindowContext::CreateInfo{}.set_surface(surface));

        //----------------------------------------------------------------------
        // Testing.. grab a shader
        {
            jaws::vulkan::Shader shader =
                device.get_shader(jaws::vulkan::ShaderCreateInfo{}.set_main_source_file("shader.vert"));
        }

        window_context.create_swap_chain(800, 600);
        // Shader* my_shader = get_shader()

        glfwSetKeyCallback(glfw_window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {}
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        });

        while (!glfwWindowShouldClose(glfw_window)) {
            // render commands...
            // swap buffers...
            glfwPollEvents();
        }

        device.wait_idle();
        window_context.destroy();
        device.destroy();

        glfwTerminate();

        return 0;
    });
}

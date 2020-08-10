#include "jaws/jaws.hpp"
#include "jaws/logging.hpp"
#include "jaws/vfs/vfs.hpp"
#include "jaws/vfs/file_system_vfs.hpp"
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

jaws::Logger* logger = nullptr;

//=========================================================================

static void glfwErrorCallback(int error, const char* description)
{
    logger->error("GLFW error {}: {}", error, description);
    JAWS_FATAL1("Error");
}

int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, [](auto, auto) {
        // Put a file-system-backed virtual file system starting in the project source dir subdir "shaders".
        jaws::get_vfs().add_backend(
            "files", std::make_unique<jaws::vfs::FileSystemVfs>(build_info::PROJECT_SOURCE_DIR / "shaders"));

        // Piggypacking onto one of jaws' logger categories here.
        logger = &jaws::get_logger();

        if (!glfwInit()) { JAWS_FATAL1("Unable to initialize GLFW"); }
        glfwSetErrorCallback(&glfwErrorCallback);
        if (!glfwVulkanSupported()) { JAWS_FATAL1("glfwVulkanSupported -> false"); }

        uint32_t required_extension_count = 0;
        const char** required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

        //----------------------------------------------------------------------
        // * Init instance w/ proper layers and instance extensions
        // * Choose extensions as instructed/available

        jaws::vulkan::Context context;
        context.create(jaws::vulkan::Context::CreateInfo{}
                           //.set_vkGetInstanceProcAddr(glfwGetInstanceProcAddress) // Should work with or without this.
                           .set_required_instance_extensions({required_extensions, required_extension_count})
                           .set_presentation_support_callback(&glfwGetPhysicalDevicePresentationSupport)
                           .set_headless(false));

        //----------------------------------------------------------------------
        // We will use GLFW3 only for getting the window and the vulkan surface.

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* glfw_window = glfwCreateWindow(800, 600, "jaws example", nullptr, nullptr);
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
        device.create(&context);

        //----------------------------------------------------------------------
        // * Chooose a surface format

        jaws::vulkan::WindowContext window_context;
        window_context.create(&device, jaws::vulkan::WindowContext::CreateInfo{}.set_surface(surface));

        //----------------------------------------------------------------------
        // Grab a shader

        {
            jaws::vulkan::ShaderPtr shader =
                device.get_shader(jaws::vulkan::ShaderCreateInfo{}.set_main_source_file("shader.vert"));
        }

        // window_context.create_swap_chain(800, 600);
        // Shader* my_shader = get_shader()


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

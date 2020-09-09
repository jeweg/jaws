#include "jaws/jaws.hpp"
#include "jaws/logging.hpp"
#include "jaws/vfs/vfs.hpp"
#include "jaws/vfs/file_system_backend.hpp"
#include "jaws/util/indentation.hpp"
#include "jaws/util/main_wrapper.hpp"

#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/framebuffer.hpp"
#include "jaws/vulkan/window_context.hpp"
#include "jaws/vulkan/shader_system.hpp"

#include "build_info.h"

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_transform.hpp"

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
        VkResult result;

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
        // Uniform buffer with view transform data

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
        glm::mat4 view =
            glm::lookAt(glm::vec3(-5.0f, 3.0f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        glm::mat4 model = glm::mat4(1.0f);
        // vulkan clip space has inverted y and half z!
        glm::mat4 clip =
            glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);
        // This is what we'll put into the buffer:
        glm::mat4 mvpc = clip * projection * view * model;

        jaws::vulkan::BufferPool::Id uniform_buffer_id;
        {
            VkBufferCreateInfo ci = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
            ci.size = sizeof(mvpc);
            ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            uniform_buffer_id = device.create_buffer(ci, VMA_MEMORY_USAGE_CPU_TO_GPU);

            void *ptr = device.map_buffer(uniform_buffer_id);
            memcpy(ptr, glm::value_ptr(mvpc), sizeof(mvpc));
            device.unmap_buffer(uniform_buffer_id);
        }

        //----------------------------------------------------------------------
        // Shaders and pipeline

        jaws::vulkan::Shader vertex_shader =
            device.get_shader(jaws::vulkan::ShaderCreateInfo{}.set_main_source_file("shader.vert"));
        jaws::vulkan::Shader fragment_shader =
            device.get_shader(jaws::vulkan::ShaderCreateInfo{}.set_main_source_file("shader.frag"));

        //----------------------------------------------------------------------

        window_context.create_swap_chain(800, 600);

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkRenderPass render_pass = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        {
            //----------------------------------------------------------------------
            // Descriptor set layout and pipeline layout

            VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
            {
                VkDescriptorSetLayoutCreateInfo ci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
                // Try to get by without VkDescriptorSetLayoutBinding
                result = vkCreateDescriptorSetLayout(device.vk_handle(), &ci, nullptr, &descriptor_set_layout);
                JAWS_VK_HANDLE_FATAL(result);
            }

            {
                VkPipelineLayoutCreateInfo ci = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
                ci.setLayoutCount = 1;
                ci.pSetLayouts = &descriptor_set_layout;
                result = vkCreatePipelineLayout(device.vk_handle(), &ci, nullptr, &pipeline_layout);
                JAWS_VK_HANDLE_FATAL(result);
            }

            //----------------------------------------------------------------------
            // Descriptor pool

            // More information at
            // https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/?utm_source=share&utm_medium=web2x&context=3

            VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
            {
                VkDescriptorPoolSize pool_size;
                pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                pool_size.descriptorCount = 1;
                VkDescriptorPoolCreateInfo ci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
                ci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                ci.maxSets = 1;
                ci.poolSizeCount = 1;
                ci.pPoolSizes = &pool_size;
                result = vkCreateDescriptorPool(device.vk_handle(), &ci, nullptr, &descriptor_pool);
                JAWS_VK_HANDLE_FATAL(result);
            }

            //----------------------------------------------------------------------
            // Allocate descriptor set and fill with descriptor set layouts

            VkDescriptorSet descriptor_set;
            {
                VkDescriptorSetAllocateInfo info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
                info.descriptorPool = descriptor_pool;
                info.descriptorSetCount = 1;
                info.pSetLayouts = &descriptor_set_layout;
                result = vkAllocateDescriptorSets(device.vk_handle(), &info, &descriptor_set);
                JAWS_VK_HANDLE_FATAL(result);
            }

#if 0
            // crashes at the moment...

            // This means "This write operation here will set the id of the bound uniform buffer to this
            // specific value in this particular argument set to the shaders"
            // Basically glUniform with a uniform buffer argument.
            VkDescriptorBufferInfo descriptor_buffer_info;
            descriptor_buffer_info.buffer = device.get_buffer(uniform_buffer_id)->handle;
            descriptor_buffer_info.offset = 0;
            descriptor_buffer_info.range = sizeof(glm::mat4);
            VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = descriptor_set;
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &descriptor_buffer_info;
            vkUpdateDescriptorSets(device.vk_handle(), 1, &write, 0, nullptr);
#endif
            //----------------------------------------------------------------------
            // Render pass and subpass.
            // The pass holds descriptions of all attachments used in its subpasses.
            // Each subpass references one or more of those attachments via its index.

            {
                VkSubpassDescription subpass_desc = {0};
                subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                VkAttachmentReference col_ref = {0};
                col_ref.attachment = 0;
                col_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                subpass_desc.colorAttachmentCount = 1;
                subpass_desc.pColorAttachments = &col_ref;

                VkSubpassDependency subpass_dep = {0};
                subpass_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
                subpass_dep.dstSubpass = 0;
                subpass_dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpass_dep.srcAccessMask = 0;
                subpass_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                subpass_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkAttachmentDescription attachment_desc;
                attachment_desc.format = window_context.get_surface_format();
                attachment_desc.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment_desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment_desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment_desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment_desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment_desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment_desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkRenderPassCreateInfo ci = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
                ci.subpassCount = 1;
                ci.pSubpasses = &subpass_desc;
                ci.dependencyCount = 1;
                ci.pDependencies = &subpass_dep;
                ci.attachmentCount = 1;
                ci.pAttachments = &attachment_desc;
                result = vkCreateRenderPass(device.vk_handle(), &ci, nullptr, &render_pass);
                JAWS_VK_HANDLE_FATAL(result);
            }

            //----------------------------------------------------------------------
            // Shader stages

            std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stages;
            {
                VkPipelineShaderStageCreateInfo ci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
                ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
                ci.module = vertex_shader.vk_handle();
                ci.pName = "main";
                pipeline_shader_stages.push_back(std::move(ci));
            }
            {
                VkPipelineShaderStageCreateInfo ci = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
                ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                ci.module = fragment_shader.vk_handle();
                ci.pName = "main";
                pipeline_shader_stages.push_back(std::move(ci));
            }

            //----------------------------------------------------------------------
            // vertex input state

            VkPipelineVertexInputStateCreateInfo vertex_input_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

            //----------------------------------------------------------------------
            // Input assembly state

            VkPipelineInputAssemblyStateCreateInfo input_assembly_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            input_assembly_state_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly_state_ci.primitiveRestartEnable = false;

            //----------------------------------------------------------------------
            // Viewport state

            std::vector<VkViewport> viewports;
            {
                VkViewport vp = {0};
                vp.x = 0;
                vp.y = 0;
                vp.width = window_context.get_width();
                vp.height = window_context.get_height();
                vp.minDepth = 0;
                vp.maxDepth = 1;
                viewports.push_back(vp);
            }

            std::vector<VkRect2D> scissor_rects;
            {
                VkRect2D r = {0};
                r.offset.x = 0;
                r.offset.y = 0;
                r.extent = window_context.get_extent();
                scissor_rects.push_back(r);
            }

            VkPipelineViewportStateCreateInfo viewport_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewport_state_ci.viewportCount = static_cast<uint32_t>(viewports.size());
            viewport_state_ci.pViewports = viewports.data();
            viewport_state_ci.scissorCount = static_cast<uint32_t>(scissor_rects.size());
            viewport_state_ci.pScissors = scissor_rects.data();

            //----------------------------------------------------------------------
            // Rasterization state

            VkPipelineRasterizationStateCreateInfo rasterization_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            rasterization_state_ci.polygonMode = VK_POLYGON_MODE_FILL;
            rasterization_state_ci.cullMode = VK_CULL_MODE_NONE;
            rasterization_state_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterization_state_ci.depthClampEnable = false;
            rasterization_state_ci.rasterizerDiscardEnable = false;
            rasterization_state_ci.depthBiasEnable = false;
            rasterization_state_ci.lineWidth = 1.f;

            //----------------------------------------------------------------------
            // Multisample state

            VkPipelineMultisampleStateCreateInfo multisample_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            multisample_state_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisample_state_ci.sampleShadingEnable = false;

            //----------------------------------------------------------------------
            // Depth-stencil state

            VkPipelineDepthStencilStateCreateInfo depth_stencil_attachment_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            depth_stencil_attachment_state_ci.depthTestEnable = true;
            depth_stencil_attachment_state_ci.depthWriteEnable = true;
            depth_stencil_attachment_state_ci.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            depth_stencil_attachment_state_ci.back.failOp = VK_STENCIL_OP_KEEP;
            depth_stencil_attachment_state_ci.back.passOp = VK_STENCIL_OP_KEEP;
            depth_stencil_attachment_state_ci.back.compareOp = VK_COMPARE_OP_ALWAYS;
            depth_stencil_attachment_state_ci.front = depth_stencil_attachment_state_ci.back;

            //----------------------------------------------------------------------
            // Color blend attachment states

            std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_states;
            {
                VkPipelineColorBlendAttachmentState s = {0};
                s.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
                                   | VK_COLOR_COMPONENT_A_BIT;
                color_blend_attachment_states.push_back(s);
            }

            VkPipelineColorBlendStateCreateInfo pipeline_color_blend_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            pipeline_color_blend_ci.attachmentCount = static_cast<uint32_t>(color_blend_attachment_states.size());
            pipeline_color_blend_ci.pAttachments = color_blend_attachment_states.data();

            //-------------------------------------------------------------------------
            // Pipeline dynamic state

            VkPipelineDynamicStateCreateInfo pipeline_dyn_state_ci = {
                VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};

            //----------------------------------------------------------------------
            // Finally, the graphics pipeline

            VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            graphics_pipeline_ci.pStages = pipeline_shader_stages.data();
            graphics_pipeline_ci.stageCount = static_cast<uint32_t>(pipeline_shader_stages.size());
            graphics_pipeline_ci.renderPass = render_pass;
            graphics_pipeline_ci.subpass = 0;
            graphics_pipeline_ci.pDynamicState = &pipeline_dyn_state_ci;
            graphics_pipeline_ci.pColorBlendState = &pipeline_color_blend_ci;
            graphics_pipeline_ci.pDepthStencilState = &depth_stencil_attachment_state_ci;
            graphics_pipeline_ci.pMultisampleState = &multisample_state_ci;
            graphics_pipeline_ci.pRasterizationState = &rasterization_state_ci;
            graphics_pipeline_ci.pViewportState = &viewport_state_ci;
            // graphics_pipeline_ci.pTessellationState = nullptr;
            graphics_pipeline_ci.pVertexInputState = &vertex_input_state_ci;
            graphics_pipeline_ci.pInputAssemblyState = &input_assembly_state_ci;

            // TODO: add pipeline cache.
            result = vkCreateGraphicsPipelines(
                device.vk_handle(), VK_NULL_HANDLE, 1, &graphics_pipeline_ci, nullptr, &pipeline);
            JAWS_VK_HANDLE_FATAL(result);
        }

        //----------------------------------------------------------------------
        // Command pool

        VkCommandPool command_pool = VK_NULL_HANDLE;
        {
            VkCommandPoolCreateInfo ci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            ci.queueFamilyIndex = device.get_queue_family(jaws::vulkan::Device::Queue::Graphics);
            result = vkCreateCommandPool(device.vk_handle(), &ci, nullptr, &command_pool);
            JAWS_VK_HANDLE_FATAL(result);
        }

        //----------------------------------------------------------------------
        // Command buffers, one for each swapchain image for now.

        std::vector<VkCommandBuffer> command_buffers(window_context._swapchain_image_views.size(), VK_NULL_HANDLE);
        {
            VkCommandBufferAllocateInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            info.commandPool = command_pool;
            info.commandBufferCount = static_cast<uint32_t>(window_context._swapchain_image_views.size());
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            result = vkAllocateCommandBuffers(device.vk_handle(), &info, command_buffers.data());
            JAWS_VK_HANDLE_FATAL(result);
            logger->info("Allocated {} command buffers.", command_buffers.size());

            // One command buffer per swap chain image.

            for (size_t i = 0; i < command_buffers.size(); ++i) {
                VkCommandBuffer cb = command_buffers[i];
                {
                    // Begin CB
                    VkCommandBufferBeginInfo info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
                    result = vkBeginCommandBuffer(cb, &info);
                    JAWS_VK_HANDLE_FATAL(result);
                }
                {
                    // Begin render pass
                    VkClearValue clear_value = {0};
                    switch (i) {
                    default:
                    case 0:
                        clear_value.color.float32[0] = 0.9;
                        clear_value.color.float32[1] = 0.5;
                        clear_value.color.float32[2] = 0;
                        clear_value.color.float32[3] = 1;
                        break;
                    case 1:
                        clear_value.color.float32[0] = 0.6;
                        clear_value.color.float32[1] = 0.5;
                        clear_value.color.float32[2] = 0;
                        clear_value.color.float32[3] = 1;
                        break;
                    case 2:
                        clear_value.color.float32[0] = 0.3;
                        clear_value.color.float32[1] = 0.5;
                        clear_value.color.float32[2] = 0;
                        clear_value.color.float32[3] = 1;
                        break;
                    }
                    // Begin render pass
                    VkRenderPassBeginInfo info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
                    info.renderPass = render_pass;
                    info.renderArea.offset.x = 0;
                    info.renderArea.offset.y = 0;
                    info.renderArea.extent = window_context.get_extent();
                    info.clearValueCount = 1;
                    info.pClearValues = &clear_value;

                    jaws::vulkan::FramebufferCreateInfo framebuffer_ci;
                    framebuffer_ci.render_pass = render_pass;
                    framebuffer_ci.width = window_context.get_width();
                    framebuffer_ci.height = window_context.get_height();
                    framebuffer_ci.layers = 1;
                    framebuffer_ci.attachments.push_back(&window_context._swapchain_image_views[i]);

                    info.framebuffer = device.get_framebuffer(framebuffer_ci);
                    vkCmdBeginRenderPass(cb, &info, VK_SUBPASS_CONTENTS_INLINE);
                }

                vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                glm::vec2 offset(0.5, 0);
                vkCmdPushConstants(
                    cb, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec2), glm::value_ptr(offset));

                vkCmdDraw(cb, 3, 1, 0, 0);

                // End render pass
                vkCmdEndRenderPass(cb);

                // End CB
                result = vkEndCommandBuffer(cb);
                JAWS_VK_HANDLE_FATAL(result);
            }
        }

        //----------------------------------------------------------------------
        // Prepare render loop

        // Semaphores for acquire-present synchronization and waiting for frames
        constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
        std::vector<VkSemaphore> image_available_semaphores(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        std::vector<VkSemaphore> render_finished_semaphores(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
        std::vector<VkFence> in_flight_fences(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

        for (auto &sema : image_available_semaphores) {
            VkSemaphoreCreateInfo ci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            result = vkCreateSemaphore(device.vk_handle(), &ci, nullptr, &sema);
            JAWS_VK_HANDLE_FATAL(result);
        }
        for (auto &sema : render_finished_semaphores) {
            VkSemaphoreCreateInfo ci = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
            result = vkCreateSemaphore(device.vk_handle(), &ci, nullptr, &sema);
            JAWS_VK_HANDLE_FATAL(result);
        }
        for (auto &fence : in_flight_fences) {
            VkFenceCreateInfo ci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            result = vkCreateFence(device.vk_handle(), &ci, nullptr, &fence);
            JAWS_VK_HANDLE_FATAL(result);
        }

        //----------------------------------------------------------------------
        // Render loop

        glfwSetKeyCallback(glfw_window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {}
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        });

        uint64_t global_frame = 0;
        while (!glfwWindowShouldClose(glfw_window)) {
            const size_t mod_frame_in_flight = global_frame % MAX_FRAMES_IN_FLIGHT;

            glfwPollEvents();

            // This makes the current thread wait if MAX_FRAMES_IN_FLIGHT are in an unfinished state.
            result = vkWaitForFences(
                device.vk_handle(),
                1,
                &in_flight_fences[mod_frame_in_flight],
                true,
                std::numeric_limits<uint64_t>::max());
            JAWS_VK_HANDLE_FATAL(result);
            result = vkResetFences(device.vk_handle(), 1, &in_flight_fences[mod_frame_in_flight]);
            JAWS_VK_HANDLE_FATAL(result);

            // Acquire image to render to
            uint32_t curr_swapchain_image_index;
            result = vkAcquireNextImageKHR(
                device.vk_handle(),
                window_context._swapchain,
                std::numeric_limits<uint64_t>::max(),
                image_available_semaphores[mod_frame_in_flight],
                VK_NULL_HANDLE,
                &curr_swapchain_image_index);
            JAWS_VK_HANDLE_FATAL(result);

            std::array<VkSemaphore, 1> wait_semaphores = {image_available_semaphores[mod_frame_in_flight]};
            std::array<VkSemaphore, 1> signal_semaphores = {render_finished_semaphores[mod_frame_in_flight]};

            // Submit command buffer
            VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &command_buffers[curr_swapchain_image_index];
            submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
            submit_info.pWaitSemaphores = wait_semaphores.data();
            submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
            submit_info.pSignalSemaphores = signal_semaphores.data();
            VkPipelineStageFlags wait_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submit_info.pWaitDstStageMask = &wait_stages;


            result = vkQueueSubmit(
                device.get_queue(jaws::vulkan::Device::Queue::Graphics),
                1,
                &submit_info,
                in_flight_fences[mod_frame_in_flight]);
            JAWS_VK_HANDLE_FATAL(result);

            // Present
            VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            present_info.waitSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
            present_info.pWaitSemaphores = signal_semaphores.data();
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &window_context._swapchain;
            present_info.pImageIndices = &curr_swapchain_image_index;
            result = vkQueuePresentKHR(device.get_queue(jaws::vulkan::Device::Queue::Present), &present_info);
            JAWS_VK_HANDLE_FATAL(result);

            ++global_frame;
        }

        device.wait_idle();
        window_context.destroy();
        device.destroy();

        glfwTerminate();

        return 0;
    });
}

#include "jaws/jaws.hpp"
#include "jaws/logging.hpp"
#include "jaws/util/main_wrapper.hpp"
#include "jaws/util/indentation.hpp"
#include "jaws/filesystem.hpp"
#include "jaws/vfs/vfs.hpp"
#include "jaws/vfs/file_system_vfs.hpp"
#include "jaws/vulkan/shader.hpp"
#include "jaws/vulkan/context.hpp"
#include "jaws/vulkan/device.hpp"

#include "build_info.h"

#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <iostream>

//=========================================================================

jaws::LoggerPtr logger;

//=========================================================================


//=========================================================================

int main(int argc, char** argv)
{
    return jaws::util::Main(argc, argv, [&](auto, auto) {
        jaws::Jaws jjj;
        jjj.create(argc, argv);

        // Piggypacking onto one of jaws' logger categories here.
        logger = jaws::get_logger_ptr();

        auto context_ci = jaws::vulkan::Context::CreateInfo{}.set_headless(true);
        jaws::vulkan::Context context;
        context.create(&jjj, context_ci);

        jjj.get_vfs().make_backend<jaws::vfs::FileSystemVfs>("shaders", build_info::PROJECT_SOURCE_DIR / "shaders");

        jaws::vulkan::Device device;
        device.create(&context);
        auto shader_ci = jaws::vulkan::ShaderCreateInfo{}
                             .set_main_source_file("shaders:simple.comp")
                             //.set_include_paths({"", ""})
                             .set_compile_definitions({std::make_pair("foo", "1")});


        // for (;;) { jaws::vulkan::ShaderPtr my_shader = device.get_shader(shader_ci); }
        jaws::vulkan::ShaderPtr my_shader = device.get_shader(shader_ci);

        logger->info("&shader: {}", (void*)my_shader.get());

#if 0
        // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        auto BuildGlslShaderModule = [](VkDevice device,
            const std::filesystem::path& file_path,
            std::vector<uint32_t>* out_spirv = nullptr) {
            if (!std::filesystem::exists(file_path)) {
                logger->error("Shader file {} does not exist!", file_path);
                //return {};
            }
            std::ifstream ifs(file_path, std::ios::binary);
            if (!ifs) {
                logger->error("Shader file {} could not be opened!", file_path);
                //return {};
            }
            std::string code((std::istreambuf_iterator<char>(ifs)), {});
            ifs.close();

            // Why I'm doing this? Because the interface forces me to pass something, and the only thing I could
            // always pass is infer_from_source, but I don't want to force the pragma everywhere.
            // Where it makes sense, I'm keeping this compatible with
            // https://github.com/google/shaderc/tree/master/glslc#311-shader-stage-specification
            shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source;
            std::string extension = file_path.extension().string();
            std::string lower_extension = extension;
            std::transform(lower_extension.begin(), lower_extension.end(), lower_extension.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            if (lower_extension == ".frag") {
                shader_kind = shaderc_glsl_default_fragment_shader;
            } else if (lower_extension == ".vert") {
                shader_kind = shaderc_glsl_default_vertex_shader;
            } else if (lower_extension == ".tesc") {
                shader_kind = shaderc_glsl_default_tess_control_shader;
            } else if (lower_extension == ".tese") {
                shader_kind = shaderc_glsl_default_tess_evaluation_shader;
            } else if (lower_extension == ".geom") {
                shader_kind = shaderc_glsl_default_geometry_shader;
            } else if (lower_extension == ".comp") {
                shader_kind = shaderc_glsl_default_compute_shader;
            }
            // TODO: Might wanna extend this.

            shaderc::Compiler compiler;
            shaderc::CompileOptions compile_options;

            // Omitted. See vulkan1/main.cpp for a start.
            //compile_options.SetIncluder(std::make_unique<MyIncludeHandler>());

            auto compile_result = compiler.CompileGlslToSpv(
                code.c_str(),
                code.size(),
                shader_kind,
                file_path.filename()
                    .string()
                    .c_str(), // file name for error msgs, doesn't have to be a valid path or filename
                compile_options);
            shaderc_compilation_status status = compile_result.GetCompilationStatus();
            if (status != shaderc_compilation_status_success) {
                logger->error(
                    "Shader compilation error ({}): {}",
                    /*jaws::vulkan::ToString(status)*/ status,
                    compile_result.GetErrorMessage());
                JAWS_FATAL1("Shader compilation failed");
            } else {
                logger->info(
                    "Successfully compiled shader ({} errors, {} warnings)",
                    compile_result.GetNumErrors(),
                    compile_result.GetNumWarnings());
            }

            VkShaderModuleCreateInfo ci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            ci.codeSize = sizeof(uint32_t) * std::distance(compile_result.cbegin(), compile_result.cend());
            ci.pCode = compile_result.cbegin();

            /*
            auto debug_write = std::fstream("/tmp/shader.spirv", std::ios::out | std::ios::binary);
            debug_write.write((char*)ci.pCode, ci.codeSize);
            debug_write.close();
            */

            VkShaderModule shader_module = VK_NULL_HANDLE;
            VkResult result;

            result = vkCreateShaderModule(device, &ci, nullptr, &shader_module);
            JAWS_VK_HANDLE_FATAL(result);

            if (out_spirv) {
                out_spirv->assign(compile_result.cbegin(), compile_result.cend());
            }

            return shader_module;
        };

        std::vector<uint32_t> spirv;
        VkShaderModule shmod = BuildGlslShaderModule(context.get_device(),
            build_info::PROJECT_SOURCE_DIR / "siimple.comp",
            &spirv);

        spirv_cross::CompilerReflection reflector(spirv);
        std::string foo = reflector.compile();

        std::cout << "[" << foo << "]\n";

        spirv_cross::ShaderResources shader_resources = reflector.get_shader_resources(reflector.get_active_interface_variables());

        // Here I must know the shader type


        /*
         class Resource:
    // The type ID of the variable which includes arrays and all type modifications.
    // This type ID is not suitable for parsing OpMemberDecoration of a struct and other decorations in general
    // since these modifications typically happen on the base_type_id.
            uint32_t type_id;

    // The base type of the declared resource.
    // This type is the base type which ignores pointers and arrays of the type_id.
    // This is mostly useful to parse decorations of the underlying type.
    // base_type_id can also be obtained with get_type(get_type(type_id).self).
        uint32_t base_type_id;
         */

        size_t foooo = shader_resources.storage_buffers.size();
        for (size_t i = 0; i < shader_resources.storage_buffers.size(); ++i) {
            const spirv_cross::Resource& resource = shader_resources.storage_buffers[i];
            const spirv_cross::SPIRType& type = reflector.get_type(resource.type_id);
            const spirv_cross::SPIRType& base_type = reflector.get_type(resource.base_type_id);

            VkDescriptorSetLayoutBinding dslb = {};
        }

            /*
        vk::DescriptorSetLayoutBinding descriptor_set_layout_binding(
            0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
             */

        // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#endif

        JAWS_FATAL1("Hello!");

        return 0;
    });
}

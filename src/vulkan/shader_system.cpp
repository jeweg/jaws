#include "pch.hpp"
#include "jaws/vulkan/shader_system.hpp"
#include "jaws/assume.hpp"
#include "jaws/vfs/vfs.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/device.hpp"
#include "jaws/vulkan/shader_resource.hpp"
#include "jaws/util/hashing.hpp"
#include "jaws/util/misc.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/hash/hash.h"
#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_reflect.hpp"

namespace jaws::vulkan {

namespace detail {

static constexpr absl::string_view to_string(shaderc_compilation_status s)
{
    switch (s) {
    case shaderc_compilation_status_success: return "success";
    case shaderc_compilation_status_invalid_stage: return "invalid stage deduction";
    case shaderc_compilation_status_compilation_error: return "compilation error";
    case shaderc_compilation_status_internal_error: return "internal error";
    case shaderc_compilation_status_null_result_object: return "null result object";
    case shaderc_compilation_status_invalid_assembly: return "invalid assembly";
    case shaderc_compilation_status_validation_error: return "validation error";
    default: return "<unknown>";
    }
}

static shaderc_shader_kind shader_kind_from_ext(std::string_view lc_ext)
{
    // Compatible w/ https://github.com/google/shaderc/tree/master/glslc#311-shader-stage-specification
    if (lc_ext == ".frag") {
        return shaderc_glsl_default_fragment_shader;
    } else if (lc_ext == ".vert") {
        return shaderc_glsl_default_vertex_shader;
    } else if (lc_ext == ".tesc") {
        return shaderc_glsl_default_tess_control_shader;
    } else if (lc_ext == ".tese") {
        return shaderc_glsl_default_tess_evaluation_shader;
    } else if (lc_ext == ".geom") {
        return shaderc_glsl_default_geometry_shader;
    } else if (lc_ext == ".comp") {
        return shaderc_glsl_default_compute_shader;
    }
    return shaderc_glsl_infer_from_source;
}

class ShaderSystemIncludeHandler : public shaderc::CompileOptions::IncluderInterface
{
private:
    const jaws::vfs::Vfs &_vfs;
    std::vector<FileWithFingerprint> *_out_included_files;

    struct FullResult
    {
        shaderc_include_result shaderc_result;
        // Must also keep the string data alive:
        std::string source_name;
        std::string content;
    };
    // Inter-member pointers, we need ptr stability before/after hash map insertion.
    using FullResultPtr = std::unique_ptr<FullResult>;

    // Results must be kept alive until told so.
    uintptr_t _next_result_id = 1;
    absl::flat_hash_map<uintptr_t, FullResultPtr> _alive_results;

public:
    ShaderSystemIncludeHandler(
        const jaws::vfs::Vfs &vfs,
        const vfs::Path &main_source_file,
        std::vector<FileWithFingerprint> *out_included_files) :
        _vfs(vfs), _out_included_files(out_included_files)
    {}

    shaderc_include_result *GetInclude(
        const char *requested_source, // foo.inc
        shaderc_include_type type,
        const char *requesting_source, // simple.comp
        size_t include_depth) override // 1
    {
        vfs::Path requesting_path(requesting_source);

        // For now, try relative only. TODO: smarter (and more) logic.

        vfs::Path try_path = requesting_path.get_parent_path() / vfs::Path(requested_source);
        try_path = _vfs.make_canonical(try_path);

        bool is_file = _vfs.lookup_backend(try_path)->is_file(try_path);

        size_t fingerprint = 0;
        std::string content = _vfs.read_text_file(try_path, &fingerprint);

        // TODO: problem: here we really need to differentiate between "empty file" and
        // file not found/other error. Including an empty file is fine, after all.
        if (!content.empty()) {
            uintptr_t this_result_id = _next_result_id++;

            FullResultPtr full_res = std::make_unique<FullResult>();
            full_res->content = content;
            full_res->source_name = try_path.string();
            full_res->shaderc_result = {};
            full_res->shaderc_result.source_name = full_res->source_name.c_str();
            full_res->shaderc_result.source_name_length = full_res->source_name.size();
            full_res->shaderc_result.content = full_res->content.c_str();
            full_res->shaderc_result.content_length = full_res->content.size();
            full_res->shaderc_result.user_data = reinterpret_cast<void *>(this_result_id);

            shaderc_include_result *returned = &full_res->shaderc_result;

            auto [iter, ok] = _alive_results.insert(std::make_pair(this_result_id, std::move(full_res)));
            JAWS_ASSUME(ok);

            _out_included_files->push_back(FileWithFingerprint(try_path, fingerprint));
            return returned;
        }

        JAWS_FATAL1("ouch");
        // nah, this will crash. how to correctly return error?
        return 0;
    }

    void ReleaseInclude(shaderc_include_result *data) override
    {
        if (data) {
            uintptr_t result_id = reinterpret_cast<uintptr_t>(data->user_data);
            auto iter = _alive_results.find(result_id);
            if (iter != _alive_results.end()) { _alive_results.erase(iter); }
        }
    }
};

}

ShaderSystem::ShaderSystem() = default;

ShaderSystem::~ShaderSystem()
{
    destroy();
}

void ShaderSystem::create(Device *device)
{
    JAWS_ASSUME(device);
    _device = device;
}


void ShaderSystem::destroy()
{
    if (_device) {
        if (_device->supports_validation_cache()) {
            vkDestroyValidationCacheEXT(_device->vk_handle(), _validation_cache, nullptr);
        }
        _device = nullptr;
    }
}


Shader ShaderSystem::get_shader(const ShaderCreateInfo &shader_ci)
{
    //-------------------------------------------------------------------------
    // Create validation cache on demand

    if (!_validation_cache && _device->supports_validation_cache()) {
        VkValidationCacheCreateInfoEXT ci = {VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT};
        JAWS_VK_HANDLE_FATAL(vkCreateValidationCacheEXT(_device->vk_handle(), &ci, nullptr, &_validation_cache));
    }

    //-------------------------------------------------------------------------
    // Cache lookup

    /*
    CachedShaderInformation *cached_shader_info =
        _shader_cache.lookup_or_remove(shader_ci, [](const ShaderCreateInfo &ci, const CachedShaderInformation &info) {
            // Check if the file fingerprints are still up-to-date.
            // If not, we recreate the cache entry. We must assume the fingerprint list
            // is incomplete anyway (more files might be #included now).
            for (auto &[path, fp] : info.involved_files) {
                size_t curr_fp = get_vfs().get_fingerprint(path);
                if (fp != curr_fp) {
                    // Returning from here means the cache lookup fails and this entry is removed.
                    return true;
                }
            }
            return false;
        });
    */

    // TODO: I disabeled the caching for the moment.

    //-------------------------------------------------------------------------
    // Load and compile to SPIR-V

    vfs::Path main_source_file = jaws::get_vfs().make_canonical(shader_ci.main_source_file);

    size_t code_fingerprint;
    std::string code = jaws::get_vfs().read_text_file(main_source_file, &code_fingerprint);
    if (code.empty()) { return nullptr; }

    shaderc::Compiler compiler;
    shaderc::CompileOptions compile_options;
    for (const auto &macro : shader_ci.compile_definitions) {
        compile_options.AddMacroDefinition(macro.first, macro.second);
    }

    // Setup include handler
    std::vector<FileWithFingerprint> involved_files = {
        FileWithFingerprint(shader_ci.main_source_file, code_fingerprint)};
    compile_options.SetIncluder(std::make_unique<detail::ShaderSystemIncludeHandler>(
        jaws::get_vfs(), shader_ci.main_source_file, &involved_files));

    // Detect shader kind (fragment, vertex, compute, ...)
    std::string lower_extension = jaws::util::to_lower(shader_ci.main_source_file.get_extension());
    shaderc_shader_kind shader_kind = detail::shader_kind_from_ext(lower_extension);

    auto compile_result = compiler.CompileGlslToSpv(
        code.c_str(),
        code.size(),
        shader_kind,
        shader_ci.main_source_file.string().c_str(),
        shader_ci.entry_point_name.c_str(),
        compile_options);

    Logger &logger = get_logger(Category::Vulkan);

    if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        logger.error(
            "Error compiling shader ({}): {}",
            detail::to_string(compile_result.GetCompilationStatus()),
            compile_result.GetErrorMessage());
        JAWS_FATAL1("Shader compilation failed");
    } else {
        logger.info(
            "Successfully compiled shader \"{}\" ({} errors, {} warnings)",
            main_source_file,
            compile_result.GetNumErrors(),
            compile_result.GetNumWarnings());
    }

    //-------------------------------------------------------------------------
    // Reflect on the generated SPIR-V
    // We will use this to infer descriptor set layouts.

    using CompiledElem = shaderc::SpvCompilationResult::element_type;
    // The # of elements (uint32_t), not bytes!
    const size_t num_compiled_elems = std::distance(compile_result.cbegin(), compile_result.cend());

    spirv_cross::CompilerReflection reflector(compile_result.cbegin(), num_compiled_elems);
    std::string reflected_json = reflector.compile();
    spirv_cross::ShaderResources shader_resources =
        reflector.get_shader_resources(reflector.get_active_interface_variables());

    //-------------------------------------------------------------------------
    // Create shader module

    VkShaderModuleCreateInfo mod_ci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    mod_ci.codeSize = num_compiled_elems * sizeof(CompiledElem);
    mod_ci.pCode = compile_result.cbegin();
    if (_device->supports_validation_cache()) { mod_ci.pNext = &_validation_cache; }


    VkShaderModule vk_shader_module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(_device->vk_handle(), &mod_ci, nullptr, &vk_shader_module);
    JAWS_VK_HANDLE_FATAL(result);

    return Shader(_shader_cache.emplace(shader_ci, _device, vk_shader_module));
}


}

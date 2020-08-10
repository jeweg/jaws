#include "pch.hpp"
#include "jaws/jaws.hpp"
#include "jaws/assume.hpp"
#include "jaws/vfs/vfs.hpp"
#include "jaws/vulkan/vulkan.hpp"
#include "jaws/vulkan/shader_system.hpp"
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
public:
    /*
// Source text inclusion via #include is supported with a pair of callbacks
// to an "includer" on the client side.  The first callback processes an
// inclusion request, and returns an include result.  The includer owns
// the contents of the result, and those contents must remain valid until the
// second callback is invoked to release the result.  Both callbacks take a
// user_data argument to specify the client context.
// To return an error, set the source_name to an empty string and put your
// error message in content.

// An include result.
    typedef struct shaderc_include_result {
        // The name of the source file.  The name should be fully resolved
        // in the sense that it should be a unique name in the context of the
        // includer.  For example, if the includer maps source names to files in
        // a filesystem, then this name should be the absolute path of the file.
        // For a failed inclusion, this string is empty.
        const char* source_name;
        size_t source_name_length;
        // The text contents of the source file in the normal case.
        // For a failed inclusion, this contains the error message.
        const char* content;
        size_t content_length;
        // User data to be passed along with this request.
        void* user_data;   } shaderc_include_result;
     */

private:
    const jaws::vfs::Vfs &_vfs;
    std::vector<ShaderSystem::FileWithFingerprint> *_out_included_files;

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
    absl::flat_hash_map<uint32_t, FullResultPtr> _alive_results;

public:
    ShaderSystemIncludeHandler(
        const jaws::vfs::Vfs &vfs,
        const vfs::Path &main_source_file,
        std::vector<ShaderSystem::FileWithFingerprint> *out_included_files) :
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
        bool is_dir = _vfs.lookup_backend(try_path)->is_dir(try_path);

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

            _out_included_files->push_back(ShaderSystem::FileWithFingerprint(try_path, fingerprint));
            return returned;
        };


        JAWS_FATAL1("ouch");
        // nah, this will crash. how to correctly return error?
        return 0;
    }

    void ReleaseInclude(shaderc_include_result *data) override
    {
        if (data) {
            uint32_t result_id = reinterpret_cast<uintptr_t>(data->user_data);
            auto iter = _alive_results.find(result_id);
            if (iter != _alive_results.end()) { _alive_results.erase(iter); }
        }
    }
};


#if 0
class ShaderSystemImpl
{
    jaws::Instance* _instance = nullptr;

    struct ShaderCacheValue
    {
        std::vector<std::string> involved_files;
        std::vector<jaws::util::file_observing::State> involved_file_states;
        std::vector<uint32_t> spirv_code;
    };
    mutable jaws::util::HashedCache<ShaderCacheValue, std::string, std::string> _shader_cache;

public:
    void create(jaws::Instance* instance)
    {
        JAWS_ASSUME(instance != nullptr);
        _instance = instance;
    }

    void destroy() {}

    Shader* get_shader(const std::string& root_source_file) const
    {
        auto& logger = jaws::GetLogger(jaws::Category::Vulkan);
        namespace fo = jaws::util::file_observing;

        //=========================================================================

#    if 0

		//=========================================================================
		// Transformation root_source_file --> spirv + involved files
		{
			ShaderCacheValue* val = _shader_cache.lookup(vfs_name, root_source_file);
			bool cached_is_still_fresh = false;
			if (val) {
				// Now we know all involved files. Let's see if any of them changed.
				bool something_changed = false;
				for (size_t i = 0; i < val->involved_files.size(); ++i) {
					if (fo::get_file_event(val->involved_files[i], val->involved_file_states[i]) != fo::Event::None) {
						something_changed = true;
						// Don't break, we want updates of all the file states.
					}
				}
				cached_is_still_fresh = !something_changed;
			}
			if (!cached_is_still_fresh) {

				// Compile and collect involved source files.

				auto* vfs = _instance->get_vfs(vfs_name);
				if (!vfs) { return nullptr; }
				if (!vfs->is_file(root_source_file)) { return nullptr; }
				std::string code = vfs->get_file_contents(root_source_file);
				if (code.empty()) {
					return nullptr;
				}

				shaderc::Compiler compiler;
				shaderc::CompileOptions compile_options;

				std::string lower_extension = jaws::util::to_lower(jaws::vfs::Vfs::extension(root_source_file));
				shaderc_shader_kind shader_kind = shader_kind_from_ext(lower_extension);

				std::vector<std::string> involved_files = { root_source_file };
				compile_options.SetIncluder(std::make_unique<ShaderSystemIncludeHandler>(vfs_name, involved_files));
				auto compile_result = compiler.CompileGlslToSpv(code.c_str(), code.size(), shader_kind, root_source_file.c_str(), compile_options);
				if (compile_result.GetCompilationStatus() != shaderc_compilation_status_success) {
					logger.error(
						"Error compiling shader ({}): {}",
						detail::to_string(compile_result.GetCompilationStatus()),
						compile_result.GetErrorMessage());
					JAWS_FATAL1("Shader compilation failed");
				} else {
					logger.info(
						"Successfully compiled shader ({} errors, {} warnings)",
						compile_result.GetNumErrors(),
						compile_result.GetNumWarnings());
				}

				ShaderCacheValue cached;
				cached.vfs_name = vfs_name;
				cached.involved_files = std::move(involved_files);
				cached.spirv_code.assign(compile_result.cbegin(), compile_result.cend());
				_shader_cache.insert(std::move(cached), vfs_name, root_source_file);

			}
			return nullptr;

		}
        //-------------<

        _compile_options.SetIncluder(std::make_unique<ShaderSystemIncludeHandler>(*this));

        auto *vfs = _instance->get_vfs(vfs_name);
        if (!vfs) { return false; }
        if (!vfs->is_file(path)) { return false; }

        std::string code = vfs->get_file_contents(path);

        // Why I'm doing this? Because the interface forces me to pass something, and the only thing I could
        // always pass is infer_from_source, but I don't want to force the pragma everywhere.
        // Where it makes sense, I'm keeping this compatible with
        // https://github.com/google/shaderc/tree/master/glslc#311-shader-stage-specification
        shaderc_shader_kind shader_kind = shaderc_glsl_infer_from_source;
        std::string extension = ".frag"; //path.extension().string();
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

        auto compile_result = _compiler.CompileGlslToSpv(
            code.c_str(),
            code.size(),
            shader_kind,
            path.c_str(),
            _compile_options);

        shaderc_compilation_status status = compile_result.GetCompilationStatus();
        if (status != shaderc_compilation_status_success) {
            logger->error(
                "Shader compilation error ({}): {}",
                jaws::vulkan::ToString(status),
                compile_result.GetErrorMessage());
            JAWS_FATAL1("Shader compilation failed");
        } else {
            logger->info(
                "Successfully compiled shader ({} errors, {} warnings)",
                compile_result.GetNumErrors(),
                compile_result.GetNumWarnings());
        }
        return device->createShaderModuleUnique(
            vk::ShaderModuleCreateInfo{}
                .setPCode(compile_result.cbegin())
                .setCodeSize(sizeof(uint32_t) * std::distance(compile_result.cbegin(), compile_result.cend())));
#    endif
    }
};
#endif

} // namespace detail

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
            vkDestroyValidationCacheEXT(_device->get_device(), _validation_cache, nullptr);
        }
        _full_info_cache.clear();
        _device = nullptr;
    }
}


ShaderPtr ShaderSystem::get_shader(const ShaderCreateInfo &ci)
{
    //-------------------------------------------------------------------------
    // Create validation cache on demand

    if (!_validation_cache && _device->supports_validation_cache()) {
        VkValidationCacheCreateInfoEXT ci = {VK_STRUCTURE_TYPE_VALIDATION_CACHE_CREATE_INFO_EXT};
        JAWS_VK_HANDLE_FATAL(vkCreateValidationCacheEXT(_device->get_device(), &ci, nullptr, &_validation_cache));
    }

    //-------------------------------------------------------------------------
    // High-level cache

#if 0
    if (FullInfo* ei = _full_info_cache.lookup_keyed(ci)) {
        // Check files
        bool files_changed = false;
        for (const auto& [path, fingerprint] : ei->involved_files_w_fingerprints) {
            size_t fp_now = jaws()->get_vfs().get_fingerprint(path);
            if (fp_now != fingerprint) {
                files_changed = true;
                break;
            }
        }
        if (!files_changed) {

            /*
            VkShaderModule sm = _device->get_sediment()->lookup_or_create(nullptr, &ei->sediment_hash, nullptr);
            JAWS_ASSUME(sm != VK_NULL_HANDLE);
            return nullptr;
             */
        };
    }
#endif

    vfs::Path main_source_file = jaws::get_vfs().make_canonical(ci.main_source_file);

    size_t code_fingerprint;
    std::string code = jaws::get_vfs().read_text_file(main_source_file, &code_fingerprint);
    if (code.empty()) { return nullptr; }

    shaderc::Compiler compiler;
    shaderc::CompileOptions compile_options;
    for (const auto &macro : ci.compile_definitions) { compile_options.AddMacroDefinition(macro.first, macro.second); }

    // Setup include handler
    std::vector<FileWithFingerprint> involved_files = {FileWithFingerprint(ci.main_source_file, code_fingerprint)};
    compile_options.SetIncluder(
        std::make_unique<detail::ShaderSystemIncludeHandler>(jaws::get_vfs(), ci.main_source_file, &involved_files));

    // Detect shader kind (fragment, vertex, compute, ...)
    std::string lower_extension = jaws::util::to_lower(ci.main_source_file.get_extension());
    shaderc_shader_kind shader_kind = detail::shader_kind_from_ext(lower_extension);


    auto compile_result = compiler.CompileGlslToSpv(
        code.c_str(),
        code.size(),
        shader_kind,
        ci.main_source_file.string().c_str(),
        ci.entry_point_name.c_str(),
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

    // std::vector<uint32_t> spirv_code;
    // spirv_code.assign(compile_result.cbegin(), compile_result.cend());

    using CompiledElem = shaderc::SpvCompilationResult::element_type;
    // The # of elements (uint32_t), not bytes!
    const size_t num_compiled_elems = std::distance(compile_result.cbegin(), compile_result.cend());

    spirv_cross::CompilerReflection reflector(compile_result.cbegin(), num_compiled_elems);
    std::string reflected_json = reflector.compile();
    spirv_cross::ShaderResources shader_resources =
        reflector.get_shader_resources(reflector.get_active_interface_variables());

    //-------------------------------------------------------------------------
    // Compile to SPIR-V

    vfs::Path main_source_file = jaws::get_vfs().make_canonical(ci.main_source_file);

    size_t code_fingerprint;
    std::string code = jaws::get_vfs().read_text_file(main_source_file, &code_fingerprint);
    if (code.empty()) { return nullptr; }

    shaderc::Compiler compiler;
    shaderc::CompileOptions compile_options;
    for (const auto &macro : ci.compile_definitions) { compile_options.AddMacroDefinition(macro.first, macro.second); }

    // Setup include handler
    std::vector<FileWithFingerprint> involved_files = {FileWithFingerprint(ci.main_source_file, code_fingerprint)};
    compile_options.SetIncluder(
        std::make_unique<detail::ShaderSystemIncludeHandler>(jaws::get_vfs(), ci.main_source_file, &involved_files));

    // Detect shader kind (fragment, vertex, compute, ...)
    std::string lower_extension = jaws::util::to_lower(ci.main_source_file.get_extension());
    shaderc_shader_kind shader_kind = detail::shader_kind_from_ext(lower_extension);


    auto compile_result = compiler.CompileGlslToSpv(
        code.c_str(),
        code.size(),
        shader_kind,
        ci.main_source_file.string().c_str(),
        ci.entry_point_name.c_str(),
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

    // std::vector<uint32_t> spirv_code;
    // spirv_code.assign(compile_result.cbegin(), compile_result.cend());

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

    ShaderPtr result_shader(new Shader(_device));
    vkCreateShaderModule(_device->get_device(), &mod_ci, nullptr, &result_shader->_shader_module);
}


} // namespace jaws::vulkan

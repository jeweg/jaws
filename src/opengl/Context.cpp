#include "pch.hpp"
#include "jaws/Instance.hpp"
#include "jaws/opengl/Context.hpp"
#include "jaws/opengl/OpenGL.hpp"

// Just include the headers and let them sort out
// platform dependency themselves. After this there
// should be exactly one declaraction of class ContextImpl.
#include "ContextImplWindows.hpp"

#include <boost/stacktrace.hpp>
#include <string>
#include <algorithm>
#include <cassert>

#include <iostream> // Temp

// TODO: this same code in in OpenGL.hpp. Why do I have to repeat it here to
// avoid an error?
#if defined(_WIN32) && !defined(APIENTRY)
#    define APIENTRY __stdcall
#endif

static void APIENTRY DebugMessageCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* user_param)
{
    auto context = const_cast<jaws::opengl::Context*>(static_cast<const jaws::opengl::Context*>(user_param));
    context->DebugMsgCallback(source, type, id, severity, length, msg);
}


namespace jaws { namespace opengl {

Context::Context() noexcept : gl(_glad_context) {}


Context::~Context() noexcept
{
    Destroy();
}


void Context::SetFormat(const Format& f)
{
    _format = f;
}


Context::Format Context::GetFormat()
{
    return _format;
}


void Context::Create(
    NativeDeviceContext device_context,
    OpenGLVersion min_version,
    OpenGLVersion max_version,
    Context* share_context,
    NativeOpenGLContext native_share_context)
{
    if (!_impl) { _impl = std::make_unique<ContextImpl>(); }

    // TODO: destroy before re-create?
    _format = _impl->Create(device_context, _format, min_version, max_version, share_context, native_share_context);

    int version = gladLoaderLoadGLContext(&_glad_context);
    if (version == 0) { JAWS_FATAL1("Failed to initialize OpenGL context"); }

    _debugging_enabled = _format.GetOption(Options::Debug);
    if (_debugging_enabled) {
        gl.Enable(GL_DEBUG_OUTPUT);
        // gl.DebugMessageControl();
        gl.DebugMessageCallback(DebugMessageCallback, this);
        InsertDebugMsg("Debugging initialized");
    }


    // During development: check color encoding of front buffer

    auto& logger = GetLogger(Category::OpenGL);

    GLint encoding = -1;
    gl.GetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
    if (encoding == GL_LINEAR)
        logger.info("default fb color is: linear");
    else if (encoding == GL_SRGB)
        logger.info("default fb color is: sRGB");
}


void Context::Destroy()
{
    _impl->Destroy();
    // Do not delete the context impl object - it might make sense
    // for it to hold onto data or resources even when no context
    // is currently in existence.
}


bool Context::MakeCurrent(NativeDeviceContext dc)
{
    // TODO: Guard against 1) no _impl created and 2) no context present (after Destroy()).
    return _impl->MakeCurrent(dc);
}


bool Context::MakeNonCurrent()
{
    return _impl->MakeNonCurrent();
}


void Context::SetVSyncMode(VSync vs)
{
    return _impl->SetVSyncMode(vs);
}


Context::InfoDict Context::GetInfo(bool include_extensions) const
{
    using namespace std;
    // using std::to_string; // Why doesn't this work?

    InfoDict info;
    info.push_back(std::make_pair("GL_VENDOR", to_string(gl.GetString(GL_VENDOR))));
    info.push_back(std::make_pair("GL_RENDERER", to_string(gl.GetString(GL_RENDERER))));
    info.push_back(std::make_pair("GL_VERSION", to_string(gl.GetString(GL_VERSION))));
    info.push_back(std::make_pair("GL_SHADING_LANGUAGE_VERSION", to_string(gl.GetString(GL_SHADING_LANGUAGE_VERSION))));

    if (include_extensions) {
        std::vector<std::string> ext_names;
        const GLubyte* ext_name = nullptr;
        GLuint i = 0;
        while (true) {
            ext_name = gl.GetStringi(GL_EXTENSIONS, i++);
            if (!ext_name) { break; }
            ext_names.push_back(to_string(ext_name));
        }
        // Clear the error flag that was set by GetStringi().
        gl.GetError();

        std::stable_sort(ext_names.begin(), ext_names.end());

        size_t index = 1;
        for (const auto& n : ext_names) { info.push_back(std::make_pair("Ext #" + std::to_string(index++), n)); }
    }

    // Let the platform impl append its own indo (like GLX extensions).
    if (_impl) { _impl->GetInfo(info, include_extensions); }
    return info;
}


void Context::DebugMsgCallback(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg)
{
    // Map to log levels and log the msg.
    auto level = spdlog::level::level_enum::info;
    if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR) {
        level = spdlog::level::level_enum::err;
    } else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        level = spdlog::level::level_enum::warn;
    }
    GetLogger(Category::OpenGL).log(level, msg);
}


void Context::InsertDebugMsg(const char* msg, GLuint id, GLenum severity)
{
    gl.DebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, id, severity, -1, msg);
}


}} // namespace jaws::opengl

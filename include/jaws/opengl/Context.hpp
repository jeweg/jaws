#pragma once
#include <jaws_export.h>
#include <jaws/opengl/OpenGL.hpp>
#include <memory>
#include <cstdint>

struct GladGLContext;

namespace jaws::opengl {

class ContextImpl;

// TOOD: realistically, we should limit to at least OpenGL 3.0.
// prior to that we don't have the modern creation functions and
// no true offscreen rendering support. It doesn't seem worth the
// hassle to work around that.

// TODO: support msaa default framebuffer.
class JAWS_API Context
{
public:
    using NativeOpenGLContext = void*;
    using NativeDeviceContext = void*;


    Context() noexcept;
    ~Context() noexcept;

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    enum class Profile
    {
        Core,
        Compatibility
    };
    enum class Options : uint32_t
    {
        Stereo = 1,
        Debug = 2,
        sRGB = 4,
        SingleBuffered = 8,
        Multisampled = 16
    };

    struct Format
    {
        int depth_bits{24};
        int stencil_bits{8};
        Profile profile{Profile::Core};
        uint32_t options{0};

        bool GetOption(Context::Options option) const { return (options & static_cast<uint32_t>(option)) != 0; }

        void SetOption(Context::Options option, bool on_off = true)
        {
            auto mask = static_cast<uint32_t>(option);
            if (on_off)
                options |= mask;
            else
                options &= ~mask;
        }
    };

    // Has no effect until Create() is called.
    void SetFormat(const Format&);

    // Before the call to create() this it the requested format, afterwards
    // it's the chosen one.
    Format GetFormat();

    // Creates or recreates the context based on the
    // currently set format. All arguments that do not end up readable in
    // the format after creation are passed directly.
    void Create(
        NativeDeviceContext device_context,
        OpenGLVersion min_version,
        OpenGLVersion max_version,
        Context* share_context = nullptr,
        NativeOpenGLContext native_share_context = nullptr);

    void Destroy();

    bool MakeCurrent(NativeDeviceContext = nullptr);

    bool MakeNonCurrent();

    // We might want to make this a queued command later on.
    // Not sure if that makes sense
    enum class VSync
    {
        Adaptive,
        Always,
        Off
    };
    void SetVSyncMode(VSync);

    // Access to the OpenGL commands and extensions.
    //   Context context;
    //   context->Enable(GL_DEPTH_TEST);
    // etc.

    // Must be current!
    using InfoDict = std::vector<std::pair<std::string, std::string>>;
    InfoDict GetInfo(bool include_extensions = true) const;

    const GladGLContext& gl;


    // Debugging
    // Might wanna move this into a sub-object.

    void InsertDebugMsg(const char* msg, GLuint id = 0, GLenum severity = GL_DEBUG_SEVERITY_NOTIFICATION);


    void DebugMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg);

private:
    friend ContextImpl;

    Format _format;
    std::unique_ptr<ContextImpl> _impl;
    GladGLContext _glad_context;

    bool _debugging_enabled{false};
};

} // namespace jaws::opengl

#pragma once

#if _WIN32

#    include "jaws/core.hpp"
#    include "jaws/opengl/Context.hpp"
#    include "jaws/windows.hpp"

namespace jaws { namespace opengl {

class JAWS_EXPORT ContextImpl
{
public:
    // Note that device_context may be nullptr here which means
    // offscreen context w/o a window. In the Context class we make
    // these cases two different functions because while they are
    // technically close, they have quite different semantics (no
    // swapping, no redering in default framebuffer).
    Context::Format Create(
        Context::NativeDeviceContext device_context,
        const Context::Format& format,
        OpenGLVersion min_version,
        OpenGLVersion max_version,
        Context* share_context,
        Context::NativeOpenGLContext native_share_context);

    void Destroy();

    bool MakeCurrent(Context::NativeDeviceContext hdc = nullptr);
    bool MakeNonCurrent();

    void SetVSyncMode(Context::VSync);

    void GetInfo(Context::InfoDict&, bool include_extensions = true) const;

private:
    static bool _wgl_initialized;

    HGLRC _context_handle{nullptr};

    // When no device context is specified on creation,
    // we create a hidden window.
    void EnsureHiddenWindow();
    HDC _hidden_window_hdc{nullptr};
    HWND _hidden_window_hwnd{nullptr};
};
}} // namespace jaws::opengl

#endif

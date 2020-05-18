#include "pch.hpp"
#if _WIN32

#    include "ContextImplWindows.hpp"
#    include "common.hpp"

#    include <glad/gl.h>
#    include <glad/wgl.h>
#    include <vector>
#    include <cassert>
#    include <algorithm>
#    include <iostream>

// https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)#Proper_Context_Creation

// TODO:https://github.com/glfw/glfw/blob/2c1fc13ee4094f579981a5e18a06dccb0a009e68/src/wgl_context.c

namespace aaa834285 {

// Returns the specified attribute of the specified pixel format
int GetPixelFormatAttrib(HDC hdc, int pixelFormat, int attrib)
{
    int value = 0;
    assert(GLAD_WGL_ARB_pixel_format); // Necessary
    if (!wglGetPixelFormatAttribivARB(hdc, pixelFormat, 0, 1, &attrib, &value)) {
        //_glfwInputErrorWin32(GLFW_PLATFORM_ERROR, "WGL: Failed to retrieve pixel format attribute");
        assert(false);
        return 0;
    }
    return value;
}


struct PixelFormat
{
    int handle{-1};
    int depth_bits{-1};
    int stencil_bits{-1};
    bool stereo{false};
    bool double_buffered{false};
    bool multisampled{false};
    bool sRGB{false};
};

std::ostream& operator<<(std::ostream& os, const PixelFormat& f)
{
    std::ios_base::fmtflags flags(os.flags());
    os << std::boolalpha;

    os << f.handle << ":\n";
    os << "    depth_bits: " << f.depth_bits << "\n";
    os << "    stencil_bits: " << f.stencil_bits << "\n";
    os << "    stereo: " << f.stereo << "\n";
    os << "    double_buffered: " << f.double_buffered << "\n";
    os << "    multisampled: " << f.multisampled << "\n";
    os << "    sRGB: " << f.sRGB << "\n";

    os.flags(flags);
    return os;
}

// Pixel formats must at least support accelerated OpenGL and non-palette 32-bit RGBA color (Windows 10 dropped
// 16-bit support, so why support it here?).
// We also ignore those without alpha channel since destination alpha is sometimes
// useful for blending operation and we likely don't win anything by dropping the alpha channel.
std::vector<PixelFormat> GetPixelFormatCandidates(HDC hdc, const jaws::opengl::Context::Format& requested_format)
{
    std::vector<PixelFormat> result;
    int count;
    if (GLAD_WGL_ARB_pixel_format) {
        count = GetPixelFormatAttrib(hdc, 1, WGL_NUMBER_PIXEL_FORMATS_ARB);
    } else {
        count = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
    }
    result.reserve(count);

    // Note that this loop is 1-based to match the pixel format APIs.
    for (int i = 1; i <= count; i++) {
        PixelFormat pf;
        pf.handle = i;
        if (GLAD_WGL_ARB_pixel_format) {
            // Modern API

            if (!GetPixelFormatAttrib(hdc, i, WGL_SUPPORT_OPENGL_ARB)
                || !GetPixelFormatAttrib(hdc, i, WGL_DRAW_TO_WINDOW_ARB)) {
                continue;
            }
            if (GetPixelFormatAttrib(hdc, i, WGL_ACCELERATION_ARB) != WGL_FULL_ACCELERATION_ARB) { continue; }
            if (GetPixelFormatAttrib(hdc, i, WGL_PIXEL_TYPE_ARB) != WGL_TYPE_RGBA_ARB) { continue; }
            if (GetPixelFormatAttrib(hdc, i, WGL_ACCELERATION_ARB) == WGL_NO_ACCELERATION_ARB) { continue; }
            if (GetPixelFormatAttrib(hdc, i, WGL_RED_BITS_ARB) != 8) { continue; }
            if (GetPixelFormatAttrib(hdc, i, WGL_GREEN_BITS_ARB) != 8) { continue; }
            if (GetPixelFormatAttrib(hdc, i, WGL_BLUE_BITS_ARB) != 8) { continue; }
            if (GetPixelFormatAttrib(hdc, i, WGL_ALPHA_BITS_ARB) != 8) { continue; }

            pf.depth_bits = GetPixelFormatAttrib(hdc, i, WGL_DEPTH_BITS_ARB);
            pf.stencil_bits = GetPixelFormatAttrib(hdc, i, WGL_STENCIL_BITS_ARB);

            if (GetPixelFormatAttrib(hdc, i, WGL_STEREO_ARB)) pf.stereo = true;
            if (GetPixelFormatAttrib(hdc, i, WGL_DOUBLE_BUFFER_ARB)) pf.double_buffered = true;
            if (GLAD_WGL_ARB_multisample) pf.multisampled = GetPixelFormatAttrib(hdc, i, WGL_SAMPLES_ARB);
            if (GLAD_WGL_ARB_framebuffer_sRGB || GLAD_WGL_EXT_framebuffer_sRGB) {
                if (GetPixelFormatAttrib(hdc, i, WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB)) pf.sRGB = true;
            }
        } else {
            // Legacy API
#    if 0
            PIXELFORMATDESCRIPTOR pfd;
            if (!DescribePixelFormat(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) { continue; }
            if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL)) { continue; }
            if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT)) { continue; }
            if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT)) { continue; }
            if (pfd.cRedBits != 8) { continue; }
            if (pfd.cGreenBits != 8) { continue; }
            if (pfd.cBlueBits != 8) { continue; }
            if (pfd.cAlphaBits != 8) { continue; }
            if (pfd.iPixelType != PFD_TYPE_RGBA) continue;
            if (pfd.cColorBits != 32) continue;

            bool double_buffered = pfd.dwFlags & PFD_DOUBLEBUFFER;

            pf.depthBits = pfd.cDepthBits;
            pf.stencilBits = pfd.cStencilBits;
            if (pfd.dwFlags & PFD_STEREO) pf.stereo = true;
            if (pfd.dwFlags & PFD_DOUBLEBUFFER) pf.double_buffered = true;
#    endif
        }
        result.push_back(pf);
    }
    return result;
}

int ChooseAndSetPixelFormat(
    HDC hdc, const jaws::opengl::Context::Format& requested_format, PIXELFORMATDESCRIPTOR& output_pfd)
{
    auto cand_formats = GetPixelFormatCandidates(hdc, requested_format);

    auto& logger = jaws::GetLogger(jaws::Category::General);

    // Now assign a penalty to each format..

    constexpr int Penalty_DepthBits = 50;
    constexpr int Penalty_StencilBits = 100;
    constexpr int Penalty_Stereo = 100000;
    constexpr int Penalty_sRGB = 1000;
    constexpr int Penalty_SingleBuffered = 20;
    constexpr int Penalty_Multisampled = 500;

    using jaws::opengl::Context;
    struct PenaltyFormatPair
    {
        int penalty{0};
        PixelFormat format;
    };
    std::vector<PenaltyFormatPair> penalties;
    penalties.reserve(cand_formats.size());
    for (auto cand : cand_formats) {
        int penalty = 0;

        if (cand.depth_bits < requested_format.depth_bits) { penalty += Penalty_DepthBits; }
        if (cand.stencil_bits < requested_format.depth_bits) { penalty += Penalty_StencilBits; }
        if (cand.stereo < requested_format.GetOption(Context::Options::Stereo)) { penalty += Penalty_Stereo; }
        if (cand.sRGB != requested_format.GetOption(Context::Options::sRGB)) { penalty += Penalty_sRGB; }
        if (cand.double_buffered == requested_format.GetOption(Context::Options::SingleBuffered)) {
            penalty += Penalty_SingleBuffered;
        }
        if (cand.multisampled != requested_format.GetOption(Context::Options::Multisampled)) {
            penalty += Penalty_Stereo;
        }

        PenaltyFormatPair pair;
        pair.penalty = penalty;
        pair.format = cand;
        penalties.push_back(pair);
    }

    // Sort them by penalty
    std::sort(penalties.begin(), penalties.end(), [](const PenaltyFormatPair& p1, const PenaltyFormatPair& p2) -> bool {
        return p1.penalty < p2.penalty;
    });

    // ..and try setting them in order of increasing penalty.
    bool ok = false;
    for (const auto& pair : penalties) {
        ok = SetPixelFormat(hdc, pair.format.handle, &output_pfd);
        if (ok) {
            // logger.info("format {} worked!", pair.format);
            logger.debug("format {} worked!", pair.format);
            // SPDLOG_DEBUG(logger, "format {} worked!", pair.format);
            break;
        } else {
            // logger.info("format {} did NOT work", pair.format);
            logger.debug("format {} did NOT work", pair.format);
            // SPDLOG_DEBUG(logger, "format {} did NOT work", pair.format);
        }
    }
    return ok;
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_CREATE: return 0;
    case WM_PAINT: {
        HDC hdc;
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: return 0;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}

static void CreateDummyWindow(HWND& out_hwnd, HDC& out_hdc)
{
    static const TCHAR WINDOW_CLASS_NAME[] = TEXT("ogl-dummy-wndclss");
    auto hInstance = jaws::GetHInstance();
    WNDCLASS wc = {};
    wc.lpfnWndProc = &WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    RegisterClass(&wc);
    HWND dummy_window = CreateWindowEx(
        0,
        WINDOW_CLASS_NAME,
        TEXT("Invisible dummy window"),
        0, // WS_OVERLAPPEDWINDOW,
        0,
        0,
        1,
        1,
        NULL,
        NULL,
        hInstance,
        NULL);
    ShowWindow(dummy_window, SW_HIDE);
    out_hwnd = dummy_window;
    out_hdc = GetDC(dummy_window);
}

static void InitializeWGL()
{
    HWND dummy_window;
    HDC hdc;
    CreateDummyWindow(dummy_window, hdc);
    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                                 1,
                                 PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
                                 PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
                                 32, // Color depth of the framebuffer.
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 24, // Number of bits for the depthbuffer
                                 8, // Number of bits for the stencilbuffer
                                 0, // Number of Aux buffers in the framebuffer.
                                 PFD_MAIN_PLANE,
                                 0,
                                 0,
                                 0,
                                 0};

    int chosen_pixel_format = ChoosePixelFormat(hdc, &pfd);
    BOOL result = SetPixelFormat(hdc, chosen_pixel_format, &pfd);
    assert(result == TRUE);
    HGLRC dummy_context = wglCreateContext(hdc);
    wglMakeCurrent(hdc, dummy_context);

    result = gladLoaderLoadWGL(hdc);
    assert(result);
    // TODO: proper fatal error handling (similiar to svkit)

    bool ok = wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, hdc);
    DestroyWindow(dummy_window);
    // UnregisterClass(WINDOW_CLASS_NAME, hInstance);

    // TODO: not sure if we must keep this context around for wgl functions to work. Probably not.
}

} // namespace aaa834285

namespace jaws { namespace opengl {
bool ContextImpl::_wgl_initialized = false;


void ContextImpl::EnsureHiddenWindow()
{
    if (!_hidden_window_hwnd) {
        // LOG(INFO) << "creating window for hidden context creation";
        aaa834285::CreateDummyWindow(_hidden_window_hwnd, _hidden_window_hdc);
    }
}


Context::Format ContextImpl::Create(
    Context::NativeDeviceContext device_context,
    const Context::Format& format,
    OpenGLVersion min_version,
    OpenGLVersion max_version,
    Context* share_context,
    Context::NativeOpenGLContext native_share_context)
{
    // TODO: make this thread-safe.
    if (!_wgl_initialized) {
        aaa834285::InitializeWGL();
        _wgl_initialized = true;
    }

    // Unfortunately, at least on Windows creating a truely offscreen context won't work,
    // see https://stackoverflow.com/a/7062980.
    // So we create a hidden window if no device_context is specified and use that.
    // We could pass nullptr to MakeCurrent which *should* omit the default framebuffer.
    // Alas, it doesn't work at all for me. Both wglMakeContextCurrentARB and
    // wglMakeCurrent return false if passed a null hdc.
    // What we will do here instead is create an invisible dummy window.
    HDC hdc = static_cast<HDC>(device_context);
    if (!hdc) {
        EnsureHiddenWindow();
        hdc = _hidden_window_hdc;
    }
    assert(GLAD_WGL_ARB_create_context);
    assert(GLAD_WGL_ARB_pixel_format);
    assert(GLAD_WGL_ARB_create_context_profile);
    assert(GLAD_WGL_EXT_swap_control);

    PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR), 0};
    bool ok = aaa834285::ChooseAndSetPixelFormat(hdc, format, pfd);
    // TODO: error handling

    // Create OpenGL context.
    // https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)#Create_Context_with_Attributes
    {
        HGLRC used_share_context = nullptr;
        if (share_context) {
            auto impl = dynamic_cast<ContextImpl*>(share_context->_impl.get());
            if (impl) { used_share_context = impl->_context_handle; }
        }
        if (!used_share_context && native_share_context) {
            used_share_context = static_cast<HGLRC>(native_share_context);
        }

        auto attempted_version = max_version;

        while (true) {
            int32_t attempted_version_major, attempted_version_minor;
            UnpackVersion(attempted_version, attempted_version_major, attempted_version_minor);

            int flags = 0;
            if (format.options & static_cast<uint32_t>(Context::Options::Debug)) { flags |= WGL_CONTEXT_DEBUG_BIT_ARB; }
            if (format.options & static_cast<uint32_t>(Context::Profile::Core)) {
                flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
            }
            std::vector<int> iattribs = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                                         attempted_version_major,
                                         WGL_CONTEXT_MINOR_VERSION_ARB,
                                         attempted_version_minor,
                                         WGL_CONTEXT_LAYER_PLANE_ARB,
                                         0,
                                         WGL_CONTEXT_PROFILE_MASK_ARB,
                                         WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                                         WGL_CONTEXT_FLAGS_ARB,
                                         flags};
            iattribs.push_back(0);
            iattribs.push_back(0);
            _context_handle = wglCreateContextAttribsARB(hdc, used_share_context, iattribs.data());
            if (_context_handle != nullptr) { break; }

            // Try next lower version if there is one.
            if (static_cast<int32_t>(attempted_version) > static_cast<int32_t>(min_version)) {
                attempted_version = static_cast<OpenGLVersion>(static_cast<int32_t>(attempted_version) - 1);
            } else {
                // TODO: error.
                assert(false);
            }
        } // Version loop
    }
    MakeCurrent(device_context);

    // TODO: We should return the format we got from the OpenGL subsystem..


    return format;
}


void ContextImpl::Destroy()
{
    MakeNonCurrent();
    wglDeleteContext(_context_handle);
    _context_handle = nullptr;

    // If we had to create the hidden window, destroy that as well.
    if (_hidden_window_hdc) { assert(_hidden_window_hwnd); }
}


bool ContextImpl::MakeCurrent(Context::NativeDeviceContext device_context)
{
    /* Passing nullptr here is officially allowed:
    "If the OpenGL context version of <hglrc> is 3.0 or above, and if
    either the <hdc> parameter of wglMakeCurrent is NULL, or both of the
    <hDrawDC> and <hReadDC> parameters of wglMakeContextCurrentARB are
    NULL, then the context is made current, but with no default
    framebuffer defined. The effects of having no default framebuffer on
    the GL are defined in Chapter 4 of the OpenGL 3.0 Specification.
     */
    HDC hdc = static_cast<HDC>(device_context);
    if (!hdc) {
        EnsureHiddenWindow();
        hdc = _hidden_window_hdc;
    }
    return wglMakeCurrent(hdc, _context_handle);
    // return wglMakeContextCurrentARB(hdc, hdc, _context_handle);
}


bool ContextImpl::MakeNonCurrent()
{
    return wglMakeCurrent(nullptr, nullptr);
}


void ContextImpl::SetVSyncMode(Context::VSync vs)
{
    if (vs == Context::VSync::Adaptive) {
        // Adaptive falls back to Always if the extension
        // is not available.
        if (GLAD_WGL_EXT_swap_control_tear) {
            // TODO: warning
            wglSwapIntervalEXT(-1);
        } else {
            wglSwapIntervalEXT(1);
        }
    } else if (vs == Context::VSync::Off) {
        wglSwapIntervalEXT(0);
    } else {
        wglSwapIntervalEXT(1);
    }
}


void ContextImpl::GetInfo(Context::InfoDict& info, bool include_extensions) const
{
    if (include_extensions) {
        const char* extensions;
        extensions = wglGetExtensionsStringEXT();
        auto ext_list = UnpackExtensionString(extensions);
        size_t i = 1;
        for (const auto& ext : ext_list) { info.push_back(std::make_pair("WGL ext #" + std::to_string(i++), ext)); }
    }
}
}} // namespace jaws::opengl

#endif

#include <jaws/jaws.hpp>
#include <jaws/opengl/Context.hpp>
#include <jaws/windows.hpp>

//#include <gl/glad.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <iostream>


void glfwErrorCallback(int error, const char* description)
{
    std::cout << "[glfw] " << description << "(" << error << ")" << std::endl;
}


int main(void)
{
    if (!glfwInit()) return -1;

    glfwSetErrorCallback(&glfwErrorCallback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window = glfwCreateWindow(640, 480, "Jaws Example 3", NULL, NULL);
    // GLFWwindow* window = glfwCreateWindow(640, 480, "My Title", glfwGetPrimaryMonitor(), NULL);

    // GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        std::cout << "Failed to initialize glfw window" << std::endl;
        glfwTerminate();
        return -1;
    }


    // Create context

    HWND hwnd = glfwGetWin32Window(window);
    HDC hdc = GetDC(hwnd);

    using jaws::opengl::Context;
    using jaws::opengl::OpenGLVersion;
    Context::Format format;
    format.depth_bits = 24;
    format.stencil_bits = 8;
    format.profile = Context::Profile::Core;
    format.SetOption(Context::Options::sRGB);
    // format.SetOption(Context::Options::Stereo); // I'd bet this didn't work.. we really need to fill a return format.

    Context context;
    context.SetFormat(format);
    context.Create(hdc, OpenGLVersion::OpenGL_4_3, OpenGLVersion::OpenGL_4_6);

    context.MakeCurrent(hdc);

    // Check color encoding of front buffer
    GLint encoding = -1;
    context.gl.GetNamedFramebufferAttachmentParameteriv(
        0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
    if (encoding == GL_LINEAR)
        std::cout << "default fb color is: linear\n";
    else if (encoding == GL_SRGB)
        std::cout << "default fb color is: sRGB\n";

    context.SetVSyncMode(Context::VSync::Adaptive);

    context.gl.ClearColor(1., 0., 0., 1.);
    while (!glfwWindowShouldClose(window)) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        context.gl.Viewport(0, 0, w, h);
        context.gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(hdc);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

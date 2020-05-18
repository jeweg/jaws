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
#if 0
    // Experimenting w/ easylogging++

    el::Configurations conf;
    conf.setToDefault();
    conf.setGlobally(el::ConfigurationType::Format, "%datetime [%level] %msg");
    el::Loggers::setDefaultConfigurations(conf, true);
    LOG(ERROR) << "Example log msg to ERROR";
    LOG(WARNING) << "Example log msg to WARNING";
    LOG(INFO) << "Example log msg to INFO";
    LOG(DEBUG) << "Example log msg to DEBUG";
    LOG(TRACE) << "Example log msg to TRACE";
    el::Loggers::getLogger("bar"); // Registers.
    CLOG(ERROR, "foo") << "Example log msg to TRACE";
    CLOG(ERROR, "bar") << "Example log msg to TRACE";
#endif

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
    format.SetOption(Context::Options::Debug);
    // format.SetOption(Context::Options::Stereo); // I'd bet this didn't work.. we really need to fill a return format.

    Context context;
    context.SetFormat(format);
    context.Create(hdc, OpenGLVersion::OpenGL_4_3, OpenGLVersion::OpenGL_4_6);
    // context.MakeCurrent(hdc); I think this is redundant.
    auto& gl = context.gl;

    gl.ClearColor(1., 0., 0., 1.);

    /*
    // Create VAO
    GLuint vao;
    gl.GenVertexArrays(1, &vao);
    gl.BindVertexArray(vao);
    */

    // ----------------------------------------------------------------------
    // Vertex input

    float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

    GLuint vbo;
    gl.GenBuffers(1, &vbo);

    // ----------------------------------------------------------------------
    // Vertex shader

    auto vertexShaderSource = R"END(
#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)END";

    unsigned int vertexShader;
    vertexShader = gl.CreateShader(GL_VERTEX_SHADER);
    gl.ShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    gl.CompileShader(vertexShader);

    int success;
    char infoLog[512];
    gl.GetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        gl.GetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // ----------------------------------------------------------------------
    // Fragment shader

    auto fragmentShaderSource = R"END(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
} 
)END";
    unsigned int fragmentShader;
    fragmentShader = gl.CreateShader(GL_FRAGMENT_SHADER);
    gl.ShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    gl.CompileShader(fragmentShader);

    // ----------------------------------------------------------------------
    // Shader program

    unsigned int shaderProgram;
    shaderProgram = gl.CreateProgram();
    gl.AttachShader(shaderProgram, vertexShader);
    gl.AttachShader(shaderProgram, fragmentShader);
    gl.LinkProgram(shaderProgram);

    // We can delete the shader objets once we have the program shader object linked.
    gl.DeleteShader(vertexShader);
    gl.DeleteShader(fragmentShader);

    gl.UseProgram(shaderProgram);

    // ----------------------------------------------------------------------
    // VAO

    unsigned int VAO;
    gl.GenVertexArrays(1, &VAO);
    gl.BindVertexArray(VAO);

#if 1
    gl.BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl.BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
#else
    gl.NamedBufferStorage(vbo, sizeof(vertices), vertices, 0);
#endif

    gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    gl.EnableVertexAttribArray(0);

    gl.UseProgram(0);
    gl.BindVertexArray(VAO);

    context.SetVSyncMode(Context::VSync::Off);

    while (!glfwWindowShouldClose(window)) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        gl.Viewport(0, 0, w, h);
        gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        gl.DrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle

        SwapBuffers(hdc);
        glfwPollEvents();
    }
    gl.DisableVertexAttribArray(0);

    glfwTerminate();
    return 0;
}

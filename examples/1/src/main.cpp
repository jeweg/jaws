#include <jaws/jaws.hpp>
#include <jaws/opengl/Context.hpp>
#include <spdlog/spdlog.h>
#include <boost/scope_exit.hpp>
#include <iostream>

constexpr bool ListExtensions = false;

int main(int argc, char** argv)
{
    jaws::Instance instance;

    instance.Create(argc, argv);
    BOOST_SCOPE_EXIT_ALL(&instance)
    {
        instance.Shutdown();
    };

    // TODO: Why is the global set_level ignored here?
    spdlog::set_level(spdlog::level::debug);

    // This works, though.
    jaws::GetLogger(jaws::Category::General).set_level(spdlog::level::debug);

    // Piggypacking onto one of jaws' loggers here.
    auto& logger = jaws::GetLogger(jaws::Category::General);

    using jaws::opengl::Context;
    using jaws::opengl::OpenGLVersion;

    Context::Format format;
    // format.color_bits = 32;
    format.depth_bits = 24;
    format.stencil_bits = 8;
    format.profile = Context::Profile::Core;
    format.SetOption(Context::Options::sRGB);
    // format.SetOption(Context::Options::Stereo); // I'd bet this didn't work.. we really need to fill a return format.

    Context context;
    context.SetFormat(format);
    context.Create(nullptr, OpenGLVersion::OpenGL_4_3, OpenGLVersion::OpenGL_4_6);


    // Get context information as key-value string pairs.
    auto info = context.GetInfo(ListExtensions);
    for (const auto& info_pair : info) { logger.info("{}: {}", info_pair.first, info_pair.second); }

    // This is how to call into OpenGL.
    // Function names are stripped by their leading "gl" and are
    // available as a context's "gl" data member.
    // Constants stay global.
    context.gl.Enable(GL_DEPTH_TEST);
}

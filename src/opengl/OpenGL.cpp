#include "pch.hpp"
#include "jaws/opengl/OpenGL.hpp"
#include <tuple>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>

namespace jaws { namespace opengl {

static std::tuple<OpenGLVersion, int32_t> opengl_version_info[] = {
    std::make_tuple(OpenGLVersion::OpenGL_1_0, 1 * 1000 + 0),
    std::make_tuple(OpenGLVersion::OpenGL_1_1, 1 * 1000 + 1),
    std::make_tuple(OpenGLVersion::OpenGL_1_2, 1 * 1000 + 2),
    std::make_tuple(OpenGLVersion::OpenGL_1_3, 1 * 1000 + 3),
    std::make_tuple(OpenGLVersion::OpenGL_1_4, 1 * 1000 + 4),
    std::make_tuple(OpenGLVersion::OpenGL_1_5, 1 * 1000 + 5),
    std::make_tuple(OpenGLVersion::OpenGL_2_0, 2 * 1000 + 0),
    std::make_tuple(OpenGLVersion::OpenGL_2_1, 2 * 1000 + 1),
    std::make_tuple(OpenGLVersion::OpenGL_3_0, 3 * 1000 + 0),
    std::make_tuple(OpenGLVersion::OpenGL_3_1, 3 * 1000 + 1),
    std::make_tuple(OpenGLVersion::OpenGL_3_2, 3 * 1000 + 2),
    std::make_tuple(OpenGLVersion::OpenGL_3_3, 3 * 1000 + 3),
    std::make_tuple(OpenGLVersion::OpenGL_4_0, 4 * 1000 + 0),
    std::make_tuple(OpenGLVersion::OpenGL_4_1, 4 * 1000 + 1),
    std::make_tuple(OpenGLVersion::OpenGL_4_2, 4 * 1000 + 2),
    std::make_tuple(OpenGLVersion::OpenGL_4_3, 4 * 1000 + 3),
    std::make_tuple(OpenGLVersion::OpenGL_4_4, 4 * 1000 + 4),
    std::make_tuple(OpenGLVersion::OpenGL_4_5, 4 * 1000 + 5),
    std::make_tuple(OpenGLVersion::OpenGL_4_6, 4 * 1000 + 6)

};


void UnpackVersion(const OpenGLVersion version, int32_t& major, int32_t& minor)
{
    auto info = opengl_version_info[static_cast<size_t>(version)];
    int32_t value = std::get<1>(info);
    major = value / 1000;
    minor = value % 1000;
}


std::string to_string(const OpenGLVersion version)
{
    int32_t major, minor;
    UnpackVersion(version, major, minor);
    return std::to_string(major) + '.' + std::to_string(minor);
}


std::vector<std::string> UnpackExtensionString(const char* ext_string)
{
    std::istringstream iss(ext_string);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    std::stable_sort(tokens.begin(), tokens.end());
    return tokens;
};


std::string to_string(const GLubyte* bytes)
{
    if (!bytes) { return std::string(); }
    return std::string(reinterpret_cast<const char*>(bytes));
}

}} // namespace jaws::opengl

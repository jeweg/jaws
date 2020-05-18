#pragma once

#if defined(_WIN32) && !defined(APIENTRY)
#    define APIENTRY __stdcall
#endif

#include <jaws_export.h>
#include "jaws/opengl/glad/glad.h"

#include <cstdint>
#include <string>
#include <vector>

namespace jaws::opengl {

enum class OpenGLVersion
{
    OpenGL_1_0 = 0,
    OpenGL_1_1,
    OpenGL_1_2,
    OpenGL_1_3,
    OpenGL_1_4,
    OpenGL_1_5,
    OpenGL_2_0,
    OpenGL_2_1,
    OpenGL_3_0,
    OpenGL_3_1,
    OpenGL_3_2,
    OpenGL_3_3,
    OpenGL_4_0,
    OpenGL_4_1,
    OpenGL_4_2,
    OpenGL_4_3,
    OpenGL_4_4,
    OpenGL_4_5,
    OpenGL_4_6
};


extern JAWS_API std::string to_string(const OpenGLVersion);
extern JAWS_API std::string to_string(const GLubyte*);

extern JAWS_API void UnpackVersion(const OpenGLVersion, int32_t& major, int32_t& minor);

extern JAWS_API std::vector<std::string> UnpackExtensionString(const char* ext_string);

}

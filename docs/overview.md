# Current vcpkg libraries used

- fmt
- spdlog
- glm
- abseil
- gtest
- vulkan (note that this does not get us the vulkan sdk, it just integrates an existing one.)
- volk
- shaderc
- spirv-cross
- rapidjson
- glfw3


# Old

Libraries used:

- boost (currently  /*for functionnames and*/ stacktraces)
vulkan-hpp
    I used to use this via vcpkg, but it's always tied to a specific Vulkan SDK.
    Here's a discussion: https://github.com/KhronosGroup/Vulkan-Hpp/issues/108
    I'm not sure what we want to do about that. In the meantime, I found that
    the vulkan sdk ships with a matching vulkan.hpp. So let's just use that.
    Tags for vulkan specs are here: https://github.com/KhronosGroup/Vulkan-Docs

fmt
    via vcpkg
spdlog
    via vcpkg
boost
    via vcpkg
glm
    via vcpkg
glfw3
    via vcpkg
    on Ubuntu 18.04 this also required manually installing libxrandr-dev, libxinerama-dev, libxcursor-dev
abseil
    via vcpkg
celero
    via vcpkg
    On Ubuntu 18.04 this also required manually installing libncurses5-dev
gtest
    via vcpkg
shaderc
    via vcpkg

glad
    This is a somewhat custom built glad 2. We have to look into whether this
    sitauion can be improved and whether this lib is actually maintained, I'm
    not sure.
    Maybe I should use the opportunity to make the OpenGL stuff optional
    and make it depend on the availability of this lib.

PerfSDK is shipped with Visual Studio and found with a shipped find script.
    However, all msdn articles about seem to say that the perf sdk will go away,
    so let's drop it now.

On Linux, I use ninja (package "ninja-build").

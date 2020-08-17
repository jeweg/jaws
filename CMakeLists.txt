cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

cmake_policy(SET CMP0072 NEW) # Prefer GLVND
cmake_policy(SET CMP0057 NEW) # Support for if (... IN_LIST ...)

####################################################
# Options
####################################################

option(BUILD_SHARED_LIBS "Build as shared library?" ON)
option(BUILD_EXAMPLES "Build the examples?" ON)
option(BUILD_TESTS "Build the tests?" ON)
option(BUILD_EXPERIMENTS "Build the experiments?" ON)
option(BUILD_UTILS "Build the utils?" ON)
option(SUPPORT_OPENGL "Support for OpenGL?" OFF)
option(SUPPORT_VULKAN "Support for Vulkan?" ON)
option(USE_COTIRE "Use cotire to generate a unity build?" ON)
option(SEPARATE_HEADER_IMPL "Separate trees for header and impl files in an IDE?" OFF)

####################################################
# Vcpkg orchestration
####################################################

# Note that this section sets CMAKE_TOOLCHAIN_FILE, so it must go before project().
include(external/vcpkg-cmake/vcpkg_cmake.cmake)
vcpkg_cmake_begin(
    VCPKG_DIR 3rd_party/vcpkg
    REPO_URL https://github.com/jweg/vcpkg
)
vcpkg_cmake_package(fmt)
vcpkg_cmake_package(glm)
vcpkg_cmake_package(spdlog)
vcpkg_cmake_package(abseil FEATURES cxx17)
vcpkg_cmake_package(xxhash)

if (SUPPORT_VULKAN)
    vcpkg_cmake_package(volk)
    vcpkg_cmake_package(spirv-cross)
    vcpkg_cmake_package(vulkan-memory-allocator)
    vcpkg_cmake_package(shaderc)
    vcpkg_cmake_package(glfw3)
endif()

if (BUILD_TESTS)
    vcpkg_cmake_package(gtest)
endif()

vcpkg_cmake_end()

#################################################################################
# Project
#################################################################################

# Modelled after https://github.com/pabloariasal/modern-cmake-sample

project(jaws VERSION 0.1 LANGUAGES C CXX)

# Is this built standd-alone or via add_subdirectory?
# Set JAWS_MASTER_PROJECT to ON if building jaws by itself!
if (NOT DEFINED JAWS_MASTER_PROJECT)
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(JAWS_MASTER_PROJECT ON)
    else()
        set(JAWS_MASTER_PROJECT OFF)
    endif()
endif ()

# Make sure that custom modules are found
#list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Having all artifacts built into a common directory is the easiest
# way to make sure sharred libraries are found etc.
set(shared_output_dir "${CMAKE_BINARY_DIR}/_output")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${shared_output_dir})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${shared_output_dir})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${shared_output_dir})

set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "DEBUG")

# I found that the NMake Makefiles generator requires this or it'll
# try to link to the debug libs in release configurations.
# Also, the empty string is needed so it falls back to the configuration-less properties,
# e.g. in the case of Vulkan::Vulkan.
# See https://gitlab.kitware.com/cmake/cmake/issues/16280
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG "DEBUG" "")
set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE "RELEASE" "")
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RELEASE" "")
set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL "RELEASE" "")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Unfortunately we cannot do this before the vcpkg package declarations.
# That's because vcpkg must go before the call to project(), but FindVulkan
# reqruires CMAKE_SIZEOF_VOID_P set which is only set after project().
if (SUPPORT_VULKAN)
    find_package(Vulkan)
    if (NOT TARGET Vulkan::Vulkan)
        message("Vulkan not found, disabling Vulkan support.")
        set(SUPPORT_VULKAN OFF)
    endif()
endif()

if (SUPPORT_OPENGL)
    find_package(OpenGL)
    if (NOT OpenGL_FOUND)
        message("OpenGL not found, disabling OpenGL support.")
        set(SUPPORT_OPENGL OFF)
    else()
        # TODO: maybe separate logic dependiing on
        # whether we hav GLVND or not, etc.
    endif()
endif()


#################################################################################
# The jaws library
#################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(src_root
    include/jaws/jaws.hpp
    include/jaws/core.hpp
    include/jaws/windows.hpp
    include/jaws/fatal.hpp
    include/jaws/errors.hpp
    include/jaws/assume.hpp
    include/jaws/logging.hpp
    include/jaws/filesystem.hpp
    src/pch.hpp
    src/jaws.cpp
    src/windows.cpp
    src/fatal.cpp
    src/errors.cpp
)
set(src_opengl
    include/jaws/opengl/opengL.hpp
    include/jaws/opengl/context.hpp
    src/opengl/opengL.cpp
    src/opengl/context.cpp
    src/opengl/context_impl_windows.hpp
    src/opengl/context_impl_windows.cpp
    include/jaws/opengl/glad/glad.h
    src/opengl/glad/glad.c
)
set(src_vulkan
    include/jaws/vulkan/fwd.hpp
    include/jaws/vulkan/vulkan.hpp
    include/jaws/vulkan/context.hpp
    include/jaws/vulkan/window_context.hpp
    include/jaws/vulkan/device.hpp
    include/jaws/vulkan/utils.hpp
    include/jaws/vulkan/shader_system.hpp
    include/jaws/vulkan/shader.hpp
    include/jaws/vulkan/extension.hpp
    #include/jaws/vulkan/sediment.hpp
    src/vulkan/context.cpp
    src/vulkan/window_context.cpp
    src/vulkan/device.cpp
    src/vulkan/utils.cpp
    src/vulkan/vulkan.cpp
    src/vulkan/to_string.cpp
    src/vulkan/to_string.hpp
    src/vulkan/to_string.inl
    src/vulkan/shader_system.cpp
    src/vulkan/extension.cpp
    #src/vulkan/sediment.cpp
)

if (WIN32)
    list(APPEND src_opengl
        include/jaws/opengl/glad/glad_wgl.h
        src/opengl/glad/glad_wgl.c
    )
elseif (UNIX)
    list(APPEND src_opengl
        include/jaws/opengl/glad/glad_glx.h
        src/opengl/glad/glad_glx.c
    )
endif()

set(src_util
    include/jaws/util/indenting_ostream.hpp
    src/util/indenting_ostream.cpp
    include/jaws/util/indentation.hpp
    include/jaws/util/timer.hpp
    src/util/timer.cpp
    include/jaws/util/main_wrapper.hpp
    include/jaws/util/misc.hpp
    include/jaws/util/instance_counter.hpp
    include/jaws/util/file_observer.hpp
    include/jaws/util/type_traits.hpp
    include/jaws/util/file_observing.hpp
    include/jaws/util/hashing.hpp
    include/jaws/util/ref_ptr.hpp
    include/jaws/util/ref_ptr.inl
    src/util/file_observer.cpp
    src/util/file_observer_impl.hpp
    src/util/file_observing.cpp
    include/jaws/util/string_lines_builder.hpp
    include/jaws/util/enumerate_range.hpp
    include/jaws/util/lru_cache.hpp
)

set(src_vfs
    include/jaws/vfs/vfs.hpp
    include/jaws/vfs/vfs_backend.hpp
    include/jaws/vfs/path.hpp
    include/jaws/vfs/file_system_vfs.hpp
    src/vfs/file_system_vfs.cpp
    src/vfs/vfs.cpp
    src/vfs/path.cpp
)

set(all_source
    ${src_root}
    ${src_util}
    ${src_vfs}
    ${src_3rdparty}
)

if (SUPPORT_OPENGL)
    list(APPEND all_source ${src_opengl})
endif()

if (SUPPORT_VULKAN)
    list(APPEND all_source ${src_vulkan})
endif()

# Source code organisation for IDEs
if (SEPARATE_HEADER_IMPL)
    # Organizes in two trees
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${all_source})
else()
    # Manual organization reflecting namespaces
    source_group("jaws" FILES ${src_root})
    source_group("jaws\\opengl" FILES ${src_opengl})
    source_group("jaws\\vulkan" FILES ${src_vulkan})
    source_group("jaws\\util" FILES ${src_util})
    source_group("jaws\\vfs" FILES ${src_vfs})
    source_group("jaws\\3rd-party" FILES ${src_3rdparty})
endif()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_library(jaws
    STATIC # See log [2019-09-28 Fri]
    ${all_source}
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(jaws PUBLIC Threads::Threads)

find_package(Filesystem COMPONENTS Final Experimental REQUIRED)
target_link_libraries(jaws PUBLIC std_filesystem)

find_package(fmt CONFIG REQUIRED)

find_package(glm CONFIG REQUIRED)
set(glm_defines
    #GLM_SWIZZLE # "Enabling swizzle operators will massively increase the size of compiled files and the compilation time" No thank you.
    GLM_PRECISION_HIGHP_FLOAT # These are the defaults.
    GLM_PRECISION_HIGHP_DOUBLE
    GLM_PRECISION_HIGHP_INT
    GLM_PRECISION_HIGHP_UINT
    GLM_FORCE_MESSAGES # Information during build
    GLM_FORCE_CXX17
    GLM_FORCE_INLINE
    #GLM_FORCE_SIZE_T_LENGTH # Makes length() return a size_t instead of an int
    GLM_FORCE_NO_CTOR_INIT # Default ctor doesn't initialize
    GLM_FORCE_EXPLICIT_CTOR
    GLM_FORCE_UNRESTRICTED_GENTYPE
    GLM_FORCE_XYZW_ONLY # Less clutter, easier debugging
    #GLM_FORCE_LEFT_HANDED # Right-handed is default.
    #GLM_FORCE_DEPTH_ZERO_TO_ONE # Not sure if useful w/ Vulkan
)

find_package(absl CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

# Alias that matches the imported target when used from installation.
# This way code within the build tree (examples, tests) can use the same target name.
add_library(jaws::jaws ALIAS jaws)

target_compile_features(jaws
PUBLIC
    cxx_std_17
)
target_compile_options(jaws
PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall>
    $<$<CXX_COMPILER_ID:MSVC>:-wd4251> # "needs to have DLL interface..." -- but mind https://stackoverflow.com/a/5664491
)
target_compile_definitions(jaws
PRIVATE
    JAWS_EXPORTS
    ${glm_defines}
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING>
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING>

)

get_target_property(jaws_lib_type jaws TYPE)
if (jaws_lib_type STREQUAL "STATIC_LIBRARY")
	target_compile_definitions(jaws PUBLIC JAWS_STATIC)
endif()

target_include_directories(jaws
PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rd-party>

	# TODO: Unfortunately, glad includes its khronos headers
	# using <> syntax which does not try file-relative lookup but requires
	# an include path. I'd like to avoid this, though.
    $<INSTALL_INTERFACE:include/jaws/opengl/glad>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/jaws/opengl/glad>

    # Users must be able to find jaws_export.h:
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# TODO: it's not yet clear at all what libs are needed for the
# public interfaces or how the public interfaces look.
# So for now we choose public/private mostly based on convenient
# lib access in the examples and experiments.
target_link_libraries(jaws
PUBLIC
    fmt::fmt
    absl::strings
    absl::hash
    absl::flat_hash_map
    glm
    spdlog::spdlog

    # Careful, the container target is deprecated and does not work
    #absl::container

PRIVATE
	xxHash::xxhash
)
if (SUPPORT_VULKAN)

	find_package(volk CONFIG REQUIRED)
	find_package(shaderc CONFIG REQUIRED)
	find_package(spirv_cross_core CONFIG REQUIRED)
	find_package(spirv_cross_glsl CONFIG REQUIRED)
	find_package(spirv_cross_reflect CONFIG REQUIRED)

    target_link_libraries(jaws
    PUBLIC
        # For now make most stuff public so we can experiment in examples.
        Vulkan::Vulkan
        volk::volk_headers
        unofficial::shaderc::combined
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-reflect
    )
endif()
if (SUPPORT_OPENGL)
    target_link_libraries(jaws
    PUBLIC
        OpenGL::GL
)
endif()
set_target_properties(jaws PROPERTIES
    DEBUG_POSTFIX "d"
    RELEASE_POSTFIX ""
)

#################################################################################
# Installation
#################################################################################

if (OFF) # Currently broken b/c std_filesytem doesn't export properly

include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/jaws)

# Install public headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install header generated by generate_export_header
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/jaws_export.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(installed_targets
    jaws
)

# Install library target and add it and dependencies to export set.
install(
    TARGETS ${installed_targets}
    EXPORT jaws-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# If the file name should have different spelling:
#set_target_properties(jsonutils PROPERTIES EXPORT_NAME JSONUtils)

# Actually write exported config w/ imported targets
install(EXPORT jaws-targets
    FILE jawsTargets.cmake
    NAMESPACE jaws::
    DESTINATION ${INSTALL_CONFIGDIR}
)

# Create a ConfigVersion.cmake file:
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/jawsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Configure config file
configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/jawsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/jawsConfig.cmake
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

# Install the config, configversion and custom find modules
install(FILES
    #${CMAKE_CURRENT_LIST_DIR}/cmake/FindRapidJSON.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/jawsConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/jawsConfigVersion.cmake
    DESTINATION ${INSTALL_CONFIGDIR}
)
endif()

#################################################################################
# Test/examples/misc
#################################################################################

function (add_jaws_executable folder target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target}
        PRIVATE
        jaws::jaws
    )
    set_target_properties(${target} PROPERTIES FOLDER ${folder})
    target_compile_features(${target}
        PRIVATE
        cxx_std_17
    )
    target_compile_definitions(${target}
        PRIVATE
        ${spdlog_defines}
    )
    target_compile_options(${target}
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:-wd4251> # "needs to have DLL interface..." -- but mind https://stackoverflow.com/a/5664491
        $<$<CXX_COMPILER_ID:GNU>:-no-pie>
    )
endfunction()

function(create_build_info)
	set(template_path "${CMAKE_CURRENT_BINARY_DIR}/build_info.h.in")
    # Note: we must escape semicolons or they will be swallowed
	# by their interpretaton as list element separators.
	set(build_info_h_in "\
#pragma once
#include <jaws/filesystem.hpp>
namespace build_info {
const std::filesystem::path PROJECT_SOURCE_DIR = \"${PROJECT_SOURCE_DIR}\"\;
}
")
	file(WRITE ${template_path} ${build_info_h_in})
	configure_file(${template_path} "build_info.h")
endfunction()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if (BUILD_TESTS)
    add_subdirectory(test)
endif()

if (BUILD_EXPERIMENTS)
    add_subdirectory(experiments)
endif()

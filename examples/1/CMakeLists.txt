if (NOT SUPPORT_OPENGL)
    message("examples/1 disabled, OpenGL not fully supported.")
    return()
endif()

set(project_name "jaws-example-1")
project(${project_name} LANGUAGES CXX)
add_executable(${project_name} src/main.cpp)
target_link_libraries(${project_name} PRIVATE jaws::jaws)
set_target_properties(${project_name} PROPERTIES FOLDER examples)

target_compile_definitions(${project_name} PRIVATE SPDLOG_DEBUG_ON)

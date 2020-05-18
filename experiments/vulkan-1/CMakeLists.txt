if (NOT SUPPORT_VULKAN)
    return()
endif()
set(project_name "vulkan-1")
project(${project_name} LANGUAGES CXX)

add_executable(${project_name}
    src/main.cpp
    src/diagnostics.hpp
)
set_target_properties(${project_name} PROPERTIES FOLDER experiments)

find_package(Vulkan REQUIRED)
target_link_libraries(${project_name} PRIVATE jaws::jaws glfw Vulkan::Vulkan 
    absl::strings
)

if (NOT SUPPORT_VULKAN)
    return()
endif()

find_package(glfw3 CONFIG REQUIRED)

set(project_name "jaws-example-vulkan1")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(examples ${project_name}
	main.cpp
)

target_link_libraries(${project_name} PRIVATE glfw)
cmut_create_build_info()

if (NOT SUPPORT_VULKAN)
    return()
endif()

find_package(glfw3 CONFIG REQUIRED)

set(project_name "jaws-example-vulkan-x")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(examples ${project_name}
	main.cpp
)

target_link_libraries(${project_name} PRIVATE glfw
    )

create_build_info()

# should be publicly exported by jaws::jaws
#add_subdirectory(../../../../../volk volk)
#target_link_libraries(${project_name} PUBLIC volk)

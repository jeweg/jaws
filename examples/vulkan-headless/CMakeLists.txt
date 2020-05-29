if (NOT SUPPORT_VULKAN)
    return()
endif()

set(project_name "jaws-example-vulkan-headless")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(examples ${project_name}
	main.cpp
)

create_build_info()


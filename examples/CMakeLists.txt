project(jaws-examples LANGUAGES NONE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(logging)
add_subdirectory(fatal)
add_subdirectory(main_wrapper)
add_subdirectory(assume)
add_subdirectory(vulkan-1)
add_subdirectory(vulkan-2)
add_subdirectory(vulkan-x)

project(jaws-examples LANGUAGES NONE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(logging)
add_subdirectory(fatal)
add_subdirectory(main_wrapper)
add_subdirectory(assume)
add_subdirectory(cpu_benchmark)
add_subdirectory(vulkan1)
add_subdirectory(vulkan2)
add_subdirectory(vulkan-headless)

# TODO: what are these? Do we still want them?
add_subdirectory(1)
add_subdirectory(2)
add_subdirectory(3)
add_subdirectory(4)



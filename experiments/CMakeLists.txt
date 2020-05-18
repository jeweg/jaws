project(jaws-experiments LANGUAGES NONE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(spdlog)
add_subdirectory(zeromq)
#add_subdirectory(vulkan-1)
#add_subdirectory(hashes)
add_subdirectory(hashmap)
#add_subdirectory(string)
add_subdirectory(abseil-hashing)

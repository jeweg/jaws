set(project_name "hashes")
project(${project_name} LANGUAGES CXX)
add_executable(${project_name}
    src/main.cpp
)
target_compile_features(${project_name}
PRIVATE
    cxx_std_17
)
target_include_directories(${project_name}
PRIVATE
   ../../src/3rd-party/xxhash_cpp
)
set_target_properties(${project_name} PROPERTIES FOLDER experiments)
target_link_libraries(${project_name} PRIVATE jaws::jaws)

find_package(celero CONFIG REQUIRED)
target_link_libraries(${project_name} PRIVATE celero)

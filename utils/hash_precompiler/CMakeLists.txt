project(hash_precompiler LANGUAGES CXX)
find_package(boost 1.68 REQUIRED)
add_executable(hash_precompiler main.cpp)
target_link_libraries(hash_precompiler PRIVATE Boost::program_options)

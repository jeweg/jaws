set(project_name "jaws-example-main_wrapper")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(examples ${project_name} main.cpp)

set(project_name "jaws-example-assume")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(examples ${project_name} main.cpp)

set(project_name "jaws-example-logging")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(examples ${project_name} main.cpp)

#target_link_libraries(${project_name} PRIVATE Boost::boost)


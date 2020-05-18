set(project_name "zeromq")
project(${project_name} LANGUAGES CXX)
add_executable(${project_name} src/main.cpp)
set_target_properties(${project_name} PROPERTIES FOLDER experiments)

#find_package(ZeroMQ CONFIG REQUIRED)
#target_link_libraries(${project_name} PRIVATE libzmq libzmq-static)

set(project_name "string")
project(${project_name} LANGUAGES CXX)

add_executable(${project_name}
    src/main.cpp
)
set_target_properties(${project_name} PROPERTIES FOLDER experiments)
target_link_libraries(${project_name} PRIVATE jaws::jaws)

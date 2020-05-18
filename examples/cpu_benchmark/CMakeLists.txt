if (OFF)
    set(project_name "jaws-example-cpu_benchmark")
    project(${project_name} LANGUAGES CXX)
    add_jaws_executable(examples ${project_name} main.cpp)

    find_package(celero CONFIG REQUIRED)
    target_link_libraries(${project_name} PRIVATE celero)

endif ()

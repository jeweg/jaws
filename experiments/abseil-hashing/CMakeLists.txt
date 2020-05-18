set(project_name "abseil_hashing")
project(${project_name} LANGUAGES CXX)
add_jaws_executable(experiments ${project_name}
    main.cpp
)
target_link_libraries(${project_name}
    PRIVATE
    absl::flat_hash_map)


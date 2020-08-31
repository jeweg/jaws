project(test)

enable_testing()
find_package(GTest CONFIG REQUIRED)

# Convenience wrapper around add_test().
function (add_gtest_executable _target)

    add_executable(${_target} ${ARGN})
    target_link_libraries(${_target}
    PRIVATE
        GTest::gtest GTest::gtest_main
		jaws::jaws
    )
    set_target_properties(${_target} PROPERTIES FOLDER test)
# TODO: this might not fully work yet --
# the test executable is fine, but the tests don't register with
# ctest. I'm not completely sure what the result should look like, though.
    #gtest_discover_tests(${_target})
endfunction()

add_subdirectory(test_instance_counter)
add_subdirectory(test_enumerate_range)
add_subdirectory(test_string_builder)
add_subdirectory(test_abseil_stringview_compat)
add_subdirectory(test_lru_cache)
add_subdirectory(test_vfs)
add_subdirectory(test_ref_ptr)
add_subdirectory(test_pool)

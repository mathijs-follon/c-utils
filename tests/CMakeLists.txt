if(TARGET utils_static)
    set(UTILS_TEST_LIB utils_static)
elseif(TARGET utils_shared)
    set(UTILS_TEST_LIB utils_shared)
else()
    message(FATAL_ERROR "No utils library target available for tests")
endif()

set(TEST_SOURCES
    test_str.c
    test_vec.c
    test_stack.c
    test_queue.c
    test_iterator.c
    test_alloc.c
    test_fs.c
)


foreach(src IN LISTS TEST_SOURCES)
    get_filename_component(name "${src}" NAME_WE)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${name} PRIVATE ${UTILS_TEST_LIB})
    target_compile_features(${name} PRIVATE c_std_11)

    if(ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${name} PRIVATE /W4)
        else()
            target_compile_options(${name} PRIVATE
                -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            )
        endif()
    endif()

    add_test(NAME ${name} COMMAND ${name})
endforeach()

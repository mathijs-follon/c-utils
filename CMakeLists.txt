cmake_minimum_required(VERSION 3.20)

project(utils
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/compile_commands.json")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/compile_commands.json"
    )
endif()

option(BUILD_SHARED "Build a shared library" OFF)
option(BUILD_STATIC "Build a static library" ON)

option(ENABLE_ASSERTS "Enable assertions" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(CUSTOM_FS "Skip built-in fs backends; link your own fs_platform_* impl" OFF)

option(UTILS_ALLOC_TLSF "Build TLSF allocator module" ON)
set(UTILS_DEFAULT_ALLOCATOR "system" CACHE STRING "Default allocator module (system|tlsf)")
set_property(CACHE UTILS_DEFAULT_ALLOCATOR PROPERTY STRINGS system tlsf)

if(UTILS_DEFAULT_ALLOCATOR STREQUAL "tlsf")
    if(NOT UTILS_ALLOC_TLSF)
        message(FATAL_ERROR "UTILS_DEFAULT_ALLOCATOR=tlsf requires UTILS_ALLOC_TLSF=ON")
    endif()
elseif(NOT UTILS_DEFAULT_ALLOCATOR STREQUAL "system")
    message(FATAL_ERROR "Unknown UTILS_DEFAULT_ALLOCATOR='${UTILS_DEFAULT_ALLOCATOR}' (use system|tlsf)")
endif()

# CUSTOM_FS leaves fs_platform_* undefined, so tests cannot link unless the
# consumer provides a backend. Default testing off when CUSTOM_FS is enabled;
# re-enable explicitly with -DBUILD_TESTING=ON after linking your backend.
if(CUSTOM_FS)
    if(NOT UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED)
        set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree." FORCE)
        set(UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED TRUE CACHE INTERNAL "")
        message(STATUS "CUSTOM_FS=ON: BUILD_TESTING defaulted to OFF")
    endif()
else()
    if(UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED)
        set(BUILD_TESTING ON CACHE BOOL "Build the testing tree." FORCE)
        unset(UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED CACHE)
        message(STATUS "CUSTOM_FS=OFF: BUILD_TESTING restored to ON")
    endif()
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(NOT UTILS_ALLOC_TLSF)
    list(FILTER SOURCES EXCLUDE REGEX ".*/alloc/tlsf/.*")
    list(FILTER SOURCES EXCLUDE REGEX ".*/alloc/tlsf_heap\\.c$")
endif()

if(NOT SOURCES)
    message(FATAL_ERROR "No source files found")
endif()

set(UTILS_TARGETS)

if(BUILD_SHARED)
    add_library(utils_shared SHARED ${SOURCES})
    list(APPEND UTILS_TARGETS utils_shared)
endif()

if(BUILD_STATIC)
    add_library(utils_static STATIC ${SOURCES})
    list(APPEND UTILS_TARGETS utils_static)
endif()

if(NOT UTILS_TARGETS)
    message(FATAL_ERROR "At least one of BUILD_SHARED or BUILD_STATIC must be enabled")
endif()

foreach(target IN LISTS UTILS_TARGETS)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    target_compile_features(${target}
        PUBLIC
            c_std_11
    )

    if(ENABLE_ASSERTS)
        target_compile_definitions(${target}
            PUBLIC
                UTILS_ENABLE_ASSERTS=1
        )
    else()
        target_compile_definitions(${target}
            PUBLIC
                UTILS_ENABLE_ASSERTS=0
        )
    endif()

    if(CUSTOM_FS)
        target_compile_definitions(${target}
            PUBLIC
                CUSTOM_FS=1
        )
    endif()

    if(UTILS_ALLOC_TLSF)
        target_compile_definitions(${target}
            PUBLIC
                UTILS_ALLOC_TLSF=1
        )
        target_include_directories(${target}
            PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/src/alloc/tlsf"
        )
        # Vendored third-party TLSF is not warning-clean under our flags.
        set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/src/alloc/tlsf/tlsf.c"
            PROPERTIES
                COMPILE_FLAGS "-Wno-conversion -Wno-sign-conversion -Wno-pedantic"
        )
    else()
        target_compile_definitions(${target}
            PUBLIC
                UTILS_ALLOC_TLSF=0
        )
    endif()

    if(UTILS_DEFAULT_ALLOCATOR STREQUAL "tlsf")
        target_compile_definitions(${target}
            PUBLIC
                UTILS_DEFAULT_ALLOCATOR_TLSF=1
                UTILS_DEFAULT_ALLOCATOR_SYSTEM=0
        )
    else()
        target_compile_definitions(${target}
            PUBLIC
                UTILS_DEFAULT_ALLOCATOR_TLSF=0
                UTILS_DEFAULT_ALLOCATOR_SYSTEM=1
        )
    endif()

    if(ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${target}
                PRIVATE
                    /W4
            )
        else()
            target_compile_options(${target}
                PRIVATE
                    -Wall
                    -Wextra
                    -Wpedantic
                    -Wconversion
                    -Wshadow
        )
        endif()
    endif()

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME utils
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endforeach()

include(GNUInstallDirs)

foreach(target IN LISTS UTILS_TARGETS)
    install(
        TARGETS ${target}
        EXPORT utilsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endforeach()

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT utilsTargets
    FILE utilsTargets.cmake
    NAMESPACE utils::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utils
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

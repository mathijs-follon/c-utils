cmake_minimum_required(VERSION 3.20)

project(utils
    VERSION 0.1.0
    LANGUAGES C
)

# True when this repo is the top-level project
# (not add_subdirectory / FetchContent).
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(UTILS_IS_TOP_LEVEL ON)
else()
    set(UTILS_IS_TOP_LEVEL OFF)
endif()

if(UTILS_IS_TOP_LEVEL)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/compile_commands.json")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${CMAKE_SOURCE_DIR}/compile_commands.json"
        )
    endif()
endif()

option(UTILS_BUILD_SHARED "Build utils shared library" OFF)
option(UTILS_BUILD_STATIC "Build utils static library" ON)
option(UTILS_ENABLE_ASSERTS "Enable utils assertions" OFF)
option(UTILS_ENABLE_WARNINGS "Enable utils compiler warnings" ${UTILS_IS_TOP_LEVEL})
option(UTILS_BUILD_TESTS "Build utils tests" ${UTILS_IS_TOP_LEVEL})
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
# consumer provides a backend. Default testing off when CUSTOM_FS is enabled.
if(CUSTOM_FS AND UTILS_BUILD_TESTS)
    if(NOT UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED)
        set(UTILS_BUILD_TESTS OFF CACHE BOOL "Build utils tests" FORCE)
        set(UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED TRUE CACHE INTERNAL "")
        message(STATUS "CUSTOM_FS=ON: UTILS_BUILD_TESTS defaulted to OFF")
    endif()
elseif(NOT CUSTOM_FS)
    if(UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED)
        set(UTILS_BUILD_TESTS ${UTILS_IS_TOP_LEVEL} CACHE BOOL "Build utils tests" FORCE)
        unset(UTILS_CUSTOM_FS_TESTING_DEFAULT_APPLIED CACHE)
        message(STATUS "CUSTOM_FS=OFF: UTILS_BUILD_TESTS restored")
    endif()
endif()

file(GLOB_RECURSE UTILS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

if(NOT UTILS_ALLOC_TLSF)
    list(FILTER UTILS_SOURCES EXCLUDE REGEX ".*/alloc/tlsf/.*")
    list(FILTER UTILS_SOURCES EXCLUDE REGEX ".*/alloc/tlsf_heap\\.c$")
endif()

if(NOT UTILS_SOURCES)
    message(FATAL_ERROR "No utils source files found")
endif()

set(UTILS_TARGETS)

if(UTILS_BUILD_SHARED)
    add_library(utils_shared SHARED ${UTILS_SOURCES})
    add_library(utils::shared ALIAS utils_shared)
    list(APPEND UTILS_TARGETS utils_shared)
endif()

if(UTILS_BUILD_STATIC)
    add_library(utils_static STATIC ${UTILS_SOURCES})
    add_library(utils::static ALIAS utils_static)
    list(APPEND UTILS_TARGETS utils_static)
endif()

if(NOT UTILS_TARGETS)
    message(FATAL_ERROR "Enable UTILS_BUILD_STATIC and/or UTILS_BUILD_SHARED")
endif()

# Preferred link target for consumers: utils::utils
if(TARGET utils_static)
    add_library(utils::utils ALIAS utils_static)
elseif(TARGET utils_shared)
    add_library(utils::utils ALIAS utils_shared)
endif()

foreach(target IN LISTS UTILS_TARGETS)
    target_include_directories(${target}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_compile_features(${target}
        PUBLIC
            c_std_11
    )

    if(UTILS_ENABLE_ASSERTS)
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

    if(UTILS_ENABLE_WARNINGS)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4)
        else()
            target_compile_options(${target} PRIVATE
                -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            )
        endif()
    endif()

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME utils
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        EXPORT_NAME $<IF:$<STREQUAL:${target},utils_static>,static,shared>
    )
endforeach()

# Fix EXPORT_NAME: generator expression above is awkward. Set explicitly.
if(TARGET utils_static)
    set_target_properties(utils_static PROPERTIES EXPORT_NAME static)
endif()
if(TARGET utils_shared)
    set_target_properties(utils_shared PROPERTIES EXPORT_NAME shared)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

foreach(target IN LISTS UTILS_TARGETS)
    install(
        TARGETS ${target}
        EXPORT utilsTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/utilsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/utilsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utils
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/utilsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/utilsConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/utilsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/utils
)

if(UTILS_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif()
endif()

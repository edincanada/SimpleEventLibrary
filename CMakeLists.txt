cmake_minimum_required(VERSION 4.0)

project(SimpleEventLibrary
    VERSION 0.1.0
    DESCRIPTION "A small header-only C++ event library"
    LANGUAGES CXX
)

include(CTest)

add_library(SimpleEventLibrary INTERFACE)
add_library(SimpleEventLibrary::SimpleEventLibrary ALIAS SimpleEventLibrary)

target_include_directories(SimpleEventLibrary
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(SimpleEventLibrary INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(SimpleEventLibrary INTERFACE /W4 /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(SimpleEventLibrary INTERFACE -Wall -Wextra -Wpedantic)
endif()

option(SIMPLE_EVENT_LIBRARY_BUILD_TESTS "Build SimpleEventLibrary tests" ${BUILD_TESTING})

if(SIMPLE_EVENT_LIBRARY_BUILD_TESTS)
    find_package(Catch2 3 QUIET)

    if(NOT Catch2_FOUND)
        include(FetchContent)

        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.1
        )

        FetchContent_MakeAvailable(Catch2)
    endif()

    add_executable(SimpleEventLibraryTests
        tests/test_event.cpp
    )

    target_link_libraries(SimpleEventLibraryTests
        PRIVATE
            SimpleEventLibrary::SimpleEventLibrary
            Catch2::Catch2WithMain
    )

    include(Catch)
    catch_discover_tests(SimpleEventLibraryTests)
endif()


# CMakeLists.txt - MICS Library Test Suite
#
# Usage:
#   cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build
#
# Optional:
#   -DBUILD_TESTING=OFF          Disable test building
#   -DCMAKE_CXX_STANDARD=17|20   Override C++ standard (default: 17)

cmake_minimum_required(VERSION 3.20)
project(MICSTest LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Compiler options
# ---------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/utf-8 /permissive- /W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Include directories (header-only library)
# ---------------------------------------------------------------------------
set(REFLECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# ---------------------------------------------------------------------------
# Catch2
# ---------------------------------------------------------------------------
find_package(Catch2 3 QUIET)

include(CTest)
if(BUILD_TESTING)
    enable_testing()
endif()

# ---------------------------------------------------------------------------
# Test executable: reflect_test
# ---------------------------------------------------------------------------
if(Catch2_FOUND)
    add_executable(${PROJECT_NAME} main.cpp)

    target_include_directories(${PROJECT_NAME} PRIVATE ${REFLECT_INCLUDE_DIR})

    target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain)

    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /permissive-)
    endif()

    if(BUILD_TESTING)
        include(Catch)
        catch_discover_tests(${PROJECT_NAME})
    endif()

    message(STATUS "Catch2 found — ${PROJECT_NAME} target created")
else()
    message(STATUS "Catch2 not found — skipping ${PROJECT_NAME} target")
    message(STATUS "Install Catch2: https://github.com/catchorg/Catch2")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
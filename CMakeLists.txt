# CMakeLists.txt - MICS Library Test Suite
#
# Usage:
#   cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   ctest --test-dir build
#
# Optional:
#   -DBUILD_TESTING=OFF          Disable test building
#   -DMICS_INSTALL=OFF           Disable CMake install rules
#   -DCMAKE_CXX_STANDARD=17|20   Override C++ standard (default: 17)
#
# Install:
#   cmake --install build --prefix <prefix>
#
# Consumers can then use:
#   find_package(MICS CONFIG REQUIRED)
#   target_link_libraries(my_target PRIVATE MICS::MICS)

cmake_minimum_required(VERSION 3.20)
project(
    MICS
    VERSION 1.0.0
    LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(MICS_IS_TOP_LEVEL ON)
else()
    set(MICS_IS_TOP_LEVEL OFF)
endif()

option(MICS_INSTALL "Generate MICS install rules" ${MICS_IS_TOP_LEVEL})
option(MICS_BUILD_TESTS "Build MICS's own test suite" ${MICS_IS_TOP_LEVEL})

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
# Header-only library target
# ---------------------------------------------------------------------------
# HLMD is bundled as a submodule but is exposed through the same include root
# as MICS, because the public headers include "SKL/HLMD/HLMD.h" directly.
set(MICS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(MICS_HLMD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/HLMD)

add_library(MICS INTERFACE)
add_library(MICS::MICS ALIAS MICS)

target_compile_features(MICS INTERFACE cxx_std_17)
target_include_directories(
    MICS INTERFACE $<BUILD_INTERFACE:${MICS_INCLUDE_DIR}> $<BUILD_INTERFACE:${MICS_HLMD_INCLUDE_DIR}>
                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# ---------------------------------------------------------------------------
# Install rules and CMake package files
# ---------------------------------------------------------------------------
if(MICS_INSTALL)
    set(MICS_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/MICS")

    install(TARGETS MICS EXPORT MICSTargets)

    # Headers keep their SKL/... layout so the includes in MICS.h resolve from
    # a single installed include directory.
    install(DIRECTORY ${MICS_INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(DIRECTORY ${MICS_HLMD_INCLUDE_DIR}/SKL/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SKL)

    install(
        EXPORT MICSTargets
        FILE MICSTargets.cmake
        NAMESPACE MICS::
        DESTINATION ${MICS_CMAKE_INSTALL_DIR})

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MICSConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/MICSConfig.cmake
        INSTALL_DESTINATION ${MICS_CMAKE_INSTALL_DIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/MICSConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MICSConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/MICSConfigVersion.cmake
            DESTINATION ${MICS_CMAKE_INSTALL_DIR})

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/licenses/MICS)
endif()

# ---------------------------------------------------------------------------
# Catch2
# ---------------------------------------------------------------------------
if(MICS_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        find_package(Catch2 3 QUIET)
    endif()
else()
    message(STATUS "MICS tests disabled — skipping MICSTest target")
endif()

# ---------------------------------------------------------------------------
# Test executable: MICSTest
# ---------------------------------------------------------------------------
if(MICS_BUILD_TESTS AND BUILD_TESTING AND Catch2_FOUND)
    add_executable(MICSTest main.cpp)

    target_link_libraries(MICSTest PRIVATE MICS::MICS Catch2::Catch2WithMain)

    if(MSVC)
        target_compile_options(MICSTest PRIVATE /utf-8 /permissive-)
    endif()

    include(Catch)
    catch_discover_tests(MICSTest)

    set_target_properties(MICSTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    message(STATUS "Catch2 found — MICSTest target created")
elseif(MICS_BUILD_TESTS AND BUILD_TESTING)
    message(STATUS "Catch2 not found — skipping MICSTest target")
    message(STATUS "Install Catch2: https://github.com/catchorg/Catch2")
elseif(MICS_BUILD_TESTS)
    message(STATUS "Testing disabled — skipping MICSTest target")
endif()

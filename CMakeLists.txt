cmake_minimum_required(VERSION 3.20)

# Define project name, version, and language
project(
    nexus_cpp
    VERSION 0.1.0
    DESCRIPTION "A modern C++20/23 asynchronous engine playground"
    LANGUAGES CXX
)

# Append custom CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Set C++ standard (Enforce C++20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler extensions (e.g., gnu++20) for ISO C++ compliance

# Import compiler flags and options module
include(CompilerFlags)

# Define the core header-only target for initial phase
add_library(nexus_core INTERFACE)
add_library(nexus::core ALIAS nexus_core)

# Configure include directories and C++ features for the interface target
target_include_directories(nexus_core
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(nexus_core
    INTERFACE
        cxx_std_20
)

# Apply warning flags
apply_compiler_flags(nexus_core)

# Build options
option(NEXUS_BUILD_TESTS "Build unit tests" ON)
option(NEXUS_BUILD_EXAMPLES "Build examples" ON)
option(NEXUS_ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" ON)

if(NEXUS_ENABLE_SANITIZERS)
    enable_sanitizers(nexus_core)
endif()

if(NEXUS_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()

if(NEXUS_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

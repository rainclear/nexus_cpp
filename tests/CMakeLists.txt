# tests/CMakeLists.txt
include(FetchContent)

# Fetch Catch2 v3 unit testing framework automatically
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Define unit test executable target
add_executable(unit_tests
    test_fixed_vector.cpp
)

# Link against the core library and Catch2 main entry point
target_link_libraries(unit_tests
    PRIVATE
        nexus::core
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(unit_tests)

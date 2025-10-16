cmake_minimum_required(VERSION 3.14)
project(HFTLogMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimizations for production
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -Wall -Wextra -Wpedantic")

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)

# Find dependencies
find_package(Threads REQUIRED)

# Include FetchContent for downloading dependencies
include(FetchContent)

if(BUILD_TESTS OR BUILD_BENCHMARKS)
    # Fetch Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
endif()

if(BUILD_BENCHMARKS)
    # Fetch Google Benchmark
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)
endif()

# Main library
add_library(log_monitor_lib STATIC
    src/log_monitor.cpp
    src/keyword_matcher.cpp
)

target_include_directories(log_monitor_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(log_monitor_lib PUBLIC
    Threads::Threads
)

# Log Generator Executable
add_executable(log_generator
    src/log_generator.cpp
)

target_link_libraries(log_generator PRIVATE
    Threads::Threads
)

# Log Monitor Executable
add_executable(log_monitor
    src/main.cpp
)

target_link_libraries(log_monitor PRIVATE
    log_monitor_lib
)

# Tests
if(BUILD_TESTS)
    add_executable(log_monitor_tests
        tests/test_keyword_matcher.cpp
        tests/test_log_monitor.cpp
    )
    
    target_link_libraries(log_monitor_tests PRIVATE
        log_monitor_lib
        GTest::gtest_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(log_monitor_tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_executable(log_monitor_benchmark
        benchmarks/benchmark_monitor.cpp
    )
    
    target_link_libraries(log_monitor_benchmark PRIVATE
        log_monitor_lib
        benchmark::benchmark
    )
endif()

# Install rules
install(TARGETS log_generator log_monitor
    RUNTIME DESTINATION bin
)

install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/config/monitor.conf.example
    DESTINATION etc
)
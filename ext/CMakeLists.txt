if(WHEELS_BUILD_TESTS OR WHEELS_BUILD_BENCHES)
    # Pull submodules
    execute_process(COMMAND git submodule update --init --depth 1)
endif()

if(WHEELS_BUILD_TESTS)
    add_subdirectory(catch2)
endif()

if(WHEELS_BUILD_BENCHES)
    set(BENCHMARK_ENABLE_TESTING OFF)
    add_subdirectory(benchmark)
endif()

add_library(wyhash INTERFACE)
target_include_directories(wyhash SYSTEM INTERFACE wyhash)

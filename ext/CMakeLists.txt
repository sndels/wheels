# Pull submodules
execute_process(COMMAND git submodule update --init --depth 1)

add_subdirectory(catch2)
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(benchmark)

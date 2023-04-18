cmake_minimum_required(VERSION 3.20)
project(wheels)

option(WHEELS_BUILD_TESTS "Include tests" ON)
option(WHEELS_BUILD_BENCHES "Include benchmarks" ON)

add_subdirectory(ext)

add_library(wheels INTERFACE)

target_include_directories(wheels
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(wheels
    INTERFACE
    wyhash
)

if(WHEELS_BUILD_TESTS)
    add_subdirectory(tests)

    add_executable(wheels_test ${TESTS_SOURCES})
    target_compile_features(wheels_test
        PUBLIC
        cxx_std_20
    )

    if(MSVC)
        # From cppbestpractices
        target_compile_options(wheels_test
            PRIVATE
            /permissive-
            /W4
            /w14242
            /w14254
            /w14263
            /w14265
            /w14287
            /we4289
            /w14296
            /w14311
            /w14545
            /w14546
            /w14547
            /w14549
            /w14555
            /w14619
            /w14640
            /w14826
            /w14905
            /w14906
            /w14928
        )
    else()
        target_compile_options(wheels_test
            PRIVATE
            -pedantic
            -Wall
            -Wextra
            -Wunused
            -Wno-missing-field-initializers
            -Wno-self-assign-overloaded
        )
    endif() # NOT MSVC

    target_link_libraries(wheels_test PRIVATE wheels Catch2WithMain)
endif()

if(WHEELS_BUILD_BENCHES)
    add_executable(wheels_bench
        ${CMAKE_CURRENT_LIST_DIR}/bench/main.cpp
    )
    target_compile_features(wheels_bench
        PUBLIC
        cxx_std_20
    )

    if(MSVC)
        # From cppbestpractices
        target_compile_options(wheels_bench
            PRIVATE
            /permissive-
            /W4
            /w14242
            /w14254
            /w14263
            /w14265
            /w14287
            /we4289
            /w14296
            /w14311
            /w14545
            /w14546
            /w14547
            /w14549
            /w14555
            /w14619
            /w14640
            /w14826
            /w14905
            /w14906
            /w14928
        )

        # Cmake uses /Ob1 for some reason so we have to override to match Release
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ob2")
    else()
        target_compile_options(wheels_bench
            PRIVATE
            -pedantic
            -Wall
            -Wextra
            -Wunused
        )
    endif() # NOT MSVC

    target_link_libraries(wheels_bench PRIVATE wheels benchmark::benchmark)
endif()

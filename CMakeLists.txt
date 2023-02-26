cmake_minimum_required(VERSION 3.20)
project(wheels)

add_subdirectory(ext)

add_library(wheels INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/allocators/allocator.hpp
    ${CMAKE_CURRENT_LIST_DIR}/allocators/linear_allocator.hpp
    ${CMAKE_CURRENT_LIST_DIR}/allocators/scoped_scratch.hpp
    ${CMAKE_CURRENT_LIST_DIR}/allocators/utils.hpp
    ${CMAKE_CURRENT_LIST_DIR}/containers/array.hpp
    ${CMAKE_CURRENT_LIST_DIR}/containers/static_array.hpp
)

target_include_directories(wheels
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(wheels_test
    ${CMAKE_CURRENT_LIST_DIR}/allocators/test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/containers/test.cpp
)
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
    )
endif() # NOT MSVC

target_link_libraries(wheels_test PRIVATE wheels Catch2WithMain)

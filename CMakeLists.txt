cmake_minimum_required(VERSION 3.20)
project(allocators)

add_subdirectory(ext)

add_library(allocators INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/allocator.hpp
    ${CMAKE_CURRENT_LIST_DIR}/linear_allocator.hpp
    ${CMAKE_CURRENT_LIST_DIR}/utils.hpp
)

target_include_directories(allocators
    INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

add_executable(allocators_test ${CMAKE_CURRENT_LIST_DIR}/test.cpp)
target_compile_features(allocators_test
    PUBLIC
    cxx_std_20
)

if(MSVC)
    # From cppbestpractices
    target_compile_options(allocators_test
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
    target_compile_options(allocators_test
        PRIVATE
        -pedantic
        -Wall
        -Wextra
        -Wunused
    )
endif() # NOT MSVC

target_link_libraries(allocators_test PRIVATE allocators Catch2WithMain)

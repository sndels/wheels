add_subdirectory(allocators)
add_subdirectory(containers)

set(TESTS_SOURCES
    ${ALLOCATOR_TESTS_SOURCES}
    ${CONTAINER_TESTS_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/owning_ptr.cpp
    ${CMAKE_CURRENT_LIST_DIR}/leak_check.cpp
    PARENT_SCOPE
)

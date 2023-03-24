add_subdirectory(allocators)
add_subdirectory(containers)

set(TESTS_SOURCES
    ${ALLOCATOR_TESTS_SOURCES}
    ${CONTAINER_TESTS_SOURCES}
    PARENT_SCOPE
)

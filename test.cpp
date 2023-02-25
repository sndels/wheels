#include <catch2/catch_test_macros.hpp>

#include "linear_allocator.hpp"
#include "utils.hpp"

TEST_CASE("aligned_offset", "[test]")
{
    REQUIRE(aligned_offset(0, 8) == 0);
    REQUIRE(aligned_offset(1, 8) == 8);
    REQUIRE(aligned_offset(4, 8) == 8);
    REQUIRE(aligned_offset(8, 8) == 8);
}

TEST_CASE("LinearAllocator", "[test]")
{
    LinearAllocator allocator{4096};
    {
        uint8_t *alloc = (uint8_t *)allocator.allocate(2048);
        REQUIRE(alloc != nullptr);
        memset(alloc, 0, 2048);
        alloc[0] = 0x12;
        alloc[2047] = 0x23;
        assert(alloc[0] == 0x12);
        assert(alloc[2047] == 0x23);
        REQUIRE(allocator.allocate(2048) != nullptr);
        REQUIRE(allocator.allocate(1) == nullptr);
    }
    allocator.reset();
    {
        void *alloc0 = allocator.allocate(2048);
        REQUIRE(alloc0 != nullptr);
        allocator.deallocate(alloc0);
        void *alloc1 = allocator.allocate(2048);
        REQUIRE(allocator.allocate(1) == nullptr);
        allocator.rewind(alloc1);
        REQUIRE(allocator.allocate(2048) == alloc1);
        allocator.rewind(alloc0);
        REQUIRE(allocator.allocate(4096) != nullptr);
    }
}

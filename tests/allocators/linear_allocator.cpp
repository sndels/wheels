#include <catch2/catch_test_macros.hpp>

#include <wheels/allocators/linear_allocator.hpp>

#include "common.hpp"

using namespace wheels;

TEST_CASE("LinearAllocator")
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

TEST_CASE("LinearAllocator::aligned_PoD")
{
    LinearAllocator allocator{4096};

    AlignedObj *aligned_alloc0 =
        (AlignedObj *)allocator.allocate(sizeof(AlignedObj));
    uint8_t *u8_alloc = (uint8_t *)allocator.allocate(sizeof(uint8_t));
    AlignedObj *aligned_alloc1 =
        (AlignedObj *)allocator.allocate(sizeof(AlignedObj));
    REQUIRE(aligned_alloc0 != nullptr);
    REQUIRE(u8_alloc != nullptr);
    REQUIRE(aligned_alloc1 != nullptr);
    REQUIRE((std::uintptr_t)aligned_alloc0 % alignof(AlignedObj) == 0);
    REQUIRE((std::uintptr_t)aligned_alloc1 % alignof(AlignedObj) == 0);
}
